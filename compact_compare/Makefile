CXX = g++
Variable = -std=c++11 -g

compact_compare: compact_compare.o
	$(CXX) $(Variable) -o compact_compare compact_compare.o

compact_compare.o: compact_compare.cpp nvlsm_types.h
	$(CXX) $(Variable) -c compact_compare.cpp
