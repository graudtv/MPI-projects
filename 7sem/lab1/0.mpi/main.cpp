#include <cmath>
#include <cxxmpi/cxxmpi.hpp>
#include <fstream>
#include <iostream>

#define SAVE_DATA 0

namespace mpi = cxxmpi;

constexpr size_t ISIZE = 5000;
constexpr size_t JSIZE = 5000;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx(&argc, &argv);

  std::vector<double> a;

  if (mpi::commRank() == 0) {
    a.resize(ISIZE * JSIZE);
    for (size_t i = 0; i < ISIZE; ++i)
      for (size_t j = 0; j < JSIZE; ++j)
        a[i * JSIZE + j] = 10 * i + j;
  }

  auto chunk = mpi::scatterFair(a, ISIZE * JSIZE, 0);

  mpi::Timer tmr;
  std::transform(chunk.begin(), chunk.end(), chunk.begin(),
                 [](double v) { return sin(2 * v); });

  if (auto res = mpi::gatherv(chunk)) {
    std::cerr << tmr.getElapsedTimeInSeconds() << std::endl;
#if SAVE_DATA
    std::ofstream os;
    os.open("output.txt");
    for (double v : res.data())
      os << v << '\n';
#endif
  }
  return 0;
}