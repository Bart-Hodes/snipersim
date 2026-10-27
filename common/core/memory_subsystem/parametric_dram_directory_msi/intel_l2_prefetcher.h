#ifndef __INTEL_L2_PREFETCHER_H
#define __INTEL_L2_PREFETCHER_H

#include "prefetcher.h"
#include "simple_prefetcher.h"

/* Intel's two L2 prefetchers together: the spatial (adjacent-line)
 * prefetcher, which completes the 128-byte aligned pair of every line
 * fetched, and the streamer, here SimplePrefetcher's strided flows (its
 * prefetcher/simple settings). The adjacent line goes first. */
class IntelL2Prefetcher : public Prefetcher
{
   public:
      IntelL2Prefetcher(String configName, core_id_t core_id, UInt32 shared_cores);
      virtual std::vector<IntPtr> getNextAddress(IntPtr current_address, core_id_t core_id, Core::mem_op_t mem_op_type, bool cache_hit, bool prefetch_hit, IntPtr eip);

   private:
      SimplePrefetcher m_streamer;
};

#endif // __INTEL_L2_PREFETCHER_H
