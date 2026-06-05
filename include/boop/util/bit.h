#pragma once

#include <limits>
#include <bit>
#include <cassert>

BOOP_HEADER_START

namespace boop {

  constexpr int clog2(int n) {
    assert(n > 0);
    if(n <= 1) {
      return 0;
    }
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
    return std::bit_width(n - 1);
#else
    int r = 0;
    --n;
    while(n > 0) {
      n >>= 1;
      ++r;
    }
    return r;
#endif
  }
  
  constexpr int pow2_ceil(int n) {
    assert(n >= 0);
    if(n == 0) {
      return 0;
    }
#if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
    unsigned r = std::bit_ceil(static_cast<unsigned>(n));
#else
    unsigned r = 1;
    while(r < static_cast<unsigned>(n)) {
      r <<= 1;
    }
#endif
    assert(r <= static_cast<unsigned>(std::numeric_limits<int>::max()));
    return static_cast<int>(r);
  }

} // namespace boop

BOOP_HEADER_END
