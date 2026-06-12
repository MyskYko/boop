#pragma once

#include <cassert>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop::rrr {

enum VarValue : char { UNDEF, rrrTRUE, rrrFALSE, TEMP_TRUE, TEMP_FALSE };

static inline VarValue DecideVarValue(VarValue x) {
  switch (x) {
  case UNDEF:
    assert(0);
  case rrrTRUE:
    return rrrTRUE;
  case rrrFALSE:
    return rrrFALSE;
  case TEMP_TRUE:
    return rrrTRUE;
  case TEMP_FALSE:
    return rrrFALSE;
  default:
    assert(0);
  }
  return UNDEF;
}

static inline char GetVarValueChar(VarValue x) {
  switch (x) {
  case UNDEF:
    return 'x';
  case rrrTRUE:
    return '1';
  case rrrFALSE:
    return '0';
  case TEMP_TRUE:
    return 't';
  case TEMP_FALSE:
    return 'f';
  default:
    assert(0);
  }
  return 'X';
}

} // namespace boop::rrr

BOOP_HEADER_END
