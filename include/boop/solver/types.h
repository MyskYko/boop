#pragma once

#include "boop/config.h"

BOOP_HEADER_START

namespace boop::solver {

  enum class Status {
    SAT,
    UNSAT,
    UNDET,
  };

} // namespace boop::solver

BOOP_HEADER_END
