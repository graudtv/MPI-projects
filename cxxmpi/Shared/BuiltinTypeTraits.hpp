#pragma once

#include "../Support/Utilities.hpp"

#include <mpi.h>

#include <limits>
#include <type_traits>

namespace cxxmpi {

/* cxxmpi::byte can be used in different type constructors */
enum class byte : unsigned char {};

static_assert(sizeof(byte) == 1, "invalid size of cxxmpi::byte");

/* BuiltinTypeTraits defines a mapping from C++ types
 * to predefined MPI types (MPI_CHAR, MPI_INT, etc) */
template <class T> struct BuiltinTypeTraits;

/* Implementation note.
 * Though MPI_CHAR and so on are compile time constants, in some
 * implementations they are defined as '#define MPI_CHAR ((void *) smth)'
 * C++ forbids usage of such casts in constexpr contexts, so the following
 * doesn't work:
 *   template <> struct BuiltinTypeTraits<signed char> {
 *     static constexpr MPI_Datatype handle = MPI_CHAR;
 *   }
 * Thus getHandle() is defined as a function
 */
#define DECLARE_MAPPING(type, mpitype)                                         \
  template <> struct BuiltinTypeTraits<type> {                                 \
    static MPI_Datatype getHandle() { return mpitype; }                        \
  };

DECLARE_MAPPING(char, std::numeric_limits<char>::is_signed ? MPI_CHAR
                                                           : MPI_UNSIGNED_CHAR)
DECLARE_MAPPING(signed char, MPI_CHAR)
DECLARE_MAPPING(unsigned char, MPI_UNSIGNED_CHAR)
DECLARE_MAPPING(short, MPI_SHORT)
DECLARE_MAPPING(unsigned short, MPI_UNSIGNED_SHORT)
DECLARE_MAPPING(int, MPI_INT)
DECLARE_MAPPING(unsigned, MPI_UNSIGNED)
DECLARE_MAPPING(long, MPI_LONG)
DECLARE_MAPPING(unsigned long, MPI_UNSIGNED_LONG)
DECLARE_MAPPING(long long, MPI_LONG_LONG)
DECLARE_MAPPING(unsigned long long, MPI_UNSIGNED_LONG_LONG)
DECLARE_MAPPING(float, MPI_FLOAT)
DECLARE_MAPPING(double, MPI_DOUBLE)
DECLARE_MAPPING(long double, MPI_LONG_DOUBLE)
DECLARE_MAPPING(bool, MPI_C_BOOL)
DECLARE_MAPPING(byte, MPI_BYTE)
DECLARE_MAPPING(wchar_t, MPI_WCHAR)

#undef DECLARE_MAPPING

template <class T, class = void>
struct isBuiltinType : public std::false_type {};

template <class T>
struct isBuiltinType<T, detail::void_t<decltype(BuiltinTypeTraits<T>::getHandle())>>
    : public std::true_type {};

static_assert(isBuiltinType<int>::value, "int is a builtin type");
static_assert(!isBuiltinType<void>::value, "void is not a builtin type");

} // namespace cxxmpi