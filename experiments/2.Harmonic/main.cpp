#include "Support/Algorithm.hpp"
#include "Support/Parsing.hpp"
#include "cxxmpi/cxxmpi.hpp"
#include <iostream>
#include <unistd.h>

namespace mpi = cxxmpi;

int handleManager(int N) {
  mpi::Timer T;
  double Summ = 0;

  for (int I = 0; I < mpi::commSize() - 1; ++I) {
    double Res = 0;
    mpi::recv(Res);
    Summ += Res;
  }
  printf("I am manager, N = %d, ElapsedTime = %fs, Result = %lg\n", N,
         T.getElapsedTimeInSeconds(), Summ);
  return 0;
}

int handleExecutor(int N) {
  mpi::Timer T;
  auto R = mpi::commRank();
  auto P = mpi::commSize() - 1; // -1 - because of manager proc
  auto WorkRange = util::WorkSplitterLinear(N, P).getRange(R).shift(1);

  double Summ = 0;
  for (int I = WorkRange.FirstIdx; I < WorkRange.LastIdx; ++I)
    Summ += 1.0 / I;

  printf("Executor %d/%d: WorkSize = %d, Indices = [%d; %d], ElapsedTime = %f, "
         "CalcResult = %lg\n",
         R + 1, P, WorkRange.size(), WorkRange.FirstIdx, WorkRange.LastIdx - 1,
         T.getElapsedTimeInSeconds(), Summ);

  /* Send result to manager */
  mpi::send(Summ, P);
  return 0;
}

void emitUsageError(const char *progname) {
  std::cerr << "Usage: " << progname << " N" << std::endl;
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  mpi::MPIContext Ctx{&argc, &argv};

  const char *progname = argv[0];
  if (argc != 2)
    emitUsageError(progname);

  int N;
  if (!util::parseInt(argv[1], &N) || N <= 0) {
    std::cerr << "Error: N must be a positive integer" << std::endl;
    emitUsageError(progname);
  }

  if (mpi::commSize() == 1) {
    std::cerr
        << "Error: not enough workers allocated (at least 2 threads required)"
        << std::endl;
    exit(EXIT_FAILURE);
  }

  if (mpi::commRank() == mpi::commSize() - 1)
    return handleManager(N);
  return handleExecutor(N);
}