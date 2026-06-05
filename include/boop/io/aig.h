#pragma once

#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

  inline int DecodeAig(std::istream &in) {
    int x = 0, i = 0;
    char ch;
    while(in.get(ch) && (ch & 0x80)) {
      x |= (ch & 0x7f) << (7 * i++);
    }
    return x | (ch << (7 * i));
  }

  inline void EncodeAig(std::ostream &out, int x) {
    assert(x >= 0);
    char ch;
    while(x & ~0x7f) {
      ch = (x & 0x7f) | 0x80;
      out << ch;
      x >>= 7;
    }
    ch = x;
    out << ch;
  }
  
  template <typename Ntk>
  int ReadAigString(const std::string &str, Ntk *pNtk) {
    assert(pNtk->GetConst0() == 0);
    std::stringstream f(str);
    std::string line;
    std::getline(f, line);
    std::stringstream ss(line);
    std::string token;
    std::getline(ss, token, ' ');
    assert(token == "aig");
    std::getline(ss, token, ' ');
    int nObjs = std::stoi(token);
    std::getline(ss, token, ' ');
    int nPis = std::stoi(token);
    std::getline(ss, token, ' ');
    int nLatches = std::stoi(token);
    std::getline(ss, token, ' ');
    int nPos = std::stoi(token);
    std::getline(ss, token, ' ');
    int nInts = std::stoi(token);
    assert(nObjs == nInts + nPis + nLatches);
    nObjs++; // constant
    // contents
    pNtk->Reserve(nObjs);
    for(int i = 0; i < nPis; i++) {
      pNtk->AddPi();
    }
    std::vector<int> vLatches(nLatches);
    for(int i = 0; i < nLatches; i++) {
      std::getline(f, token);
      vLatches[i] = std::stoi(token);
      pNtk->AddPi();
    }
    std::vector<int> vPos(nPos);
    for(int i = 0; i < nPos; i++) {
      std::getline(f, token);
      vPos[i] = std::stoi(token);
    }
    for(int i = nPis + nLatches + 1; i < nObjs; i++) {
      int n0 = i + i - DecodeAig(f);
      int n1 = n0 - DecodeAig(f);
      pNtk->AddAnd(n1 >> 1, n0 >> 1, n1 & 1, n0 & 1);
    }
    for(int i = 0; i < nLatches; i++) {
      pNtk->AddPo(vLatches[i] >> 1, vLatches[i] & 1);
    }
    for(int i = 0; i < nPos; i++) {
      pNtk->AddPo(vPos[i] >> 1, vPos[i] & 1);
    }
    return nLatches;
  }

  template <typename Ntk>
  std::string CreateAig(const Ntk *pNtk, int nLatches) {
    std::vector<int> vValues(pNtk->GetNumNodes());
    int nNodes = 0;
    vValues[pNtk->GetConst0()] = nNodes++ << 1;
    pNtk->ForEachPi([&](int nId) {
      vValues[nId] = nNodes++ << 1;
    });
    pNtk->ForEachInt([&](int nId) {
      if(pNtk->GetNumFanins(nId) == 0) { // constant 1
        vValues[nId] = vValues[pNtk->GetConst0()] ^ 1;
      } else if(pNtk->GetNumFanins(nId) == 1) { // buffer/inverter
        vValues[nId] = vValues[pNtk->GetFanin(nId, 0)] ^ static_cast<int>(pNtk->GetCompl(nId, 0));
      } else {
        vValues[nId] = nNodes << 1;
        nNodes += pNtk->GetNumFanins(nId) - 1;
      }
    });
    std::stringstream ss;
    pNtk->ForEachInt([&](int nId) {
      if(pNtk->GetNumFanins(nId) > 1) {
        int i = pNtk->GetNumFanins(nId) - 1;
        int n0 = vValues[pNtk->GetFanin(nId, i)] ^ static_cast<int>(pNtk->GetCompl(nId, i));
        i--;
        int n1 = vValues[pNtk->GetFanin(nId, i)] ^ static_cast<int>(pNtk->GetCompl(nId, i));
        i--;
        if(n0 < n1) {
          std::swap(n0, n1);
        }
        EncodeAig(ss, vValues[nId] - n0);
        EncodeAig(ss, n0 - n1);
        while(i >= 0) {
          EncodeAig(ss, 2);
          EncodeAig(ss, vValues[nId] - (vValues[pNtk->GetFanin(nId, i)] ^ static_cast<int>(pNtk->GetCompl(nId, i))));
          i--;
          vValues[nId] += 2;
        }
      }
    });
    std::stringstream ss0;
    ss0 << "aig " << nNodes - 1 << " " << pNtk->GetNumPis() - nLatches << " " << nLatches << " " << pNtk->GetNumPos() - nLatches << " " << nNodes - pNtk->GetNumPis() - 1 << std::endl;
    pNtk->ForEachPoDriver([&](int nFi, bool fCompl) {
      ss0 << (vValues[nFi] ^ static_cast<int>(fCompl)) << std::endl;
    });
    return ss0.str() + ss.str();
  }

  template <typename Ntk>
  int ReadAig(const std::string &filename, Ntk *pNtk) {
    std::stringstream ss;
    std::ifstream f(filename, std::ios_base::binary);
    ss << f.rdbuf();
    std::string str = ss.str();
    return ReadAigString(str, pNtk);
  }

  template <typename Ntk>
  void WriteAig(const std::string &filename, const Ntk *pNtk, int nLatches = 0) {
    std::string str = CreateAig(pNtk, nLatches);
    std::ofstream f(filename, std::ios_base::binary);
    f << str;
  }

} // namespace boop

BOOP_HEADER_END
