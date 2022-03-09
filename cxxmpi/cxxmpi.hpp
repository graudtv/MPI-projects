#pragma once

#include "impl/ArrayRef.hpp"
#include "impl/DataTypeSelector.hpp"
#include "impl/Utilities.hpp"
#include <type_traits>

namespace cxxmpi {

using namespace portal::llvm;

/* These 2 are just for completeness, consider using MPIContext instead */
void init(int *argc, char ***argv) {
  detail::exitOnError(MPI_Init(argc, argv));
}
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

template <class T, class TypeSelector = DataTypeSelector<T>>
void send(ArrayRef<T> data, int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Send(data.data(), data.size(), TypeSelector::value(),
                               dst, tag, comm));
}

template <class T, class TypeSelector = DataTypeSelector<T>>
void send(const T &data, int dst, int tag = 0, MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(
      MPI_Send(&data, 1, TypeSelector::value(), dst, tag, comm));
}

template <class T, class TypeSelector = DataTypeSelector<T>>
void recv(MutableArrayRef<T> data, int src = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG,
          MPI_Comm comm = MPI_COMM_WORLD,
          MPI_Status *status = MPI_STATUS_IGNORE) {
  detail::exitOnError(MPI_Recv(data.data(), data.size(), TypeSelector::value(),
                               src, tag, comm, status));
}

template <class T, class TypeSelector = DataTypeSelector<T>>
void recv(T &data, int src = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG,
          MPI_Comm comm = MPI_COMM_WORLD,
          MPI_Status *status = MPI_STATUS_IGNORE) {
  detail::exitOnError(
      MPI_Recv(&data, 1, TypeSelector::value(), src, tag, comm, status));
}

class Timer {
public:
  Timer() : InitialTime(wtime()) {}
  double getElapsedTimeInSeconds() const { return wtime() - InitialTime; }

private:
  double InitialTime;
};

} // namespace cxxmpi