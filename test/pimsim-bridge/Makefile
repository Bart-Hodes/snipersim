TARGET=bridge_test
include ../shared/Makefile.shared
CXXFLAGS=$(SNIPER_CXXFLAGS)

$(TARGET): $(TARGET).o
	$(CXX) $(TARGET).o $(SNIPER_LDFLAGS) -o $(TARGET)

PIMSIM_ROOT ?= $(abspath ../../../pimsim)

va_raw: va_raw.cc ../../include/pimsim_sniper.h
	$(CXX) $(CXXFLAGS) -I$(PIMSIM_ROOT)/include va_raw.cc $(SNIPER_LDFLAGS) -o va_raw

run_$(TARGET):
	../../run-sniper -v -n 1 -c gainestown -- ./bridge_test
