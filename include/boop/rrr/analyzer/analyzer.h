#pragma once

#include "boop/util/util.h"

BOOP_HEADER_START

namespace boop::rrr {

  template <typename Ntk, typename Sim, typename Sat>
  class Analyzer {
  public:
    struct Parameter {
      int nVerbose = 0;
      typename Sim::Parameter parSim;
      typename Sat::Parameter parSat;
    };

  public:
    // lifecycle
    Analyzer(const Parameter &par);
    void AssignNetwork(Ntk *pNtk, bool fReuse);
    void SetPrintLine(std::function<void(const std::string &)> fnPrintLine);

    // checks
    bool CheckRedundancy(int nId, int nIdx);
    bool CheckFeasibility(int nId, int nFi, bool fCompl);

    // summary
    void ResetSummary();
    Summary<int> GetStatsSummary() const;
    Summary<Duration> GetTimesSummary() const;

  private:
    Ntk *pNtk_;
    const Parameter par_;
    std::function<void(const std::string &)> fnPrintLine_;
    Sim sim_;
    Sat sat_;

    // print
    template<typename... Args>
    void Print(int nVerboseLevel, Args &&...args);
  };

  // lifecycle

  template <typename Ntk, typename Sim, typename Sat>
  Analyzer<Ntk, Sim, Sat>::Analyzer(const Parameter &par)
    : pNtk_(nullptr),
      par_(par),
      sim_(par.parSim),
      sat_(par.parSat) {
  }
  
  template <typename Ntk, typename Sim, typename Sat>
  void Analyzer<Ntk, Sim, Sat>::AssignNetwork(Ntk *pNtk, bool fReuse) {
    pNtk_ = pNtk;
    sim_.AssignNetwork(pNtk_, fReuse);
    sat_.AssignNetwork(pNtk_, fReuse);
  }

  template <typename Ntk, typename Sim, typename Sat>
  void Analyzer<Ntk, Sim, Sat>::SetPrintLine(std::function<void(const std::string &)> fnPrintLine) {
    sim_.SetPrintLine(fnPrintLine);
    sat_.SetPrintLine(fnPrintLine);
    fnPrintLine_ = std::move(fnPrintLine);
  }

  // checks

  template <typename Ntk, typename Sim, typename Sat>
  bool Analyzer<Ntk, Sim, Sat>::CheckRedundancy(int nId, int nIdx) {
    if(!sim_.CheckRedundancy(nId, nIdx)) {
      Print(1, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", "not redundant");
      return false;
    }
    Print(0, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", (sim_.IsExhaustive() ? "already redundant" : "seems redundant"));
    if(sim_.IsExhaustive()) {
      return true;
    }
    typename Sat::Result r = sat_.CheckRedundancy(nId, nIdx);
    if(r == Sat::Result::UNSAT) {
      Print(0, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", "redundant");
      return true;
    }
    if(r == Sat::Result::SAT) {
      Print(0, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", "NOT redundant");
      sim_.AddCex(sat_.GetCex());
      return false;
    }
    Print(0, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", "undetermined");
    return false;
  }
  
  template <typename Ntk, typename Sim, typename Sat>
  bool Analyzer<Ntk, Sim, Sat>::CheckFeasibility(int nId, int nFi, bool fCompl) {
    if(!sim_.CheckFeasibility(nId, nFi, fCompl)) {
      Print(1, "node", nId, ",", "fanin", fCompl, nFi, ":", "not feasible");
      return false;
    }
    Print(0, "node", nId, ",", "fanin", fCompl, nFi, ":", (sim_.IsExhaustive() ? "already feasible" : "seems feasible"));
    if(sim_.IsExhaustive()) {
      return true;
    }
    typename Sat::Result r = sat_.CheckFeasibility(nId, nFi, fCompl);
    if(r == Sat::Result::UNSAT) {
      Print(0, "node", nId, ",", "fanin", fCompl, nFi, ":", "feasible");
      return true;
    }
    if(r == Sat::Result::SAT) {
      Print(0, "node", nId, ",", "fanin", fCompl, nFi, ":", "NOT feasible");
      sim_.AddCex(sat_.GetCex());
      return false;
    }
    Print(0, "node", nId, ",", "fanin", fCompl, nFi, ":", "undetermined");
    return false;
  }

  // summary

  template <typename Ntk, typename Sim, typename Sat>
  void Analyzer<Ntk, Sim, Sat>::ResetSummary() {
    sim_.ResetSummary();
    sat_.ResetSummary();
  }

  template <typename Ntk, typename Sim, typename Sat>
  Summary<int> Analyzer<Ntk, Sim, Sat>::GetStatsSummary() const {
    Summary<int> summary = sim_.GetStatsSummary();
    Summary<int> summary2 = sat_.GetStatsSummary();
    summary.insert(summary.end(), summary2.begin(), summary2.end());
    return summary;
  }

  template <typename Ntk, typename Sim, typename Sat>
  Summary<Duration> Analyzer<Ntk, Sim, Sat>::GetTimesSummary() const {
    Summary<Duration> summary = sim_.GetTimesSummary();
    Summary<Duration> summary2 = sat_.GetTimesSummary();
    summary.insert(summary.end(), summary2.begin(), summary2.end());
    return summary;
  }
  
  // print

  template <typename Ntk, typename Sim, typename Sat>
  template <typename... Args>
  void Analyzer<Ntk, Sim, Sat>::Print(int nVerboseLevel, Args &&...args) {
    if(fnPrintLine_ && par_.nVerbose > nVerboseLevel) {
      std::stringstream ss;
      for(int i = 0; i < nVerboseLevel; i++) {
        ss << "\t";
      }
      PrintNext(ss, std::forward<Args>(args)...);
      fnPrintLine_(ss.str());
    }
  }
  
} // namespace boop::rrr

BOOP_HEADER_END
