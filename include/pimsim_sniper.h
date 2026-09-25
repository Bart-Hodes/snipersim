#ifndef PIMSIM_SNIPER_H
#define PIMSIM_SNIPER_H

// The command the application sends to the pimsim hook with
// SimUser(PIMSIM_CMD_CALL, &call): one PIM driver call (PIM_SYS_* in pimsim's
// include/pimsim/pim_abi.h). Shared between Sniper
// (common/system/pimsim_hook.cc) and the application (pimsim's sw/libpim).
//
// Sniper config ([pimsim] section, or -g --pimsim/...):
//   config    = path of pimsim's INI (e.g. pimsim/configs/upmem.cfg)
//   <section>/<key> = value   overrides pimsim's section/key, e.g.
//               -g --pimsim/pim/num_dpus=4   or   [pimsim/pim] num_dpus = 4
//   overrides = "section/key=value;..." (config files only: -g cannot parse it)
// pimsim is created at the first PIMSIM_CMD_CALL; its statistics go to
// pimsim.stats in the Sniper output directory.

#include <stdint.h>

/* arg = struct pimsim_call*; returns 0, or 1 if Sniper has no pimsim.
 * (It must never return 42: this fork's SIFT reader does not reply to a
 * magic instruction with result 42, a MimicOS special case.) */
#define PIMSIM_CMD_CALL 0x50494d00ul /* "PIM\0" */

/* One driver call. The calling core is stalled until the call completes in
 * simulated time (DPU run, transfer, ...); that stall is also reported. */
struct pimsim_call {
  uint64_t num;       /* PIM_SYS_* */
  uint64_t a0, a1, a2;
  int64_t ret;        /* out: the call's result (0 / count, or -errno) */
  uint64_t stall_ps;  /* out: simulated time the core was stalled */
};

#endif
