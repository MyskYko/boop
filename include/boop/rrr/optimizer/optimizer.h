#pragma once

#include <functional>
#include <iterator>
#include <map>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "boop/util/util.h"

BOOP_HEADER_START

namespace boop::rrr {

  // NOTE: it is assumed that trivial collapse/decompose does not change cost,
  //       and trivial fanin ordering change may happen even with fChanged = false

  template <typename Ntk, typename Ana>
  class Optimizer {
  public:
    struct Parameter {
      int nVerbose = 0;
      int nSortType = -1;
      bool fSortInitial = false;
      bool fSortPerNode = true;
      int nFlow = 0;
      int nReductionMethod = 0;
      int nDistance = 0;
      bool fCompatible = false;
      bool fGreedy = true;
      bool fNonlinearCost = false;
      typename Ana::Parameter parAna;
    };

    // lifecycle
    Optimizer(const Parameter &par, std::function<Cost(Ntk *)> fnObjective);
    void AssignNetwork(Ntk *pNtk, bool fReuse = false);
    void SetPrintLine(std::function<void(const std::string &)> fnPrintLine);

    // run
    void Run(int nSeed = 0, Seconds nTimeout = 0);
    void SetNumSamples(int nSamples);
    void SetNumTargets(int nTargets);
    void SetNumMax(int nMax);

    // summary
    void ResetSummary();
    Summary<int> GetStatsSummary() const;
    Summary<Duration> GetTimesSummary() const;
    
  private:
    struct Stats {
      int nTriedFis = 0;
      int nAddedFis = 0;
      int nTried = 0;
      int nAdded = 0;
      int nChanged = 0;
      int nUps = 0;
      int nEqs = 0;
      int nDowns = 0;
      Duration durationAdd = 0;
      Duration durationReduce = 0;

      void Reset() {
        nTriedFis = 0;
        nAddedFis = 0;
        nTried = 0;
        nAdded = 0;
        nChanged = 0;
        nUps = 0;
        nEqs = 0;
        nDowns = 0;
        durationAdd = 0;
        durationReduce = 0;
      }

      Stats &operator+=(const Stats &other) {
        nTriedFis += other.nTriedFis;
        nAddedFis += other.nAddedFis;
        nTried    += other.nTried;
        nAdded    += other.nAdded;
        nChanged  += other.nChanged;
        nUps      += other.nUps;
        nEqs      += other.nEqs;
        nDowns    += other.nDowns;
        durationAdd    += other.durationAdd;
        durationReduce += other.durationReduce;
        return *this;
      }

      void RecordChange(Cost cost, Cost costNew) {
        nChanged++;
        if(costNew < cost) {
          nDowns++;
        } else if (costNew == cost) {
          nEqs++;
        } else {
          nUps++;
        }
      }
      
      std::string GetString() const {
        std::stringstream ss;
        PrintNext(ss, "tried node/fanin", "=", nTried, "/", nTriedFis, ",",
                  "added node/fanin", "=", nAdded, "/", nAddedFis, ",",
                  "changed", "=", nChanged, ",",
                  "up/eq/dn", "=", nUps, "/", nEqs, "/", nDowns);
        return ss.str();
      }
    };
    
    Ntk *pNtk_;

    const Parameter par_;
    int nSortType_;
    Seconds nTimeout_; // assigned upon Run
    std::function<Cost(Ntk *)> fnObjective_;
    std::function<void(const std::string &)> fnPrintLine_;

    int nSamples_;
    int nTargets_;
    int nMax_;

    Ana ana_;
    std::mt19937 rng_;
    std::vector<int> vTmp_;
    std::map<int, std::set<int>> mNewFanins_;
    TimePoint timeStart_;

    std::vector<int> vRandPiOrder_;
    std::vector<double> vRandCosts_;

    int nTarget_;
    std::vector<bool> vTfoMarks_;

    std::map<std::string, Stats> mStats_;
    Stats statsLocal_;

    // print
    template <typename... Args>
    void Print(int nVerboseLevel, Args &&...args);
    
    // callback
    void ActionCallback(const Action &action);

    // topology
    void MarkTfo(int nId);

    // time
    bool Timeout();

    // sort fanins
    void SetRandPiOrder();
    void SetRandCosts();
    void SortFanins(int nId);
    void SortFanins();

    // remove fanins
    bool RemoveRedundantFanins(int nId, bool fRemoveUnused = false);
    bool RemoveRedundantFaninsRandom(int nId, bool fRemoveUnused = false);

    // remove redundancy
    bool RemoveRedundancyOneTraversal(bool fRandom, bool fSubRoutine = false);
    bool RemoveRedundancy(bool fRandom);
    
    // reduce
    bool Reduce();

    // addition
    template <typename T>
    T SingleAdd(int nId, T begin, T end);
    int MultiAdd(int nId, const std::vector<int> &vCands);
    void Undo(int nSlot);

    // resub
    void ResubCleanup(int nId, int nSlot, bool fTried, bool fAdded);
    void SingleResub(int nId, const std::vector<int> &vCands);
    void MultiResub(int nId, const std::vector<int> &vCands);
    bool SingleResubStop(int nId, const std::vector<int> &vCands);
    bool MultiResubStop(int nId, const std::vector<int> &vCands);

    // multi-target resub
    void MultiTargetTrivialCollapse(std::vector<int> &vTargets);
    bool MultiTargetAdd(const std::vector<int> &vTargets, bool fRandomAddition);
    void MultiTargetResubCleanup(const std::vector<int> &vTargets, int nSlot, bool fTried, bool fAdded);
    void MultiTargetResub(std::vector<int> vTargets, bool fRandomAddition);
    bool MultiTargetResubStop(std::vector<int> vTargets, bool fRandomAddition);

    // apply
    void ApplyReverseTopologically(const std::function<bool(int)> &fn);
    void ApplyRandomly(const std::function<bool(int)> &fn);
    void ApplyCombinationRandomly(int k, const std::function<bool(const std::vector<int> &)> &fn);
    void ApplyCombinationSampled(int k, const std::function<bool(const std::vector<int> &)> &fn);
    void ApplyMultisetSampled(int k, const std::function<bool(const std::vector<int> &)> &fn);

    // run helper
    template <typename ApplyFn, typename ResubFn>
    void RunHelper(ApplyFn &&fnApply, const std::vector<ResubFn> &vResubs, const std::vector<std::string> &vNames, bool fStopAtChange, bool fRandomAddition);
  };

  // lifecycle

  template <typename Ntk, typename Ana>
  Optimizer<Ntk, Ana>::Optimizer(const Parameter &par, std::function<Cost(Ntk *)> fnObjective)
    : pNtk_(nullptr),
      par_(par),
      fnObjective_(std::move(fnObjective)),
      nSortType_(par.nSortType),
      nSamples_(-1),
      nTargets_(-1),
      nMax_(-1),
      ana_(par.parAna),
      nTarget_(-1) {
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::AssignNetwork(Ntk *pNtk, bool fReuse) {
    pNtk_ = pNtk;
    nSamples_ = -1;
    nTargets_ = -1;
    nMax_ = -1;
    nTarget_ = -1;
    pNtk_->AddCallback([this](const Action &action) {
      ActionCallback(action);
    });
    ana_.AssignNetwork(pNtk_, fReuse);
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SetPrintLine(std::function<void(const std::string &)> fnPrintLine) {
    ana_.SetPrintLine(fnPrintLine);
    fnPrintLine_ = std::move(fnPrintLine);
  }
  
  // run

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::Run(int nSeed, Seconds nTimeout) {
    rng_.seed(nSeed);
    vRandPiOrder_.clear();
    vRandCosts_.clear();
    if(par_.nSortType < 0) {
      nSortType_ = rng_() % 18;
      Print(0, "fanin cost function =", nSortType_);
    }
    nTimeout_ = nTimeout;
    timeStart_ = GetCurrentTime();
    if(par_.fSortInitial) {
      SortFanins();
    }
    Reduce();
    switch(par_.nFlow) {
    case 0:
      RunHelper([&](auto &&fn) { ApplyReverseTopologically(std::forward<decltype(fn)>(fn)); },
                { [&](int nId, const std::vector<int> &vCands) { SingleResub(nId, vCands); return true; } },
                {"single"}, false, false);
      break;
    case 1:
      RunHelper([&](auto &&fn) { ApplyReverseTopologically(std::forward<decltype(fn)>(fn)); },
                { [&](int nId, const std::vector<int> &vCands) { MultiResub(nId, vCands); return true; } },
                {"multi"}, false, false);
      break;
    case 2: {
      Cost cost = fnObjective_(pNtk_);
      while(true) {
        RunHelper([&](auto &&fn) { ApplyReverseTopologically([&](int nId) { fn(nId); }); },
                  { [&](int nId, const std::vector<int> &vCands) { SingleResub(nId, vCands); return true; } },
                  {"single"}, false, false);
        RunHelper([&](auto &&fn) { ApplyReverseTopologically(std::forward<decltype(fn)>(fn)); },
                  { [&](int nId, const std::vector<int> &vCands) { MultiResub(nId, vCands); return true; } },
                  {"multi"}, false, false);
        Cost newCost = fnObjective_(pNtk_);
        if(newCost < cost) {
          cost = newCost;
        } else {
          break;
        }
      }
      break;
    }
    case 3:
      RunHelper([&](auto &&fn) { ApplyRandomly(std::forward<decltype(fn)>(fn)); },
                { [&](int nId, const std::vector<int> &vCands) { return SingleResubStop(nId, vCands); },
                  [&](int nId, const std::vector<int> &vCands) { return MultiResubStop(nId, vCands); } },
                {"single", "multi"}, true, true);
      break;
    case 4:
      RunHelper([&](auto &&fn) { ApplyRandomly(std::forward<decltype(fn)>(fn)); },
                { [&](int nId, const std::vector<int> &vCands) { return SingleResubStop(nId, vCands); } },
                {"single"}, true, true);
      break;
    case 5:
      RunHelper([&](auto &&fn) { ApplyRandomly(std::forward<decltype(fn)>(fn)); },
                { [&](int nId, const std::vector<int> &vCands) { return MultiResubStop(nId, vCands); } },
                {"multi"}, true, true);
      break;
    case 6:
      statsLocal_.Reset();
      ApplyMultisetSampled(nTargets_, [&](const std::vector<int> &vTargets) {
        MultiTargetResub(vTargets, true);
        return false;
      });
      mStats_["mt"] += statsLocal_;
      Print(0, "mt", ":", statsLocal_.GetString());
      break;
    case 7:
      statsLocal_.Reset();
      ApplyMultisetSampled(nTargets_, [&](const std::vector<int> &vTargets) {
        return MultiTargetResubStop(vTargets, true);
      });
      mStats_["mt"] += statsLocal_;
      Print(0, "mt", ":", statsLocal_.GetString());
      break;
    default:
      assert(0);
    }
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SetNumSamples(int nSamples) {
    nSamples_ = nSamples;
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SetNumTargets(int nTargets) {
    nTargets_ = nTargets;
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SetNumMax(int nMax) {
    nMax_ = nMax;
  }

  // summary
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ResetSummary() {
    mStats_.clear();
    ana_.ResetSummary();
  }

  template <typename Ntk, typename Ana>
  Summary<int> Optimizer<Ntk, Ana>::GetStatsSummary() const {
    Summary<int> summary;
    for(const auto &entry : mStats_) {
      summary.emplace_back("opt " + entry.first + " tried node", entry.second.nTried);
      summary.emplace_back("opt " + entry.first + " tried fanin", entry.second.nTriedFis);
      summary.emplace_back("opt " + entry.first + " added node", entry.second.nAdded);
      summary.emplace_back("opt " + entry.first + " added fanin", entry.second.nAddedFis);
      summary.emplace_back("opt " + entry.first + " changed", entry.second.nChanged);
      summary.emplace_back("opt " + entry.first + " up", entry.second.nUps);
      summary.emplace_back("opt " + entry.first + " eq", entry.second.nEqs);
      summary.emplace_back("opt " + entry.first + " dn", entry.second.nDowns);
    }
    Summary<int> summary2 = ana_.GetStatsSummary();
    summary.insert(summary.end(), summary2.begin(), summary2.end());
    return summary;
  }

  template <typename Ntk, typename Ana>
  Summary<Duration> Optimizer<Ntk, Ana>::GetTimesSummary() const {
    Summary<Duration> summary;
    for(const auto &entry : mStats_) {
      summary.emplace_back("opt " + entry.first + " add", entry.second.durationAdd);
      summary.emplace_back("opt " + entry.first + " reduce", entry.second.durationReduce);
    }
    Summary<Duration> summary2 = ana_.GetTimesSummary();
    summary.insert(summary.end(), summary2.begin(), summary2.end());
    return summary;
  }
  
  // print

  template <typename Ntk, typename Ana>
  template <typename... Args>
  void Optimizer<Ntk, Ana>::Print(int nVerboseLevel, Args &&...args) {
    if(fnPrintLine_ && par_.nVerbose > nVerboseLevel) {
      std::stringstream ss;
      for(int i = 0; i < nVerboseLevel; i++) {
        ss << "\t";
      }
      PrintNext(ss, std::forward<Args>(args)...);
      fnPrintLine_(ss.str());
    }
  }

  // callback
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ActionCallback(const Action &action) {
    if(par_.nVerbose > 4) {
      std::stringstream ss = GetActionDescription(action);
      std::string str;
      std::getline(ss, str);
      Print(4, str);
      while(std::getline(ss, str)) {
        Print(5, str);
      }
    }
    switch(action.type) {
    case REMOVE_FANIN:
      if(action.id != nTarget_) {
        nTarget_ = -1;
      }
      break;
    case REMOVE_UNUSED:
      break;
    case REMOVE_BUFFER:
    case REMOVE_CONST:
      if(action.id == nTarget_) {
        nTarget_ = -1;
      }
      break;
    case ADD_FANIN:
      if(action.id != nTarget_) {
        nTarget_ = -1;
      }
      break;
    case TRIVIAL_COLLAPSE:
      break;
    case TRIVIAL_DECOMPOSE:
      nTarget_ = -1;
      break;
    case SORT_FANINS:
      break;
    case READ:
      nTarget_ = -1;
      break;
    case SAVE:
      break;
    case LOAD:
      // nTarget_ = -1; // this is not always needed, so do it manually
      break;
    case POP_BACK:
      break;
    default:
      assert(0);
    }
  }

  // topology

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::MarkTfo(int nId) {
    // includes itself
    if(nId == nTarget_) {
      return;
    }
    nTarget_ = nId;
    vTfoMarks_.clear();
    vTfoMarks_.resize(pNtk_->GetNumNodes());
    vTfoMarks_[nId] = true;
    pNtk_->template ForEachTfo<false, true, true, false>(nId, [&](int fo) {
      vTfoMarks_[fo] = true;
    });
  }
  
  // time

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::Timeout() {
    if(nTimeout_) {
      if(DurationInSeconds(timeStart_, GetCurrentTime()) > nTimeout_) {
        return true;
      }
    }
    return false;
  }

  // sort fanins

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SetRandPiOrder() {
    if(int_size(vRandPiOrder_) != pNtk_->GetNumPis()) {
      vRandPiOrder_.clear();
      vRandPiOrder_.resize(pNtk_->GetNumPis());
      std::iota(vRandPiOrder_.begin(), vRandPiOrder_.end(), 0);
      std::shuffle(vRandPiOrder_.begin(), vRandPiOrder_.end(), rng_);
    }
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SetRandCosts() {
    std::uniform_real_distribution<> dis(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
    while(int_size(vRandCosts_) < pNtk_->GetNumNodes()) {
      vRandCosts_.push_back(dis(rng_));
    }
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SortFanins(int nId) {
    switch(nSortType) {
    case 0: // no sorting
      break;
    case 1: // prioritize internals
      pNtk_->SortFanins(nId, [&](int i, int j) {
        return !pNtk_->IsPi(i) && pNtk_->IsPi(j);
      });
      break;
    case 2: // prioritize internals with (reversely) sorted PIs
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return pNtk_->GetPiIndex(i) > pNtk_->GetPiIndex(j);
        }
        return pNtk_->IsPi(j);
      });
      break;
    case 3: // prioritize internals with random PI order
      SetRandPiOrder();
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return vRandPiOrder_[pNtk_->GetPiIndex(i)] > vRandPiOrder_[pNtk_->GetPiIndex(j)];
        }
        return pNtk_->IsPi(j);
      });
      break;
    case 4: // smaller fanout takes larger cost
      pNtk_->SortFanins(nId, [&](int i, int j) {
        return pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j);
      });
      break;
    case 5: // fanout + PI
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && !pNtk_->IsPi(j)) {
          return false;
        }
        if(!pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return true;
        }
        return pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j);
      });
      break;
    case 6: // fanout + sorted PI
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return pNtk_->GetPiIndex(i) > pNtk_->GetPiIndex(j);
        }
        if(pNtk_->IsPi(i)) {
          return false;
        }
        if(pNtk_->IsPi(j)) {
          return true;
        }
        return pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j);
      });
      break;
    case 7: // fanout + random PI
      SetRandPiOrder();
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return vRandPiOrder_[pNtk_->GetPiIndex(i)] > vRandPiOrder_[pNtk_->GetPiIndex(j)];
        }
        if(pNtk_->IsPi(i)) {
          return false;
        }
        if(pNtk_->IsPi(j)) {
          return true;
        }
        return pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j);
      });
      break;
    case 8: // reverse topological order + PI
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return false;
        }
        if(pNtk_->IsPi(i)) {
          return false;
        }
        if(pNtk_->IsPi(j)) {
          return true;
        }
        return pNtk_->GetIntIndex(i) > pNtk_->GetIntIndex(j);
      });
      break;
    case 9: // reverse topological order + sorted PI
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return pNtk_->GetPiIndex(i) > pNtk_->GetPiIndex(j);
        }
        if(pNtk_->IsPi(i)) {
          return false;
        }
        if(pNtk_->IsPi(j)) {
          return true;
        }
        return pNtk_->GetIntIndex(i) > pNtk_->GetIntIndex(j);
      });
      break;
    case 10: // reverse topological order + random PI
      SetRandPiOrder();
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return vRandPiOrder_[pNtk_->GetPiIndex(i)] > vRandPiOrder_[pNtk_->GetPiIndex(j)];
        }
        if(pNtk_->IsPi(i)) {
          return false;
        }
        if(pNtk_->IsPi(j)) {
          return true;
        }
        return pNtk_->GetIntIndex(i) > pNtk_->GetIntIndex(j);
      });
      break;
    case 11: // topo + fanout + PI
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && !pNtk_->IsPi(j)) {
          return false;
        }
        if(!pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return true;
        }
        if(pNtk_->GetNumFanouts(i) > pNtk_->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j)) {
          return true;
        }
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return false;
        }
        return pNtk_->GetIntIndex(i) > pNtk_->GetIntIndex(j);
      });
      break;
    case 12: // topo + fanout + sorted PI
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return pNtk_->GetPiIndex(i) > pNtk_->GetPiIndex(j);
        }
        if(pNtk_->IsPi(i)) {
          return false;
        }
        if(pNtk_->IsPi(j)) {
          return true;
        }
        if(pNtk_->GetNumFanouts(i) > pNtk_->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j)) {
          return true;
        }
        return pNtk_->GetIntIndex(i) > pNtk_->GetIntIndex(j);
      });
      break;
    case 13: // topo + fanout + random PI
      SetRandPiOrder();
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return vRandPiOrder_[pNtk_->GetPiIndex(i)] > vRandPiOrder_[pNtk_->GetPiIndex(j)];
        }
        if(pNtk_->IsPi(i)) {
          return false;
        }
        if(pNtk_->IsPi(j)) {
          return true;
        }
        if(pNtk_->GetNumFanouts(i) > pNtk_->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j)) {
          return true;
        }
        return pNtk_->GetIntIndex(i) > pNtk_->GetIntIndex(j);
      });
      break;
    case 14: // random order
      SetRandCosts();
      pNtk_->SortFanins(nId, [&](int i, int j) {
        return vRandCosts_[i] > vRandCosts_[j];
      });
      break;
    case 15: // random + PI
      SetRandCosts();
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && !pNtk_->IsPi(j)) {
          return false;
        }
        if(!pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return true;
        }
        return vRandCosts_[i] > vRandCosts_[j];
      });
      break;
    case 16: // random + fanout
      SetRandCosts();
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->GetNumFanouts(i) > pNtk_->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j)) {
          return true;
        }
        return vRandCosts_[i] > vRandCosts_[j];
      });
      break;
    case 17: // random + fanout + PI
      SetRandCosts();
      pNtk_->SortFanins(nId, [&](int i, int j) {
        if(pNtk_->IsPi(i) && !pNtk_->IsPi(j)) {
          return false;
        }
        if(!pNtk_->IsPi(i) && pNtk_->IsPi(j)) {
          return true;
        }
        if(pNtk_->GetNumFanouts(i) > pNtk_->GetNumFanouts(j)) {
          return false;
        }
        if(pNtk_->GetNumFanouts(i) < pNtk_->GetNumFanouts(j)) {
          return true;
        }
        return vRandCosts_[i] > vRandCosts_[j];
      });
      break;
    default:
      assert(0);
    }
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SortFanins() {
    pNtk_->ForEachInt([&](int nId) {
      SortFanins(nId);
    });
  }
  
  // remove fanins

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::RemoveRedundantFanins(int nId, bool fRemoveUnused) {
    assert(pNtk_->GetNumFanouts(nId) > 0);
    bool fReduced = false;
    for(int nIdx = 0; nIdx < pNtk_->GetNumFanins(nId); nIdx++) {
      if(mNewFanins_.count(nId)) {
        int nFi = pNtk_->GetFanin(nId, nIdx);
        if(mNewFanins_[nId].count(nFi)) {
          continue;
        }
      }
      if(ana_.CheckRedundancy(nId, nIdx)) {
        int nFi = pNtk_->GetFanin(nId, nIdx);
        pNtk_->RemoveFanin(nId, nIdx);
        fReduced = true;
        nIdx--;
        if(fRemoveUnused && pNtk_->IsInt(nFi) && pNtk_->GetNumFanouts(nFi) == 0) {
          pNtk_->RemoveUnused(nFi, true);
        }
      }
    }
    return fReduced;
  }
  
  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::RemoveRedundantFaninsRandom(int nId, bool fRemoveUnused) {
    assert(pNtk_->GetNumFanouts(nId) > 0);
    bool fReduced = false;
    vTmp_.resize(pNtk_->GetNumFanins(nId));
    std::iota(vTmp_.begin(), vTmp_.end(), 0);
    std::shuffle(vTmp_.begin(), vTmp_.end(), rng_);
    for(int i = 0; i < int_size(vTmp_); i++) {
      int nIdx = vTmp_[i];
      if(mNewFanins_.count(nId)) {
        int nFi = pNtk_->GetFanin(nId, nIdx);
        if(mNewFanins_[nId].count(nFi)) {
          continue;
        }
      }
      if(ana_.CheckRedundancy(nId, nIdx)) {
        int nFi = pNtk_->GetFanin(nId, nIdx);
        pNtk_->RemoveFanin(nId, nIdx);
        fReduced = true;
        for(int j = i + 1; j < int_size(vTmp_); j++) {
          if(vTmp_[j] > nIdx) {
            vTmp_[j]--;
          }
        }
        if(fRemoveUnused && pNtk_->IsInt(nFi) && pNtk_->GetNumFanouts(nFi) == 0) {
          pNtk_->RemoveUnused(nFi, true);
        }
      }
    }
    return fReduced;
  }

  // remove redundancy

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::RemoveRedundancyOneTraversal(bool fRandom, bool fSubRoutine) {
    TimePoint timeStart;
    if(!fSubroutine) {
      timeStart = GetCurrentTime();
    }
    bool fReduced = false;
    std::vector<int> vInts = pNtk_->GetInts();
    if(fRandom) {
      std::shuffle(vInts.begin(), vInts.end(), rng_);
    }
    for(auto it = vInts.crbegin(); it != vInts.crend(); ++it) {
      if(!pNtk_->IsInt(*it)) {
        continue;
      }
      if(pNtk_->GetNumFanouts(*it) == 0) {
        pNtk_->RemoveUnused(*it);
        continue;
      }
      if(par_.fSortPerNode) {
        SortFanins(*it);
      }
      if(fRandom) {
        fReduced |= RemoveRedundantFaninsRandom(*it);
      } else {
        fReduced |= RemoveRedundantFanins(*it);
      }
      if(pNtk_->GetNumFanins(*it) <= 1) {
        pNtk_->Propagate(*it);
      }
    }
    if(!fSubRoutine) {
      TimePoint timeEnd = GetCurrentTime();
      statsLocal_.durationReduce += Duration(timeStart, timeEnd);
    }
    return fReduced;
  }

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::RemoveRedundancy(bool fRandom) {
    TimePoint timeStart = GetCurrentTime();
    bool fReduced = false;
    while(RemoveRedundancyOneTraversal(fRandom, true)) {
      fReduced = true;
      if(!par_.fSortPerNode) {
        SortFanins();
      }
    }
    TimePoint timeEnd = GetCurrentTime();
    statsLocal_.durationReduce += Duration(timeStart, timeEnd);
    return fReduced;
  }  

  // reduce

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::Reduce() {
    bool r;
    switch(par_.nReductionMethod) {
    case 0:
      r = RemoveRedundancy(false);
      break;
    case 1:
      r = RemoveRedundancy(true);
      break;
    default:
      assert(0);
    }
    return r;
  }

  // addition

  template <typename Ntk, typename Ana>
  template <typename T>
  T Optimizer<Ntk, Ana>::SingleAdd(int nId, T begin, T end) {
    TimePoint timeStart = GetCurrentTime();
    MarkTfo(nId);
    pNtk_->ForEachFanin(nId, [&](int nFi) {
      vTfoMarks_[nFi] = true;
    });
    T it = begin;
    for(; it != end; ++it) {
      int nCand = *it;
      if(!pNtk_->IsInt(nCand) && !pNtk_->IsPi(nCand)) {
        continue;
      }
      if(vTfoMarks_[nCand]) {
        continue;
      }
      statsLocal_.nTriedFis++;
      if(ana_.CheckFeasibility(nId, nCand, false)) {
        pNtk_->AddFanin(nId, nCand, false);
        statsLocal_.nAddedFis++;
      } else if(pNtk_->UseComplementedEdges() && ana_.CheckFeasibility(nId, nCand, true)) {
        pNtk_->AddFanin(nId, nCand, true);
        statsLocal_.nAddedFis++;
      } else {
        continue;
      }
      mNewFanins_[nId].insert(nCand);
      break;
    }
    pNtk_->ForEachFanin(nId, [&](int nFi) {
      vTfoMarks_[nFi] = false;
    });
    TimePoint timeEnd = GetCurrentTime();
    statsLocal_.durationAdd += Duration(timeStart, timeEnd);
    return it;
  }

  template <typename Ntk, typename Ana>
  int Optimizer<Ntk, Ana>::MultiAdd(int nId, const std::vector<int> &vCands) {
    TimePoint timeStart = GetCurrentTime();
    MarkTfo(nId);
    pNtk_->ForEachFanin(nId, [&](int nFi) {
      vTfoMarks_[nFi] = true;
    });
    int nAddedFis = 0;
    for(int nCand : vCands) {
      if(!pNtk_->IsInt(nCand) && !pNtk_->IsPi(nCand)) {
        continue;
      }
      if(vTfoMarks_[nCand]) {
        continue;
      }
      statsLocal_.nTriedFis++;
      if(ana_.CheckFeasibility(nId, nCand, false)) {
        pNtk_->AddFanin(nId, nCand, false);
        statsLocal_.nAddedFis++;
      } else if(pNtk_->UseComplementedEdges() && ana_.CheckFeasibility(nId, nCand, true)) {
        pNtk_->AddFanin(nId, nCand, true);
        statsLocal_.nAddedFis++;
      } else {
        continue;
      }
      mNewFanins_[nId].insert(nCand);
      nAddedFis++;
      if(nAddedFis == nMax_) {
        break;
      }
    }
    pNtk_->ForEachFanin(nId, [&](int nFi) {
      vTfoMarks_[nFi] = false;
    });
    TimePoint timeEnd = GetCurrentTime();
    statsLocal_.durationAdd += Duration(timeStart, timeEnd);
    return nAddedFis;
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::Undo(int nSlot) {
    if(par_.fCompatible) {
      pNtk_->Load(nSlot);
      return;
    }
    for(const auto &entry : mNewFanins_) {
      int nId = entry.first;
      for(int nFi : entry.second) {
        int nIdx = pNtk_->FindFanin(nId, nFi);
        pNtk_->RemoveFanin(nId, nIdx);
      }
    }
  }

  // resub

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ResubCleanup(int nId, int nSlot, bool fTried, bool fAdded) {
    if(pNtk_->IsInt(nId) && pNtk_->GetNumFanins(nId) > 2) {
      pNtk_->TrivialDecompose(nId);
      SortFanins();
      if(par_.fCompatible) {
        Reduce();
      }
    }
    if(nSlot >= 0) {
      pNtk_->PopBack();
    }
    if(fTried) {
      statsLocal_.nTried++;
    }
    if(fAdded) {
      statsLocal_.nAdded++;
    }
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::SingleResub(int nId, const std::vector<int> &vCands) {
    assert(pNtk_->GetNumFanouts(nId) != 0);
    assert(pNtk_->GetNumFanins(nId) > 1);
    pNtk_->TrivialCollapse(nId);
    int nSlot = -2; // nSlot < -1 prevents unexpected auto-allocation
    if(par_.fCompatible || (par_.fGreedy && par_.fNonlinearCost)) {
      nSlot = pNtk_->Save();
    }
    Cost cost = fnObjective_(pNtk_);
    bool fTried = false, fAdded = false;
    for(auto it = vCands.cbegin(); it != vCands.cend(); ++it) {
      if(Timeout() || !pNtk_->IsInt(nId)) {
        break;
      }
      fTried = true;
      it = SingleAdd(nId, it, vCands.cend());
      if(it == vCands.cend()) {
        break;
      }
      fAdded = true;
      Print(2, "cand", *it, "(", int_distance(vCands.cbegin(), it) + 1, "/", int_size(vCands), ")", ":", "cost", "=", cost);
      if(Reduce()) {
        Cost costNew = fnObjective_(pNtk_);
        assert(par_.fNonlinearCost || costNew <= cost);
        statsLocal_.RecordChange(cost, costNew);
        if(par_.fCompatible) {
          pNtk_->Save(nSlot);
          cost = costNew;
        } else if(par_.fGreedy && par_.fNonlinearCost) {
          if(costNew <= cost) {
            pNtk_->Save(nSlot);
            cost = costNew;
          } else {
            pNtk_->Load(nSlot);
          }
        } else {
          cost = costNew;
        }
      } else {
        Undo(nSlot);
      }
      mNewFanins_.clear();
    }
    ResubCleanup(nId, nSlot, fTried, fAdded);
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::MultiResub(int nId, const std::vector<int> &vCands) {
    assert(pNtk_->GetNumFanouts(nId) != 0);
    assert(pNtk_->GetNumFanins(nId) > 1);
    int nSlot = -2; // nSlot < -1 prevents unexpected auto-allocation
    if(par_.fCompatible || par_.fGreedy) {
      nSlot = pNtk_->Save();
    }
    pNtk_->TrivialCollapse(nId);
    const Cost cost = fnObjective_(pNtk_);
    bool fTried = true, fAdded = false;
    if(MultiAdd(nId, vCands)) {
      fAdded = true;
      if(Reduce()) {
        mNewFanins_.clear();
        Reduce();
        Cost costNew = fnObjective_(pNtk_);
        statsLocal_.RecordChange(cost, costNew);
        if(par_.fGreedy && costNew > cost) {
          pNtk_->Load(nSlot);
        }
      } else {
        Undo(nSlot);
        mNewFanins_.clear();
      }
    }
    ResubCleanup(nId, nSlot, fTried, fAdded);
  }
  
  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::SingleResubStop(int nId, const std::vector<int> &vCands) {
    assert(pNtk_->GetNumFanouts(nId) != 0);
    assert(pNtk_->GetNumFanins(nId) > 1);
    Print(2, "method", "=", "single");
    pNtk_->TrivialCollapse(nId);
    const int nSlot = pNtk_->Save();
    const std::set<int> sFanins = pNtk_->GetExtendedFanins(nId);
    Print(3, "extended fanins", ":", sFanins);
    const Cost cost = fnObjective_(pNtk_);
    bool fTried = false, fAdded = false, fChanged = false;
    for(auto it = vCands.cbegin(); it != vCands.cend(); ++it) {
      if(Timeout()) {
        break;
      }
      assert(pNtk_->IsInt(nId));
      fTried = true;
      it = SingleAdd(nId, it, vCands.cend());
      if(it == vCands.cend()) {
        break;
      }
      fAdded = true;
      Print(3, "cand", *it, "(", int_distance(vCands.cbegin(), it) + 1, "/", int_size(vCands), ")", ":", "cost", "=", cost);
      if(Reduce()) {
        mNewFanins_.clear();
        Cost costNew = fnObjective_(pNtk_);
        fChanged = true;
        if(costNew == cost) {
          if(pNtk_->IsInt(nId)) {
            std::set<int> sNewFanins = pNtk_->GetExtendedFanins(nId);
            Print(3, "new extended fanins", ":", sNewFanins);
            if(sFanins == sNewFanins) {
              fChanged = false;
            }
          }
        } else if(costNew > cost) {
          assert(par_.fNonlinearCost);
          if(par_.fGreedy) {
            fChanged = false;
          }
        }
        if(fChanged) {
          statsLocal_.RecordChange(cost, costNew);
          break;
        }
        pNtk_->Load(nSlot);
      } else {
        Undo(nSlot);
        mNewFanins_.clear();
      }
    }
    ResubCleanup(nId, nSlot, fTried, fAdded);
    return fChanged;
  }

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::MultiResubStop(int nId, const std::vector<int> &vCands) {
    assert(pNtk_->GetNumFanouts(nId) != 0);
    assert(pNtk_->GetNumFanins(nId) > 1);
    Print(2, "method", "=", "multi");
    const int nSlot = pNtk_->Save();
    pNtk_->TrivialCollapse(nId);
    std::set<int> sFanins = pNtk_->GetExtendedFanins(nId);
    Print(3, "extended fanins", ":", sFanins);
    const Cost cost = fnObjective_(pNtk_);
    bool fTried = true, fAdded = false, fChanged = false;
    if(MultiAdd(nId, vCands)) {
      fAdded = true;
      if(Reduce()) {
        mNewFanins_.clear();
        Reduce();
        Cost costNew = fnObjective_(pNtk_);
        fChanged = true;
        if(costNew == cost) {
          if(pNtk_->IsInt(nId)) {
            std::set<int> sNewFanins = pNtk_->GetExtendedFanins(nId);
            Print(3, "new extended fanins", ":", sNewFanins);
            if(sFanins == sNewFanins) {
              fChanged = false;
            }
          }
        } else if(costNew > cost) {
          if(par_.fGreedy) {
            fChanged = false;
          }
        }
        if(fChanged) {
          statsLocal_.RecordChange(cost, costNew);
        } else {
          pNtk_->Load(nSlot);          
        }
      } else {
        Undo(nSlot);
        mNewFanins_.clear();
      }
    }
    ResubCleanup(nId, nSlot, fTried, fAdded);
    return fChanged;
  }
  
  // multi-target resub

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::MultiTargetTrivialCollapse(std::vector<int> &vTargets) {
    for(int nId : vTargets) {
      if(pNtk_->IsInt(nId)) {
        pNtk_->TrivialCollapse(nId);
      }
    }
    for(auto it = vTargets.begin(); it != vTargets.end();) {
      if(!pNtk_->IsInt(*it)) {
        it = vTargets.erase(it);
      } else {
        ++it;
      }
    }
  }
  
  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::MultiTargetAdd(const std::vector<int> &vTargets, bool fRandomAddition) {
    bool fAdded = false;
    for(int nId : vTargets) {
      std::vector<int> vCands;
      if(par_.nDistance) {
        vCands = pNtk_->GetNeighbors(nId, true, par_.nDistance);
      } else {
        vCands = pNtk_->GetPisInts();
      }
      if(fRandomAddition) {
        std::shuffle(vCands.begin(), vCands.end(), rng_);
      }
      fAdded |= MultiAdd(nId, vCands);
    }
    return fAdded;
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::MultiTargetResubCleanup(const std::vector<int> &vTargets, int nSlot, bool fTried, bool fAdded) {
    bool fDecomposed = false;
    for(int nId : vTargets) {
      if(pNtk_->IsInt(nId) && pNtk_->GetNumFanins(nId) > 2) {
        pNtk_->TrivialDecompose(nId);
        fDecomposed = true;
      }
    }
    if(fDecomposed) {
      SortFanins();
      if(par_.fCompatible) {
        Reduce();
      }
    }
    if(nSlot >= 0) {
      pNtk_->PopBack();
    }
    if(fTried) {
      statsLocal_.nTried++;
    }
    if(fAdded) {
      statsLocal_.nAdded++;
    }
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::MultiTargetResub(std::vector<int> vTargets, bool fRandomAddition) {
    const int nSlot = pNtk_->Save();
    const Cost cost = fnObjective_(pNtk_);
    MultiTargetTrivialCollapse(vTargets);
    Print(2, "targets", ":", vTargets);
    bool fTried = true, fAdded = false;
    if(MultiTargetAdd(vTargets, fRandomAddition)) {
      fAdded = true;
      if(Reduce()) {
        mNewFanins_.clear();
        Reduce();
        Cost costNew = fnObjective_(pNtk_);
        statsLocal_.RecordChange(cost, costNew);
        if(par_.fGreedy && costNew > cost) {
          pNtk_->Load(nSlot);
          nTarget_ = -1;
        }
      } else {
        pNtk_->Load(nSlot);
        nTarget_ = -1;
        mNewFanins_.clear();
      }
    }
    MultiTargetResubCleanup(vTargets, nSlot, fTried, fAdded);
  }

  template <typename Ntk, typename Ana>
  bool Optimizer<Ntk, Ana>::MultiTargetResubStop(std::vector<int> vTargets, bool fRandomAddition) {
    const int nSlot = pNtk_->Save();
    const Cost cost = fnObjective_(pNtk_);
    MultiTargetTrivialCollapse(vTargets);
    Print(2, "targets", ":", vTargets);
    std::set<int> sTargets(vTargets.begin(), vTargets.end());
    std::map<int, std::set<int>> mFanins;
    for(int nId : sTargets) {
      std::set<int> sFanins = pNtk_->GetExtendedFanins(nId);
      Print(3, "extended fanins", nId, ":", sFanins);
      mFanins[nId] = std::move(sFanins);
    }    
    bool fTried = true, fAdded = false, fChanged = false;
    if(MultiTargetAdd(vTargets, fRandomAddition)) {
      fAdded = true;
      if(Reduce()) {
        mNewFanins_.clear();
        Reduce();
        Cost costNew = fnObjective_(pNtk_);
        fChanged = true;
        if(costNew == cost) {
          fChanged = false;
          for(int nId : sTargets) {
            if(!pNtk_->IsInt(nId)) {
              fChanged = true;
              break;
            }
          }
          if(!fChanged) {
            for(int nId : sTargets) {
              std::set<int> sNewFanins = pNtk_->GetExtendedFanins(nId);
              Print(3, "new extended fanins", nId, ":", sNewFanins);
              if(mFanins[nId] != sNewFanins) {
                fChanged = true;
                break;
              }
            }
          }
        } else if(costNew > cost) {
          if(par_.fGreedy) {
            fChanged = false;
          }
        }
        if(fChanged) {
          statsLocal_.RecordChange(cost, costNew);
        } else {
          pNtk_->Load(nSlot);
          nTarget_ = -1;
        }
      } else {
        pNtk_->Load(nSlot);
        nTarget_ = -1;
        mNewFanins_.clear();
      }
    }
    MultiTargetResubCleanup(vTargets, nSlot, fTried, fAdded);
    return fChanged;
  }

  // apply

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyReverseTopologically(const std::function<bool(int)> &fn) {
    std::vector<int> vInts = pNtk_->GetInts();
    for(auto it = vInts.crbegin(); it != vInts.crend(); ++it) {
      if(Timeout()) {
        break;
      }
      if(!pNtk_->IsInt(*it)) {
        continue;
      }
      Print(1, "node", *it, "(", int_distance(vInts.crbegin(), it) + 1, "/", int_size(vInts), ")", ":", "cost", "=", fnObjective_(pNtk_));
      if(fn(*it)) {
        break;
      }
    }
  }

 template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyRandomly(const std::function<bool(int)> &fn) {
    std::vector<int> vInts = pNtk_->GetInts();
    std::shuffle(vInts.begin(), vInts.end(), rng_);
    for(auto it = vInts.cbegin(); it != vInts.cend(); ++it) {
      if(Timeout()) {
        break;
      }
      if(!pNtk_->IsInt(*it)) {
        continue;
      }
      Print(1, "node", *it, "(", int_distance(vInts.cbegin(), it) + 1, "/", int_size(vInts), ")", ":", "cost", "=", fnObjective_(pNtk_));
      if(fn(*it)) {
        break;
      }
    }
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyCombinationRandomly(int k, const std::function<bool(const std::vector<int> &)> &fn) {
    std::vector<int> vInts = pNtk_->GetInts();
    std::shuffle(vInts.begin(), vInts.end(), rng_); // order is decided here, so it's not truly exhaustive
    int nTried = 0;
    int nCombs = k * (k - 1) / 2;
    ForEachCombination(int_size(vInts), k, [&](const std::vector<int> &vIdxs) {
      Print(1, "comb", vIdxs, "(", ++nTried, "/", nCombs, ")");
      assert(int_size(vIdxs) == k);
      if(Timeout()) {
        return true;
      }
      std::vector<int> vTargets(k);
      for(int i = 0; i < k; i++) {
        vTargets[i] = vInts[vIdxs[i]];
      }
      return fn(vTargets) || nTried == nSamples_;
    });
  }
  
  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyCombinationSampled(int k, const std::function<bool(const std::vector<int> &)> &fn) {
    const int nInts = pNtk_->GetNumInts();
    assert(nInts >= k);
    std::vector<int> vInts = pNtk_->GetInts();
    for(int nTried = 0; nTried < nSamples_; nTried++) {
      if(Timeout()) {
        break;
      }
      std::set<int> sIdxs;
      while(int_size(sIdxs) < k) {
        int nIdx = rng_() % nInts;
        sIdxs.insert(nIdx);
      }
      std::vector<int> vIdxs(sIdxs.begin(), sIdxs.end());
      std::shuffle(vIdxs.begin(), vIdxs.end(), rng_);
      Print(1, "comb", vIdxs, "(", nTried + 1, "/", nSamples_, ")");
      std::vector<int> vTargets(k);
      for(int i = 0; i < k; i++) {
        vTargets[i] = vInts[vIdxs[i]];
      }
      if(fn(vTargets)) {
        break;
      }
    }
  }

  template <typename Ntk, typename Ana>
  void Optimizer<Ntk, Ana>::ApplyMultisetSampled(int k, const std::function<bool(const std::vector<int> &)> &fn) {
    const int nInts = pNtk_->GetNumInts();
    assert(nInts >= 1);
    std::vector<int> vInts = pNtk_->GetInts();
    for(int nTried = 0; nTried < nSamples_; nTried++) {
      if(Timeout()) {
        break;
      }
      std::vector<int> vIdxs;
      vIdxs.reserve(k);
      for(int j = 0; j < k; j++) {
        int nIdx = rng_() % nInts;
        vIdxs.push_back(nIdx);
      }
      Print(1, "comb", vIdxs, "(", nTried + 1, "/", nSamples_, ")");
      std::vector<int> vTargets(k);
      for(int j = 0; j < k; j++) {
        vTargets[j] = vInts[vIdxs[j]];
      }
      if(fn(vTargets)) {
        break;
      }
    }
  }
  
  // run helper

  template <typename Ntk, typename Ana>
  template <typename ApplyFn, typename ResubFn>
  void Optimizer<Ntk, Ana>::RunHelper(ApplyFn &&fnApply, const std::vector<ResubFn> &vResubs, const std::vector<std::string> &vNames, bool fStopAtChange, bool fRandomAddition) {
    assert(!vResubs.empty());
    assert(vResubs.size() == vNames.size());
    const bool fRandomSelection = vResubs.size() > 1;
    std::vector<int> vCands;
    if(!par_.nDistance) {
      vCands = pNtk_->GetPisInts();
      if(fRandomAddition) {
        std::shuffle(vCands.begin(), vCands.end(), rng_);
      }
    }
    std::vector<Stats> vStats;
    if(!fRandomSelection) {
      statsLocal_.Reset();
    } else {
      vStats.resize(vResubs.size());
    }
    int nTried = 0;
    fnApply([&](int nId) {
      if(fRandomSelection) {
        statsLocal_.Reset();
      }
      if(par_.nDistance) {
        vCands = pNtk_->GetNeighbors(nId, true, par_.nDistance);
        if(fRandomAddition) {
          std::shuffle(vCands.begin(), vCands.end(), rng_);
        }
      }
      int i = fRandomSelection? rng_() % vResubs.size(): 0;
      bool fChanged = vResubs[i](nId, vCands);
      if(fRandomSelection) {
        vStats[i] += statsLocal_;
      }
      nTried++;
      if(fStopAtChange && fChanged) {
        return true;
      }
      if(!par_.nDistance && fChanged) {
        vCands = pNtk_->GetPisInts();
        if(fRandomAddition) {
          std::shuffle(vCands.begin(), vCands.end(), rng_);
        }
      }
      return nTried == nSamples_;
    });
    if(fRandomSelection) {
      for(int i = 0; i < int_size(vResubs); i++) {
        mStats_[vNames[i]] = vStats[i];
        Print(0, vNames[i], ":", vStats[i].GetString());
      }
    } else {
      mStats_[vNames[0]] = statsLocal_;
      Print(0, vNames[0], ":", statsLocal_.GetString());
    }
  }

} // namespace boop::rrr

BOOP_HEADER_END
