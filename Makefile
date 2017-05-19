analysis:
	clang-tidy main.cpp -checks=* -- -std=c++14 -I/usr/include/c++/6.3.0
	clang-tidy dataflow.hpp -checks=* -- -std=c++14 -I/usr/include/c++/6.3.0
