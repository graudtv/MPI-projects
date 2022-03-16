#pragma once

#include "../Shared/misc.hpp"
#include "../Support/Utilities.hpp"

#include <array>
#include <string>
#include <vector>

namespace cxxmpi {

Status probe(int src, int tag = MPI_ANY_TAG, MPI_Comm comm = MPI_COMM_WORLD) {
  MPI_Status res;
  detail::exitOnError(MPI_Probe(src, tag, comm, &res));
  return res;
}

/* Send scalar
 * ScalarT could be
 * - Elementary type (cxxmpi::isBuiltinType<ScalarT>::value == true)
 *   This includes int, float, double, char and most of other commonly-used
 * types.
 * - struct/class, which was adapted with CXXMPI_ADAPT_STRUCT()
 * - Anything else, for which you can explicitly provide TypeSelector.
 *   Note, that providing such is quite tricky for arrays with unknown length,
 *   so consider using send() version which takes array-like data (see below)
 *   If you need to send array of arrays, or a structure containing array,
 * consider sending multiple messages
 */
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>>
void send(const ScalarT &data, int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(
      MPI_Send(&data, 1, TypeSelector::getHandle(), dst, tag, comm));
}

/* Send std::vector
 * Note that TypeSelector selects type of array element, i.e. of scalar value */
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>,
          class Allocator>
void send(const std::vector<ScalarT, Allocator> &data, int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Send(data.data(), data.size(),
                               TypeSelector::getHandle(), dst, tag, comm));
}

/* Send std::array */
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>,
          size_t N>
void send(const std::array<ScalarT, N> &data, int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Send(data.data(), data.size(),
                               TypeSelector::getHandle(), dst, tag, comm));
}

/* Send C-array */
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>,
          size_t N>
void send(const ScalarT (&data)[N], int dst, int tag = 0,
          MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(
      MPI_Send(data, N, TypeSelector::getHandle(), dst, tag, comm));
}

/* Send std::basic_string
 * This should be used with caution, cause char/wchar representation
 * is much more likely to differ on different platforms (in comparison with
 * other types) Note. Impl relies on that basic_string stores data contigiously,
 * which must be true since C++11
 */
template <class CharT, class TypeSelector = DatatypeSelector<CharT>,
          class Traits, class Allocator>
void send(const std::basic_string<CharT, Traits, Allocator> &s, int dst,
          int tag = 0, MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(
      MPI_Send(s.data(), s.size(), TypeSelector::getHandle(), dst, tag, comm));
}

/* Send single data element with user-specified data type */
inline void send(const void *data, Datatype type, int dst, int tag = 0,
                 MPI_Comm comm = MPI_COMM_WORLD) {
  detail::exitOnError(MPI_Send(data, 1, type.getHandle(), dst, tag, comm));
}

/* receive scalar */
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>>
void recv(ScalarT &data, int src = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG,
          MPI_Comm comm = MPI_COMM_WORLD,
          MPI_Status *status = MPI_STATUS_IGNORE) {
  detail::exitOnError(
      MPI_Recv(&data, 1, TypeSelector::getHandle(), src, tag, comm, status));
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
  MPI_Datatype type = TypeSelector::getHandle();
  /* note that we must use source and tag from Status. It is necessary
   * to handle a case when source == MPI_ANY_SOURCE and/or tag == MPI_ANY_TAG,
   * because we may accidentally receive the wrong message (which
   * probably requires other buffer size) */
  detail::exitOnError(
      MPI_Recv(&data[0] + initial_sz, msg_sz, type, status.source(), status.tag(), comm, nullptr));
  return TypedStatus{status.getRaw(), type};
}

} // namespace detail

/* Receive std::vector
 * Received data is appended to data (dynamic extension policy)
 * MPI_Probe will be called before MPI_Recv to get message length
 * Number of appended elements could be checked via returned Status
 */
template <class ScalarT, class TypeSelector = DatatypeSelector<ScalarT>,
          class Allocator>
TypedStatus recv(std::vector<ScalarT, Allocator> &data,
                 int src = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG,
                 MPI_Comm comm = MPI_COMM_WORLD) {
  return detail::recvIntoExpandableContainer<TypeSelector>(data, src, tag,
                                                           comm);
}

/* Receive std::basic_string */
template <class CharT, class TypeSelector = DatatypeSelector<CharT>,
          class Traits, class Allocator>
TypedStatus recv(std::basic_string<CharT, Traits, Allocator> &data,
                 int src = MPI_ANY_SOURCE, int tag = MPI_ANY_TAG,
                 MPI_Comm comm = MPI_COMM_WORLD) {
  return detail::recvIntoExpandableContainer<TypeSelector>(data, src, tag,
                                                           comm);
}

/* receive single */
inline void recv(void *data, Datatype type, int src = MPI_ANY_SOURCE,
                 int tag = MPI_ANY_TAG, MPI_Comm comm = MPI_COMM_WORLD,
                 MPI_Status *status = MPI_STATUS_IGNORE) {
  detail::exitOnError(MPI_Recv(data, 1, type.getHandle(), src, tag, comm, status));
}

} // namespace cxxmpi