#include <cmath>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <vector>

#define SAVE_DATA 0

constexpr size_t ISIZE = 5000;
constexpr size_t JSIZE = 5000;

int main(int argc, char *argv[]) {
  std::vector<double> a(ISIZE * JSIZE);

  for (size_t i = 0; i < ISIZE; ++i)
    for (size_t j = 0; j < JSIZE; ++j)
      a[i * JSIZE + j] = 10 * i + j;

  double begin = omp_get_wtime();

#pragma omp parallel for
  for (size_t i = 0; i < ISIZE * JSIZE; ++i)
    a[i] = sin(2 * a[i]);
  double end = omp_get_wtime();

  std::cerr << end - begin << std::endl;
#if SAVE_DATA
  std::ofstream os;
  os.open("output.txt");
  for (double v : res.data())
    os << v << '\n';
#endif
  return 0;
}
