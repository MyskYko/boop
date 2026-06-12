#pragma once

#include <cassert>
#include <chrono>

#include "boop/config.h"
#include "boop/util/types.h"

BOOP_HEADER_START

namespace boop {

static inline TimePoint get_current_time() { return Clock::now(); }

static inline Seconds get_duration_in_seconds(TimePoint timeStart,
                                              TimePoint timeEnd) {
  Seconds nSeconds =
      std::chrono::duration_cast<std::chrono::seconds>(timeEnd - timeStart)
          .count();
  assert(nSeconds >= 0);
  return nSeconds;
}

static inline Duration get_duration(TimePoint timeStart, TimePoint timeEnd) {
  Duration duration =
      std::chrono::duration<Duration>(timeEnd - timeStart).count();
  assert(duration >= 0);
  return duration;
}

} // namespace boop

BOOP_HEADER_END
