#pragma once

#include <initializer_list>
#include <vector>
#include <set>

#include <cadical.hpp>

#include "boop/solver/logic_encoder.h"
#include "boop/solver/cardinality_encoder.h"

namespace boop::solver {

  class CadicalSolver {
  public:
    static constexpr int zero = 0x7fffffff;
    static constexpr int one = -zero;

    enum class Status {
      SATISFIABLE,
      UNSATISFIABLE,
      UNDETERMINED
    };
    
    LogicEncoder<CadicalSolver> logic;
    CardinalityEncoder<CadicalSolver, true> cardinality;

    // lifecycle
    CadicalSolver();
    
    // variable
    int NewVar();

    // clause
    void AddClause(std::initializer_list<int> ilsLits);
    void AddClause(const std::vector<int> &vLits);

    //solve
    Status Solve();
    Status Solve(const std::vector<int> &vAssumptions, std::set<int> &sCore);

    // result
    bool Value(int i);

  private:
    int nVars_;
    int nClauses_;
    CaDiCaL::Solver s_;

    template <class It>
    void AddClauseIntRange(It it, It itEnd);
    void AddClauseInt(std::initializer_list<int> ilsLits);
    void AddClauseInt(const std::vector<int> &vLits);

    Status SolveInt(const std::vector<int> &vAssumptions, std::set<int> &sCore);

    bool ValueInt(int i);
    void AtMostOneInt(const std::vector<int> &vLits);
    void AtMostKInt(const std::vector<int> &vLits, int k);

    friend class LogicEncoder<CadicalSolver>;
    friend class CardinalityEncoder<CadicalSolver, false>;
    friend class CardinalityEncoder<CadicalSolver, true>;
  };

  // lifecycle

  CadicalSolver::CadicalSolver()
    : logic(*this),
      cardinality(*this),
      nVars_(0),
      nClauses_(0) {
  }

  // variable
  
  int CadicalSolver::NewVar() {
    nVars_++;
    return s_.declare_one_more_variable();
  }

  // clause
  
  void CadicalSolver::AddClause(std::initializer_list<int> ilsLits) {
    logic.AddClause(ilsLits.begin(), ilsLits.end(), int_size(ilsLits));
  }
  
  void CadicalSolver::AddClause(const std::vector<int> &vLits) {
    logic.AddClause(vLits.begin(), vLits.end(), int_size(vLits));
  }

  // solve
  
  CadicalSolver::Status CadicalSolver::Solve() {
    int res = s_.solve();
    if(CaDiCaL::Status::SATISFIABLE == res) {
      return Status::SATISFIABLE;
    }
    if(CaDiCaL::Status::UNSATISFIABLE == res) {
      return Status::UNSATISFIABLE;
    }
    return Status::UNDETERMINED;
  }

  CadicalSolver::Status CadicalSolver::Solve(const std::vector<int> &vAssumptions, std::set<int> &sCore) {
    return logic.Solve(vAssumptions, sCore);
  }

  // value

  bool CadicalSolver::Value(int i) {
    return logic.Value(i);
  }
  
  // private

  template <class It>
  void CadicalSolver::AddClauseIntRange(It it, It itEnd) {
    for(; it != itEnd; ++it) {
      assert(*it != zero && *it != one);
      s_.add(*it);
    }
    s_.add(0);
    nClauses_++;
  }
  
  void CadicalSolver::AddClauseInt(std::initializer_list<int> ilsLits) {
    AddClauseIntRange(ilsLits.begin(), ilsLits.end());
  }
  
  void CadicalSolver::AddClauseInt(const std::vector<int> &vLits) {
    AddClauseIntRange(vLits.begin(), vLits.end());
  }

  CadicalSolver::Status CadicalSolver::SolveInt(const std::vector<int> &vAssumptions, std::set<int> &sCore) {
    for(int i : vAssumptions) {
      s_.assume(i);
    }
    int res = s_.solve();
    for(int i : vAssumptions) {
      if(s_.failed(i)) {
        sCore.insert(i);
      }
    }
    if(CaDiCaL::Status::SATISFIABLE == res) {
      return Status::SATISFIABLE;
    }
    if(CaDiCaL::Status::UNSATISFIABLE == res) {
      return Status::UNSATISFIABLE;
    }
    return Status::UNDETERMINED;
  }
  
  bool CadicalSolver::ValueInt(int i) {
    return s_.val(i) > 0;
  }
  
  void CadicalSolver::AtMostOneInt(const std::vector<int> &vLits) {
    cardinality.Bimander(vLits, 2);
  }
  
  void CadicalSolver::AtMostKInt(const std::vector<int> &vLits, int k) {
    std::vector<int> res;
    cardinality.OddEvenSel4(vLits, res, k + 1);
    AddClause({-res[k]});
  }

} // namespace boop::solver
