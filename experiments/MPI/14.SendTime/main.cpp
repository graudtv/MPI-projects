#include "cxxmpi/cxxmpi.hpp"

namespace mpi = cxxmpi;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};
  auto Rank = mpi::commRank();
  int Dummy = 0;

  if (mpi::commSize() == 1) {
    std::cerr << ">= 2 processes expected" << std::endl;
    return EXIT_FAILURE;
  }

  constexpr int NCycles = 100000;
  mpi::Timer Tmr;

  for (int I = 0; I < NCycles; ++I) {
    if (Rank == 0) {
      mpi::send(Dummy, Rank + 1);
      mpi::recv(Dummy);
    } else {
      mpi::recv(Dummy);
      mpi::send(Dummy, (Rank + 1) % mpi::commSize());
    }
  }
  if (Rank == 0)
    std::cout << Tmr.getElapsedTimeInSeconds() / (mpi::commSize() * NCycles) * 1000000 << "us" << std::endl;
  return 0;
}