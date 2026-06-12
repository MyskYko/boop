#pragma once

#include <algorithm>
#include <bitset>
#include <climits>
#include <cstddef>
#include <iterator>
#include <sstream>
#include <string>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop::vec_ops {

template <typename It> inline void Clear(int n, It it) {
  using T = typename std::iterator_traits<It>::value_type;
  std::fill(it, it + n, T{0});
}

template <typename It> inline void Fill(int n, It it) {
  using T = typename std::iterator_traits<It>::value_type;
  std::fill(it, it + n, ~T{0});
}

template <typename ItDst, typename ItSrc>
inline void Copy(int n, ItDst itDst, ItSrc itSrc, bool fCompl) {
  if (!fCompl) {
    for (int i = 0; i < n; i++, ++itDst, ++itSrc) {
      *itDst = *itSrc;
    }
  } else {
    for (int i = 0; i < n; i++, ++itDst, ++itSrc) {
      *itDst = ~(*itSrc);
    }
  }
}

template <typename ItDst, typename ItSrc0, typename ItSrc1>
inline void And(int n, ItDst itDst, ItSrc0 itSrc0, ItSrc1 itSrc1, bool fCompl0,
                bool fCompl1) {
  if (!fCompl0) {
    if (!fCompl1) {
      for (int i = 0; i < n; i++, ++itDst, ++itSrc0, ++itSrc1) {
        *itDst = (*itSrc0) & (*itSrc1);
      }
    } else {
      for (int i = 0; i < n; i++, ++itDst, ++itSrc0, ++itSrc1) {
        *itDst = (*itSrc0) & ~(*itSrc1);
      }
    }
  } else {
    if (!fCompl1) {
      for (int i = 0; i < n; i++, ++itDst, ++itSrc0, ++itSrc1) {
        *itDst = ~(*itSrc0) & (*itSrc1);
      }
    } else {
      for (int i = 0; i < n; i++, ++itDst, ++itSrc0, ++itSrc1) {
        *itDst = ~(*itSrc0) & ~(*itSrc1);
      }
    }
  }
}

template <typename ItDst, typename ItSrc0, typename ItSrc1>
inline void Xor(int n, ItDst itDst, ItSrc0 itSrc0, ItSrc1 itSrc1,
                bool fCompl1) {
  if (!fCompl1) {
    for (int i = 0; i < n; i++, ++itDst, ++itSrc0, ++itSrc1) {
      *itDst = (*itSrc0) ^ (*itSrc1);
    }
  } else {
    for (int i = 0; i < n; i++, ++itDst, ++itSrc0, ++itSrc1) {
      *itDst = (*itSrc0) ^ ~(*itSrc1);
    }
  }
}

template <typename It> inline bool IsZero(int n, It it, bool fCompl) {
  if (!fCompl) {
    for (int i = 0; i < n; i++, ++it) {
      if (*it) {
        return false;
      }
    }
  } else {
    for (int i = 0; i < n; i++, ++it) {
      if (~(*it)) {
        return false;
      }
    }
  }
  return true;
}

template <typename It, typename T>
inline bool IsZero(int n, It it, bool fCompl, T mask) {
  if (mask == ~T{0}) {
    return IsZero(n, it, fCompl);
  }
  if (n <= 0) {
    return true;
  }
  if (!IsZero(n - 1, it, fCompl)) {
    return false;
  }
  std::advance(it, n - 1);
  return !((fCompl ? ~(*it) : *it) & mask);
}

template <typename ItX, typename ItY>
inline bool IsEq(int n, ItX itX, ItY itY, bool fCompl) {
  if (!fCompl) {
    for (int i = 0; i < n; i++, ++itX, ++itY) {
      if (*itX != *itY) {
        return false;
      }
    }
  } else {
    for (int i = 0; i < n; i++, ++itX, ++itY) {
      if (*itX != ~(*itY)) {
        return false;
      }
    }
  }
  return true;
}

template <typename ItX, typename ItY, typename T>
inline bool IsEq(int n, ItX itX, ItY itY, bool fCompl, T mask) {
  if (mask == ~T{0}) {
    return IsEq(n, itX, itY, fCompl);
  }
  if (n <= 0) {
    return true;
  }
  if (!IsEq(n - 1, itX, itY, fCompl)) {
    return false;
  }
  std::advance(itX, n - 1);
  std::advance(itY, n - 1);
  return !(((*itX) ^ (fCompl ? ~(*itY) : *itY)) & mask);
}

template <typename It> inline std::stringstream GetStringStream(int n, It it) {
  constexpr std::size_t nBits =
      sizeof(typename std::iterator_traits<It>::value_type) * CHAR_BIT;
  std::stringstream ss;
  std::string strDelim;
  for (int i = 0; i < n; i++, ++it) {
    ss << strDelim << std::bitset<nBits>(*it);
    strDelim = "\n";
  }
  return ss;
}

template <typename It, typename T>
inline std::stringstream GetStringStream(int n, It it, T mask) {
  constexpr std::size_t nBits =
      sizeof(typename std::iterator_traits<It>::value_type) * CHAR_BIT;
  std::stringstream ss = GetStringStream(n - 1, it);
  if (n <= 0) {
    return ss;
  }
  if (n > 1) {
    ss << "\n";
  }
  std::advance(it, n - 1);
  ss << std::bitset<nBits>((*it) & mask);
  return ss;
}

} // namespace boop::vec_ops

BOOP_HEADER_END
