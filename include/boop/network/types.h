#pragma once

#include <cassert>
#include <string>
#include <sstream>
#include <vector>

#include "boop/config.h"
#include "boop/util/print.h"

BOOP_HEADER_START

namespace boop {
  
  enum NodeType {
    PI,
    PO,
    AND,
    XOR,
    LUT
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

  static inline char const *GetActionTypeCstr(Action const &action) {
    switch(action.type) {
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
    std::string delim = " : ";
    if(action.id != -1) {
      ss << delim;
      PrintNext(ss, "node", action.id);
      delim = " , ";
    }
    if(action.fi != -1) {
      ss << delim;
      PrintNext(ss, "fanin", static_cast<bool>(action.c), action.fi);
      delim = " , ";
    }
    if(action.idx != -1) {
      ss << delim;
      PrintNext(ss, "index", action.idx);
    }
    ss << std::endl;
    if(!action.vFanins.empty()) {
      ss << "fanins : ";
      PrintNext(ss, action.vFanins);
    }
    if(!action.vIndices.empty()) {
      ss << "indices : ";
      PrintNext(ss, action.vIndices);
    }
    if(!action.vFanouts.empty()) {
      ss << "fanouts : ";
      PrintNext(ss, action.vFanouts);
    }
    return ss;
  }

} // namespace boop

BOOP_HEADER_END
