#pragma once

#include "../Shared/misc.hpp"
#include "../Util/WorkSplitter.hpp"

#include <cassert>

namespace cxxmpi {

void barrier(MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Barrier(comm));
}

/* bcast scalar */
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>>
void bcast(ScalarT &data, int root, MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Bcast(&data, 1, TypeSelector::getHandle(), root, comm));
}

/* If current process was the root in cxxmpi::gather(), GatherResult
 * contains the gathered data and isValid() == true.
 * Otherwise, i.e. for all other processes, it contains empty vector
 * and isValid() == false
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
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>>
GatherResult<ScalarT> gather(ScalarT &value_to_send, int root = 0,
                             MPI_Comm comm = MPI_COMM_WORLD) {
  std::vector<ScalarT> result;
  auto type = TypeSelector::getHandle();
  bool is_root = (commRank(comm) == root);

  /* allocate space on the root */
  if (is_root)
    result.resize(commSize(comm));

  detail::exitOnError(
      MPI_Gather(&value_to_send, 1, type, &result[0], 1, type, root, comm));
  return is_root ? GatherResult<ScalarT>{std::move(result)}
                 : GatherResult<ScalarT>{};
}

/* Scatters the data as much fairly as possible, i.e. all processes will
 * get approximatelly the same amount of data
 * data argument is taken into account only for process with rank == root.
 * For all other processes it must be empty due to debug simplification */
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>>
std::vector<ScalarT> scatterFair(std::vector<ScalarT> &data, size_t data_sz,
                                 int root, MPI_Comm comm = MPI_COMM_WORLD) {
  // TODO: optimize a case when data can be scattered evenly,
  // i.e. plain MPI_Scatter can be used (data.size() % comm_sz == 0)
  auto rank = commRank(comm);
  auto comm_sz = commSize(comm);
  if (rank == root) {
    assert(data.size() == data_sz &&
           "passed data_sz value must match the size of the passed vector");
  } else {
    assert(data.size() == 0 && "data must be empty for non-root procesees");
  }

  auto splitter = util::WorkSplitterLinear{static_cast<int>(data_sz), comm_sz};
  auto sizes = splitter.getSizes();
  auto displs = splitter.getDisplacements();
  auto type = TypeSelector::getHandle();
  std::vector<ScalarT> result(sizes[rank]);

  detail::exitOnError(MPI_Scatterv(&data[0], &sizes[0], &displs[0], type,
                                   &result[0], sizes[rank], type, root, comm));
  return result;
}

} // namespace cxxmpi