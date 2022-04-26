all: compile

compile: prog

prog: main.cpp
	mpicxx -std=c++14 -I../../.. $(CXXFLAGS) $< -o $@

clean:
	rm -f prog
