#include "Support/OstreamHelpers.hpp"
#include "cxxmpi/cxxmpi.hpp"
#include <numeric>

namespace mpi = cxxmpi;

template <class Printable> auto printvec(const Printable &P) {
  return util::wrapSquareBrackets(util::join(P, ", "));
}

constexpr size_t ArraySz = 40;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};
  auto Rank = mpi::commRank();
  auto CommSz = mpi::commSize();

  auto S = util::WorkSplitterLinear{ArraySz, CommSz};
  auto Sizes = S.getSizes();
  auto Displs = S.getDisplacements();
  assert(Sizes.size() == CommSz);
  assert(Displs.size() == CommSz);

  /* Curly brackets must not be used here!!! */
  std::vector<int> RcvBuf(Sizes[Rank]);

  /* Almost zero memory cost for every process (except root)
   * Data is put only if Rank == 0 */
  std::vector<int> SndBuf;

  if (Rank == 0) {
    SndBuf.resize(ArraySz);
    std::iota(SndBuf.begin(), SndBuf.end(), 0);
    std::cout << mpi::whoami << ": [root]: sizes: " << printvec(Sizes)
              << std::endl;
    std::cout << mpi::whoami << ": [root]: displacements: " << printvec(Displs)
              << std::endl;
    std::cout << mpi::whoami << ": [root]: sending data: " << printvec(SndBuf)
              << std::endl;
  }
  MPI_Scatterv(&SndBuf[0], &Sizes[0], &Displs[0], MPI_INT, &RcvBuf[0],
               Sizes[Rank], MPI_INT, 0, MPI_COMM_WORLD);

  std::cout << mpi::whoami << ": received data: " << printvec(RcvBuf)
            << std::endl;

  return 0;
}