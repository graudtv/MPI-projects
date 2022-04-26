all: compile

compile: prog prog-linear

prog: main.cpp
	mpicxx -std=c++11 -lgmp -I../../.. ${CXXFLAGS} $< -o $@

prog-linear: main.cpp
	mpicxx -std=c++11 -lgmp -I../../.. ${CXXFLAGS} -DLINEAR $< -o $@

clean:
	rm -f prog prog-linear
