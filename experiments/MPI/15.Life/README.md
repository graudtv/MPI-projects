# Game Of Life
### Running
```
mdkir build
cd build
cmake .. && make
mpirun -n 4 life ../assets/1.txt
```

### Generate map from text
```
figlet -f banner  'Game Of Life' | tr ' ' '.' | tr '#' 'x'
```
