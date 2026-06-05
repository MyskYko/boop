#pragma once

#include <initializer_list>
#include <memory>
#include <set>
#include <vector>

#include <cadical.hpp>

BOOP_HEADER_START

namespace boop::solver {

  class CadicalSolver {
  public:
    // lifecycle
    CadicalSolver();
    void Clear();
    
    // variable
    int NewVar();
    int NewVars(int n);

    // clause
    void AddClause(std::initializer_list<int> ilsLits);
    void AddClause(const std::vector<int> &vLits);
    template <class It>
    void AddClause(It it, It itEnd);
    bool IsInconsistent();

    // option
    void SetConflictLimit(int nConflictLimit);
    
    // solve
    Status Solve(const std::vector<int> *vAssumptions, std::set<int> *sCore);

    // result
    bool Value(int nLit);

  private:
    int nVars_;
    int nClauses_;
    std::unique_ptr<CaDiCaL::Solver> pSolver_;
  };

  // lifecycle

  CadicalSolver::CadicalSolver()
    : nVars_(0),
      nClauses_(0),
      pSolver_(std::make_unique<CaDiCaL::Solver>()) {
  }

  void CadicalSolver::Clear() {
    nVars_ = 0;
    nClauses_ = 0;
    pSolver_ = std::make_unique<CaDiCaL::Solver>();
  }

  // variable
  
  int CadicalSolver::NewVar() {
    nVars_++;
    return pSolver_->declare_one_more_variable();
  }

  int CadicalSolver::NewVars(int n) {
    nVars_ += n;
    int nLit = pSolver_->declare_more_variables(n);
    return nLit + 1 - n;
  }
  
  // clause

  void CadicalSolver::AddClause(std::initializer_list<int> ilsLits) {
    AddClause(ilsLits.begin(), ilsLits.end());
  }
  
  void CadicalSolver::AddClause(const std::vector<int> &vLits) {
    AddClause(vLits.begin(), vLits.end());
  }
  
  template <class It>
  void CadicalSolver::AddClause(It it, It itEnd) {
    for(; it != itEnd; ++it) {
      pSolver_->add(*it);
    }
    pSolver_->add(0);
    nClauses_++;
  }

  bool CadicalSolver::IsInconsistent() {
    return pSolver_->inconsistent();
  }

  // option
  
  void CadicalSolver::SetConflictLimit(int nConflictLimit) {
    pSolver_->limit("conflicts", nConflictLimit);
  }
  
  // solve
  
  Status CadicalSolver::Solve(const std::vector<int> *vAssumptions, std::set<int> *sCore) {
    if(vAssumptions != nullptr) {
      for(int nLit : *vAssumptions) {
        pSolver_->assume(nLit);
      }
    }
    int nRes = pSolver_->solve();
    if(vAssumptions != nullptr && sCore != nullptr) {
      for(int nLit : *vAssumptions) {
        if(pSolver_->failed(nLit)) {
          sCore->insert(nLit);
        }
      }
    }
    if(CaDiCaL::Status::SATISFIABLE == nRes) {
      return Status::SAT;
    }
    if(CaDiCaL::Status::UNSATISFIABLE == nRes) {
      return Status::UNSAT;
    }
    return Status::UNDET;
  }

  // value
  
  bool CadicalSolver::Value(int nLit) {
    if (nLit < 0) {
      return pSolver_->val(nLit) < 0;
    }
    return pSolver_->val(nLit) > 0;
  }
  
} // namespace boop::solver

BOOP_HEADER_END
