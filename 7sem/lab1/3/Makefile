CXXFLAGS += -O2

all: compile

compile: prog

prog: main.cpp
	$(CXX) -fopenmp -std=c++11 -I../../ $(CXXFLAGS) $< -o $@

stat: prog
	@echo "Running..."
	@rm -f omp.dat
	@for n in $(shell seq $(shell nproc)); do \
		printf "%d " $$n >>omp.dat; \
		OMP_NUM_THREADS=$$n ./prog 2>>omp.dat; \
	done
	@echo "Written results to omp.dat"

plot: omp.dat
	../../../scripts/plot.py -i omp.dat

clean:
	rm -f prog *.dat output.txt
