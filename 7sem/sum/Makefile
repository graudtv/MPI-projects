all: compile

compile: prog

prog: main.cpp
	$(CXX) -fopenmp -std=c++11 -I../../ $(CXXFLAGS) $< -o $@

clean:
	rm -f prog *.dat output.txt
