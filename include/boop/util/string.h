#pragma once

#include <cstring>
#include <string_view>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

  inline bool CopyString(char *dst, int dst_size, std::string_view src) {
    if(dst == nullptr) {
      return true;
    }
    if(dst_size <= 0 || src.size() + 1 > static_cast<std::size_t>(dst_size)) {
      return false;
    }
    std::memcpy(dst, src.data(), src.size());
    dst[src.size()] = '\0';
    return true;
  }

} // namespace boop

BOOP_HEADER_END
