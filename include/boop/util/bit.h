#pragma once

#include <bit>
#include <cassert>
#include <limits>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

constexpr int clog2(int n) {
  assert(n > 0);
  if (n <= 1) {
    return 0;
  }
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
  return std::bit_width(n - 1);
#else
  int nResult = 0;
  --n;
  while (n > 0) {
    n >>= 1;
    ++nResult;
  }
  return nResult;
#endif
}

constexpr int pow2_ceil(int n) {
  assert(n >= 0);
  if (n == 0) {
    return 0;
  }
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
  unsigned uResult = std::bit_ceil(static_cast<unsigned>(n));
#else
  unsigned uResult = 1;
  while (uResult < static_cast<unsigned>(n)) {
    uResult <<= 1;
  }
#endif
  assert(uResult <= static_cast<unsigned>(std::numeric_limits<int>::max()));
  return static_cast<int>(uResult);
}

} // namespace boop

BOOP_HEADER_END
