makefile_dir = $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

CXXFLAGS += -std=c++17 \
	    -I $(makefile_dir)/../../external \
	    -DCL_HPP_MINIMUM_OPENCL_VERSION=120 \
	    -DCL_HPP_TARGET_OPENCL_VERSION=120

ifeq ($(shell uname -s),Darwin)
LDFLAGS += -framework OpenCL
else
LDFLAGS += -lOpenCL
endif


prog: main.o
	$(CXX) $< $(LDFLAGS) -o $@

clean:
	rm -f main.o prog

