#!/usr/bin/env bash
# Run one program under Sniper and classify the outcome as OK, CRASH or STUCK.
#   ./run.sh <name> [extra run-sniper args...] -- ./prog [args]
# Log: results/<name>.log, Sniper output: results/<name>.out/
# STUCK = timeout, or the sniper process idle (<1% CPU) for $IDLE seconds.
set -u
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
name=$1; shift
cfg=${CFG:-address_translation_schemes/multicore/coffee_lake_9900k_18c.cfg}
limit=${LIMIT:-300}; IDLE=${IDLE:-30}
log=$here/results/$name.log; out=$here/results/$name.out
rm -rf "$out"; mkdir -p "$out"
export SNIPER_ROOT=$root TMPDIR=$out
start=$(date +%s)
"$root/run-sniper" -d "$out" -c "$cfg" -g --perf_model/mmu/sanity_checks_enabled=false "$@" >"$log" 2>&1 &
pid=$!
idle=0; verdict=""
while kill -0 $pid 2>/dev/null; do
  sleep 5
  el=$(( $(date +%s) - start ))
  if (( el > limit )); then verdict="STUCK (timeout ${limit}s)"; break; fi
  s=$(pgrep -P $pid -x sniper || pgrep -x sniper | head -1)
  cpu=$( [ -n "$s" ] && ps -o pcpu= -p $s | cut -d. -f1 || echo 0 )
  if (( ${cpu:-0} < 1 )); then idle=$((idle + 5)); else idle=0; fi
  if (( idle >= IDLE )); then verdict="STUCK (sniper idle ${IDLE}s)"; break; fi
done
if [ -n "$verdict" ]; then
  pkill -9 -P $pid 2>/dev/null; for p in $(pgrep -x sniper); do kill -9 $p; done
  kill -9 $pid 2>/dev/null
fi
wait $pid 2>/dev/null; rc=$?
if [ -z "$verdict" ]; then
  if grep -aqE "assert|Assertion|Segmentation|SIGSEGV|Aborted|terminate called" "$log"; then verdict="CRASH (exit $rc)"
  elif [ $rc -ne 0 ]; then verdict="FAIL (exit $rc)"
  else verdict="OK"; fi
fi
el=$(( $(date +%s) - start ))
echo "== $name: $verdict after ${el}s  (log: $log)" | tee -a "$log"
grep -aE "assert|Assertion|Segmentation|FAIL|BRIDGE TEST" "$log" | grep -v "^==" | head -3
