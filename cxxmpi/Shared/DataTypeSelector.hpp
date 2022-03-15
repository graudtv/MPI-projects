#pragma once

#include "BuiltinTypeTraits.hpp"

namespace cxxmpi {

/* This class has the same semantics as BuiltinTypeTraits, but users
 * of cxxmpi (where are a lot of them, for example me, me and also me) are
 * allowed to provided specialization for their own types.
 * It is added to leave BuiltinTypeTraits pure
 *
 * Second template argument is obviously for SFINAE
 */
template <class T, class = void> struct DatatypeSelector;

template <class T>
struct DatatypeSelector<T, detail::enable_if_t<isBuiltinType<T>::value>> {
  static MPI_Datatype getHandle() { return BuiltinTypeTraits<T>::getHandle(); }
};

} // namespace cxxmpi