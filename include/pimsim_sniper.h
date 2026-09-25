#ifndef PIMSIM_SNIPER_H
#define PIMSIM_SNIPER_H

// Commands the application sends to the pimsim hook with SimUser(cmd, arg).
// Shared between Sniper (common/system/pimsim_hook.cc) and the app side.
//
// Phase 0 (bridge test) commands only; the real PIM driver ABI comes later.

#include <stdint.h>

#define PIMSIM_CMD_BASE      0x50494d00ul  /* "PIM\0" */
#define PIMSIM_CMD_ECHO      (PIMSIM_CMD_BASE + 0)  /* returns arg + 1 */
#define PIMSIM_CMD_DELAY_NS  (PIMSIM_CMD_BASE + 1)  /* stall this core for arg ns */
#define PIMSIM_CMD_NOW_PS    (PIMSIM_CMD_BASE + 2)  /* this core's elapsed time, ps */
#define PIMSIM_CMD_READ      (PIMSIM_CMD_BASE + 3)  /* arg = struct pimsim_buf*; returns FNV-1a of the bytes */
#define PIMSIM_CMD_WRITE     (PIMSIM_CMD_BASE + 4)  /* arg = struct pimsim_buf*; fills bytes with (i * 7 + seed) */
#define PIMSIM_CMD_LAST      PIMSIM_CMD_WRITE

/* NOTE: this fork's SIFT reader (sift_reader.cc) never replies to a magic
 * instruction whose result is 42 (a MimicOS special case), so no command may
 * return 42 -- the application would block forever. */

struct pimsim_buf {
  uint64_t addr;
  uint64_t size;
  uint64_t seed;
};

#endif
