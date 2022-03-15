#include "Support/Parsing.hpp"
#include "cxxmpi/cxxmpi.hpp"
#include <iomanip>
#include <iostream>
#include <numeric>

namespace mpi = cxxmpi;

int computeHarmonic(int N, bool VerboseModeEnabled) {
  mpi::Timer T;
  auto Rank = mpi::commRank();
  auto CommSz = mpi::commSize();
  auto WorkRange = util::WorkSplitterLinear(N, CommSz).getRange(Rank).shift(1);

  double Summ = 0;
  for (int I = WorkRange.FirstIdx; I < WorkRange.LastIdx; ++I)
    Summ += 1.0 / I;

  if (VerboseModeEnabled) {
    printf(
        "Executor %d/%d: WorkSize = %d, Indices = [%d; %d], ElapsedTime = %f, "
        "CalcResult = %lg\n",
        Rank + 1, CommSz, WorkRange.size(), WorkRange.FirstIdx,
        WorkRange.LastIdx - 1, T.getElapsedTimeInSeconds(), Summ);
  }

  if (auto GatherResult = mpi::gather(Summ)) {
    auto Data = GatherResult.takeData();

    /* These is the result we wanted to compute */
    double UltimateResult = std::accumulate(Data.begin(), Data.end(), 0.0);
    if (VerboseModeEnabled) {
      printf("I am manager, N = %d, ElapsedTime = %fs, Result = %lg\n", N,
             T.getElapsedTimeInSeconds(), UltimateResult);
    } else {
      std::cout << std::setprecision(
                       std::numeric_limits<long double>::digits10 + 1)
                << UltimateResult << std::endl;
    }
  }
  return 0;
}

void emitUsageError(const char *ProgName) {
  fprintf(stderr,
          "Usage: mpirun ... %s [OPTIONS] N\n"
          "Descr: this program calculates result of 1 + 1/2 + 1/3 + 1/4 + ... "
          "+ 1/N\n"
          "OPTIONS:\n"
          "    -q      -- print only result on success\n",
          ProgName);
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};
  bool VerboseModeEnabled = true;

  const char *Progname = argv[0];
  ++argv, --argc;

  if (argc >= 1 && strcmp(argv[0], "-q") == 0) {
    VerboseModeEnabled = false;
    ++argv;
    --argc;
  }
  if (argc != 1) {
    std::cerr << "Error: N argument expected" << std::endl;
    emitUsageError(Progname);
  }

  int N;
  if (!util::parseInt(argv[0], &N) || N <= 0) {
    std::cerr << "Error: N must be a positive integer" << std::endl;
    emitUsageError(Progname);
  }

  return computeHarmonic(N, VerboseModeEnabled);
}