makefile_dir = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CXXFLAGS += -std=c++17 \
	    -I $(makefile_dir)/../../external \
	    -I $(makefile_dir)/../../external/popl/include \
	    -DCL_HPP_MINIMUM_OPENCL_VERSION=120 \
	    -DCL_HPP_TARGET_OPENCL_VERSION=120

LDFLAGS += -framework OpenCL


all: prog benchmark

prog: main.cpp
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) -o $@

benchmark: main.cpp
	$(CXX) $< $(CXXFLAGS) $(LDFLAGS) -DBENCH -lbenchmark -o $@

clean:
	rm -f main.o prog benchmark

