#pragma once

#include <cstring>
#include <string_view>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

inline bool copy_string(char *pDst, int nDstSize, std::string_view strSrc) {
  if (pDst == nullptr) {
    return true;
  }
  if (nDstSize <= 0 || strSrc.size() + 1 > static_cast<std::size_t>(nDstSize)) {
    return false;
  }
  std::memcpy(pDst, strSrc.data(), strSrc.size());
  pDst[strSrc.size()] = '\0';
  return true;
}

} // namespace boop

BOOP_HEADER_END
