#pragma once

#include <vector>
#include <algorithm>

#include "boop/util/util.h"

namespace boop::solver {

  template <class Solver, bool kDirect = false>
  class CardinalityEncoder {
  public:
    // lifecycle
    CardinalityEncoder(Solver &solver);

    // interface
    void AtMostOne(const std::vector<int> &vLits);
    void Onehot(const std::vector<int> &vLits);
    void AtMostK(const std::vector<int> &vLits, int k);

    // at most one
    void Pairwise(const std::vector<int> &vLits);
    void Bimander(const std::vector<int> &vLits, int nBim);
    
    // at most k
    void PwNet(std::vector<int> vLits, std::vector<int> &vRes);
    void OddEvenSel4(const std::vector<int> &vLits, std::vector<int> &vRes, int k);
    void PairwiseSel(const std::vector<int> &vLits, std::vector<int> &vRes, int k);
  
  private:
    Solver &solver_;

    // helper
    void CheckNoConstants(const std::vector<int> &vLits);
    void Comparator2(int x1, int x2, int y1, int y2);

    // pw
    void Comparator(int a, int b, int &c, int &d);
    void PwSplit(const std::vector<int> &vLits, std::vector<int> &v1, std::vector<int> &v2);
    void PwMerge(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes);
    void PwSort(const std::vector<int> &vLits, std::vector<int> &vRes);

    // direct
    static bool PreferDirectMerge(int n, int k);
    void DirectMerge(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes, int k);
    void DirectCardClauses(const std::vector<int> &vLits, int nStart, int nPos, int j, std::vector<int> &vArgs);
    void DirectNetwork(const std::vector<int> &vLits, std::vector<int> &vRes, int k);
    void DirectCombine4(const std::vector<int> &x, const std::vector<int> &y, std::vector<int>& vOutVars, int k);

    // odd even 4
    void OddEvenCombine(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes, int k);
    void OddEvenMerge4(const std::vector<int> in[], std::vector<int> &vRes, int k);
    void OddEvenSel4Rec(const std::vector<int> &vLits, std::vector<int> &vRes, int k);

    // pairwise
    void PairwiseMerge(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes, int k);
    void DirectPairwiseMerge(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes, int k);
    void PairwiseSelRec(const std::vector<int> &vLits, std::vector<int> &vRes, int k);
  };

  // lifecycle

  template <class Solver, bool kDirect>
  CardinalityEncoder<Solver, kDirect>::CardinalityEncoder(Solver &solver)
    : solver_(solver) {
  }

  // interface

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::AtMostOne(const std::vector<int> &vLits) {
    std::vector<int> vLits2;
    vLits2.reserve(vLits.size());
    bool fOne = false;
    for(int i : vLits) {
      if(i == solver_.one) {
        if(fOne) {
          solver_.AddClauseInt({});
          return;
        }
        fOne = true;
        continue;
      }
      if(i == solver_.zero) {
        continue;
      }
      vLits2.push_back(i);
    }
    if(fOne) {
      for(int i : vLits2) {
        solver_.AddClauseInt({-i});
      }
      return;
    }
    solver_.AtMostOneInt(vLits2);
  }

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::Onehot(const std::vector<int> &vLits) {
    AtMostOne(vLits);
    solver_.AddClause(vLits);
  }

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::AtMostK(const std::vector<int> &vLits, int k) {
    if(k < 0) {
      solver_.AddClauseInt({});
      return;
    }
    std::vector<int> vLits2;
    vLits2.reserve(vLits.size());
    for(int i : vLits) {
      if(i == solver_.one) {
        if(!k) {
          solver_.AddClauseInt({});
          return;
        }
        k--;
        continue;
      }
      if(i == solver_.zero) {
        continue;
      }
      vLits2.push_back(i);
    }
    if(int_size(vLits2) <= k) {
      return;
    }
    if(!k) {
      for(int i : vLits2) {
        solver_.AddClauseInt({-i});
      }
      return;
    }
    if(k == 1) {
      solver_.AtMostOneInt(vLits2);
    } else {
      solver_.AtMostKInt(vLits2, k);
    }
  }

  // at most one
  
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::Pairwise(const std::vector<int> &vLits) {
    CheckNoConstants(vLits);
    for(int i = 1; i < int_size(vLits); i++) {
      for(int j = 0; j < i; j++) {
        solver_.AddClauseInt({-vLits[i], -vLits[j]});
      }
    }
  }

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::Bimander(const std::vector<int> &vLits, int nBim) {
    CheckNoConstants(vLits);
    assert(nBim > 0);
    const int n = int_size(vLits);
    const int nBins = (n + nBim - 1) / nBim;
    const int nWidth = clog2(nBins);
    std::vector<int> vBinary(nWidth);
    for(int i = 0; i < nWidth; i++) {
      vBinary[i] = solver_.NewVar();
    }
    std::vector<int> vLits2;
    vLits2.reserve(nBim);
    for(int i = 0; i < nBins; i++) {
      vLits2.clear();
      for(int j = 0; j < nBim && i * nBim + j < n; j++) {
        vLits2.push_back(vLits[i * nBim + j]);
      }
      const int nLits = int_size(vLits2);
      if(nLits > 1) {
        for(int p = 0; p < nLits; p++) {
          for(int q = p + 1; q < nLits; q++) {
            solver_.AddClauseInt({-vLits2[p], -vLits2[q]});
          }
        }
      }
      for(int k = 0; k < nWidth; k++) {
        if((i >> k) & 1) {
          for(int j = 0; j < nLits; j++) {
            solver_.AddClauseInt({-vLits2[j], vBinary[k]});
          }
        } else {
          for(int j = 0; j < nLits; j++) {
            solver_.AddClauseInt({-vLits2[j], -vBinary[k]});
          }
        }
      }
    }
  }

  // at most k
  
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::PwNet(std::vector<int> vLits, std::vector<int> &vRes) {
    vRes.clear();
    const int n = pow2_ceil(int_size(vLits));
    vLits.resize(n, solver_.zero);
    PwSort(vLits, vRes);
  }

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::OddEvenSel4(const std::vector<int> &vLits, std::vector<int> &vRes, int k) {
    CheckNoConstants(vLits);
    vRes.clear();
    if(vLits.empty()) {
      return;
    }
    OddEvenSel4Rec(vLits, vRes, k);
  }

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::PairwiseSel(const std::vector<int> &vLits, std::vector<int> &vRes, int k) {
    CheckNoConstants(vLits);
    vRes.clear();
    PairwiseSelRec(vLits, vRes, k);
  }

  // helper

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::CheckNoConstants(const std::vector<int> &vLits) {
    for(int i : vLits) {
      assert(i != solver_.zero);
      assert(i != solver_.one);
    }
  }
  
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::Comparator(int a, int b, int &c, int &d) {
    // a and b may be solver_.zero
    c = solver_.logic.Or2(a, b);
    d = solver_.logic.And2(a, b);
  }

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::Comparator2(int x1, int x2, int y1, int y2) {
    // x1 or x2 -> y1
    // x1 and x2 -> y2
    solver_.AddClauseInt({-x1, y1});
    solver_.AddClauseInt({-x2, y1});
    solver_.AddClauseInt({-x1, -x2, y2});
  }

  // pw

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::PwSplit(const std::vector<int> &vLits, std::vector<int> &v1, std::vector<int> &v2) {
    assert(vLits.size() % 2 == 0);
    int n = int_size(vLits) / 2;
    v1.resize(n);
    v2.resize(n);
    for(int i = 0; i < n; i++) {
      Comparator(vLits[i + i], vLits[i + i + 1], v1[i], v2[i]);
    }
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::PwMerge(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes) {
    std::vector<int> vNext1, vNext2, vOut1, vOut2;
    assert(v1.size() == v2.size());
    int n = int_size(v1);
    if(n == 1) {
      vRes.push_back(v1[0]);
      vRes.push_back(v2[0]);
      return;
    }
    assert(n % 2 == 0);
    vNext1.resize(n / 2);
    vNext2.resize(n / 2);
    for(int i = 0; i < n / 2; i++) {
      vNext1[i] = v1[i + i];
      vNext2[i] = v2[i + i];
    }
    PwMerge(vNext1, vNext2, vOut1);
    for(int i = 0; i < n / 2; i++) {
      vNext1[i] = v1[i + i + 1];
      vNext2[i] = v2[i + i + 1];
    }
    PwMerge(vNext1, vNext2, vOut2);
    vRes.resize(n + n);
    vRes[0] = vOut1[0];
    for(int i = 0; i < n - 1; i++) {
      Comparator(vOut2[i], vOut1[i + 1], vRes[i + i + 1], vRes[i + i + 2]);
    }
    vRes[n + n - 1] = vOut2[n - 1];
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::PwSort(const std::vector<int> &vLits, std::vector<int> &vRes) {
    assert(vRes.empty());
    if(vLits.size() == 1) {
      vRes.push_back(vLits[0]);
      return;
    }
    std::vector<int> v1, v2, vOut1, vOut2;
    PwSplit(vLits, v1, v2);
    PwSort(v1, vOut1);
    PwSort(v2, vOut2);
    PwMerge(vOut1, vOut2, vRes);
  }

  // direct

  template <class Solver, bool kDirect>
  bool CardinalityEncoder<Solver, kDirect>::PreferDirectMerge(int n, int k) {
    static const int minTest = 94, maxTest = 183;
    static const int nBound[] = {94+171, 95+150, 96+177, 97+156, 98+135, 99+126,
                                 100+141,101+128,102+119,103+110,104+121,105+112,106+103,107+98, 108+109,109+100,
                                 110+95, 111+90, 112+97, 113+92, 114+84, 115+82, 116+89, 117+84, 118+76, 119+74,
                                 120+81, 121+76, 122+71, 123+70, 124+73, 125+69, 126+67, 127+63, 128+69, 129+64,
                                 130+63, 131+58, 132+62, 133+60, 134+56, 135+54, 136+57, 137+56, 138+52, 139+50,
                                 140+53, 141+52, 142+48, 143+46, 144+49, 145+48, 146+44, 147+43, 148+45, 149+44,
                                 150+41, 151+39, 152+42, 153+40, 154+39, 155+38, 156+38, 157+37, 158+35, 159+34,
                                 160+37, 161+33, 162+32, 163+31, 164+33, 165+32, 166+28, 167+27, 168+29, 169+28,
                                 170+27, 171+26, 172+26, 173+24, 174+23, 175+22, 176+22, 177+21, 178+20, 179+19,
                                 180+21, 181+20, 182+16, 183+15, 184+17, 185+16, 186+15, 187+14, 188+13, 189+12,
                                 190+11, 191+10, 192+10, 193+9,  194+8,  195+7,  196+6,  197+5,  198+4,  199+3,
                                 200+2,  201+1};
    if(k > n) {
      k = n;
    }
    if(k == 1) {
      return true;
    }
    if(k >= 4 && k < minTest && n >= 10) {
      return true;
    }
    if(k >= minTest && k <= maxTest && n < nBound[k - minTest]) {
      return true;
    }
    return false;
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::DirectMerge(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes, int k) {
    assert(vRes.empty());
    int n1 = std::min(k, int_size(v1));
    int n2 = std::min(k, int_size(v2));
    if(k > n1 + n2) {
      k = n1 + n2;
    }
    vRes.reserve(k);
    if(n2 == 0) {
      for(int i = 0; i < k; i++) {
        vRes.push_back(v1[i]);
      }
      return;
    }
    if(n1 == 0) {
      for(int i = 0; i < k; i++) {
        vRes.push_back(v2[i]);
      }
      return;
    } 
    for(int i = 0; i < k; i++) {
      vRes.push_back(solver_.NewVar());
    }
    for(int i = 0; i < n1; i++) {
      solver_.AddClauseInt({-v1[i], vRes[i]});
    }
    for(int i = 0; i < n2; i++) {
      solver_.AddClauseInt({-v2[i], vRes[i]});
    }
    for(int j = 0; j < n2; j++) {
      for(int i = 0; i < std::min(n1, k - j - 1); i++) {
        solver_.AddClauseInt({-v1[i], -v2[j], vRes[i + j + 1]});
      }
    }
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::DirectCardClauses(const std::vector<int> &vLits, int nStart, int nPos, int j, std::vector<int> &vArgs) {
    if(nPos == j) {
      solver_.AddClauseInt(vArgs);
      return;
    }
    int n = int_size(vLits);
    for(int i = nStart; i <= n - (j - nPos); i++) {
      vArgs[nPos] = -vLits[i];
      DirectCardClauses(vLits, i + 1, nPos + 1, j, vArgs);
    }
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::DirectNetwork(const std::vector<int> &vLits, std::vector<int> &vRes, int k) {
    assert(vRes.empty());
    int n = vLits.size();
    if(k == 0 || k > n) {
      k = n;
    }
    vRes.reserve(k);
    for(int i = 0; i < k; i++) {
      vRes.push_back(solver_.NewVar());
    }
    for(int j = 1; j <= k; j++) {
      std::vector<int> vArgs(j, solver_.zero);
      vArgs.push_back(vRes[j - 1]);
      DirectCardClauses(vLits, 0, 0, j, vArgs);
    }
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::DirectCombine4(std::vector<int> const &v1, std::vector<int> const &v2, std::vector<int>& vRes, int k) {
    assert(vRes.empty());
    int n1 = int_size(v1);
    int n2 = int_size(v2);
    assert(n1 >= n2);
    assert(n1 <= n2 + 4);
    assert(n1 >= 2);
    assert(n2 >= 1);
    if(k > n1 + n2) {
      k = n1 + n2;
    }
    vRes.reserve(k + 1);
    vRes.push_back(v1[0]);
    int nLast = (k < n1 + n2 || k % 2 == 1 || n1 == n2 + 2)? k : k - 1;
    for(int i = 0, j = 1; j < nLast; j++, i = j / 2) {
      int ret = solver_.NewVar();
      vRes.push_back(ret);
      if(j % 2 == 0) {
        if(i + 1 < n1 && i < n2 + 2) {
          if(i >= 2) {
            solver_.AddClauseInt({-v1[i + 1], -v2[i - 2], ret});
          } else {
            solver_.AddClauseInt({-v1[i + 1], ret});
          }
        }
        if(i < n1 && i < n2 + 1) {
          solver_.AddClauseInt({-v1[i], -v2[i - 1], ret});
        }
      } else {
        if(i > 0 && i + 2 < n1) {
          solver_.AddClauseInt({-v1[i + 2], ret});
        }
        if(i < n2) {
          solver_.AddClauseInt({-v2[i], ret});
        }
        if(i + 1 < n1 && i < n2 + 1) {
          if(i > 0) {
            solver_.AddClauseInt({-v1[i + 1], -v2[i - 1], ret});
          } else {
            solver_.AddClauseInt({-v1[i + 1], ret});
          }
        }
      }
    }
    if(k == n1 + n2 && k % 2 == 0 && n1 != n2 + 2) {
      vRes.push_back(n1 == n2 ? v2[n2 - 1] : v1[n1 - 1]);
    }
    if(k < n1 + n2) {
      solver_.AddClauseInt({-v1[n1 - 1], -v2[n2 - 1]});
      if(k + 1 < n1 + n2) {
        solver_.AddClauseInt({-v1[n1 - 2], -v2[n2 - 1]});
        if(n2 >= 2) {
          solver_.AddClauseInt({-v1[n1 - 1], -v2[n2 - 2]});
        }
      }
    }
  }

  
  // odd even 4

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::OddEvenCombine(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes, int k) {
    assert(vRes.empty());
    int n1 = int_size(v1);
    int n2 = int_size(v2);
    if(k > n1 + n2) {
      k = n1 + n2;
    }
    vRes.reserve((k + 1) / 2 + 1);
    vRes.push_back(v1[0]);
    for(int i = 0; i < (k - 1) / 2; i++) {
      vRes.push_back(solver_.NewVar());
      vRes.push_back(solver_.NewVar());
      Comparator2(v2[i], v1[i + 1], vRes[i * 2 + 1], vRes[i * 2 + 2]);
    }
    if(k % 2 == 0) {
      if(k < n1 + n2) {
        int ret = solver_.NewVar();
        vRes.push_back(ret);
        solver_.AddClauseInt({-v2[k / 2 - 1], ret});
        solver_.AddClauseInt({-v1[k / 2], ret});
      } else if(n1 == n2) {
        vRes.push_back(v2[k / 2 - 1]);
      } else {
        vRes.push_back(v1[k / 2]);
      }
    }
    if(k < n1 + n2) {
      solver_.AddClauseInt({-v1[n1 - 1], -v2[n2 - 1]});
    }
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::OddEvenMerge4(const std::vector<int> vIns[], std::vector<int> &vRes, int k) {
    assert(vRes.empty());
    int nn[4] = {int_size(vIns[0]), int_size(vIns[1]), int_size(vIns[2]), int_size(vIns[3])};
    assert(nn[0] > 0);
    assert(nn[0] >= nn[1]);
    assert(nn[1] >= nn[2]);
    assert(nn[2] >= nn[3]);
    k = std::min(k, nn[0] + nn[1] + nn[2] + nn[3]);
    for(int j = 0; j < 4; j++) {
      if(nn[j] > k){
        nn[j] = k;
      }
    }
    if(nn[1] == 0) {
      vRes = vIns[0];
      return;
    }
    if(nn[0] == 1) {
      std::vector<int> vLits;
      vLits.reserve(4);
      for(int j = 0; j < 4; j++) {
        if(nn[j] > 0) {
          vLits.push_back(vIns[j][0]);
        }
      }
      DirectNetwork(vLits, vRes, k);
      return;
    }
    std::vector<int> vOddEvens[2][4], vOut1, vOut2;
    for(int j = 0; j < 4; j++) {
      vOddEvens[0][j].reserve((nn[j] + 1) / 2);
      vOddEvens[1][j].reserve(nn[j] / 2);
    }
    for(int j = 0; j < 4; j++) {
      for(int i = 0; i < nn[j]; i++) {
        vOddEvens[i % 2][j].push_back(vIns[j][i]);
      }
    }
    OddEvenMerge4(vOddEvens[0], vOut1, k / 2 + 2);
    OddEvenMerge4(vOddEvens[1], vOut2, k / 2);
    if(nn[2] > 0) {
      DirectCombine4(vOut1, vOut2, vRes, k);
    } else {
      OddEvenCombine(vOut1, vOut2, vRes, k);
    }
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::OddEvenSel4Rec(const std::vector<int> &vLits, std::vector<int> &vRes, int k) {
    int n = int_size(vLits);
    assert(k >= 0);
    assert(k <= n);
    assert(n > 0);
    if(k == 0) {
      for(int i : vLits) {
        solver_.AddClauseInt({-i});
      }
      return;
    }
    if(n == 1) {
      vRes.push_back(vLits[0]);
      return;
    }
    if(n <= 4 || (kDirect && (k <= 1 || (k == 2 && n <= 9) || n <= 6))) {
      DirectNetwork(vLits, vRes, k);
      return;
    }
    int nn[4], kk[4];
    int p2 = pow2_ceil((k + 5) / 6);
    if(n >= 8 && 4 * p2 <= n)  {
      nn[1] = nn[2] = nn[3] = p2;
    } else if(n < 8 || k == n) {
      nn[1] = (n + 2) / 4;
      nn[2] = (n + 1) / 4;
      nn[3] = n / 4;
    } else {
      nn[1] = nn[2] = nn[3] = k / 4;
    }
    nn[0] = n - nn[1] - nn[2] - nn[3];
    std::vector<int> vIns[4], vOuts[4];
    for(int base = 0, j = 0; j < 4; base += nn[j], j++) {
      for(int i = 0; i < nn[j]; i++) {
        vIns[j].push_back(vLits[base + i]);
      }
    }
    for(int j = 0; j < 4; j++) {
      kk[j] = std::min(k, nn[j]);
      OddEvenSel4Rec(vIns[j], vOuts[j], kk[j]);
    }
    if(kDirect && PreferDirectMerge(kk[0] + kk[1] + kk[2] + kk[3], k)) {
      std::vector<int> vOut1, vOut2;
      DirectMerge(vOuts[0], vOuts[1], vOut1, std::min(kk[0] + kk[1], k));
      DirectMerge(vOuts[2], vOuts[3], vOut2, std::min(kk[2] + kk[3], k));
      DirectMerge(vOut1, vOut2, vRes, k);
    } else {
      OddEvenMerge4(vOuts, vRes, k);
    }
  }

  // pairwise

  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::DirectPairwiseMerge(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes, int k) {
    assert(vRes.empty());
    int n1 = std::min(k, int_size(v1));
    int n2 = std::min(k, int_size(v2));
    if(k > n1 + n2) {
      k = n1 + n2;
    }
    vRes.reserve(k);
    if(n2 == 0) {
      for(int i = 0; i < k; i++) {
        vRes.push_back(v1[i]);
      }
      return;
    }
    for(int i = 0; i < k; i++) {
      vRes.push_back(solver_.NewVar());
    }
    for(int i = 0; i < n1; i++) {
      solver_.AddClauseInt({-v1[i], vRes[i]});
    }
    for(int i = 0; i < std::min(n2, k / 2); i++) {
      solver_.AddClauseInt({-v2[i], vRes[2 * i + 1]});
    }
    for(int j = 0; j < n2; j++) {
      for(int i = j + 1; i < std::min(n1, k - j - 1); i++) {
        solver_.AddClauseInt({-v1[i], -v2[j], vRes[i + j + 1]});
      }
    }
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::PairwiseMerge(const std::vector<int> &v1, const std::vector<int> &v2, std::vector<int> &vRes, int k) {
    assert(vRes.empty());
    int n1 = int_size(v1);
    int n2 = int_size(v2);
    std::vector<int> v1Int = v1, v2Int = v2;
    int h = pow2_ceil(n1);
    for(; n2 < k / 2; n2++) {
      v2Int.push_back(solver_.zero);
    }
    while(h > 1) {
      h = h / 2;
      for(int j = 0; j < n2; j++) {
        if(j + h < n1) {
          int nOut1, nOut2;
          if(v1Int[j + h] == solver_.zero) {
            nOut1 = v1Int[j + h];
            nOut2 = v2Int[j];
          } else if(v2Int[j] == solver_.zero) {
            nOut2 = v1Int[j + h];
            nOut1 = v2Int[j];
          } else {
            nOut1 = solver_.NewVar();
            nOut2 = solver_.NewVar();
            Comparator2(v1Int[j + h], v2Int[j], nOut2, nOut1);
          }
          v1Int[j + h] = nOut1;
          v2Int[j] = nOut2;
        }
      }
    }
    vRes.reserve(k);
    for(int j = 0; j < k; j++) {
      if(j % 2 == 0) {
        vRes.push_back(v1Int[j / 2]);
      } else {
        vRes.push_back(v2Int[j / 2]);
      }
      assert(vRes[j] != solver_.zero);
    }
    for(int j = (k + 1) / 2; j < n1; j++) {
      if(v1Int[j] != solver_.zero) {
        solver_.AddClauseInt({-v1Int[j]});
      }
    }
  }
  template <class Solver, bool kDirect>
  void CardinalityEncoder<Solver, kDirect>::PairwiseSelRec(const std::vector<int> &vLits, std::vector<int> &vRes, int k) {
    assert(vRes.empty());
    int n = int_size(vLits);
    assert(k >= 0);
    k = std::min(k, n);
    if(k == 0) {
      for(int i : vLits) {
        solver_.AddClauseInt({-i});
      }
      return;
    }
    if(n == 1) {
      vRes.push_back(vLits[0]);
      return;
    }
    if(n <= 2 || (kDirect && (k <= 1 || (k == 2 && n <= 9) || n <= 6))) {
      DirectNetwork(vLits, vRes, k);
      return;
    }
    int n1, n2;
    if(n <= 7) {
      n2 = n / 2;
    } else {
      int p2 = pow2_ceil((k + 2) / 3);
      if(p2 <= k / 2) {
        n2 = p2;
      } else {
        n2 = k - p2;
      }
    }
    n1 = n - n2;
    std::vector<int> v1, v2;
    v1.reserve(n1);
    v2.reserve(n2);
    for(int i = 0; i < n2; i++) {
      v1.push_back(solver_.NewVar());
      v2.push_back(solver_.NewVar());
      Comparator2(vLits[2 * i], vLits[2 * i + 1], v1[i], v2[i]);
    }
    for(int i = n2; i < n1; i++) {
      v1.push_back(vLits[n2 + i]);
    }
    std::vector<int> vOut1, vOut2;
    PairwiseSelRec(v1, vOut1, std::min(k, n1));
    PairwiseSelRec(v2, vOut2, std::min(k / 2, n2));
    if(kDirect && PreferDirectMerge(int_size(vOut1) + int_size(vOut2), k)) {
      DirectPairwiseMerge(vOut1, vOut2, vRes, k);
    } else {
      PairwiseMerge(vOut1, vOut2, vRes, k);
    }
  }

} // namespace boop::solver
