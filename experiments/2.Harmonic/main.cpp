#include "Support/Algorithm.hpp"
#include "Support/Parsing.hpp"
#include "cxxmpi/cxxmpi.hpp"
#include <iomanip>
#include <iostream>
#include <unistd.h>

namespace mpi = cxxmpi;

int handleManager(int N, bool VerboseModeEnabled = true) {
  mpi::Timer T;
  double Summ = 0;

  for (int I = 0; I < mpi::commSize() - 1; ++I) {
    double Res = 0;
    mpi::recv(Res);
    Summ += Res;
  }
  if (VerboseModeEnabled) {
    printf("I am manager, N = %d, ElapsedTime = %fs, Result = %lg\n", N,
           T.getElapsedTimeInSeconds(), Summ);
  } else {
    std::cout << std::setprecision(std::numeric_limits<long double>::digits10 +
                                   1)
              << Summ << std::endl;
  }
  return 0;
}

int handleExecutor(int N, bool VerboseModeEnabled = true) {
  mpi::Timer T;
  auto R = mpi::commRank();
  auto P = mpi::commSize() - 1; // -1 - because of manager proc
  auto WorkRange = util::WorkSplitterLinear(N, P).getRange(R).shift(1);

  double Summ = 0;
  for (int I = WorkRange.FirstIdx; I < WorkRange.LastIdx; ++I)
    Summ += 1.0 / I;

  if (VerboseModeEnabled) {
    printf(
        "Executor %d/%d: WorkSize = %d, Indices = [%d; %d], ElapsedTime = %f, "
        "CalcResult = %lg\n",
        R + 1, P, WorkRange.size(), WorkRange.FirstIdx, WorkRange.LastIdx - 1,
        T.getElapsedTimeInSeconds(), Summ);
  }

  /* Send result to manager */
  mpi::send(Summ, P);
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

  if (mpi::commSize() == 1) {
    std::cerr
        << "Error: not enough workers allocated (at least 2 threads required)"
        << std::endl;
    exit(EXIT_FAILURE);
  }

  if (mpi::commRank() == mpi::commSize() - 1)
    return handleManager(N, VerboseModeEnabled);
  return handleExecutor(N, VerboseModeEnabled);
}