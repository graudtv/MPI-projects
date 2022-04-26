all: compile

compile: prog

prog: main.cpp
	mpicxx -std=c++11 -I../../.. $(CXXFLAGS) $< -o $@

check: compile
	python3 check.py

clean:
	rm -f prog
