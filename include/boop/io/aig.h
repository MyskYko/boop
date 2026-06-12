#pragma once

#include <cassert>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

inline int DecodeAig(std::istream &is) {
  int nValue = 0, nShift = 0;
  char cByte;
  while (is.get(cByte) && (cByte & 0x80)) {
    nValue |= (cByte & 0x7f) << (7 * nShift++);
  }
  return nValue | (cByte << (7 * nShift));
}

inline void EncodeAig(std::ostream &os, int nValue) {
  assert(nValue >= 0);
  char cByte;
  while (nValue & ~0x7f) {
    cByte = (nValue & 0x7f) | 0x80;
    os << cByte;
    nValue >>= 7;
  }
  cByte = nValue;
  os << cByte;
}

template <typename Ntk>
int ReadAigString(const std::string &strAig, Ntk *pNtk) {
  assert(pNtk->GetConst0() == 0);
  std::stringstream ssInput(strAig);
  std::string strLine;
  std::getline(ssInput, strLine);
  std::stringstream ssHeader(strLine);
  std::string strToken;
  std::getline(ssHeader, strToken, ' ');
  assert(strToken == "aig");
  std::getline(ssHeader, strToken, ' ');
  int nObjs = std::stoi(strToken);
  std::getline(ssHeader, strToken, ' ');
  int nPis = std::stoi(strToken);
  std::getline(ssHeader, strToken, ' ');
  int nLatches = std::stoi(strToken);
  std::getline(ssHeader, strToken, ' ');
  int nPos = std::stoi(strToken);
  std::getline(ssHeader, strToken, ' ');
  int nInts = std::stoi(strToken);
  assert(nObjs == nInts + nPis + nLatches);
  nObjs++; // constant
  // contents
  pNtk->Reserve(nObjs);
  for (int i = 0; i < nPis; i++) {
    pNtk->AddPi();
  }
  std::vector<int> vLatches(nLatches);
  for (int i = 0; i < nLatches; i++) {
    std::getline(ssInput, strToken);
    vLatches[i] = std::stoi(strToken);
    pNtk->AddPi();
  }
  std::vector<int> vPos(nPos);
  for (int i = 0; i < nPos; i++) {
    std::getline(ssInput, strToken);
    vPos[i] = std::stoi(strToken);
  }
  for (int i = nPis + nLatches + 1; i < nObjs; i++) {
    int nLit0 = i + i - DecodeAig(ssInput);
    int nLit1 = nLit0 - DecodeAig(ssInput);
    pNtk->AddAnd(nLit1 >> 1, nLit0 >> 1, nLit1 & 1, nLit0 & 1);
  }
  for (int i = 0; i < nLatches; i++) {
    pNtk->AddPo(vLatches[i] >> 1, vLatches[i] & 1);
  }
  for (int i = 0; i < nPos; i++) {
    pNtk->AddPo(vPos[i] >> 1, vPos[i] & 1);
  }
  return nLatches;
}

template <typename Ntk> std::string CreateAig(const Ntk *pNtk, int nLatches) {
  std::vector<int> vValues(pNtk->GetNumNodes());
  int nNodes = 0;
  vValues[pNtk->GetConst0()] = nNodes++ << 1;
  pNtk->ForEachPi([&](int nId) { vValues[nId] = nNodes++ << 1; });
  pNtk->ForEachInt([&](int nId) {
    if (pNtk->GetNumFanins(nId) == 0) { // constant 1
      vValues[nId] = vValues[pNtk->GetConst0()] ^ 1;
    } else if (pNtk->GetNumFanins(nId) == 1) { // buffer/inverter
      vValues[nId] = vValues[pNtk->GetFanin(nId, 0)] ^
                     static_cast<int>(pNtk->GetCompl(nId, 0));
    } else {
      vValues[nId] = nNodes << 1;
      nNodes += pNtk->GetNumFanins(nId) - 1;
    }
  });
  std::stringstream ssBody;
  pNtk->ForEachInt([&](int nId) {
    if (pNtk->GetNumFanins(nId) > 1) {
      int i = pNtk->GetNumFanins(nId) - 1;
      int nLit0 = vValues[pNtk->GetFanin(nId, i)] ^
                  static_cast<int>(pNtk->GetCompl(nId, i));
      i--;
      int nLit1 = vValues[pNtk->GetFanin(nId, i)] ^
                  static_cast<int>(pNtk->GetCompl(nId, i));
      i--;
      if (nLit0 < nLit1) {
        std::swap(nLit0, nLit1);
      }
      EncodeAig(ssBody, vValues[nId] - nLit0);
      EncodeAig(ssBody, nLit0 - nLit1);
      while (i >= 0) {
        EncodeAig(ssBody, 2);
        EncodeAig(ssBody,
                  vValues[nId] - (vValues[pNtk->GetFanin(nId, i)] ^
                                  static_cast<int>(pNtk->GetCompl(nId, i))));
        i--;
        vValues[nId] += 2;
      }
    }
  });
  std::stringstream ssHeader;
  ssHeader << "aig " << nNodes - 1 << " " << pNtk->GetNumPis() - nLatches << " "
           << nLatches << " " << pNtk->GetNumPos() - nLatches << " "
           << nNodes - pNtk->GetNumPis() - 1 << std::endl;
  pNtk->ForEachPoDriver([&](int nFi, bool fCompl) {
    ssHeader << (vValues[nFi] ^ static_cast<int>(fCompl)) << std::endl;
  });
  return ssHeader.str() + ssBody.str();
}

template <typename Ntk> int ReadAig(const std::string &strFilename, Ntk *pNtk) {
  std::stringstream ssContents;
  std::ifstream ifs(strFilename, std::ios_base::binary);
  ssContents << ifs.rdbuf();
  std::string strAig = ssContents.str();
  return ReadAigString(strAig, pNtk);
}

template <typename Ntk>
void WriteAig(const std::string &strFilename, const Ntk *pNtk,
              int nLatches = 0) {
  std::string strAig = CreateAig(pNtk, nLatches);
  std::ofstream ofs(strFilename, std::ios_base::binary);
  ofs << strAig;
}

} // namespace boop

BOOP_HEADER_END
