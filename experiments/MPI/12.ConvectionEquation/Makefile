all: compile

compile: prog

prog: main.cpp
	mpicxx -std=c++11 -I../../.. -lmatheval $(CXXFLAGS) $< -o $@

check: prog
	./regression.pl

clean:
	rm -f prog
