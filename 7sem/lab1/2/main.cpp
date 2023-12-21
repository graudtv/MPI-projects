#include <cmath>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <vector>

constexpr size_t ISIZE = 5000;
constexpr size_t JSIZE = 5000;

#define SAVE_DATA 0

#define arr(i, j) a[(i)*JSIZE + (j)]
#define res(i, j) r[(i)*JSIZE + (j)]

int main(int argc, char *argv[]) {
  std::vector<double> a(ISIZE * JSIZE);
  std::vector<double> r(ISIZE * JSIZE);

  for (size_t i = 0; i < ISIZE; ++i)
    for (size_t j = 0; j < JSIZE; ++j)
      arr(i, j) = 10 * i + j;

  double begin = omp_get_wtime();
#pragma omp parallel for
  for (size_t i = 0; i < ISIZE; ++i)
    for (size_t j = 0; j < JSIZE; ++j)
      res(i, j) = sin(0.1 * arr(i + 4, j + 2));
  double end = omp_get_wtime();

  std::cerr << end - begin << std::endl;
#if SAVE_DATA
  std::ofstream os;
  os.open("output.txt");
  for (size_t i = 0; i < ISIZE; ++i) {
    for (size_t j = 0; j < JSIZE; ++j)
      os << res(i, j) << " ";
    os << '\n';
  }
#endif
  return 0;
}
