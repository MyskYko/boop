#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

BOOP_HEADER_START

namespace boop::rrr {

  enum VarValue: char {
    UNDEF,
    rrrTRUE,
    rrrFALSE,
    TEMP_TRUE,
    TEMP_FALSE
  };


} // namespace boop::rrr

BOOP_HEADER_END
