#pragma once

#include <mpi.h>

namespace cxxmpi {

template <class T> struct DataTypeSelector {};

#define DECLARE_MAPPING(type, mpitype)                                         \
  template <> struct DataTypeSelector<type> {                                  \
    static MPI_Datatype value() { return mpitype; }                            \
  };

DECLARE_MAPPING(char, MPI_CHAR)
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

#undef DECLARE_MAPPING

} // namespace cxxmpi