#include "cxxmpi/cxxmpi.hpp"

namespace mpi = cxxmpi;

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};

  auto Rank = mpi::commRank();
  std::string Str(Rank + 1, Rank + '1');
  std::cout << mpi::whoami << ": " << Str << std::endl;

  if (auto Res = mpi::gatherv(Str)) {
    std::string ResStr = Res.takeData();
    std::cout << "root: " << ResStr << std::endl;
  }

  return 0;
}