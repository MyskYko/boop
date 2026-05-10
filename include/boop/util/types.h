#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <cassert>

BOOP_HEADER_START

namespace boop {

  using Seconds = int64_t;
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  using Duration = double;
  using Cost = double;

  template <typename T>
  using Summary = std::vector<std::pair<std::string, T>>;

} // namespace boop

BOOP_HEADER_END
