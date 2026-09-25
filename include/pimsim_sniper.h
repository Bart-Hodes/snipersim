#ifndef PIMSIM_SNIPER_H
#define PIMSIM_SNIPER_H

// Commands the application sends to the pimsim hook with SimUser(cmd, arg).
// Shared between Sniper (common/system/pimsim_hook.cc) and the app side.
//
// PIMSIM_CMD_CALL carries one PIM driver call (PIM_SYS_* in pimsim's
// include/pimsim/pim_abi.h); the other commands are for testing the bridge.
//
// Sniper config ([pimsim] section, or -g --pimsim/...):
//   config    = path of pimsim's INI (e.g. pimsim/configs/upmem.cfg)
//   overrides = "section/key=value;..." applied on top
// pimsim is created at the first PIMSIM_CMD_CALL; its statistics go to
// pimsim.stats in the Sniper output directory.

#include <stdint.h>

#define PIMSIM_CMD_BASE      0x50494d00ul  /* "PIM\0" */
#define PIMSIM_CMD_ECHO      (PIMSIM_CMD_BASE + 0)  /* returns arg + 1 */
#define PIMSIM_CMD_DELAY_NS  (PIMSIM_CMD_BASE + 1)  /* stall this core for arg ns */
#define PIMSIM_CMD_NOW_PS    (PIMSIM_CMD_BASE + 2)  /* this core's elapsed time, ps */
#define PIMSIM_CMD_READ      (PIMSIM_CMD_BASE + 3)  /* arg = struct pimsim_buf*; returns FNV-1a of the bytes */
#define PIMSIM_CMD_WRITE     (PIMSIM_CMD_BASE + 4)  /* arg = struct pimsim_buf*; fills bytes with (i * 7 + seed) */
#define PIMSIM_CMD_CALL      (PIMSIM_CMD_BASE + 5)  /* arg = struct pimsim_call*; returns 0 (1: no pimsim) */
#define PIMSIM_CMD_LAST      PIMSIM_CMD_CALL

/* NOTE: this fork's SIFT reader (sift_reader.cc) never replies to a magic
 * instruction whose result is 42 (a MimicOS special case), so no command may
 * return 42 -- the application would block forever. */

/* One driver call. The calling core is stalled until the call completes in
 * simulated time (DPU run, transfer, ...); that stall is also reported. */
struct pimsim_call {
  uint64_t num;       /* PIM_SYS_* */
  uint64_t a0, a1, a2;
  int64_t ret;        /* out: the call's result (0 / count, or -errno) */
  uint64_t stall_ps;  /* out: simulated time the core was stalled */
};

struct pimsim_buf {
  uint64_t addr;
  uint64_t size;
  uint64_t seed;
};

#endif
