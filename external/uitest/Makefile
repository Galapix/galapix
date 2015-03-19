all: test_examples

test_examples: Makefile include/uitest/uitest.hpp examples/examples.cpp src/uitest_main.cpp src/uitest.cpp
	$(CXX) -g -std=c++14 -Wall -Iinclude/ src/uitest_main.cpp src/uitest.cpp examples/examples.cpp -o test_examples

.PHONY: all

# EOF #
