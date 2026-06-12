#pragma once

#include <cassert>
#include <initializer_list>
#include <vector>
#include <set>
#include <cstdlib>
#include <type_traits>

#include "boop/config.h"
#include "boop/solver/types.h"
#include "boop/util/size.h"

BOOP_HEADER_START

namespace boop::solver {

  template <typename T, typename = void>
  struct has_new_vars: std::false_type {};

  template <typename T>
  struct has_new_vars<T, std::void_t<decltype(std::declval<T &>().NewVars(std::declval<int>()))>>: std::true_type {};

  template <typename T, typename = void>
  struct has_is_inconsistent: std::false_type {};

  template <typename T>
  struct has_is_inconsistent<T, std::void_t<decltype(std::declval<T &>().IsInconsistent())>>: std::true_type {};
  
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
    void Clear();

    // variable
    int NewVar(); // returned as a positive literal
    int NewVars(int n); // returns the lowest of new consecutive literals
    
    // literal
    int Regular(int nLit);
    int Compl(int nLit);
    int NotCond(int nLit, bool fCompl);
    bool IsCompl(int nLit);

    // clause
    void AddClause(std::initializer_list<int> ilsLits);
    void AddClause(const std::vector<int> &vLits);
    template <class It>
    void AddClause(It it, It itEnd, int n);
    bool IsInconsistent();

    // option
    void SetConflictLimit(int nConflictLimit);

    // solve
    Status Solve(const std::vector<int> *vAssumptions = nullptr, std::set<int> *sCore = nullptr);

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
  
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  void Solver<Internal, Logic, Cardi>::Clear() {
    internal_.Clear();
  }
  
  // variable
  
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  int Solver<Internal, Logic, Cardi>::NewVar() {
    return internal_.NewVar();
  }

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  int Solver<Internal, Logic, Cardi>::NewVars(int n) {
    if constexpr(has_new_vars<Internal>::value) {
      return internal_.NewVars(n);
    }
    int nLit = -1;
    for(int i = 0; i < n; i++) {
      int nLitNew = NewVar();
      assert(nLit == -1 || nLitNew == nLit + 1);
      nLit = nLitNew;
    }
    return nLit + 1 - n;
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
  int Solver<Internal, Logic, Cardi>::NotCond(int nLit, bool fCompl) {
    return fCompl ? -nLit : nLit;
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

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  bool Solver<Internal, Logic, Cardi>::IsInconsistent() {
    if constexpr(has_is_inconsistent<Internal>::value) {
      return internal_.IsInconsistent();
    }
    return false;
  }
  
  // option
  
  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  void Solver<Internal, Logic, Cardi>::SetConflictLimit(int nConflictLimit) {
    internal_.SetConflictLimit(nConflictLimit);
  }
  
  // solve

  template <typename Internal, template <typename> class Logic, template <typename> class Cardi>
  Status Solver<Internal, Logic, Cardi>::Solve(const std::vector<int> *vAssumptions, std::set<int> *sCore) {
    if(vAssumptions == nullptr) {
      return internal_.Solve(vAssumptions, sCore);
    }
    std::vector<int> vAssumptions2;
    vAssumptions2.reserve(vAssumptions->size());
    for(int nLit : *vAssumptions) {
      if(nLit == zero) {
        if(sCore != nullptr) {
          sCore->insert(nLit);
        }
        return Status::UNSAT;
      }
      if(nLit != one) {
        vAssumptions2.push_back(nLit);
      }
    }
    return internal_.Solve(&vAssumptions2, sCore);
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
