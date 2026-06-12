#pragma once

#include <cassert>
#include <functional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "boop/config.h"
#include "boop/network/types.h"
#include "boop/rrr/types.h"
#include "boop/solver/types.h"
#include "boop/util/print.h"
#include "boop/util/time.h"
#include "boop/util/types.h"

BOOP_HEADER_START

namespace boop::rrr {

template <typename Ntk, typename Sol> class SatBasedSolver {
public:
  struct Parameter {
    int nVerbose = 0;
    int nConflictLimit = 0;
  };

  enum class Result {
    SAT,
    UNSAT,
    UNDET,
  };

  // lifecycle
  SatBasedSolver(const Parameter &par);
  void AssignNetwork(Ntk *pNtk, bool fReuse);
  void SetPrintLine(std::function<void(const std::string &)> fnPrintLine);

  // checks
  Result CheckRedundancy(int nId, int nIdx);
  Result CheckFeasibility(int nId, int nFi, bool fCompl);

  // cex
  void Justify(std::vector<VarValue> &vValues, int nId);
  std::vector<VarValue> GetCex();

  // stats
  void ResetSummary();
  Summary<int> GetStatsSummary() const;
  Summary<Duration> GetTimesSummary() const;

private:
  Ntk *pNtk_;
  const Parameter par_;
  std::function<void(const std::string &)> fnPrintLine_;

  Sol sol_;
  bool fTrivialUnsat_;
  bool fNoDontCare_;
  int nTarget_;
  std::vector<int> vVarLits_;
  std::vector<int> vVarLitsInv_;
  std::vector<int> vLits_;
  std::vector<VarValue> vValues_;
  std::vector<VarValue> vValuesInv_;
  bool fUpdate_;

  int nCalls_;
  int nSats_;
  int nUnsats_;
  Duration durationRedundancy_;
  Duration durationFeasibility_;

  // print
  template <typename... Args> void Print(int nVerboseLevel, Args &&...args);

  // callback
  void ActionCallback(const Action &action);

  // encode
  void EncodeNode(std::vector<int> &vVarLits, int nId);
  void EncodeMiter();
};

// lifecycle

template <typename Ntk, typename Sol>
SatBasedSolver<Ntk, Sol>::SatBasedSolver(const Parameter &par)
    : pNtk_(nullptr), par_(par), fTrivialUnsat_(false), fNoDontCare_(false),
      nTarget_(-1), fUpdate_(false) {
  ResetSummary();
}

template <typename Ntk, typename Sol>
void SatBasedSolver<Ntk, Sol>::AssignNetwork(Ntk *pNtk, bool fReuse) {
  (void)fReuse;
  fTrivialUnsat_ = false;
  fNoDontCare_ = false;
  nTarget_ = -1;
  fUpdate_ = false;
  pNtk_ = pNtk;
  pNtk_->AddCallback(std::bind(&SatBasedSolver<Ntk, Sol>::ActionCallback, this,
                               std::placeholders::_1));
}

template <typename Ntk, typename Sol>
void SatBasedSolver<Ntk, Sol>::SetPrintLine(
    std::function<void(const std::string &)> fnPrintLine) {
  fnPrintLine_ = std::move(fnPrintLine);
}

// checks

template <typename Ntk, typename Sol>
typename SatBasedSolver<Ntk, Sol>::Result
SatBasedSolver<Ntk, Sol>::CheckRedundancy(int nId, int nIdx) {
  TimePoint timeStart = get_current_time();
  if (fUpdate_ || nId != nTarget_) {
    fUpdate_ = false;
    nTarget_ = nId;
    EncodeMiter();
  }
  if (fTrivialUnsat_) {
    Print(0, "trivially UNSATISFIABLE");
    durationRedundancy_ += get_duration(timeStart, get_current_time());
    return Result::UNSAT;
  }
  vLits_.clear();
  assert(pNtk_->GetNodeType(nId) == AND);
  pNtk_->template ForEachFanin<true, true, false>(
      nId, [&](int nIdx2, int nFi, bool fCompl) {
        if (nIdx == nIdx2) {
          vLits_.push_back(sol_.NotCond(vVarLits_[nFi], !fCompl));
        } else {
          vLits_.push_back(sol_.NotCond(vVarLits_[nFi], fCompl));
        }
      });
  Print(0, "solving with assumptions:", vLits_);
  nCalls_++;
  sol_.SetConflictLimit(par_.nConflictLimit);
  solver::Status status = sol_.Solve(&vLits_, nullptr);
  if (status == solver::Status::UNSAT) {
    Print(0, "UNSATISFIABLE");
    nUnsats_++;
    durationRedundancy_ += get_duration(timeStart, get_current_time());
    return Result::UNSAT;
  }
  if (status == solver::Status::UNDET) {
    Print(0, "UNDETERMINED");
    durationRedundancy_ += get_duration(timeStart, get_current_time());
    return Result::UNDET;
  }
  assert(status == solver::Status::SAT);
  Print(0, "SATISFIABLE");
  nSats_++;
  vValues_.clear();
  vValues_.resize(pNtk_->GetNumNodes());
  pNtk_->ForEachPi([&](int nId) {
    vValues_[nId] = sol_.Value(vVarLits_[nId]) ? TEMP_TRUE : TEMP_FALSE;
  });
  vValuesInv_ = vValues_;
  pNtk_->ForEachInt([&](int nId) {
    vValues_[nId] = sol_.Value(vVarLits_[nId]) ? TEMP_TRUE : TEMP_FALSE;
    vValuesInv_[nId] = sol_.Value(vVarLitsInv_[nId]) ? TEMP_TRUE : TEMP_FALSE;
  });
  pNtk_->template ForEachFanin<true, true, false>(
      nId, [&](int nIdx2, int nFi, bool fCompl) {
        assert((vValues_[nFi] == TEMP_TRUE) ^ (nIdx == nIdx2) ^ fCompl);
        vValues_[nFi] = DecideVarValue(vValues_[nFi]);
        vValuesInv_[nFi] = DecideVarValue(vValuesInv_[nFi]);
      });
  durationRedundancy_ += get_duration(timeStart, get_current_time());
  return Result::SAT;
}

template <typename Ntk, typename Sol>
typename SatBasedSolver<Ntk, Sol>::Result
SatBasedSolver<Ntk, Sol>::CheckFeasibility(int nId, int nFi, bool fCompl) {
  TimePoint timeStart = get_current_time();
  if (fUpdate_ || nId != nTarget_) {
    fUpdate_ = false;
    nTarget_ = nId;
    EncodeMiter();
  }
  if (fTrivialUnsat_) {
    Print(0, "trivially UNSATISFIABLE");
    durationFeasibility_ += get_duration(timeStart, get_current_time());
    return Result::UNSAT;
  }
  vLits_.clear();
  assert(pNtk_->GetNodeType(nId) == AND);
  vLits_.push_back(vVarLits_[nId]);
  vLits_.push_back(sol_.NotCond(vVarLits_[nFi], !fCompl));
  Print(0, "solving with assumptions:", vLits_);
  nCalls_++;
  sol_.SetConflictLimit(par_.nConflictLimit);
  solver::Status status = sol_.Solve(&vLits_, nullptr);
  if (status == solver::Status::UNSAT) {
    Print(0, "UNSATISFIABLE");
    nUnsats_++;
    durationFeasibility_ += get_duration(timeStart, get_current_time());
    return Result::UNSAT;
  }
  if (status == solver::Status::UNDET) {
    Print(0, "UNDETERMINED");
    durationFeasibility_ += get_duration(timeStart, get_current_time());
    return Result::UNDET;
  }
  assert(status == solver::Status::SAT);
  Print(0, "SATISFIABLE");
  nSats_++;
  vValues_.clear();
  vValues_.resize(pNtk_->GetNumNodes());
  pNtk_->ForEachPi([&](int nId) {
    vValues_[nId] = sol_.Value(vVarLits_[nId]) ? TEMP_TRUE : TEMP_FALSE;
  });
  vValuesInv_ = vValues_;
  pNtk_->ForEachInt([&](int nId) {
    vValues_[nId] = sol_.Value(vVarLits_[nId]) ? TEMP_TRUE : TEMP_FALSE;
    vValuesInv_[nId] = sol_.Value(vVarLitsInv_[nId]) ? TEMP_TRUE : TEMP_FALSE;
  });
  assert(vValues_[nId] == TEMP_TRUE);
  assert(vValuesInv_[nId] == TEMP_FALSE);
  vValues_[nId] = DecideVarValue(vValues_[nId]);
  vValuesInv_[nId] = DecideVarValue(vValuesInv_[nId]);
  assert((vValues_[nFi] == TEMP_TRUE) ^ !fCompl);
  assert((vValuesInv_[nFi] == TEMP_TRUE) ^ !fCompl);
  vValues_[nFi] = DecideVarValue(vValues_[nFi]);
  vValuesInv_[nFi] = DecideVarValue(vValuesInv_[nFi]);
  durationFeasibility_ += get_duration(timeStart, get_current_time());
  return Result::SAT;
}

// cex

template <typename Ntk, typename Sol>
void SatBasedSolver<Ntk, Sol>::Justify(std::vector<VarValue> &vValues,
                                       int nId) {
  switch (pNtk_->GetNodeType(nId)) {
  case AND:
    if (vValues[nId] == rrrTRUE) {
      pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
        assert((vValues[nFi] == TEMP_TRUE || vValues[nFi] == rrrTRUE) ^ fCompl);
        vValues[nFi] = DecideVarValue(vValues[nFi]);
      });
    } else if (vValues[nId] == rrrFALSE) {
      bool fFound = false;
      pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
        if (fFound) {
          return;
        }
        if (fCompl) {
          if (vValues[nFi] == rrrTRUE) {
            fFound = true;
          }
        } else {
          if (vValues[nFi] == rrrFALSE) {
            fFound = true;
          }
        }
      });
      if (!fFound) {
        pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
          if (fFound) {
            return;
          }
          if (fCompl) {
            if (vValues[nFi] == TEMP_TRUE) {
              fFound = true;
              vValues[nFi] = DecideVarValue(vValues[nFi]);
            }
          } else {
            if (vValues[nFi] == TEMP_FALSE) {
              fFound = true;
              vValues[nFi] = DecideVarValue(vValues[nFi]);
            }
          }
        });
      }
    }
    break;
  default:
    assert(0);
  }
}

template <typename Ntk, typename Sol>
std::vector<VarValue> SatBasedSolver<Ntk, Sol>::GetCex() {
  if (par_.nVerbose) {
    std::stringstream ss;
    pNtk_->ForEachPi([&](int nId) { ss << GetVarValueChar(vValues_[nId]); });
    Print(0, "cex:", ss.str());
  }
  // reverse simulation
  if (fNoDontCare_) {
    pNtk_->template ForEachInt<true>([&](int nId) { Justify(vValues_, nId); });
  } else {
    // pick po
    bool fFound = false;
    pNtk_->ForEachPoDriver([&](int nFi) {
      if (vVarLits_[nFi] != vVarLitsInv_[nFi] &&
          vValues_[nFi] != vValuesInv_[nFi]) {
        vValues_[nFi] = DecideVarValue(vValues_[nFi]);
        vValuesInv_[nFi] = DecideVarValue(vValuesInv_[nFi]);
        fFound = true;
        return true;
      }
      return false;
    });
    assert(fFound);
    // observability
    std::vector<bool> vVisited(pNtk_->GetNumNodes());
    pNtk_->template ForEachTfo<false, true, false, true>(
        nTarget_, [&](int nFo) {
          vVisited[nFo] = true;
          Justify(vValues_, nFo);
          Justify(vValuesInv_, nFo);
        });
    assert(vValues_[nTarget_] == rrrTRUE || vValues_[nTarget_] == rrrFALSE);
    assert(vValuesInv_[nTarget_] == rrrTRUE ||
           vValuesInv_[nTarget_] == rrrFALSE);
    // justify
    pNtk_->template ForEachInt<true>([&](int nId) {
      if (vVisited[nId]) {
        return;
      }
      if (vValuesInv_[nId] == rrrTRUE || vValuesInv_[nId] == rrrFALSE) {
        assert(nId == nTarget_ ||
               vValuesInv_[nId] == DecideVarValue(vValues_[nId]));
        vValues_[nId] = DecideVarValue(vValues_[nId]);
      }
      Justify(vValues_, nId);
    });
  }
  if (par_.nVerbose) {
    std::stringstream ss;
    pNtk_->ForEachPi([&](int nId) { ss << GetVarValueChar(vValues_[nId]); });
    Print(0, "pex:", ss.str());
  }
  // debug
  pNtk_->ForEachInt([&](int nId) {
    switch (pNtk_->GetNodeType(nId)) {
    case AND:
      if (vValues_[nId] == rrrTRUE) {
        pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
          assert(!fCompl || vValues_[nFi] == rrrFALSE);
          assert(fCompl || vValues_[nFi] == rrrTRUE);
        });
      } else if (vValues_[nId] == rrrFALSE) {
        bool fFound = false;
        pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
          if (fFound) {
            return;
          }
          if (fCompl) {
            if (vValues_[nFi] == rrrTRUE) {
              fFound = true;
            }
          } else {
            if (vValues_[nFi] == rrrFALSE) {
              fFound = true;
            }
          }
        });
        assert(fFound);
      }
      break;
    default:
      assert(0);
    }
  });
  // retrieve partial cex
  std::vector<VarValue> vPartialCex;
  pNtk_->ForEachPi([&](int nId) {
    if (vValues_[nId] == rrrTRUE || vValues_[nId] == rrrFALSE) {
      vPartialCex.push_back(vValues_[nId]);
    } else {
      vPartialCex.push_back(UNDEF);
    }
  });
  return vPartialCex;
}

// stats

template <typename Ntk, typename Sol>
void SatBasedSolver<Ntk, Sol>::ResetSummary() {
  nCalls_ = 0;
  nSats_ = 0;
  nUnsats_ = 0;
  durationRedundancy_ = 0;
  durationFeasibility_ = 0;
}

template <typename Ntk, typename Sol>
Summary<int> SatBasedSolver<Ntk, Sol>::GetStatsSummary() const {
  Summary<int> summary;
  summary.emplace_back("sat call", nCalls_);
  summary.emplace_back("sat satisfiable", nSats_);
  summary.emplace_back("sat unsatisfiable", nUnsats_);
  return summary;
}

template <typename Ntk, typename Sol>
Summary<Duration> SatBasedSolver<Ntk, Sol>::GetTimesSummary() const {
  Summary<Duration> summary;
  summary.emplace_back("sat redundancy", durationRedundancy_);
  summary.emplace_back("sat feasibility", durationFeasibility_);
  return summary;
}

// print

template <typename Ntk, typename Sol>
template <typename... Args>
void SatBasedSolver<Ntk, Sol>::Print(int nVerboseLevel, Args &&...args) {
  if (fnPrintLine_ && par_.nVerbose > nVerboseLevel) {
    std::stringstream ss;
    for (int i = 0; i < nVerboseLevel; i++) {
      ss << "\t";
    }
    print_next(ss, std::forward<Args>(args)...);
    fnPrintLine_(ss.str());
  }
}

// callback

template <typename Ntk, typename Sol>
void SatBasedSolver<Ntk, Sol>::ActionCallback(const Action &action) {
  if (nTarget_ == -1) {
    return;
  }
  switch (action.type) {
  case REMOVE_FANIN:
    if (action.id != nTarget_) {
      fUpdate_ = true;
    }
    break;
  case REMOVE_UNUSED:
    break;
  case REMOVE_BUFFER:
  case REMOVE_CONST:
    if (action.id == nTarget_) {
      nTarget_ = -1;
    }
    break;
  case ADD_FANIN:
    if (action.id != nTarget_) {
      fUpdate_ = true;
    }
    break;
  case TRIVIAL_COLLAPSE:
    break;
  case TRIVIAL_DECOMPOSE:
    fUpdate_ = true; // necessary if function or don't care of new edge needs to
                     // be considered
    break;
  case SORT_FANINS:
    break;
  case READ:
    fTrivialUnsat_ = false;
    fNoDontCare_ = false;
    nTarget_ = -1;
    fUpdate_ = false;
    break;
  case SAVE:
    break;
  case LOAD:
    nTarget_ = -1;
    break;
  case POP_BACK:
    break;
  default:
    assert(0);
  }
}

// encode

template <typename Ntk, typename Sol>
void SatBasedSolver<Ntk, Sol>::EncodeNode(std::vector<int> &vVarLits, int nId) {
  vVarLits[nId] = sol_.one;
  assert(pNtk_->GetNodeType(nId) == AND);
  pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
    if (vVarLits[nId] == sol_.one) {
      vVarLits[nId] = sol_.NotCond(vVarLits[nFi], fCompl);
    } else {
      int nLit = sol_.NotCond(vVarLits[nFi], fCompl);
      int nLitNew = sol_.logic.And2(vVarLits[nId], nLit);
      Print(1, "node", nId, ":", nLitNew, "=", vVarLits[nId], "&", nLit);
      assert(!sol_.IsInconsistent());
      vVarLits[nId] = nLitNew;
    }
  });
}

template <typename Ntk, typename Sol>
void SatBasedSolver<Ntk, Sol>::EncodeMiter() {
  vVarLits_.clear();
  sol_.Clear();
  fTrivialUnsat_ = false;
  fNoDontCare_ = false;
  vVarLits_.resize(pNtk_->GetNumNodes());
  vVarLits_[pNtk_->GetConst0()] = sol_.zero;
  pNtk_->ForEachPi([&](int nId) { vVarLits_[nId] = sol_.NewVar(); });
  Print(0, "encoding network");
  pNtk_->ForEachInt([&](int nId) { EncodeNode(vVarLits_, nId); });
  vVarLitsInv_ = vVarLits_;
  vVarLitsInv_[nTarget_] = sol_.Compl(vVarLitsInv_[nTarget_]);
  if (pNtk_->IsPoDriver(nTarget_)) {
    fNoDontCare_ = true;
    return;
  }
  Print(0, "encoding an inverted copy");
  pNtk_->template ForEachTfo<false, true, true, false>(
      nTarget_, [&](int nId) { EncodeNode(vVarLitsInv_, nId); });
  Print(0, "encoding miter xors");
  vLits_.clear();
  pNtk_->ForEachPoDriver([&](int nFi) {
    assert(nFi != nTarget_);
    if (vVarLits_[nFi] != vVarLitsInv_[nFi]) {
      int nLit = sol_.logic.Xor2(vVarLits_[nFi], vVarLitsInv_[nFi]);
      Print(1, nLit, "=", vVarLits_[nFi], "^", vVarLitsInv_[nFi]);
      assert(!sol_.IsInconsistent());
      vLits_.push_back(nLit);
    }
  });
  Print(0, "adding miter output clause");
  Print(1, vLits_);
  if (vLits_.empty()) {
    fTrivialUnsat_ = true;
    return;
  }
  sol_.AddClause(vLits_);
  assert(!sol_.IsInconsistent());
}

} // namespace boop::rrr

BOOP_HEADER_END
