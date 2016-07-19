.PHONY: ALL
ALL: testprog

CXXFLAGS+=-IX-Mem/src/include -Wall -std=c++11

testprog: main.o benchmark_kernels.o
	$(CXX) -o $@ $^



