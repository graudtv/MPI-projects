benchmark_file="correct-exp-1000000-digits.txt"

test() {
  prog=$1
  prec=$2
  echo -n "running test '$prog $prec'...";
  correct=$(head -c $(expr "$prec") "${benchmark_file}")
  # run prog + cut one symbol to not deal with rounding of last symbol
  experimental=$($prog "$prec" | head -c $(expr "$prec"))
  # linear=$(./prog-linear "$prec")
  if [ "$correct" == "$experimental" ]; then
    echo -e " \t -- [OK]";
    return 0
  else
    echo -e " \t -- [FAILED]";
    echo "correct      = $correct"
    echo "experimental = $experimental"
    exit 1
  fi
}

test "./prog" 100
test "./prog" 1000
test "./prog" 10000
test "./prog-linear" 100
test "./prog-linear" 1000
test "./prog-linear" 10000
test "mpirun -n 2 ./prog" 100
test "mpirun -n 4 ./prog" 1000
test "mpirun -n 4 ./prog" 10000
test "mpirun -n 8 ./prog" 100
test "mpirun -n 8 ./prog" 1000
test "mpirun -n 8 ./prog" 10000
test "mpirun -n 8 ./prog" 100000
test "mpirun -n 32 ./prog" 100000

