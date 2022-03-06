#pragma once

#include <cstdlib>

namespace cxxmpi {
namespace detail {

void exitOnError(int RetCode) {
  if (RetCode)
    exit(EXIT_FAILURE);
}

} // namespace detail
} // namespace cxxmpi