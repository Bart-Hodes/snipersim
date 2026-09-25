#include "pimsim_hook.h"
#include "pimsim_sniper.h"
#include "simulator.h"
#include "hooks_manager.h"
#include "magic_server.h"
#include "core_manager.h"
#include "core.h"
#include "performance_model.h"
#include "instruction.h"
#include "log.h"
#include "config.h"
#include "config.hpp"
#ifdef PIMSIM_ENABLED
#include "bridge/pimsim_bridge.h"
#endif

#include <chrono>
#include <vector>

namespace
{

// App memory is accessed through the frontend (no timing, no cache effects).
// Large buffers are moved in chunks to bound the size of each request.
const UInt64 kChunk = 1 << 20;

void appRead(Core *core, UInt64 addr, char *dst, UInt64 size)
{
   for (UInt64 off = 0; off < size; off += kChunk)
   {
      UInt32 n = std::min(kChunk, size - off);
      core->accessMemory(Core::NONE, Core::READ, addr + off, dst + off, n, Core::MEM_MODELED_NONE);
   }
}

void appWrite(Core *core, UInt64 addr, const char *src, UInt64 size)
{
   for (UInt64 off = 0; off < size; off += kChunk)
   {
      UInt32 n = std::min(kChunk, size - off);
      core->accessMemory(Core::NONE, Core::WRITE, addr + off, const_cast<char*>(src) + off, n, Core::MEM_MODELED_NONE);
   }
}

#ifdef PIMSIM_ENABLED
pimsim_bridge *s_pim = NULL;

// LOG_PRINT_ERROR drops its message in NDEBUG builds; pimsim errors (DPU
// faults, deadlocks, bad configuration) must always be visible.
[[noreturn]] void fatal(const char *what, const char *err)
{
   fprintf(stderr, "[PIMSIM] error: %s: %s\n", what, err);
   fflush(stderr);
   exit(1);
}

String cfgString(const char *key)
{
   return Sim()->getCfg()->hasKey(key) ? Sim()->getCfg()->getString(key) : String("");
}

// Every key in a subsection of [pimsim] is a pimsim override:
// -g --pimsim/pim/num_dpus=4  or  [pimsim/pim] num_dpus = 4.
void collectOverrides(const config::Section &sec, const String &prefix, String &out)
{
   for (const auto &k : sec.getKeys())
      out += prefix + k.first + "=" + k.second->getString() + "\n";
   for (const auto &sub : sec.getSubsections())
      collectOverrides(*sub.second, prefix + sub.first + "/", out);
}

String pimsimOverrides()
{
   String out = cfgString("pimsim/overrides") + "\n";
   if (Sim()->getCfg()->getRoot().hasSection("pimsim"))
      for (const auto &sub : Sim()->getCfg()->getSection("pimsim").getSubsections())
         collectOverrides(*sub.second, sub.first + "/", out);
   return out;
}

void memAccess(void *ctx, uint64_t addr, void *buf, uint64_t n, int write)
{
   Core *core = static_cast<Core*>(ctx);
   if (write)
      appWrite(core, addr, static_cast<const char*>(buf), n);
   else
      appRead(core, addr, static_cast<char*>(buf), n);
}

// One PIM driver call: run it in pimsim at this core's current time, write
// the result back into the app's descriptor and stall the core until done.
SInt64 pimCall(Core *core, UInt64 desc_addr)
{
   char err[512] = "";
   if (!s_pim)
   {
      s_pim = pimsim_bridge_create(cfgString("pimsim/config").c_str(), pimsimOverrides().c_str(), err, sizeof err);
      if (!s_pim)
         fatal("cannot create the PIM system", err);
   }
   pimsim_call c;
   appRead(core, desc_addr, reinterpret_cast<char*>(&c), sizeof c);
   const UInt64 now = core->getPerformanceModel()->getElapsedTime().getPS();
   int64_t ret;
   uint64_t done;
   if (pimsim_bridge_call(s_pim, c.num, c.a0, c.a1, c.a2, now, memAccess, core, &ret, &done, err, sizeof err) != 0)
      fatal("driver call failed", err);
   c.ret = ret;
   c.stall_ps = done - now;
   if (c.stall_ps)
      core->getPerformanceModel()->queuePseudoInstruction(
         new DelayInstruction(SubsecondTime::PS(c.stall_ps), DelayInstruction::PIM_WAIT));
   appWrite(core, desc_addr, reinterpret_cast<const char*>(&c), sizeof c);
   return 0;
}

SInt64 onSimEnd(UInt64, UInt64)
{
   if (s_pim)
   {
      const String path = Sim()->getConfig()->formatOutputFileName("pimsim.stats");
      if (pimsim_bridge_dump_stats(s_pim, path.c_str()) != 0)
         fprintf(stderr, "[PIMSIM] cannot write %s\n", path.c_str());
      pimsim_bridge_destroy(s_pim);
      s_pim = NULL;
   }
   return 0;
}
#endif

double seconds(std::chrono::steady_clock::time_point t0)
{
   return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
}

SInt64 onMagicUser(UInt64, UInt64 argument)
{
   const MagicServer::MagicMarkerType *m = reinterpret_cast<MagicServer::MagicMarkerType*>(argument);
   const UInt64 cmd = m->arg0, arg = m->arg1;
   if (cmd < PIMSIM_CMD_BASE || cmd > PIMSIM_CMD_LAST)
      return -1;  // not ours

   Core *core = Sim()->getCoreManager()->getCoreFromID(m->core_id);

   switch (cmd)
   {
      case PIMSIM_CMD_ECHO:
         return arg + 1;

      case PIMSIM_CMD_DELAY_NS:
         core->getPerformanceModel()->queuePseudoInstruction(
            new DelayInstruction(SubsecondTime::NS(arg), DelayInstruction::PIM_WAIT));
         return 0;

      case PIMSIM_CMD_NOW_PS:
         return core->getPerformanceModel()->getElapsedTime().getPS();

      case PIMSIM_CMD_READ:
      case PIMSIM_CMD_WRITE:
      {
         pimsim_buf b;
         appRead(core, arg, reinterpret_cast<char*>(&b), sizeof b);
         std::vector<char> data(b.size);
         auto t0 = std::chrono::steady_clock::now();
         UInt64 ret = 0;
         if (cmd == PIMSIM_CMD_READ)
         {
            appRead(core, b.addr, data.data(), b.size);
            ret = 1469598103934665603ull;
            for (char c : data)
               ret = (ret ^ static_cast<unsigned char>(c)) * 1099511628211ull;
         }
         else
         {
            for (UInt64 i = 0; i < b.size; ++i)
               data[i] = static_cast<char>(i * 7 + b.seed);
            appWrite(core, b.addr, data.data(), b.size);
         }
         double s = seconds(t0);
         printf("[PIMSIM] %s %lu bytes in %.3f s (%.1f MB/s)\n", cmd == PIMSIM_CMD_READ ? "read" : "write",
                b.size, s, b.size / s / 1e6);
         return ret;
      }

      case PIMSIM_CMD_CALL:
#ifdef PIMSIM_ENABLED
         return pimCall(core, arg);
#else
         return 1;  // Sniper was built without pimsim
#endif
   }
   return -1;
}

} // namespace

void PimsimHook::init()
{
   Sim()->getHooksManager()->registerHook(HookType::HOOK_MAGIC_USER, onMagicUser, 0);
#ifdef PIMSIM_ENABLED
   Sim()->getHooksManager()->registerHook(HookType::HOOK_SIM_END, onSimEnd, 0);
#endif
}
