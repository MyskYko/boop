#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "boop/config.h"
#include "boop/util/util.h"

BOOP_HEADER_START

namespace boop {

  template <typename Ntk>
  class Partitioner {
  public:
    struct Parameter {
      int nVerbose = 0;
      int nPartitionSize = 1000;
      int nPartitionSizeMin = 10;
      int nPartitionInputMax = 0;
    };

    // lifecycle
    Partitioner(const Parameter &par);
    void AssignNetwork(Ntk *pNtk);
    void SetPrintLine(std::function<void(const std::string &)> fnPrintLine);

    // partition
    Ntk *Extract(int nSeed);
    void Insert(Ntk *pSubNtk);

    // queries
    std::vector<int> GetInputs(Ntk *pSubNtk) const;
    bool IsTooSmall(Ntk *pNtk) const;

  private:
    using PartitionIo = std::tuple<std::set<int>, std::vector<int>, std::vector<bool>, std::vector<int>>;

    Ntk *pNtk_;
    const Parameter par_;
    std::function<void(const std::string &)> fnPrintLine_;
    std::map<Ntk *, PartitionIo> mSubNtk2Io_;
    std::set<int> sBlocked_;
    std::vector<bool> vFailed_;

    // print
    template <typename... Args>
    void Print(int nVerboseLevel, Args &&...args);

    // partition
    void GetIo(const std::set<int> &sNodes, std::set<int> &sInputs, std::set<int> &sOutputs);
    std::set<int> GetFanouts(const std::set<int> &sNodes, const std::set<int> &sOutputs);
    std::set<int> GetUnblockedNeighborsAndInners(int nId, int nRadius);
    Ntk *ExtractDisjoint(int nId);
  };

  // lifecycle

  template <typename Ntk>
  Partitioner<Ntk>::Partitioner(const Parameter &par)
    : pNtk_(nullptr),
      par_(par) {
  }
  
  template <typename Ntk>
  void Partitioner<Ntk>::AssignNetwork(Ntk *pNtk) {
    pNtk_ = pNtk;
    assert(mSubNtk2Io_.empty());
    assert(sBlocked_.empty());
    vFailed_.clear();
  }

  template <typename Ntk>
  void Partitioner<Ntk>::SetPrintLine(std::function<void(const std::string &)> fnPrintLine) {
    fnPrintLine_ = std::move(fnPrintLine);
  }

  // partition
  
  template <typename Ntk>
  Ntk *Partitioner<Ntk>::Extract(int nSeed) {
    vFailed_.resize(pNtk_->GetNumNodes());
    std::mt19937 rng(nSeed);
    std::vector<int> vInts = pNtk_->GetInts();
    std::shuffle(vInts.begin(), vInts.end(), rng);
    for(int i = 0; i < int_size(vInts); i++) {
      int nId = vInts[i];
      if(vFailed_[nId]) {
        continue;
      }
      if(!sBlocked_.count(nId)) {
        Print(0, "try partitioning with node", nId, "(", i, "/", int_size(vInts), ")");
        Ntk *pSubNtk = ExtractDisjoint(nId);
        if(pSubNtk) {
          Print(0, "got partitioning with node", nId, "(", i, "/", int_size(vInts), ")", "with size", pSubNtk->GetNumInts());
          return pSubNtk;
        }
      }
      vFailed_[nId] = true;
    }
    return nullptr;
  }
  
  template <typename Ntk>
  void Partitioner<Ntk>::Insert(Ntk *pSubNtk) {
    PartitionIo &io = mSubNtk2Io_.at(pSubNtk);
    for(int nId : std::get<0>(io)) {
      sBlocked_.erase(nId);
    }
    std::pair<std::vector<int>, std::vector<bool>> vNewSignals = pNtk_->Insert(pSubNtk, std::get<1>(io), std::get<2>(io), std::get<3>(io));
    std::vector<int> &vOldOutputs = std::get<3>(io);
    std::vector<int> &vNewOutputs = vNewSignals.first;
    std::vector<bool> &vNewCompls = vNewSignals.second;
    // need to remap updated outputs that are used as inputs in other partitions
    std::map<int, int> mOutput2Idx;
    for(int nIdx = 0; nIdx < int_size(vOldOutputs); ++nIdx) {
      mOutput2Idx[vOldOutputs[nIdx]] = nIdx;
    }
    for(auto &entry : mSubNtk2Io_) {
      if(entry.first == pSubNtk) {
        continue;
      }
      std::vector<int> &vInputs = std::get<1>(entry.second);
      std::vector<bool> &vCompls = std::get<2>(entry.second);
      for(int i = 0; i < int_size(vInputs); i++) {
        if(mOutput2Idx.count(vInputs[i])) {
          int nIdx = mOutput2Idx[vInputs[i]];
          vInputs[i] = vNewOutputs[nIdx];
          vCompls[i] = vCompls[i] ^ vNewCompls[nIdx];
        }
      }
    }
    delete pSubNtk;
    mSubNtk2Io_.erase(pSubNtk);
    vFailed_.clear(); // clear, there isn't really a way to track
  }
  
  // queries

  template <typename Ntk>
  std::vector<int> Partitioner<Ntk>::GetInputs(Ntk *pSubNtk) const {
    return std::get<1>(mSubNtk2Io_.at(pSubNtk));
  }

  template <typename Ntk>
  bool Partitioner<Ntk>::IsTooSmall(Ntk *pNtk) const {
    return pNtk->GetNumInts() <= par_.nPartitionSizeMin;
  }

  // print
  
  template <typename Ntk>
  template <typename... Args>
  void Partitioner<Ntk>::Print(int nVerboseLevel, Args &&...args) {
    if(fnPrintLine_ && par_.nVerbose > nVerboseLevel) {
      std::stringstream ss;
      for(int i = 0; i < nVerboseLevel; i++) {
        ss << "\t";
      }
      PrintNext(ss, std::forward<Args>(args)...);
      fnPrintLine_(ss.str());
    }
  }

  // partition

  template <typename Ntk>
  void Partitioner<Ntk>::GetIo(const std::set<int> &sNodes, std::set<int> &sInputs, std::set<int> &sOutputs) {
    // TODO: can be done using marks rather than count
    sInputs.clear();
    sOutputs.clear();
    for(int nId : sNodes) {
      pNtk_->ForEachFanin(nId, [&](int nFi) {
        if(!sNodes.count(nFi)) {
          sInputs.insert(nFi);
        }
      });
      bool fOutput = false;
      pNtk_->ForEachFanout(nId, [&](int nFo) {
        if(!sNodes.count(nFo)) {
          fOutput = true;
        }
      });
      if(fOutput) {
        sOutputs.insert(nId);
      }
    }
  }
  
  template <typename Ntk>
  std::set<int> Partitioner<Ntk>::GetFanouts(const std::set<int> &sNodes, const std::set<int> &sOutputs) {
    std::set<int> sFanouts;
    for(int nId : sOutputs) {
      pNtk_->template ForEachFanout<false, false>(nId, [&](int nFo) {
        if(!sNodes.count(nFo)) {
          sFanouts.insert(nFo);
        }
      });
    }
    return sFanouts;
  }
  
  template <typename Ntk>
  std::set<int> Partitioner<Ntk>::GetUnblockedNeighborsAndInners(int nId, int nRadius) {
    // return empty set on failure
    // TODO: not sure why we are not using PIs here
    std::vector<int> vNodes = pNtk_->GetNeighbors(nId, false, nRadius);
    vNodes.push_back(nId);
    std::set<int> sNodes(vNodes.begin(), vNodes.end());
    Print(2, "radius", NS(), nRadius, ":", "size =", int_size(sNodes));
    for(auto it = sNodes.begin(); it != sNodes.end();) {
      if(sBlocked_.count(*it)) {
        it = sNodes.erase(it);
      } else {
        ++it;
      }
    }
    Print(2, "unblocked:", "size =", int_size(sNodes));
    if(int_size(sNodes) > par_.nPartitionSize) {
      return std::set<int>();
    }
    std::set<int> sInputs, sOutputs;
    GetIo(sNodes, sInputs, sOutputs);
    Print(3, "nodes:", sNodes);
    Print(3, "inputs:", sInputs);
    Print(3, "outputs:", sOutputs);
    if(par_.nPartitionInputMax && int_size(sInputs) > par_.nPartitionInputMax) {
      return std::set<int>();
    }
    std::set<int> sFanouts = GetFanouts(sNodes, sOutputs);
    std::vector<int> vInners = pNtk_->GetInners(sFanouts, sInputs);
    while(!vInners.empty()) {
      Print(2, "inner size =", int_size(vInners));
      for(int nInner : vInners) {
        if(sBlocked_.count(nInner)) {
          return std::set<int>();
        }
      }
      sNodes.insert(vInners.begin(), vInners.end());
      Print(2, "expanded:", "size =", int_size(sNodes));
      if(int_size(sNodes) > par_.nPartitionSize) {
        return std::set<int>();
      }
      GetIo(sNodes, sInputs, sOutputs);
      Print(3, "nodes:", sNodes);
      Print(3, "inputs:", sInputs);
      Print(3, "outputs:", sOutputs);
      if(par_.nPartitionInputMax && int_size(sInputs) > par_.nPartitionInputMax) {
        return std::set<int>();
      }
      sFanouts = GetFanouts(sNodes, sOutputs);
      vInners = pNtk_->GetInners(sFanouts, sInputs);
    }
    return sNodes;
  }
  
  template <typename Ntk>
  Ntk *Partitioner<Ntk>::ExtractDisjoint(int nId) {
    assert(!sBlocked_.count(nId));
    int nRadius = 1;
    std::set<int> sNodes = GetUnblockedNeighborsAndInners(nId, nRadius);
    Print(1, "radius", NS(), nRadius, ":", "size =", int_size(sNodes));
    if(sNodes.empty()) {
      return nullptr;
    }
    std::set<int> sNodesNew = GetUnblockedNeighborsAndInners(nId, nRadius + 1);
    Print(1, "radius", NS(), nRadius + 1, ":", "size =", int_size(sNodesNew));
    while(!sNodesNew.empty()) {
      if(int_size(sNodes) == int_size(sNodesNew)) {
        break;
      }
      sNodes = sNodesNew;
      ++nRadius;
      sNodesNew = GetUnblockedNeighborsAndInners(nId, nRadius + 1);
      Print(1, "radius", NS(), nRadius + 1, ":", "size =", int_size(sNodesNew));
    }
    if(int_size(sNodes) < par_.nPartitionSizeMin) {
      return nullptr;
    }
    std::set<int> sInputs, sOutputs;
    GetIo(sNodes, sInputs, sOutputs);
    for(const auto &entry : mSubNtk2Io_) {
      if(!pNtk_->IsReachable(sOutputs, std::get<1>(entry.second))) {
        continue;
      }
      if(!pNtk_->IsReachable(std::get<3>(entry.second), sInputs)) {
        continue;
      }
      return nullptr;
    }
    assert(int_size(sNodes) <= par_.nPartitionSize);
    assert(int_size(sNodes) >= par_.nPartitionSizeMin);
    std::vector<int> vInputs(sInputs.begin(), sInputs.end());
    std::vector<int> vOutputs(sOutputs.begin(), sOutputs.end());
    std::unique_ptr<Ntk> pSubNtk = pNtk_->Extract(sNodes, vInputs, vOutputs);
    assert(pSubNtk);
    // TODO: use unique_ptr everywhere
    Ntk *pSubNtkRaw = pSubNtk.release();
    for(int nNode : sNodes) {
      sBlocked_.insert(nNode);
    }
    mSubNtk2Io_.emplace(pSubNtkRaw, std::make_tuple(std::move(sNodes), std::move(vInputs), std::vector<bool>(vInputs.size()), std::move(vOutputs)));
    return pSubNtkRaw;
  }
  
} // namespace boop

BOOP_HEADER_END
