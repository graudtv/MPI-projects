#pragma once

#include <cassert>

namespace util {

bool parseInt(const char *s, int *res) {
  if(sscanf(s, "%d", res) != 1)
    return false;
  return true;
}

} // namespace util
