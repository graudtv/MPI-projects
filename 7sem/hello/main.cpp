#include <omp.h>
#include <cstdio>

int main(int argc, char *argv[]) {
#pragma omp parallel
  printf("thread [%d/%d]: Hello, world!\n", omp_get_thread_num(), omp_get_num_threads());
  return 0;
}
