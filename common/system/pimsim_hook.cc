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
   }
   return -1;
}

} // namespace

void PimsimHook::init()
{
   Sim()->getHooksManager()->registerHook(HookType::HOOK_MAGIC_USER, onMagicUser, 0);
}
