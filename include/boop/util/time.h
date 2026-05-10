#pragma once

#include <cassert>
#include <chrono>

#include "boop/util/types.h"

BOOP_HEADER_START

namespace boop {
  
  static inline TimePoint GetCurrentTime() {
    return Clock::now();
  }
  
  static inline Seconds GetDurationInSeconds(TimePoint start, TimePoint end) {
    Seconds t = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();
    assert(t >= 0);
    return t;
  }
  
  static inline Duration GetDuration(TimePoint start, TimePoint end) {
    Duration t = std::chrono::duration<Duration>(end - start).count();
    assert(t >= 0);
    return t;
  }

} // namespace boop

BOOP_HEADER_END
