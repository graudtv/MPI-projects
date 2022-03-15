#pragma once

#include "BuiltinTypeTraits.hpp"
#include <cassert>

namespace cxxmpi {

/* Represents MPI_Datatype, both builtin and user-defined
 *
 * It is user's responsibility to ensure that commit() and free()
 * calls for underlying type are valid, i.e. commit() is not called for
 * builtin type or free() is not called twice. Wherever possible,
 * usage of cxxmpi::TypeCommitGuard is encouraged to commit user-defined
 * types safely
 */
class Datatype {
public:
  Datatype() : type(MPI_DATATYPE_NULL) {}
  Datatype(MPI_Datatype t) : type(t) { assertNotNull(); }

  MPI_Datatype getHandle() const { return type; }

  bool isNull() const { return type == MPI_DATATYPE_NULL; }

  void commit() {
    assertNotNull();
    detail::exitOnError(MPI_Type_commit(&type));
  }

  void free() {
    assertNotNull();
    detail::exitOnError(MPI_Type_free(&type));
  }

private:
  MPI_Datatype type;

  void assertNotNull() const {
    assert(!isNull() && "Type is not allowed to be null here");
  }
};

/* RAII wrapper for Datatype::commit() and Datatype::free() */
class TypeCommitGuard {
public:
  TypeCommitGuard(Datatype t) : type(t) { type.commit(); }
  ~TypeCommitGuard() { type.free(); }

  TypeCommitGuard(const TypeCommitGuard &other) = delete;
  TypeCommitGuard(TypeCommitGuard &&other) = delete;
  TypeCommitGuard &operator=(const TypeCommitGuard &other) = delete;
  TypeCommitGuard &operator=(TypeCommitGuard &&other) = delete;

private:
  Datatype type;
};

/* get builtin types: int, float, ... */
template <class BuiltinT> Datatype getBuiltinType() {
  return BuiltinTypeTraits<BuiltinT>::getHandle();
}

Datatype createContiguousType(Datatype old_type, size_t count) {
  MPI_Datatype new_type;
  detail::exitOnError(MPI_Type_contiguous(static_cast<int>(count),
                                          old_type.getHandle(), &new_type));
  return Datatype{new_type};
}

template <class BuiltinT> Datatype createContiguousType(size_t count) {
  return createContiguosType(getBuiltinType<BuiltinT>(), count);
}

} // namespace cxxmpi