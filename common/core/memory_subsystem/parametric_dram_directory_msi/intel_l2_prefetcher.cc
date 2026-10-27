#include "intel_l2_prefetcher.h"

static const IntPtr LINE_SIZE = 64;

IntelL2Prefetcher::IntelL2Prefetcher(String configName, core_id_t core_id, UInt32 shared_cores)
   : m_streamer(configName, core_id, shared_cores)
{
}

std::vector<IntPtr>
IntelL2Prefetcher::getNextAddress(IntPtr current_address, core_id_t core_id, Core::mem_op_t mem_op_type, bool cache_hit, bool prefetch_hit, IntPtr eip)
{
   const IntPtr line = current_address & ~(LINE_SIZE - 1);
   const IntPtr adjacent = line ^ LINE_SIZE; // the other line of the 128-byte pair
   std::vector<IntPtr> addresses(1, adjacent);
   // The streamer's first address is the current one: leave it and the
   // adjacent line out
   for (IntPtr address : m_streamer.getNextAddress(current_address, core_id, mem_op_type, cache_hit, prefetch_hit, eip))
   {
      address &= ~(LINE_SIZE - 1);
      if (address != line && address != adjacent)
         addresses.push_back(address);
   }
   return addresses;
}
