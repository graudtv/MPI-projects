all: compile

compile: prog

prog: main.cpp
	mpicxx -std=c++11 -I../.. -lmatheval $(CXXFLAGS) $< -o $@

clean:
	rm -f prog
