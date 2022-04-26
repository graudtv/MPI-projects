## Useful commands:
```bash
make                                # build
make clean && CXXFLAGS='-O2' make   # rebuild with optimizations
./prog --help                       # print usage
./regression.pl                     # run tests
./benchmark.pl                      # run benchmarks

# visualize results
mpirun -n 4 \
  | ./prog -X 6.28 -T 6.28 -M 100 -K 1000 --file input_known.txt --data \
  | tr ' ' '\n' \
  | gnuplot -p -e 'plot "< cat -" with lines'

# run without creating temporary input file
echo "sin(x) 0 cos(x+t)" \
  | tr ' ' '\n' \
  | ./prog -X 6 -T 0.5 -M 600 -K 100 --data
```

## Notes
- _lab.pdf_ - the task description
- _input.txt_ - contains some random equation, I'm not sure if it makes sense.
Used in benchmarks
- _input_known.txt_ - contains equation with solution u(t, x) = sin(x) * cos(t).
Used in vizualization command above
- M should be < K, otherwise computation will probably give
incorrect results

## My benchmark results
Run on 2.3 GHz Intel Core i5, 4 cores

Without optimizations:
```
--------------------------------------
| M     | K     | NProc | Time       |
--------------------------------------
| 1024  | 1024  | 1     | 0.07s      |
| 1024  | 4096  | 1     | 0.28s      |
| 1024  | 16384 | 1     | 1.12s      |
| 1024  | 65536 | 1     | 4.79s      |
--------------------------------------
| 1024  | 1024  | 1     | 0.07s      |
| 4096  | 1024  | 1     | 0.28s      |
| 16384 | 1024  | 1     | 1.09s      |
| 65536 | 1024  | 1     | 4.41s      |
--------------------------------------
| 16384 | 16384 | 1     | 17.65s     |
| 16384 | 16384 | 2     | 8.90s      |
| 16384 | 16384 | 3     | 6.29s      |
| 16384 | 16384 | 4     | 4.83s      |
| 16384 | 16384 | 8     | 5.36s      |
| 16384 | 16384 | 16    | 5.74s      |
--------------------------------------
```
-O2:
```
--------------------------------------
| M     | K     | NProc | Time       |
--------------------------------------
| 1024  | 1024  | 1     | 0.04s      |
| 1024  | 4096  | 1     | 0.15s      |
| 1024  | 16384 | 1     | 0.65s      |
| 1024  | 65536 | 1     | 2.59s      |
--------------------------------------
| 1024  | 1024  | 1     | 0.04s      |
| 4096  | 1024  | 1     | 0.17s      |
| 16384 | 1024  | 1     | 0.67s      |
| 65536 | 1024  | 1     | 2.93s      |
--------------------------------------
| 16384 | 16384 | 1     | 10.24s     |
| 16384 | 16384 | 2     | 5.12s      |
| 16384 | 16384 | 3     | 3.47s      |
| 16384 | 16384 | 4     | 2.81s      |
| 16384 | 16384 | 8     | 2.89s      |
| 16384 | 16384 | 16    | 3.14s      |
--------------------------------------

```
