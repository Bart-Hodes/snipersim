TARGET=bridge_test
include ../shared/Makefile.shared
CXXFLAGS=$(SNIPER_CXXFLAGS)

$(TARGET): $(TARGET).o
	$(CXX) $(TARGET).o $(SNIPER_LDFLAGS) -o $(TARGET)

run_$(TARGET):
	../../run-sniper -v -n 1 -c gainestown -- ./bridge_test
