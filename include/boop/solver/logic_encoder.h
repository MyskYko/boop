#pragma once

#include <vector>

#include "boop/util/util.h"

BOOP_HEADER_START

namespace boop::solver {
  
  template <class Solver>
  class LogicEncoder {
  public:
    // lifecycle
    LogicEncoder(Solver &solver);

    // alias
    int Compl(int nLit);
    
    // helper with existing variables
    void And2(int nIn1, int nIn2, int nOut);
    void Or2(int nIn1, int nIn2, int nOut);
    void Xor2(int nIn1, int nIn2, int nOut);
    void AndN(std::vector<int> vLits, int nOut);
    void OrN(std::vector<int> vLits, int nOut);

    // helper with new variables
    int And2(int nIn1, int nIn2);
    int Or2(int nIn1, int nIn2);
    int Xor2(int nIn1, int nIn2);
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

  // alias
  
  template <class Solver>
  int LogicEncoder<Solver>::Compl(int nLit) {
    return solver_.Compl(nLit);
  }
  
  // helper with existing variables
  
  template <class Solver>
  void LogicEncoder<Solver>::And2(int nIn1, int nIn2, int nOut) {
    solver_.AddClause({nIn1, Compl(nOut)});
    solver_.AddClause({nIn2, Compl(nOut)});
    solver_.AddClause({Compl(nIn1), Compl(nIn2), nOut});
  }
  
  template <class Solver>
  void LogicEncoder<Solver>::Or2(int nIn1, int nIn2, int nOut) {
    And2(Compl(nIn1), Compl(nIn2), Compl(nOut));
  }

  template <class Solver>
  void LogicEncoder<Solver>::Xor2(int nIn1, int nIn2, int nOut) {
    solver_.AddClause({Compl(nIn1), nIn2, nOut});
    solver_.AddClause({nIn1, Compl(nIn2), nOut});
    solver_.AddClause({nIn1, nIn2, Compl(nOut)});
    solver_.AddClause({Compl(nIn1), Compl(nIn2), Compl(nOut)});
  }
  
  template <class Solver>
  void LogicEncoder<Solver>::AndN(std::vector<int> vLits, int nOut) {
    for(int nLit : vLits) {
      solver_.AddClause({nLit, Compl(nOut)});
    }
    for(int i = 0; i < int_size(vLits); i++) {
      vLits[i] = Compl(vLits[i]);
    }
    vLits.push_back(nOut);
    solver_.AddClause(vLits);
  }
  
  template <class Solver>
  void LogicEncoder<Solver>::OrN(std::vector<int> vLits, int nOut) {
    for(int nLit : vLits) {
      solver_.AddClause({Compl(nLit), nOut});
    }
    vLits.push_back(Compl(nOut));
    solver_.AddClause(vLits);
  }

  // helper with new variables

  template <class Solver>
  int LogicEncoder<Solver>::And2(int nIn1, int nIn2) {
    int nOut;
    if(nIn1 == solver_.zero || nIn2 == solver_.zero) {
      nOut = solver_.zero;
    } else if(nIn1 == solver_.one) {
      nOut = nIn2;
    } else if(nIn2 == solver_.one) {
      nOut = nIn1;
    } else {
      nOut = solver_.NewVar();
      solver_.internal_.AddClause({nIn1, Compl(nOut)});
      solver_.internal_.AddClause({nIn2, Compl(nOut)});
      solver_.internal_.AddClause({Compl(nIn1), Compl(nIn2), nOut});
    }
    return nOut;
  }

  template <class Solver>
  int LogicEncoder<Solver>::Or2(int nIn1, int nIn2) {
    return Compl(And2(Compl(nIn1), Compl(nIn2)));
  }

  template <class Solver>
  int LogicEncoder<Solver>::Xor2(int nIn1, int nIn2) {
    int nOut;
    if(nIn1 == solver_.zero) {
      nOut = nIn2;
    } else if(nIn1 == solver_.one) {
      nOut = Compl(nIn2);
    } else if(nIn2 == solver_.zero) {
      nOut = nIn1;
    } else if(nIn2 == solver_.one) {
      nOut = Compl(nIn1);
    } else {
      nOut = solver_.NewVar();
      Xor2(nIn1, nIn2, nOut);
    }
    return nOut;
  }

  template <class Solver>
  int LogicEncoder<Solver>::AndN(const std::vector<int> &vLits) {
    std::vector<int> vLits2;
    vLits2.reserve(vLits.size());
    for(int nLit : vLits) {
      if(nLit == solver_.one) {
        continue;
      }
      if(nLit == solver_.zero) {
        return solver_.zero;
      }
      vLits2.push_back(nLit);
    }
    if(vLits2.empty()) {
      return solver_.one;
    }
    int nOut = solver_.NewVar();
    AndN(vLits2, nOut);
    return nOut;
  }

  template <class Solver>
  int LogicEncoder<Solver>::OrN(std::vector<int> const &vLits) {
    std::vector<int> vLits2;
    vLits2.reserve(vLits.size());
    for(int nLit : vLits) {
      if(nLit == solver_.one) {
        return solver_.one;
      }
      if(nLit == solver_.zero) {
        continue;
      }
      vLits2.push_back(nLit);
    }
    if(vLits2.empty()) {
      return solver_.zero;
    }
    int nOut = solver_.NewVar();
    OrN(vLits2, nOut);
    return nOut;
  }
  
} // namespace boop::solver

BOOP_HEADER_END
