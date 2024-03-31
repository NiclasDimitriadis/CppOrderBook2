CXX = g++-13
COMPILER_FLAGS = -std=c++23 -O3 -march=native -mtune=native
INCLUDE_DIRS = -I ../../src/production -I ../../src/testing                                       -I ../../dependencies/TMP_lib/src/production -I ../../dependencies/SeqLockQueue/src/production
UNITS = Record_latencies_single_threaded Record_latencies_multithreaded Profiling_single_threaded
EXECUTABLES = $(addprefix build/,$(UNITS))
OBJ_FILES = $(addsuffix .o,$(EXECUTABLES))


all: $(OBJ_FILES) $(EXECUTABLES)

build/%.o: ../../src/testing/%.cpp
	$(CXX) $(COMPILER_FLAGS) $(INCLUDE_DIRS) -c $< -o $@

build/%: build/%.o
	$(CXX) $(COMPILER_FLAGS) $(INCLUDE_DIRS) $< -o $@


.PHONY: clean all

clean:
	rm -rf build/Profiling_*
