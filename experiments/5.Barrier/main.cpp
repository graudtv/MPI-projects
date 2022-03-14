#include "cxxmpi/cxxmpi.hpp"
#include <iostream>
#include <random>

namespace mpi = cxxmpi;

constexpr size_t Min = 5e7;
constexpr size_t Max = 5e9;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};

  std::default_random_engine Rnd{std::random_device{}()};
  size_t Count = std::uniform_int_distribution<size_t>{Min, Max}(Rnd);

  printf("[%d/%d]: counting till %zu\n", mpi::commRank() + 1, mpi::commSize(), Count);
  int Dummy = 0;
  for (size_t I = 0; I < Count; ++I)
    ++Dummy;
  printf("[%d/%d]: ready\n", mpi::commRank() + 1, mpi::commSize());

  mpi::barrier();
  /* same: 
   * MPI_Barrier(MPI_COMM_WORLD); */
  printf("[%d/%d]: now I known that everybody is ready!\n", mpi::commRank() + 1, mpi::commSize());

  return 0;
}