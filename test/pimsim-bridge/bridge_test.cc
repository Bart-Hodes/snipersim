// Phase 0 check of the pimsim <-> Sniper bridge: return values, injected
// delays, and app-memory access from the hook.
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "sim_api.h"
#include "pimsim_sniper.h"

static int failures;
#define CHECK(c, ...) do { if (!(c)) { printf("FAIL: " __VA_ARGS__); printf("\n"); ++failures; } } while (0)

static volatile uint64_t sink;
static void work(void) { for (uint64_t i = 0; i < 10000; ++i) sink += i * i; }

static uint64_t fnv(const unsigned char* p, uint64_t n) {
  uint64_t h = 1469598103934665603ull;
  for (uint64_t i = 0; i < n; ++i) h = (h ^ p[i]) * 1099511628211ull;
  return h;
}

int main(void) {
  setvbuf(stdout, NULL, _IONBF, 0);
  CHECK(SimInSimulator(), "not running in Sniper");
  SimSetInstrumentMode(SIM_OPT_INSTRUMENT_FASTFORWARD);

  uint64_t e = SimUser(PIMSIM_CMD_ECHO, 100);
  printf("echo(100) = %lu\n", e);
  CHECK(e == 101, "echo");
  CHECK(SimUser(PIMSIM_CMD_BASE - 1, 0) == (uint64_t)-1, "unknown cmd should return -1");

  // Baseline: time for work() alone, then work() after a 1 ms delay.
  // Only the timing checks run in detailed mode; the big buffer fills and
  // checksums run in fast-forward. Not SimRoiEnd(): in this recorder it ends
  // the simulation, after which magic calls are no longer delivered.
  SimSetInstrumentMode(SIM_OPT_INSTRUMENT_DETAILED);
  work();
  uint64_t t0 = SimUser(PIMSIM_CMD_NOW_PS, 0);
  work();
  uint64_t t1 = SimUser(PIMSIM_CMD_NOW_PS, 0);
  SimUser(PIMSIM_CMD_DELAY_NS, 1000000);
  uint64_t t2 = SimUser(PIMSIM_CMD_NOW_PS, 0);
  work();
  uint64_t t3 = SimUser(PIMSIM_CMD_NOW_PS, 0);
  SimSetInstrumentMode(SIM_OPT_INSTRUMENT_FASTFORWARD);
  printf("work: %lu ps; delay->now: %lu ps; delay+work: %lu ps\n", t1 - t0, t2 - t1, t3 - t1);
  CHECK(t3 - t1 >= 1000000000ull, "1 ms delay not visible after work()");
  printf("delay visible at next magic: %s\n", t2 - t1 >= 1000000000ull ? "yes" : "no");

  // Hook reads app memory.
  uint64_t n = 64ull << 20;
  unsigned char* buf = (unsigned char*)malloc(n);
  for (uint64_t i = 0; i < n; ++i) buf[i] = (unsigned char)(i * 13 + 5);
  pimsim_buf d = {(uint64_t)buf, n, 0};
  uint64_t h = SimUser(PIMSIM_CMD_READ, (uint64_t)&d);
  CHECK(h == fnv(buf, n), "read checksum mismatch");

  // Hook writes app memory.
  n = 16ull << 20;
  d = pimsim_buf{(uint64_t)buf, n, 3};
  SimUser(PIMSIM_CMD_WRITE, (uint64_t)&d);
  uint64_t bad = 0;
  for (uint64_t i = 0; i < n; ++i) bad += buf[i] != (unsigned char)(i * 7 + 3);
  CHECK(bad == 0, "write: %lu bytes wrong", bad);

  printf(failures ? "BRIDGE TEST FAILED (%d)\n" : "BRIDGE TEST OK\n", failures);
  return failures != 0;
}
