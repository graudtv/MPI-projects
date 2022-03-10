#pragma once

#include "../Shared/DataTypeSelector.hpp"
#include "../Support/Utilities.hpp"

#include <array>
#include <vector>
#include <string>

namespace cxxmpi {

Status probe(int src, int tag = MPI_ANY_TAG, MPI_Comm comm = MPI_COMM_WORLD) {
  MPI_Status res;
  detail::exitOnError(MPI_Probe(src, tag, comm, &res));
  return res;
}

/* Send scalar
 * ScalarT could be
 * - Elementary type (cxxmpi::isElementaryType<ScalarT>::value == true)
 * - struct/class, which was adapted with CXXMPI_ADAPT_STRUCT()
 * - Anything else, for which you can explicitly provide TypeSelector.
 *   Note, that providing such is quite tricky for arrays with unknown length,
 *   so consider using send() version which takes array-like data (see below)
 *   If you need to send array of arrays, or a structure containing array,
 * consider sending multiple messages
 */
template <class ScalarT, class TypeSelector = DataTypeSelector<ScalarT>>
void send(const ScalarT &data, int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(
      MPI_Send(&data, 1, TypeSelector::value(), dst, tag, comm));
}

/* Send std::vector
 * Note that TypeSelector selects type of array element, i.e. of scalar value */
template <class ScalarT, class TypeSelector = DataTypeSelector<ScalarT>,
          class Allocator>
void send(const std::vector<ScalarT, Allocator> &data, int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Send(data.data(), data.size(), TypeSelector::value(),
                               dst, tag, comm));
}

/* Send std::array */
template <class ScalarT, class TypeSelector = DataTypeSelector<ScalarT>,
          size_t N>
void send(const std::array<ScalarT, N> &data, int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Send(data.data(), data.size(), TypeSelector::value(),
                               dst, tag, comm));
}

/* Send C-array */
template <class ScalarT, class TypeSelector = DataTypeSelector<ScalarT>,
          size_t N>
void send(const ScalarT (&data)[N], int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Send(data, N, TypeSelector::value(), dst, tag, comm));
}

/* Send std::basic_string
 * This should be used with caution, cause char/wchar representation
 * is much more likely to differ on different platforms (comparing to other
 * types)
 * Note. Impl relies on that basic_string stores data contigiously,
 * which must be true since C++11
 */
template <class CharT, class TypeSelector = DataTypeSelector<CharT>,
          class Traits, class Allocator>
void send(const std::basic_string<CharT, Traits, Allocator> &s, int dst,
          int tag = 0, MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(
      MPI_Send(s.data(), s.size(), TypeSelector::value(), dst, tag, comm));
}

/* receive scalar */
template <class T, class TypeSelector = DataTypeSelector<T>>
void recv(T &data, int src = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG,
          MPI_Comm comm = MPI_COMM_WORLD,
          MPI_Status *status = MPI_STATUS_IGNORE) {
  detail::exitOnError(
      MPI_Recv(&data, 1, TypeSelector::value(), src, tag, comm, status));
}

namespace detail {

template <class TypeSelector, class Container>
TypedStatus recvIntoExpandableContainer(Container &data, int src, int tag,
                                        MPI_Comm comm) {
  using ScalarT = typename Container::value_type;

  auto initial_sz = data.size();

  /* wait + expand */
  Status status = probe(src, tag, comm);
  size_t msg_sz = status.getCountAs<ScalarT, TypeSelector>();
  data.resize(data.size() + msg_sz);
  /* receive data */
  MPI_Datatype type = TypeSelector::value();
  detail::exitOnError(MPI_Recv(&data[0] + initial_sz, msg_sz, type, src, tag,
                               comm, nullptr));
  return TypedStatus{status.getRaw(), type};
}

} // namespace detail

/* Receive std::vector
 * Received data is appended to data (dynamic extension policy)
 * MPI_Probe will be called before MPI_Recv to get message length
 * Number of appended elements could be checked via returned Status
 */
template <class ScalarT, class TypeSelector = DataTypeSelector<ScalarT>,
          class Allocator>
TypedStatus recv(std::vector<ScalarT, Allocator> &data,
                 int src = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG,
                 MPI_Comm comm = MPI_COMM_WORLD) {
  return detail::recvIntoExpandableContainer<TypeSelector>(data, src, tag,
                                                           comm);
}

/* Receive std::basic_string */
template <class CharT, class TypeSelector = DataTypeSelector<CharT>,
          class Traits, class Allocator>
TypedStatus recv(std::basic_string<CharT, Traits, Allocator> &data,
                 int src = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG,
                 MPI_Comm comm = MPI_COMM_WORLD) {
  return detail::recvIntoExpandableContainer<TypeSelector>(data, src, tag,
                                                           comm);
}

} // namespace cxxmpi