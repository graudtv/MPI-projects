CXXFLAGS += -O2

all: compile

compile: prog

prog: main.cpp
	mpicxx -std=c++11 -I../../.. $(CXXFLAGS) $< -o $@

stat: prog
	@echo "Running..."
	@rm -f mpi.dat
	@for n in $(shell seq $(shell nproc)); do \
		printf "%d " $$n >>mpi.dat; \
		mpirun -n $$n --use-hwthread-cpus prog 2>>mpi.dat; \
	done
	@echo "Written results to mpi.dat"

plot: mpi.dat
	../../../scripts/plot.py -i mpi.dat

clean:
	rm -f prog *.dat output.txt
