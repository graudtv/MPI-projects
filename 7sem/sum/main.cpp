#include <omp.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("Usage: ./prog N\n");
    exit(1);
  }

  long N = atoi(argv[1]);
  double sum = 0;
  auto begin = omp_get_wtime();
#pragma omp parallel for reduction(+: sum)
  for (long i = 1; i < N; ++i)
    sum += 1.0 / i;
  auto end = omp_get_wtime();

  printf("sum(%ld) = %lg\n", N, sum);
  printf("elapsed: %.4lgs\n", end - begin);
  return 0;
}
