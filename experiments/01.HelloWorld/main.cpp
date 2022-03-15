#include "cxxmpi/cxxmpi.hpp"

namespace mpi = cxxmpi;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx(&argc, &argv);
  printf("commsize = %d, rank = %d\n", mpi::commSize(), mpi::commRank());
  return 0;
}