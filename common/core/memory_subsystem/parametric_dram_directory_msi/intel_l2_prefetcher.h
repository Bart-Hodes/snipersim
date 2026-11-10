#ifndef __INTEL_L2_PREFETCHER_H
#define __INTEL_L2_PREFETCHER_H

#include "prefetcher.h"

#include <vector>

/* Intel's two L2 prefetchers together:
 * - spatial (adjacent-line): an L2 miss also fetches the other line of its
 *   128-byte aligned pair.
 * - streamer: tracks one stream per 4 KB page. Two steps in the same direction
 *   confirm the stream; from then on each access fetches the next lines in that
 *   direction, inside the page. Scattered accesses never confirm, so they
 *   prefetch nothing.
 * The adjacent line goes first. Settings: prefetcher/intel_l2/{streams,degree}. */
class IntelL2Prefetcher : public Prefetcher
{
   public:
      IntelL2Prefetcher(String configName, core_id_t core_id, UInt32 shared_cores);
      virtual std::vector<IntPtr> getNextAddress(IntPtr current_address, core_id_t core_id, Core::mem_op_t mem_op_type, bool cache_hit, bool prefetch_hit, IntPtr eip);

   private:
      struct Stream
      {
         IntPtr page = 0;
         IntPtr line = 0; // last line seen
         SInt64 direction = 0; // +1, -1, or 0 before the first step
         bool confirmed = false;
      };

      const UInt32 m_degree;
      std::vector<Stream> m_streams;
      UInt32 m_next_victim; // round robin
};

#endif // __INTEL_L2_PREFETCHER_H
