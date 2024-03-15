CXX?=clang++
CXXFLAGS=-I. -Wall -g -fPIC -O3 -std=c++17
LIBS = -lpfm

harness.o: perf_event.hpp assert.hpp

libharness.so: harness.o
	$(CXX) -shared -o $@ $^ $(LIBS)

harness_test: harness_test.o
	$(CXX) -o $@ $^ -ldl

test: libharness.so harness_test
	./harness_test

test-v8: D8=../v8/out/x64.release-harness/d8
test-v8: D8_ARGS=--initial-heap-size=21 --max-heap-size=21 --no-lazy --predictable --predictable-gc-schedule --expose-gc-as=v8GC
test-v8: HARNESS_ARGS=--harness --harness_lib=/home/wenyuz/v8/running/harness/libharness.so
test-v8: GC_BEFORE_HARNESS=true # If true, requires --expose-gc-as=v8GC in D7_ARGS
test-v8: libharness.so
	cd ../../v8 && ./tools/dev/gm.py x64.release-harness
	cd .. && ../v8/out/x64.release-harness/d8 $(D8_ARGS) ./bin/octane.js -- ../octane box2d 5 $(GC_BEFORE_HARNESS)
	cd .. && ../v8/out/x64.release-harness/d8 $(D8_ARGS) $(HARNESS_ARGS) ./bin/octane.js -- ../octane box2d 5 $(GC_BEFORE_HARNESS)

clean:
	rm -f *.o *.so *_test
