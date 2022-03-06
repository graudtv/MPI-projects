#pragma once

#include "impl/ArrayRef.hpp"
#include "impl/DataTypeSelector.hpp"
#include "impl/Utilities.hpp"

namespace cxxmpi {

using portal::llvm::ArrayRef;

/* These 2 are just for completeness, consider using MPIContext instead */
void init(int *argc, char ***argv) { detail::exitOnError(MPI_Init(argc, argv)); }
void finalize() { detail::exitOnError(MPI_Finalize()); }

struct MPIContext {
  MPIContext(int *argc, char ***argv) { init(argc, argv); }
  ~MPIContext() { finalize(); }
};

bool initialized() {
  int res;
  detail::exitOnError(MPI_Initialized(&res));
  return res;
}

bool finalized() {
  int res;
  detail::exitOnError(MPI_Finalized(&res));
  return res;
}

int commSize(MPI_Comm comm = MPI_COMM_WORLD) {
  int res;
  detail::exitOnError(MPI_Comm_size(comm, &res));
  return res;
}

int commRank(MPI_Comm comm = MPI_COMM_WORLD) {
  int res;
  detail::exitOnError(MPI_Comm_rank(comm, &res));
  return res;
}

double wtime() { return MPI_Wtime(); }

double wtick() { return MPI_Wtick(); }

class Timer {
public:
  Timer() : InitialTime(wtime()) {}
  double getElapsedTimeInSeconds() const { return wtime() - InitialTime; }

private:
  double InitialTime;
};

} // namespace cxxmpi