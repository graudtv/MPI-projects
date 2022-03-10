#pragma once

#include "ElementaryType.hpp"

namespace cxxmpi {

/* This class has the same semantics as ElementaryType, but users
 * of cxxmpi (where are a lot of them, for example me, me and also me) are
 * allowed to provided specialization for their own types.
 * It is added to leave ElemtaryType pure
 *
 * Second template argument is obviously for SFINAE
 */
template <class T, class = void> struct DataTypeSelector;

template <class T>
struct DataTypeSelector<T, detail::enable_if_t<isElementaryType<T>::value>> {
  static MPI_Datatype value() { return ElementaryType<T>::value(); }
};

} // namespace cxxmpi