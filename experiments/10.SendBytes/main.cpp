#include "cxxmpi/cxxmpi.hpp"
#include "Support/OstreamHelpers.hpp"
#include <iostream>
#include <numeric>

namespace mpi = cxxmpi;

template <class Container> auto printvec(const Container &C) {
  return util::wrapSquareBrackets(util::join(C, ", "));
}

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};

  if (mpi::commSize() != 2) {
    std::cerr << "2 processes expected" << std::endl;
    return EXIT_FAILURE;
  }

  std::array<int32_t, 10> DataToSend;
  std::iota(DataToSend.begin(), DataToSend.end(), 0);

  std::array<int32_t, 10> RcvBuf;

  auto Ty = mpi::createContiguousType<int32_t>(10);
  // This also works:
  //// auto Ty = mpi::createContiguousType<mpi::byte>(sizeof DataToSend[0] * 10);

  mpi::TypeCommitGuard Guard{Ty};

  if (mpi::commRank() == 0) {
    std::cout << "[0]: send: " << printvec(DataToSend) << std::endl;
    mpi::send(DataToSend.data(), Ty, 1);
    return 0;
  }
  /* rank == 1 */
  mpi::recv(RcvBuf.data(), Ty);
  std::cout << "[1]: recv: " << printvec(RcvBuf) << std::endl;
  if (!std::equal(RcvBuf.begin(), RcvBuf.end(), DataToSend.begin())) {
    std::cerr << "Data corruption!" << std::endl;
    return EXIT_FAILURE;
  }
  std::cerr << "Everything is ok" << std::endl;
  return 0;
}