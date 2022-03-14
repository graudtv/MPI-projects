#pragma once

#include "../Shared/DataTypeSelector.hpp"
#include "../Shared/misc.hpp"
#include "../Support/Utilities.hpp"
#include <cassert>

namespace cxxmpi {

void barrier(MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Barrier(comm));
}

/* bcast scalar */
template <class ScalarT, class TypeSelector = DataTypeSelector<ScalarT>>
void bcast(ScalarT &data, int root, MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Bcast(&data, 1, TypeSelector::value(), root, comm));
}

/* If current process was the root in cxxmpi::gather(), GatherResult
 * contains the gather data and isValid() == true.
 * Otherwise, i.e. for all other processes except the one, it
 * contains empty vector and isValid() == false
 *
 * It enables the following syntax:
 *
 * int ValueToSend = someComplexComputation(); // executed by each process
 * if (auto Result = mpi::gather(ValueToSend, 0)) {
 *   // only process with rank = 0 (root) will enter with block
 *   std::vector<int> Data = Result.takeData();
 *   ... // do smth with data
 * }
 */
template <class T> class GatherResult {
public:
  /* Check if current process was the root in gather */
  bool isValid() const { return IsValid; }
  operator bool() const { return isValid(); }

  std::vector<T> takeData() {
    assertValid();
    return std::move(Data);
  }
  std::vector<T> &data() {
    assertValid();
    return Data;
  }
  const std::vector<T> &data() const {
    assertValid();
    return Data;
  }

  GatherResult() : IsValid(false) {}
  GatherResult(std::vector<T> Vec) : Data(std::move(Vec)), IsValid(true) {}

  GatherResult(const GatherResult &Other) = delete;
  GatherResult &operator=(const GatherResult &Other) = delete;

  GatherResult(GatherResult &&Other) = default;
  GatherResult &operator=(GatherResult &&Other) = default;

private:
  std::vector<T> Data;
  bool IsValid;

  void assertValid() {
    assert(isValid() && "Trying to use gather results from the wrong process");
  }
};

/* Gather scalar
 * See description of GatherResult above */
template <class ScalarT, class TypeSelector = DataTypeSelector<ScalarT>>
GatherResult<ScalarT> gather(ScalarT &value_to_send, int root = 0,
                             MPI_Comm comm = MPI_COMM_WORLD) {
  std::vector<ScalarT> result;
  auto type = TypeSelector::value();
  bool is_root = (commRank(comm) == root);

  /* allocate space on the root */
  if (is_root)
    result.resize(commSize(comm));

  detail::exitOnError(
      MPI_Gather(&value_to_send, 1, type, &result[0], 1, type, root, comm));
  return is_root ? GatherResult<ScalarT>{std::move(result)}
                 : GatherResult<ScalarT>{};
}

} // namespace cxxmpi