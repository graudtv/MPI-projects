/* Miscellaneous functions */

#pragma once

#include "Datatype.hpp"
#include "DatatypeSelector.hpp"
#include <iostream>

namespace cxxmpi {

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

/* Prefer using Status interface */
int getCount(const MPI_Status &s, MPI_Datatype type) {
  int res;
  detail::exitOnError(MPI_Get_count(&s, type, &res));
  return res;
}

/* Wrapper for MPI_Status */
class Status {
public:
  Status() = default;
  Status(MPI_Status s) : status(std::move(s)) {}

  int source() const { return status.MPI_SOURCE; }
  int tag() const { return status.MPI_TAG; }
  int error() const { return status.MPI_ERROR; }

  /* If you specify the wrong type you will probably get smth wrong.
   * If possible, use TypedStatus::getCount() */
  template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>>
  size_t getCountAs() const {
    return ::cxxmpi::getCount(status, TypeSelector::getHandle());
  }

  MPI_Status getRaw() const { return status; }
  MPI_Status &getRawRef() { return status; }

protected:
  MPI_Status status;
};

/* TypedStatus knows his data type and thus can acquire element count without
 * any additional information */
class TypedStatus final : public Status {
public:
  TypedStatus(MPI_Status s, MPI_Datatype t)
      : Status(std::move(s)), datatype(t) {}

  size_t getCount() const { return ::cxxmpi::getCount(status, datatype); }
  Datatype type() const { return Datatype{datatype}; }

private:
  MPI_Datatype datatype;
};

class Timer {
public:
  Timer() : InitialTime(wtime()) {}
  double getElapsedTimeInSeconds() const { return wtime() - InitialTime; }
  void reset() { InitialTime = wtime(); }

private:
  double InitialTime;
};

/* Small ostream manip for convenience 
 * Example:
 * std::cout << whoami << ": Hello, world!" << std::endl;
 */
std::ostream &whoami(std::ostream &Os) {
  return Os << '[' << commRank() + 1 << '/' << commSize() << "]";
}

} // namespace cxxmpi