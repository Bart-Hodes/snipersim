#include "intel_l2_prefetcher.h"
#include "simulator.h"
#include "config.hpp"

static const IntPtr LINE_SIZE = 64;
static const IntPtr PAGE_SIZE = 4096;

IntelL2Prefetcher::IntelL2Prefetcher(String configName, core_id_t core_id, UInt32 shared_cores)
   : m_degree(Sim()->getCfg()->getIntArray("perf_model/" + configName + "/prefetcher/intel_l2/degree", core_id))
   , m_streams(Sim()->getCfg()->getIntArray("perf_model/" + configName + "/prefetcher/intel_l2/streams", core_id))
   , m_next_victim(0)
{
}

std::vector<IntPtr>
IntelL2Prefetcher::getNextAddress(IntPtr current_address, core_id_t core_id, Core::mem_op_t mem_op_type, bool cache_hit, bool prefetch_hit, IntPtr eip)
{
   const IntPtr line = current_address & ~(LINE_SIZE - 1);
   const IntPtr page = current_address & ~(PAGE_SIZE - 1);
   const IntPtr adjacent = line ^ LINE_SIZE; // the other line of the 128-byte pair

   std::vector<IntPtr> addresses;
   if (!cache_hit)
      addresses.push_back(adjacent);

   Stream *stream = nullptr;
   for (Stream &s : m_streams)
      if (s.page == page)
         stream = &s;

   if (!stream)
   {
      stream = &m_streams[m_next_victim];
      m_next_victim = (m_next_victim + 1) % m_streams.size();
      *stream = Stream();
      stream->page = page;
      stream->line = line;
      return addresses;
   }

   if (line == stream->line)
      return addresses;

   const SInt64 direction = line > stream->line ? 1 : -1;
   stream->confirmed = direction == stream->direction;
   stream->direction = direction;
   stream->line = line;

   if (stream->confirmed)
      for (UInt32 i = 1; i <= m_degree; ++i)
      {
         const IntPtr address = line + direction * SInt64(i * LINE_SIZE);
         if ((address & ~(PAGE_SIZE - 1)) != page)
            break;
         if (cache_hit || address != adjacent)
            addresses.push_back(address);
      }
   return addresses;
}
