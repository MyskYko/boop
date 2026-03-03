#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

BOOP_HEADER_START

namespace boop {

  using Seconds = int64_t;
  using Clock = std::chrono::steady_clock;
  using TimePoint = std::chrono::time_point<Clock>;

  template <typename T>
  using Summary = std::vector<std::pair<std::string, T>>;

  using Duration = double;
  using Cost = double;

  enum NodeType {
    PI,
    PO,
    AND,
    XOR,
    LUT
  };

  enum SatResult {
    SAT,
    UNSAT,
    UNDET
  };

  enum VarValue: char {
    UNDEF,
    rrrTRUE,
    rrrFALSE,
    TEMP_TRUE,
    TEMP_FALSE
  };

  enum ActionType {
    NONE,
    REMOVE_FANIN,
    REMOVE_UNUSED,
    REMOVE_BUFFER,
    REMOVE_CONST,
    ADD_FANIN,
    TRIVIAL_COLLAPSE,
    TRIVIAL_DECOMPOSE,
    TRIVIAL_SHARE,
    SORT_FANINS,
    READ,
    SAVE,
    LOAD,
    POP_BACK,
    INSERT
  };

  struct Action {
    ActionType type = NONE;
    int id = -1;
    int idx = -1;
    int fi = -1;
    bool c = false;
    std::vector<int> vFanins;
    std::vector<int> vIndices;
    std::vector<int> vFanouts;
  };

} // namespace boop

BOOP_HEADER_END
