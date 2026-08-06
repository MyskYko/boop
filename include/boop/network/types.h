#pragma once

#include <cassert>
#include <sstream>
#include <string>
#include <vector>

#include "boop/config.h"
#include "boop/util/print.h"

BOOP_HEADER_START

namespace boop {

enum NodeType { CONST, PI, PO, AND, XOR, LUT, CELL, BLACK_BOX };

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
  int nId = -1;
  int nIdx = -1;
  int nFi = -1;
  bool fCompl = false;
  std::vector<int> vFanins;
  std::vector<int> vIndices;
  std::vector<int> vFanouts;
};

static inline char const *GetActionTypeCstr(Action const &action) {
  switch (action.type) {
  case REMOVE_FANIN:
    return "remove fanin";
  case REMOVE_UNUSED:
    return "remove unused";
  case REMOVE_BUFFER:
    return "remove buffer";
  case REMOVE_CONST:
    return "remove const";
  case ADD_FANIN:
    return "add fanin";
  case TRIVIAL_COLLAPSE:
    return "trivial collapse";
  case TRIVIAL_DECOMPOSE:
    return "trivial decompose";
  case SORT_FANINS:
    return "sort fanins";
  case READ:
    return "read";
  case SAVE:
    return "save";
  case LOAD:
    return "load";
  case POP_BACK:
    return "pop back";
  case INSERT:
    return "insert";
  default:
    assert(0);
  }
  return "";
}

static inline std::stringstream GetActionDescription(Action const &action) {
  std::stringstream ss;
  ss << GetActionTypeCstr(action);
  std::string strDelim = " : ";
  if (action.nId != -1) {
    ss << strDelim;
    print_next(ss, "node", action.nId);
    strDelim = " , ";
  }
  if (action.nFi != -1) {
    ss << strDelim;
    print_next(ss, "fanin", static_cast<bool>(action.fCompl), action.nFi);
    strDelim = " , ";
  }
  if (action.nIdx != -1) {
    ss << strDelim;
    print_next(ss, "index", action.nIdx);
  }
  ss << std::endl;
  if (!action.vFanins.empty()) {
    ss << "fanins : ";
    print_next(ss, action.vFanins);
  }
  if (!action.vIndices.empty()) {
    ss << "indices : ";
    print_next(ss, action.vIndices);
  }
  if (!action.vFanouts.empty()) {
    ss << "fanouts : ";
    print_next(ss, action.vFanouts);
  }
  return ss;
}

} // namespace boop

BOOP_HEADER_END
