#pragma once

#include <vector>
#include <set>
#include <algorithm>

#include "boop/util/util.h"

namespace boop::solver {
  
  template <class Solver>
  class LogicEncoder {
  public:
    // lifecycle
    LogicEncoder(Solver &solver);

    // clause
    template <class It>
    void AddClause(It it, It itEnd, int n);

    // solve
    typename Solver::Status Solve(const std::vector<int> &vAssumptions, std::set<int> &sCore);

    // value
    bool Value(int i);
    
    // helper with existing variables
    void And2(int a, int b, int c);
    void Or2(int a, int b, int c);
    void Xor2(int a, int b, int c);
    void AndN(std::vector<int> vLits, int r);
    void OrN(std::vector<int> vLits, int r);

    // helper with new variables
    int And2(int a, int b);
    int Or2(int a, int b);
    int Xor2(int a, int b);
    int AndN(const std::vector<int> &vLits);
    int OrN(const std::vector<int> &vLits);

  private:
    Solver &solver_;
  };

  // lifecycle

  template <class Solver>
  LogicEncoder<Solver>::LogicEncoder(Solver &solver)
    : solver_(solver) {
  }
  
  // clause
  
  template <class Solver>
  template <class It>
  void LogicEncoder<Solver>::AddClause(It it, It itEnd, int n) {
    std::vector<int> vLits;
    vLits.reserve(n);
    for(; it != itEnd; ++it) {
      if(*it == solver_.one) {
        return;
      }
      if(*it == solver_.zero) {
        continue;
      }
      vLits.push_back(*it);
    }
    solver_.AddClauseInt(vLits);
  }

  // solve

  template <class Solver>
  typename Solver::Status LogicEncoder<Solver>::Solve(const std::vector<int> &vAssumptions, std::set<int> &sCore) {
    std::vector<int> vAssumptions2;
    vAssumptions2.reserve(vAssumptions.size());
    for(int i : vAssumptions) {
      if(i == solver_.zero) {
        sCore.insert(i);
        return Solver::Status::UNSATISFIABLE;
      }
      if(i != solver_.one) {
        vAssumptions2.push_back(i);
      }
    }
    return solver_.SolveInt(vAssumptions2, sCore);
  }
  
  // value
  
  template <class Solver>
  bool LogicEncoder<Solver>::Value(int i) {
    if(i == solver_.zero) {
      return false;
    }
    if(i == solver_.one) {
      return true;
    }
    return solver_.ValueInt(i);
  }
  
  // helper with existing variables
  
  template <class Solver>
  void LogicEncoder<Solver>::And2(int a, int b, int c) {
    solver_.AddClause({a, -c});
    solver_.AddClause({b, -c});
    solver_.AddClause({-a, -b, c});
  }
  
  template <class Solver>
  void LogicEncoder<Solver>::Or2(int a, int b, int c) {
    And2(-a, -b, -c);
  }

  template <class Solver>
  void LogicEncoder<Solver>::Xor2(int a, int b, int c) {
    solver_.AddClause({a, b, -c});
    solver_.AddClause({-a, -b, -c});
    solver_.AddClause({-a, b, c});
    solver_.AddClause({a, -b, c});
  }
  
  template <class Solver>
  void LogicEncoder<Solver>::AndN(std::vector<int> vLits, int r) {
    for(int i : vLits) {
      solver_.AddClause({i, -r});
    }
    for(int i = 0; i < int_size(vLits); i++) {
      vLits[i] = -vLits[i];
    }
    vLits.push_back(r);
    solver_.AddClause(vLits);
  }
  
  template <class Solver>
  void LogicEncoder<Solver>::OrN(std::vector<int> vLits, int r) {
    for(int i : vLits) {
      solver_.AddClause({-i, r});
    }
    vLits.push_back(-r);
    solver_.AddClause(vLits);
  }

  // helper with new variables

  template <class Solver>
  int LogicEncoder<Solver>::And2(int a, int b) {
    int c;
    if(a == solver_.zero || b == solver_.zero) {
      c = solver_.zero;
    } else if(a == solver_.one) {
      c = b;
    } else if(b == solver_.one) {
      c = a;
    } else {
      c = solver_.NewVar();
      solver_.AddClauseInt({a, -c});
      solver_.AddClauseInt({b, -c});
      solver_.AddClauseInt({-a, -b, c});
    }
    return c;
  }

  template <class Solver>
  int LogicEncoder<Solver>::Or2(int a, int b) {
    return -And2(-a, -b);
  }

  template <class Solver>
  int LogicEncoder<Solver>::Xor2(int a, int b) {
    int c;
    if(a == solver_.zero) {
      c = b;
    } else if(a == solver_.one) {
      c = -b;
    } else if(b == solver_.zero) {
      c = a;
    } else if(b == solver_.one) {
      c = -a;
    } else {
      c = solver_.NewVar();
      Xor2(a, b, c);
    }
    return c;
  }

  template <class Solver>
  int LogicEncoder<Solver>::AndN(const std::vector<int> &vLits) {
    std::vector<int> vLits2;
    vLits2.reserve(vLits.size());
    for(int i : vLits) {
      if(i == solver_.one) {
        continue;
      }
      if(i == solver_.zero) {
        return solver_.zero;
      }
      vLits2.push_back(i);
    }
    if(vLits2.empty()) {
      return solver_.one;
    }
    int r = solver_.NewVar();
    AndN(vLits2, r);
    return r;
  }

  template <class Solver>
  int LogicEncoder<Solver>::OrN(std::vector<int> const &vLits) {
    std::vector<int> vLits2;
    vLits2.reserve(vLits.size());
    for(int i : vLits) {
      if(i == solver_.one) {
        return solver_.one;
      }
      if(i == solver_.zero) {
        continue;
      }
      vLits2.push_back(i);
    }
    if(vLits2.empty()) {
      return solver_.zero;
    }
    int r = solver_.NewVar();
    OrN(vLits2, r);
    return r;
  }
  
} // namespace boop::solver
