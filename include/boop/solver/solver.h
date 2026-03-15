#pragma once

#include <initializer_list>
#include <vector>
#include <set>
#include <cstdlib>

BOOP_HEADER_START

namespace boop::solver {
  
  enum class Status {
    SATISFIABLE,
    UNSATISFIABLE,
    UNDETERMINED,
  };
    
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  class Solver {
  public:
    static constexpr int zero = 0x7fffffff;
    static constexpr int one = -zero;
    
    using This = Solver<Internal, Logic, Cardi>;
    friend class Logic<This>;
    friend class Cardi<This>;
    
    Logic<This> logic;
    Cardi<This> cardinality;

    // lifecycle
    Solver();

    // variable
    int NewVar(); // returned as a positive literal
    
    // literal
    int Regular(int nLit);
    int Compl(int nLit);
    bool IsCompl(int nLit);

    // clause
    void AddClause(std::initializer_list<int> ilsLits);
    void AddClause(const std::vector<int> &vLits);
    template <class It>
    void AddClause(It it, It itEnd, int n);

    //solve
    Status Solve();
    Status Solve(const std::vector<int> &vAssumptions, std::set<int> &sCore);

    // result
    bool Value(int nLit);

  private:
    Internal internal_;
  };

  // lifecycle

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  Solver<Internal, Logic, Cardi>::Solver()
    : logic(*this),
      cardinality(*this) {
  }
  
  // variable
  
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  int Solver<Internal, Logic, Cardi>::NewVar() {
    return internal_.NewVar();
  }

  // literal
  
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  int Solver<Internal, Logic, Cardi>::Regular(int nLit) {
    return std::abs(nLit);
  }

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  int Solver<Internal, Logic, Cardi>::Compl(int nLit) {
    return -nLit;
  }

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  bool Solver<Internal, Logic, Cardi>::IsCompl(int nLit) {
    return nLit < 0;
  }

  // clause

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  void Solver<Internal, Logic, Cardi>::AddClause(std::initializer_list<int> ilsLits) {
    AddClause(ilsLits.begin(), ilsLits.end(), int_size(ilsLits));
  }

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  void Solver<Internal, Logic, Cardi>::AddClause(const std::vector<int> &vLits) {
    AddClause(vLits.begin(), vLits.end(), int_size(vLits));
  }
  
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  template <class It>
  void Solver<Internal, Logic, Cardi>::AddClause(It it, It itEnd, int n) {
    std::vector<int> vLits;
    vLits.reserve(n);
    for(; it != itEnd; ++it) {
      if(*it == one) {
        return;
      }
      if(*it == zero) {
        continue;
      }
      vLits.push_back(*it);
    }
    internal_.AddClause(vLits);
  }

  // solve

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  Status Solver<Internal, Logic, Cardi>::Solve() {
    return internal_.Solve();
  }
  
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  Status Solver<Internal, Logic, Cardi>::Solve(const std::vector<int> &vAssumptions, std::set<int> &sCore) {
    std::vector<int> vAssumptions2;
    vAssumptions2.reserve(vAssumptions.size());
    for(int nLit : vAssumptions) {
      if(nLit == zero) {
        sCore.insert(nLit);
        return Status::UNSATISFIABLE;
      }
      if(nLit != one) {
        vAssumptions2.push_back(nLit);
      }
    }
    return internal_.Solve(vAssumptions2, sCore);
  }

  // value
  
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  bool Solver<Internal, Logic, Cardi>::Value(int nLit) {
    if(nLit == zero) {
      return false;
    }
    if(nLit == one) {
      return true;
    }
    return internal_.Value(nLit);
  }
  
} // namespace boop::solver

BOOP_HEADER_END
