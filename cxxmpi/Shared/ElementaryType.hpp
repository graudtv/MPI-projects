#pragma once

#include "../Support/Utilities.hpp"
#include <mpi.h>

#include <limits>
#include <type_traits>

namespace cxxmpi {

/* ElementaryType defines a mapping from C++ types
 * to predefined MPI types (MPI_CHAR, MPI_INT, etc) */
template <class T> struct ElementaryType;

/* Implementation note.
 * Though MPI_CHAR and so on are compile time constants, in some
 * implementations they are defined as '#define MPI_CHAR ((void *) smth)'
 * C++ forbids usage of such casts in constexpr contexts, so the following
 * doesn't work:
 *   template <> struct ElementaryType<signed char> {
 *     static constexpr MPI_Datatype value = MPI_CHAR;
 *   }
 * Thus value is defined as a function
 */
#define DECLARE_MAPPING(type, mpitype)                                         \
  template <> struct ElementaryType<type> {                                    \
    static MPI_Datatype value() { return mpitype; }                            \
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
DECLARE_MAPPING(wchar_t, MPI_WCHAR)

#undef DECLARE_MAPPING

template <class T, class = void>
struct isElementaryType : public std::false_type {};

template <class T>
struct isElementaryType<T, detail::void_t<decltype(ElementaryType<T>::value())>>
    : public std::true_type {};

static_assert(isElementaryType<int>::value, "int is elementary type");
static_assert(!isElementaryType<void>::value, "void is not an elementary type");

} // namespace cxxmpi