#include "cxxmpi/cxxmpi.hpp"

namespace mpi = cxxmpi;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};

  int MagicNums[2] = {};
  float FPMagic = 0;

  if (mpi::commRank() == 0) {
    MagicNums[0] = 228;
    MagicNums[1] = 337;
    FPMagic = 1.111;
  }
  /* bcast using raw API */
  MPI_Bcast(MagicNums, sizeof MagicNums / sizeof MagicNums[0], MPI_INT,
            /*root=*/0, MPI_COMM_WORLD);

  /* bcast scalar */
  mpi::bcast(FPMagic, /*root=*/0);

  /* Now every process has the same values in MagicNums */
  printf("[%d/%d]: Received magic numbers: %d, %d and %lg\n", mpi::commRank() + 1,
         mpi::commSize(), MagicNums[0], MagicNums[1], FPMagic);

  return 0;
}