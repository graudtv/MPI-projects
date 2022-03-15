#include "Support/OstreamHelpers.hpp"
#include "cxxmpi/cxxmpi.hpp"
#include <numeric>

namespace mpi = cxxmpi;

template <class Container> auto printvec(const Container &C) {
  return util::wrapSquareBrackets(util::join(C, ", "));
}

constexpr size_t ArraySz = 40;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};

  auto Rank = mpi::commRank();
  constexpr int RootIdx = 0;

  std::vector<int> SndBuf;
  if (Rank == RootIdx) {
    SndBuf.resize(ArraySz);
    std::iota(SndBuf.begin(), SndBuf.end(), 0);
    std::cout << mpi::whoami << ": [root]: sending data: " << printvec(SndBuf)
              << std::endl;
  }
  auto RcvBuf = mpi::scatterFair(SndBuf, ArraySz, RootIdx);
  std::cout << mpi::whoami << ": received data: " << printvec(RcvBuf)
            << std::endl;

  return 0;
}