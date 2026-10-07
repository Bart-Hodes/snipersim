# PrIM VA on 4 simulated DPUs through raw pimsim driver calls (va_raw.cc).
#   make run      (needs pimsim around or next to snipersim, with its VA DPU kernel:
#                  make -C ../pimsim/apps/prim/VA NR_DPUS=4)
TARGET=va_raw
include ../shared/Makefile.shared
CXXFLAGS=$(SNIPER_CXXFLAGS)
PIMSIM_ROOT ?= $(abspath $(if $(wildcard ../../../src/pim/pim_system.h),../../..,../../../pimsim))

$(TARGET): $(TARGET).cc ../../include/pimsim_sniper.h
	$(CXX) $(CXXFLAGS) -I$(PIMSIM_ROOT)/include $< $(SNIPER_LDFLAGS) -o $@

run_$(TARGET): $(TARGET)
	SNIPER_ROOT=$(abspath ../..) ../../run-sniper -d out \
	  -c address_translation_schemes/coffee_lake_9900k.cfg \
	  -g --perf_model/mmu/sanity_checks_enabled=false \
	  -g --pimsim/config=$(PIMSIM_ROOT)/configs/upmem.cfg \
	  -g --pimsim/pim/num_dpus=4 \
	  -- ./$(TARGET) $(PIMSIM_ROOT)/apps/prim/VA/bin/dpu_code

CLEAN_EXTRA=out
