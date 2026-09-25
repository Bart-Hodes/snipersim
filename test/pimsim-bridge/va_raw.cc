// Phase 2 check: PrIM VA on 4 simulated DPUs, driven through raw PIM driver
// calls (PIMSIM_CMD_CALL) from an x86 program running under Sniper.
//
// Same shape as pimsim's golden run (tests/golden/prim_va_4dpu.*): 4 DPUs,
// 65536 int32 per DPU and vector, args + A + B pushed, B pulled back.
//
//   ./va_raw <path to pimsim/apps/prim/VA/bin/dpu_code>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sim_api.h"
#include "pimsim_sniper.h"
#include "pimsim/pim_abi.h"

static const unsigned kDpus = 4;
static const uint32_t kElems = 65536;  // per DPU
static const uint32_t kBytes = kElems * sizeof(int32_t);

static int failures;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); ++failures; } } while (0)

static pimsim_call call(uint64_t num, uint64_t a0 = 0, uint64_t a1 = 0, uint64_t a2 = 0) {
  pimsim_call c = {num, a0, a1, a2, 0, 0};
  uint64_t st = SimUser(PIMSIM_CMD_CALL, (uint64_t)&c);
  if (st != 0) { printf("FAIL: PIMSIM_CMD_CALL status %lu (Sniper built without pimsim?)\n", (unsigned long)st); exit(1); }
  return c;
}

// One push/pull: the same symbol range on every DPU, one host buffer each.
static pimsim_call xfer(uint32_t dir, const char* sym, uint64_t offset, uint64_t size, void* const bufs[]) {
  pim_xfer_entry_t e[kDpus];
  for (unsigned i = 0; i < kDpus; ++i) e[i] = pim_xfer_entry_t{i, 0, (uint64_t)bufs[i]};
  pim_xfer_t d = {dir, kDpus, (uint64_t)sym, offset, size, (uint64_t)e};
  return call(PIM_SYS_XFER, (uint64_t)&d);
}

int main(int argc, char** argv) {
  setvbuf(stdout, NULL, _IONBF, 0);
  if (argc < 2) { printf("usage: va_raw dpu_code\n"); return 2; }
  SimSetInstrumentMode(SIM_OPT_INSTRUMENT_FASTFORWARD);

  int32_t* A = (int32_t*)malloc(kDpus * kBytes);
  int32_t* B = (int32_t*)malloc(kDpus * kBytes);
  int32_t* C = (int32_t*)malloc(kDpus * kBytes);
  srand(1);
  for (uint32_t i = 0; i < kDpus * kElems; ++i) { A[i] = rand(); B[i] = rand(); }
  struct { uint32_t size, transfer_size, kernel; } args[kDpus];
  void *pa[kDpus], *pA[kDpus], *pB[kDpus], *pC[kDpus];
  for (unsigned i = 0; i < kDpus; ++i) {
    args[i] = {kBytes, kBytes, 0};
    pa[i] = &args[i];
    pA[i] = A + i * kElems;
    pB[i] = B + i * kElems;
    pC[i] = C + i * kElems;
  }

  // The PIM part runs in detailed mode.
  SimSetInstrumentMode(SIM_OPT_INSTRUMENT_DETAILED);
  pimsim_call c = call(PIM_SYS_ALLOC, kDpus);
  CHECK(c.ret == kDpus, "alloc returned %ld", (long)c.ret);
  c = call(PIM_SYS_LOAD, (uint64_t)argv[1]);
  CHECK(c.ret == 0, "load returned %ld", (long)c.ret);

  uint64_t t0 = SimUser(PIMSIM_CMD_NOW_PS, 0);
  pimsim_call x1 = xfer(PIM_XFER_TO_DPU, "DPU_INPUT_ARGUMENTS", 0, sizeof args[0], pa);
  pimsim_call x2 = xfer(PIM_XFER_TO_DPU, PIM_MRAM_HEAP_SYMBOL, 0, kBytes, pA);
  pimsim_call x3 = xfer(PIM_XFER_TO_DPU, PIM_MRAM_HEAP_SYMBOL, kBytes, kBytes, pB);
  uint64_t t1 = SimUser(PIMSIM_CMD_NOW_PS, 0);
  pimsim_call l = call(PIM_SYS_LAUNCH, 0, kDpus, 0);  // synchronous
  uint64_t t2 = SimUser(PIMSIM_CMD_NOW_PS, 0);
  pimsim_call x4 = xfer(PIM_XFER_FROM_DPU, PIM_MRAM_HEAP_SYMBOL, kBytes, kBytes, pC);
  uint64_t t3 = SimUser(PIMSIM_CMD_NOW_PS, 0);
  call(PIM_SYS_FREE);
  SimSetInstrumentMode(SIM_OPT_INSTRUMENT_FASTFORWARD);

  CHECK(x1.ret == 0 && x2.ret == 0 && x3.ret == 0 && l.ret == 0 && x4.ret == 0, "a transfer or launch failed: %ld %ld %ld %ld %ld",
        (long)x1.ret, (long)x2.ret, (long)x3.ret, (long)l.ret, (long)x4.ret);
  printf("stall ps: args %lu, A %lu, B %lu, launch %lu, C %lu\n", (unsigned long)x1.stall_ps,
         (unsigned long)x2.stall_ps, (unsigned long)x3.stall_ps, (unsigned long)l.stall_ps, (unsigned long)x4.stall_ps);
  printf("host time ps: CPU-DPU %lu, launch %lu, DPU-CPU %lu\n", (unsigned long)(t1 - t0), (unsigned long)(t2 - t1),
         (unsigned long)(t3 - t2));

  uint64_t bad = 0;
  for (uint32_t i = 0; i < kDpus * kElems; ++i) bad += C[i] != (int32_t)((uint32_t)A[i] + (uint32_t)B[i]);
  CHECK(bad == 0, "%lu wrong elements", (unsigned long)bad);
  printf(failures ? "VA RAW FAILED (%d)\n" : "VA RAW OK\n", failures);
  return failures != 0;
}
