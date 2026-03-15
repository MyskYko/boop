#pragma once

#include <initializer_list>
#include <vector>
#include <set>

#include <cadical.hpp>

BOOP_HEADER_START

namespace boop::solver {

  class CadicalSolver {
  public:
    // lifecycle
    CadicalSolver();
    
    // variable
    int NewVar();

    // clause
    void AddClause(std::initializer_list<int> ilsLits);
    void AddClause(const std::vector<int> &vLits);
    template <class It>
    void AddClause(It it, It itEnd);

    //solve
    Status Solve();
    Status Solve(const std::vector<int> &vAssumptions, std::set<int> &sCore);

    // result
    bool Value(int nLit);

  private:
    int nVars_;
    int nClauses_;
    CaDiCaL::Solver solver_;
  };

  // lifecycle

  CadicalSolver::CadicalSolver()
    : nVars_(0),
      nClauses_(0) {
  }

  // variable
  
  int CadicalSolver::NewVar() {
    nVars_++;
    return solver_.declare_one_more_variable();
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
      solver_.add(*it);
    }
    solver_.add(0);
    nClauses_++;
  }
  
  // solve
  
  Status CadicalSolver::Solve() {
    int res = solver_.solve();
    if(CaDiCaL::Status::SATISFIABLE == res) {
      return Status::SATISFIABLE;
    }
    if(CaDiCaL::Status::UNSATISFIABLE == res) {
      return Status::UNSATISFIABLE;
    }
    return Status::UNDETERMINED;
  }

  Status CadicalSolver::Solve(const std::vector<int> &vAssumptions, std::set<int> &sCore) {
    for(int nLit : vAssumptions) {
      solver_.assume(nLit);
    }
    int nRes = solver_.solve();
    for(int nLit : vAssumptions) {
      if(solver_.failed(nLit)) {
        sCore.insert(nLit);
      }
    }
    if(CaDiCaL::Status::SATISFIABLE == nRes) {
      return Status::SATISFIABLE;
    }
    if(CaDiCaL::Status::UNSATISFIABLE == nRes) {
      return Status::UNSATISFIABLE;
    }
    return Status::UNDETERMINED;
  }

  // value
  
  bool CadicalSolver::Value(int nLit) {
    if (nLit < 0) {
      return solver_.val(nLit) < 0;
    }
    return solver_.val(nLit) > 0;
  }
  
} // namespace boop::solver

BOOP_HEADER_END
