#pragma once

#include <cstdlib>
#include <type_traits>

namespace cxxmpi {
namespace detail {

void exitOnError(int RetCode) {
  if (RetCode)
    exit(EXIT_FAILURE);
}

struct NonCopyableAndMovable {
  NonCopyableAndMovable() = default;
  ~NonCopyableAndMovable() = default;
  NonCopyableAndMovable(const NonCopyableAndMovable &other) = delete;
  NonCopyableAndMovable(NonCopyableAndMovable &&other) = delete;
  NonCopyableAndMovable &operator =(const NonCopyableAndMovable &other) = delete;
  NonCopyableAndMovable &operator =(NonCopyableAndMovable &&other) = delete;
};

/* I try to stick to C++11 because I have to work with old machine.
 * There is some handy stuff from further standards which is easy
 * to implement */

template <class...> using void_t = void;

template <bool B, class T = void>
using enable_if_t = typename std::enable_if<B, T>::type;

} // namespace detail
} // namespace cxxmpi