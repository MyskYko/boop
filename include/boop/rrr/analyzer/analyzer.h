#pragma once

#include "boop/util/util.h"

BOOP_HEADER_START

namespace boop {

  template <typename Ntk, typename Sim, typename Sol>
  class Analyzer {
  public:
    struct Parameter {
      int nVerbose = 0;
      typename Sim::Parameter parSim;
      typename Sol::Parameter parSol;
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
    Sol sol_;

    // print
    template<typename... Args>
    void Print(int nVerboseLevel, Args &&...args);
  };

  // lifecycle

  template <typename Ntk, typename Sim, typename Sol>
  Analyzer<Ntk, Sim, Sol>::Analyzer(const Parameter &par)
    : pNtk_(nullptr),
      par_(par),
      sim_(par.parSim),
      sol_(par.parSol) {
  }
  
  template <typename Ntk, typename Sim, typename Sol>
  void Analyzer<Ntk, Sim, Sol>::AssignNetwork(Ntk *pNtk, bool fReuse) {
    pNtk_ = pNtk;
    sim_.AssignNetwork(pNtk_, fReuse);
    sol_.AssignNetwork(pNtk_, fReuse);
  }

  template <typename Ntk, typename Sim, typename Sol>
  void Analyzer<Ntk, Sim, Sol>::SetPrintLine(std::function<void(const std::string &)> fnPrintLine) {
    sim_.SetPrintLine(fnPrintLine);
    sol_.SetPrintLine(fnPrintLine);
    fnPrintLine_ = std::move(fnPrintLine);
  }

  // checks

  template <typename Ntk, typename Sim, typename Sol>
  bool Analyzer<Ntk, Sim, Sol>::CheckRedundancy(int nId, int nIdx) {
    if(!sim_.CheckRedundancy(nId, nIdx)) {
      Print(1, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", "not redundant");
      return false;
    }
    Print(0, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", (sim_.IsExhaustive() ? "already redundant" : "seems redundant"));
    if(sim_.IsExhaustive()) {
      return true;
    }
    SatResult r = sol_.CheckRedundancy(nId, nIdx);
    if(r == UNSAT) {
      Print(0, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", "redundant");
      return true;
    }
    if(r == SAT) {
      Print(0, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", "NOT redundant");
      sim_.AddCex(sol_.GetCex());
      return false;
    }
    Print(0, "node", nId, ",", "fanin", pNtk_->GetCompl(nId, nIdx), pNtk_->GetFanin(nId, nIdx), ",", "index", nIdx, ":", "undetermined");
    return false;
  }
  
  template <typename Ntk, typename Sim, typename Sol>
  bool Analyzer<Ntk, Sim, Sol>::CheckFeasibility(int nId, int nFi, bool fCompl) {
    if(!sim_.CheckFeasibility(nId, nFi, fCompl)) {
      Print(1, "node", nId, ",", "fanin", fCompl, nFi, ":", "not feasible");
      return false;
    }
    Print(0, "node", nId, ",", "fanin", fCompl, nFi, ":", (sim_.IsExhaustive() ? "already feasible" : "seems feasible"));
    if(sim_.IsExhaustive()) {
      return true;
    }
    SatResult r = sol_.CheckFeasibility(nId, nFi, fCompl);
    if(r == UNSAT) {
      Print(0, "node", nId, ",", "fanin", fCompl, nFi, ":", "feasible");
      return true;
    }
    if(r == SAT) {
      Print(0, "node", nId, ",", "fanin", fCompl, nFi, ":", "NOT feasible");
      sim_.AddCex(sol_.GetCex());
      return false;
    }
    Print(0, "node", nId, ",", "fanin", fCompl, nFi, ":", "undetermined");
    return false;
  }

  // summary

  template <typename Ntk, typename Sim, typename Sol>
  void Analyzer<Ntk, Sim, Sol>::ResetSummary() {
    sim_.ResetSummary();
    sol_.ResetSummary();
  }

  template <typename Ntk, typename Sim, typename Sol>
  Summary<int> Analyzer<Ntk, Sim, Sol>::GetStatsSummary() const {
    Summary<int> summary = sim_.GetStatsSummary();
    Summary<int> summary2 = sol_.GetStatsSummary();
    summary.insert(summary.end(), summary2.begin(), summary2.end());
    return summary;
  }

  template <typename Ntk, typename Sim, typename Sol>
  Summary<Duration> Analyzer<Ntk, Sim, Sol>::GetTimesSummary() const {
    Summary<Duration> summary = sim_.GetTimesSummary();
    Summary<Duration> summary2 = sol_.GetTimesSummary();
    summary.insert(summary.end(), summary2.begin(), summary2.end());
    return summary;
  }
  
  // print

  template <typename Ntk, typename Sim, typename Sol>
  template <typename... Args>
  inline void Analyzer<Ntk, Sim, Sol>::Print(int nVerboseLevel, Args &&...args) {
    if(fnPrintLine_ && par_.nVerbose > nVerboseLevel) {
      std::stringstream ss;
      for(int i = 0; i < nVerboseLevel; i++) {
        ss << "\t";
      }
      PrintNext(ss, std::forward<Args>(args)...);
      fnPrintLine_(ss.str());
    }
  }
  
} // namespace boop

BOOP_HEADER_END
