#include <cmath>
#include <cxxmpi/cxxmpi.hpp>
#include <fstream>
#include <iostream>

#define LINEAR 0
#define SAVE_DATA 0

namespace mpi = cxxmpi;

constexpr size_t ISIZE = 5000;
constexpr size_t JSIZE = 5000;

#define arr(i, j) a[(i)*JSIZE + (j)]

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx(&argc, &argv);

  std::vector<double> a(ISIZE * JSIZE);
  for (size_t i = 0; i < ISIZE; ++i)
    for (size_t j = 0; j < JSIZE; ++j)
      arr(i, j) = 10 * i + j;

  mpi::Timer tmr;
#if LINEAR
  for (size_t i = 1; i < ISIZE; ++i)
    for (size_t j = 8; j < JSIZE; ++j)
      arr(i, j) = sin(5 * arr(i - 1, j - 8));
#else
  auto rank = mpi::commRank();
  auto comm_sz = mpi::commSize();
  auto splitter = util::WorkSplitterLinear{static_cast<int>(JSIZE), comm_sz};
  auto sizes = splitter.getSizes();
  auto displs = splitter.getDisplacements();
  auto type = mpi::DatatypeSelector<double>::getHandle();

  std::vector<double> buf(sizes[rank]);
  for (size_t i = 1; i < ISIZE; ++i) {
    double *row = &a[(i - 1) * JSIZE];
    /* Scatter previous row */
    auto err = MPI_Scatterv(row, sizes.data(), displs.data(), type, buf.data(),
                            sizes[rank], type, 0, MPI_COMM_WORLD);
    assert(err == MPI_SUCCESS && "MPI error");
    /* Each worker calculates its part of the current row */
    std::transform(buf.begin(), buf.end(), buf.begin(),
                   [](double v) { return std::sin(5 * v); });
    /* Gather results, save to the current row on root */
    if (auto res = mpi::gatherv(buf)) {
      std::copy_n(res.data().data(), JSIZE - 8, &a[i * JSIZE] + 8);
    }
  }
#endif

  if (mpi::commRank() == 0) {
    std::cerr << tmr.getElapsedTimeInSeconds() << std::endl;
#if SAVE_DATA
    std::ofstream os;
    os.open("output.txt");
    for (size_t i = 0; i < ISIZE; ++i) {
      for (size_t j = 0; j < JSIZE; ++j)
        os << arr(i, j) << " ";
      os << '\n';
    }
#endif
  }
  return 0;
}
