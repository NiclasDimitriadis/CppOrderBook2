CXX = g++-14
COMPILER_FLAGS = -std=c++23 -O3 -ggdb
INCLUDE_DIRS = -I ../../src/production -I ../../src/testing\
	 -I ../../dependencies/TMP_lib/src/production -I ../../dependencies/SeqLockQueue/src/production
UNITS = Unittests_Auxil Unittests_AtomicGuard Unittests_FileToTuples Unittests_FIXMockSocket\
	 Unittests_FIXSocketHandler Unittests_OrderBookBucket  Unittests_OrderBook
EXECUTABLES = $(addprefix build/,$(UNITS))
OBJ_FILES = $(addsuffix .o,$(EXECUTABLES))


all: $(OBJ_FILES) $(EXECUTABLES)

build/%.o: ../../src/testing/%.cpp
	$(CXX) $(COMPILER_FLAGS) $(INCLUDE_DIRS) -c $< -o $@

build/%: build/%.o
	$(CXX) $(COMPILER_FLAGS) $(INCLUDE_DIRS) $< -o $@	


.PHONY: clean all

clean:
	rm -rf  build/Unittests_*
