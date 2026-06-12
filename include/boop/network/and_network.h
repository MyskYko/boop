#pragma once

#include <algorithm>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "boop/config.h"
#include "boop/util/util.h"
#include "boop/network/types.h"

BOOP_HEADER_START

namespace boop {

  class AndNetwork {
  public:
    using Callback = std::function<void(const Action &)>;

    // lifecycle
    AndNetwork();
    AndNetwork(const AndNetwork &other);
    
    // initialization (should not be called after optimization has started)
    void Clear(bool fClearNetwork = true, bool fClearCallbacks = true, bool fClearBackups = true);
    void Reserve(int nReserve);
    int AddPi();
    int AddAnd(int nId0, int nId1, bool fCompl0, bool fCompl1);
    int AddAnd(const std::vector<int> &vFanins, const std::vector<bool> &vCompls);
    int AddPo(int nId, bool fCompl);
    void ChangePiOrder(const std::vector<int> &vOrder);

    // network properties
    bool UseComplementedEdges() const;
    bool HasMultipleNodeTypes() const;
    int GetNumNodes() const; // number of allocated nodes (max id + 1)
    int GetNumPis() const;
    int GetNumInts() const;
    int GetNumPos() const;
    int GetNumFanins() const;
    int GetNumLevels() const;
    int GetConst0() const;
    int GetPi(int nIdx) const;
    int GetPo(int nIdx) const;
    std::vector<int> GetPis() const;
    std::vector<int> GetInts() const;
    std::vector<int> GetPisInts() const;
    std::vector<int> GetPos() const;

    // node properties
    bool IsPi(int nId) const;
    bool IsInt(int nId) const;
    bool IsPo(int nId) const;
    NodeType GetNodeType(int nId) const; // TODO: rethink NodeType type
    bool IsPoDriver(int nId) const;
    int GetPiIndex(int nId) const;
    int GetIntIndex(int nId) const;
    int GetPoIndex(int nId) const;
    int GetNumFanins(int nId) const;
    int GetNumFanouts(int nId) const;
    int GetFanin(int nId, int nIdx) const;
    bool GetCompl(int nId, int nIdx) const;
    int FindFanin(int nId, int nFi) const;

    // graph
    std::set<int> GetExtendedFanins(int nId);
    bool IsReconvergent(int nId);
    std::vector<int> GetNeighbors(int nId, bool fPis, int nHops);
    template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
    bool IsReachable(const Container<Ts...> &srcs, const Container2<Ts2...> &dsts);
    template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
    std::vector<int> GetInners(const Container<Ts...> &srcs, const Container2<Ts2...> &dsts);

    // network traversal
    // TODO: add fOutIdx when fanout index is relevant (ForEachPoDriver, ForEachFanin, ForEachFanout)
    template <bool fReverse = false, typename Func>
    void ForEachPi(const Func &func) const;
    template <bool fReverse = false, typename Func>
    void ForEachPo(const Func &func) const;
    template <bool fPos = false, bool fReverse = false, typename Func>
    void ForEachPoDriver(const Func &func) const;
    template <bool fReverse = false, typename Func>
    void ForEachInt(const Func &func) const;
    template <bool fReverse = false, typename Func>
    void ForEachPiInt(const Func &func) const;
    template <bool fIdx = false, bool fPi = true, bool fReverse = false, typename Func>
    void ForEachFanin(int nId, const Func &func) const;
    template <bool fIdx = false, bool fPo = true, bool fReverse = false, typename Func>
    void ForEachFanout(int nId, const Func &func) const;

    template <bool fPi = true, bool fGlobalStop = true, bool fTopo = false, bool fReverse = false, typename Func>
    void ForEachTfi(int nId, const Func &func);
    template <bool fPi = true, bool fGlobalStop = true, bool fTopo = false, bool fReverse = false, template <typename...> typename Container, typename... Ts, typename Func>
    void ForEachTfiEnd(int nId, const Container<Ts...> &ends, const Func &func);
    template <bool fPi = true, bool fGlobalStop = true, bool fTopo = false, bool fReverse = false, template <typename...> typename Container, typename... Ts, typename Func>
    void ForEachTfis(const Container<Ts...> &ids, const Func &func);

    template <bool fPo = true, bool fGlobalStop = true, bool fTopo = false, bool fReverse = false, typename Func>
    void ForEachTfo(int nId, const Func &func);
    template <bool fPo = true, bool fGlobalStop = true, bool fTopo = false, bool fReverse = false, template <typename...> typename Container, typename... Ts, typename Func>
    void ForEachTfoEnd(int nId, const Container<Ts...> &ends, const Func &func);
    template <bool fPo = true, bool fGlobalStop = true, bool fTopo = false, bool fReverse = false, template <typename...> typename Container, typename... Ts, typename Func>
    void ForEachTfos(const Container<Ts...> &ids, const Func &func);

    // extraction
    template <template <typename...> typename Container, typename... Ts>
    std::unique_ptr<AndNetwork> Extract(const Container<Ts...> &ids, const std::vector<int> &vInputs, const std::vector<int> &vOutputs);

    // actions
    void Read(const AndNetwork &from);
    template <typename Ntk, typename Reader>
    int Read(const Ntk &from, const Reader &reader);
    void RemoveFanin(int nId, int nIdx);
    void RemoveUnused(int nId, bool fRecursive = false, bool fSweeping = false);
    void RemoveBuffer(int nId);
    void RemoveConst(int nId);
    void AddFanin(int nId, int nFi, bool fCompl);
    bool TrivialCollapse(int nId);
    bool TrivialCollapse();
    int TrivialDecompose(int nId, int nFanins);
    void TrivialDecompose(int nId);
    void SortFanins(int nId, const std::vector<int> &vIndices);
    template <typename Func>
    void SortFanins(int nId, const Func &cost);
    std::pair<std::vector<int>, std::vector<bool>> Insert(AndNetwork *pNtk, const std::vector<int> &vInputs, const std::vector<bool> &vCompls, const std::vector<int> &vOutputs);
    
    // cleanup
    void Propagate(int nId = -1); // all nodes unless specified
    void Sweep(bool fPropagate = true);
    
    // save & load
    int Save(int nSlot = -1); // slot is assigned automatically unless specified
    void Load(int nSlot);
    void PopBack(); // deletes the last backup entry
    
    // misc
    int AddCallback(const Callback &callback);
    void DeleteCallback(int nIndex);
    void Print() const;
    
  private:
    // network data
    int nNodes_; // number of allocated nodes
    std::vector<int> vPis_;
    std::vector<int> vPos_;
    std::list<int> lInts_; // internal nodes in topological order
    std::set<int> sInts_; // internal nodes as a set
    std::vector<std::vector<int>> vvFaninEdges_; // complementable edges, no duplicated fanins allowed (including complements), and nodes without fanins are treated as const-1
    std::vector<int> vRefs_; // reference count (number of fanouts)
    
    // traversal state
    bool fLockTrav_;
    unsigned nTrav_;
    std::vector<unsigned> vTrav_;
    
    // constant propagation state
    bool fPropagating_;
    
    // callbacks
    std::vector<Callback> vCallbacks_;
    
    // backups
    std::vector<AndNetwork> vBackups_;

    // conversion between node and edge
    int Node2Edge(int nId, bool fCompl) const { return (nId << 1) + static_cast<int>(fCompl); }
    int Edge2Node(int nEdge) const { return nEdge >> 1; }
    bool EdgeIsCompl(int nEdge) const { return nEdge & 1; }

    // helpers
    int CreateNode();
    void SortInts(std::list<int>::iterator it);
    unsigned StartTraversal(int n = 1);
    void EndTraversal();
    template <bool fPi = true, bool fGlobalStop = true, typename Func>
    bool ForEachTfiRec(int nId, const Func &func);
    template <bool fPi = true, bool fGlobalStop = true, bool fReverse = false, typename It, typename Func>
    void ForEachTfiTopoInt(It it, unsigned nSkip, const Func &func);
    template <bool fPo = true, bool fGlobalStop = true, bool fReverse = false, typename It, typename Func>
    void ForEachTfoInt(It it, unsigned nSkip, const Func &func);
    void Copy(const AndNetwork &from);
    void TakenAction(const Action &action) const;
  };

  // lifecycle
  
  inline AndNetwork::AndNetwork()
    : nNodes_(0),
      fLockTrav_(false),
      nTrav_(0),
      fPropagating_(false) {
    vvFaninEdges_.emplace_back();
    vRefs_.push_back(0);
    nNodes_++;
  }

  inline AndNetwork::AndNetwork(const AndNetwork &other)
    : fLockTrav_(false),
      nTrav_(0),
      fPropagating_(false) {
    Copy(other);
  }

  // initialization

  inline void AndNetwork::Clear(bool fClearNetwork, bool fClearCallbacks, bool fClearBackups) {
    if(fClearNetwork) {
      nNodes_ = 0;
      vPis_.clear();
      vPos_.clear();
      lInts_.clear();
      sInts_.clear();
      vvFaninEdges_.clear();
      vRefs_.clear();
      fLockTrav_ = false;
      nTrav_ = 0;
      vTrav_.clear();
      fPropagating_ = false;
      vvFaninEdges_.emplace_back();
      vRefs_.push_back(0);
      nNodes_++;
    }
    if(fClearCallbacks) {
      vCallbacks_.clear();
    }
    if(fClearBackups) {
      vBackups_.clear();
    }
  }

  inline void AndNetwork::Reserve(int nReserve) {
    vvFaninEdges_.reserve(nReserve);
    vRefs_.reserve(nReserve);
  }

  inline int AndNetwork::AddPi() {
    vPis_.push_back(nNodes_);
    vvFaninEdges_.emplace_back();
    vRefs_.push_back(0);
    assert(!check_int_max(nNodes_));
    return nNodes_++;
  }
  
  inline int AndNetwork::AddAnd(int nId0, int nId1, bool fCompl0, bool fCompl1) {
    assert(nId0 >= 0 && nId0 < nNodes_);
    assert(nId1 >= 0 && nId1 < nNodes_);
    // TODO: it is a philosophical question whether to allow dangling nodes or not
    assert(nId0 != nId1);
    assert(!check_int_max(nNodes_));
    lInts_.push_back(nNodes_);
    sInts_.insert(nNodes_);
    vRefs_[nId0]++;
    vRefs_[nId1]++;
    vvFaninEdges_.emplace_back(std::initializer_list<int>{Node2Edge(nId0, fCompl0), Node2Edge(nId1, fCompl1)});
    vRefs_.push_back(0);
    return nNodes_++;
  }

  inline int AndNetwork::AddAnd(const std::vector<int> &vFanins, const std::vector<bool> &vCompls) {
    assert(vFanins.size() == vCompls.size());
    assert(!check_int_max(nNodes_));
    lInts_.push_back(nNodes_);
    sInts_.insert(nNodes_);
    vvFaninEdges_.emplace_back(vFanins.size());
    for(int nIdx = 0; nIdx < int_size(vFanins); nIdx++) {
      assert(vFanins[nIdx] >= 0 && vFanins[nIdx] < nNodes_);
      vRefs_[vFanins[nIdx]]++;
      vvFaninEdges_[nNodes_][nIdx] = Node2Edge(vFanins[nIdx], vCompls[nIdx]);
    }
    vRefs_.push_back(0);
    return nNodes_++;
  }

  inline int AndNetwork::AddPo(int nId, bool fCompl) {
    assert(nId >= 0 && nId < nNodes_);
    assert(!check_int_max(nNodes_));
    vPos_.push_back(nNodes_);
    vRefs_[nId]++;
    vvFaninEdges_.emplace_back(std::initializer_list<int>{Node2Edge(nId, fCompl)});
    vRefs_.push_back(0);
    return nNodes_++;
  }

  inline void AndNetwork::ChangePiOrder(const std::vector<int> &vOrder) {
    assert(vOrder.size() == vPis_.size());
    std::vector<int> vPisNew(vPis_.size());
    for(int nIdx = 0; nIdx < int_size(vPis_); nIdx++) {
      int nOldIdx = vOrder[nIdx];
      assert(nOldIdx >= 0 && nOldIdx < int_size(vPis_));
      vPisNew[nIdx] = vPis_[nOldIdx];
    }
    vPis_ = std::move(vPisNew);
  }

  
  // network properties
  
  inline bool AndNetwork::UseComplementedEdges() const {
    return true;
  }
  
  inline bool AndNetwork::HasMultipleNodeTypes() const {
    return false;
  }
  
  inline int AndNetwork::GetNumNodes() const {
    return nNodes_;
  }

  inline int AndNetwork::GetNumPis() const {
    return int_size(vPis_);
  }
  
  inline int AndNetwork::GetNumInts() const {
    return int_size(lInts_);
  }
  
  inline int AndNetwork::GetNumPos() const {
    return int_size(vPos_);
  }

  inline int AndNetwork::GetNumLevels() const {
    int nMaxLevel = 0;
    std::vector<int> vLevels(nNodes_);
    for(int nId : lInts_) {
      for(int nFaninEdge : vvFaninEdges_[nId]) {
        int nFi = Edge2Node(nFaninEdge);
        if(vLevels[nId] < vLevels[nFi]) {
          vLevels[nId] = vLevels[nFi];
        }
      }
      vLevels[nId] += 1;
      if(nMaxLevel < vLevels[nId]) {
        nMaxLevel = vLevels[nId];
      }
    }
    return nMaxLevel;
  }

  inline int AndNetwork::GetConst0() const {
    return 0;
  }

  inline int AndNetwork::GetPi(int nIdx) const {
    return vPis_[nIdx];
  }

  inline int AndNetwork::GetPo(int nIdx) const {
    return vPos_[nIdx];
  }

  inline std::vector<int> AndNetwork::GetPis() const {
    return vPis_;
  }

  inline std::vector<int> AndNetwork::GetInts() const {
    return std::vector<int>(lInts_.begin(), lInts_.end());
  }

  inline std::vector<int> AndNetwork::GetPisInts() const {
    std::vector<int> vPisInts = vPis_;
    vPisInts.insert(vPisInts.end(), lInts_.begin(), lInts_.end());
    return vPisInts;
  }
  
  inline std::vector<int> AndNetwork::GetPos() const {
    return vPos_;
  }
  
  // node properties

  inline bool AndNetwork::IsPi(int nId) const {
    return GetNumFanins(nId) == 0 && std::find(vPis_.begin(), vPis_.end(), nId) != vPis_.end();
  }

  inline bool AndNetwork::IsInt(int nId) const {
    return sInts_.count(nId);
  }

  inline bool AndNetwork::IsPo(int nId) const {
    return GetNumFanouts(nId) == 0 && std::find(vPos_.begin(), vPos_.end(), nId) != vPos_.end();
  }

  inline NodeType AndNetwork::GetNodeType(int nId) const {
    if(IsPi(nId)) {
      return PI;
    }
    if(IsPo(nId)) {
      return PO;
    }
    return AND;
  }

  inline bool AndNetwork::IsPoDriver(int nId) const {
    for(int nPo : vPos_) {
      if(GetFanin(nPo, 0) == nId) {
        return true;
      }
    }
    return false;
  }

  inline int AndNetwork::GetPiIndex(int nId) const {
    assert(check_int_size(vPis_));
    std::vector<int>::const_iterator it = std::find(vPis_.begin(), vPis_.end(), nId);
    assert(it != vPis_.end());
    return int_distance(vPis_.begin(), it);
  }

  inline int AndNetwork::GetIntIndex(int nId) const {
    assert(check_int_size(lInts_));
    int nIdx = 0;
    auto it = lInts_.begin();
    for(; it != lInts_.end(); ++it) {
      if(*it == nId) {
        break;
      }
      nIdx++;
    }
    assert(it != lInts_.end());
    return nIdx;
  }
  
  inline int AndNetwork::GetPoIndex(int nId) const {
    assert(check_int_size(vPos_));
    auto it = std::find(vPos_.begin(), vPos_.end(), nId);
    assert(it != vPos_.end());
    return int_distance(vPos_.begin(), it);
  }

  inline int AndNetwork::GetNumFanins(int nId) const {
    return int_size(vvFaninEdges_[nId]);
  }

  inline int AndNetwork::GetNumFanins() const {
    int nFanins = 0;
    ForEachInt([&](int nId) {
      nFanins += GetNumFanins(nId);
    });
    return nFanins;
  }

  inline int AndNetwork::GetNumFanouts(int nId) const {
    return vRefs_[nId];
  }

  inline int AndNetwork::GetFanin(int nId, int nIdx) const {
    return Edge2Node(vvFaninEdges_[nId][nIdx]);
  }

  inline bool AndNetwork::GetCompl(int nId, int nIdx) const {
    return EdgeIsCompl(vvFaninEdges_[nId][nIdx]);
  }

  inline int AndNetwork::FindFanin(int nId, int nFi) const {
    for(int nIdx = 0; nIdx < GetNumFanins(nId); nIdx++) {
      if(GetFanin(nId, nIdx) == nFi) {
        return nIdx;
      }
    }
    return -1;
  }

  // graph

  inline std::set<int> AndNetwork::GetExtendedFanins(int nId) {
    while(GetNumFanouts(nId) == 1) {
      int nIdNew = -1;
      ForEachFanout<false, false, false>(nId, [&](int nFo, bool fCompl) {
        if(!fCompl) {
          nIdNew = nFo;
        }
      });
      if(nIdNew != -1) {
        nId = nIdNew;
      } else {
        break;
      }
    }
    std::vector<int> vFaninEdges = vvFaninEdges_[nId];
    for(int nIdx = 0; nIdx < int_size(vFaninEdges);) {
      int nFaninEdge = vFaninEdges[nIdx];
      int nFi = Edge2Node(nFaninEdge);
      bool fCompl = EdgeIsCompl(nFaninEdge);
      if(!IsPi(nFi) && !fCompl && vRefs_[nFi] == 1) {
        auto it = vFaninEdges.begin() + nIdx;
        it = vFaninEdges.erase(it);
        vFaninEdges.insert(it, vvFaninEdges_[nFi].begin(), vvFaninEdges_[nFi].end());
      } else {
        ++nIdx;
      }
    }
    std::set<int> sFanins;
    for(int nFaninEdge : vFaninEdges) {
      sFanins.insert(Edge2Node(nFaninEdge));
    }
    return sFanins;
  }
  
  inline bool AndNetwork::IsReconvergent(int nId) {
    if(GetNumFanouts(nId) <= 1) {
      return false;
    }
    unsigned nTravStart = StartTraversal(GetNumFanouts(nId));
    int nIdx = 0;
    ForEachFanout<false, false, false>(nId, [&](int nFo) {
      vTrav_[nFo] = nTravStart + nIdx;
      ++nIdx;
    });
    if(nIdx <= 1) {
      EndTraversal();
      return false;
    }
    auto it = lInts_.begin();
    while(it != lInts_.end() && vTrav_[*it] < nTravStart) {
      ++it;
    }
    if(it != lInts_.end()) {
      ++it;
    }
    for(; it != lInts_.end(); ++it) {
      for(int nFaninEdge : vvFaninEdges_[*it]) {
        int nFi = Edge2Node(nFaninEdge);
        if(vTrav_[nFi] >= nTravStart) {
          if(vTrav_[*it] >= nTravStart && vTrav_[*it] != vTrav_[nFi]) {
            EndTraversal();
            return true;
          }
          vTrav_[*it] = vTrav_[nFi];
        }
      }
    }
    EndTraversal();
    return false;
  }

  inline std::vector<int> AndNetwork::GetNeighbors(int nId, bool fPis, int nHops) {
    StartTraversal();
    vTrav_[nId] = nTrav_;
    std::vector<int> vPrevs;
    std::vector<int> vNexts;
    vNexts.push_back(nId);
    for(int i = 0; i < nHops; i++) {
      vPrevs.swap(vNexts);
      for(int nNode : vPrevs) {
        ForEachFanin(nNode, [&](int nFi) {
          if(vTrav_[nFi] != nTrav_) {
            vNexts.push_back(nFi);
            vTrav_[nFi] = nTrav_;
          }
        });
        ForEachFanout<false, false, false>(nNode, [&](int nFo) {
          if(vTrav_[nFo] != nTrav_) {
            vNexts.push_back(nFo);
            vTrav_[nFo] = nTrav_;
          }
        });
      }
      vPrevs.clear();
    }
    vTrav_[nId] = 0;
    std::vector<int> vNeighbors;
    if(fPis) {
      ForEachPiInt([&](int nId) {
        if(vTrav_[nId] == nTrav_) {
          vNeighbors.push_back(nId);
        }
      });
    } else {
      ForEachInt([&](int nId) {
        if(vTrav_[nId] == nTrav_) {
          vNeighbors.push_back(nId);
        }
      });
    }
    EndTraversal();
    return vNeighbors;
  }

  template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
  inline bool AndNetwork::IsReachable(const Container<Ts...> &srcs, const Container2<Ts2...> &dsts) {
    if(srcs.empty() || dsts.empty()) {
      return false;
    }
    unsigned nDst = StartTraversal(2);
    for(int nId : dsts) {
      vTrav_[nId] = nDst;
    }
    for(int nId : srcs) {
      if(vTrav_[nId] == nDst) {
        EndTraversal();
        return true;
      }
      vTrav_[nId] = nTrav_;
    }
    auto it = lInts_.begin();
    while(it != lInts_.end() && vTrav_[*it] != nTrav_) {
      ++it;
    }
    for(; it != lInts_.end(); ++it) {
      if(vTrav_[*it] == nTrav_) {
        continue;
      }
      for(int nFaninEdge : vvFaninEdges_[*it]) {
        if(vTrav_[Edge2Node(nFaninEdge)] == nTrav_) {
          if(vTrav_[*it] == nDst) {
            EndTraversal();
            return true;
          }
          vTrav_[*it] = nTrav_;
          break;
        }
      }
    }
    for(int nPo : vPos_) {
      if(vTrav_[nPo] == nTrav_) {
        continue;
      }
      if(vTrav_[GetFanin(nPo, 0)] == nTrav_) {
        if(vTrav_[nPo] == nDst) {
          EndTraversal();
          return true;
        }
        vTrav_[nPo] = nTrav_;
      }
    }
    EndTraversal();
    return false;
  }

  template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
  inline std::vector<int> AndNetwork::GetInners(const Container<Ts...> &srcs, const Container2<Ts2...> &dsts) {
    if(srcs.empty() || dsts.empty()) {
      return std::vector<int>();
    }
    unsigned nTravStart = StartTraversal(4);
    unsigned nDst = nTravStart;
    unsigned nTfo = nTravStart + 1;
    unsigned nInner = nTravStart + 2;
    for(int nId : dsts) {
      vTrav_[nId] = nDst;
    }
    for(int nId : srcs) {
      if(vTrav_[nId] == nDst) {
        vTrav_[nId] = nInner;
      } else {
        vTrav_[nId] = nTfo;
      }
    }
    auto it = lInts_.begin();
    while(it != lInts_.end() && vTrav_[*it] != nTfo) {
      ++it;
    }
    for(; it != lInts_.end(); ++it) {
      if(vTrav_[*it] >= nTfo) { // TFO or inner
        continue;
      }
      for(int nFaninEdge : vvFaninEdges_[*it]) {
        if(vTrav_[Edge2Node(nFaninEdge)] == nTfo) {
          if(vTrav_[*it] == nDst) {
            vTrav_[*it] = nInner;
          } else {
            vTrav_[*it] = nTfo;
          }
          break;
        }
      }
    }
    std::vector<int> vInners;
    for(int nId : dsts) {
      if(vTrav_[nId] == nInner) {
        vInners.push_back(nId);
        vTrav_[nId] = nTrav_;
        ForEachTfiRec(nId, [&](int nFi) {
          if(vTrav_[nFi] == nTfo || vTrav_[nFi] == nInner) {
            vInners.push_back(nFi);
          }
        });
      }
    }
    EndTraversal();
    return vInners;
  }

  // network traversal

  template <bool fReverse, typename Func>
  inline void AndNetwork::ForEachPi(const Func &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, int>::value, "for each PI function format error");
    static_assert(!(is_invokable<Func, int>::value && is_invokable<Func, int, int>::value), "for each PI function must match exactly one callback format");
    for(int i = 0; i < GetNumPis(); i++) {
      int nIdx;
      if constexpr(fReverse) {
        nIdx = GetNumPis() - 1 - i;
      } else {
        nIdx = i;
      }
      if constexpr(is_invokable<Func, int>::value) {
        if(invoke_and_return_stop(func, GetPi(nIdx))) {
          return;
        }
      } else {
        if(invoke_and_return_stop(func, nIdx, GetPi(nIdx))) {
          return;
        }
      }
    }
  }
  
  template <bool fReverse, typename Func>
  inline void AndNetwork::ForEachPo(const Func &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, int>::value, "for each PO function format error");
    static_assert(!(is_invokable<Func, int>::value && is_invokable<Func, int, int>::value), "for each PO function must match exactly one callback format");
    for(int i = 0; i < GetNumPos(); i++) {
      int nIdx;
      if constexpr(fReverse) {
        nIdx = GetNumPos() - 1 - i;
      } else {
        nIdx = i;
      }
      if constexpr(is_invokable<Func, int>::value) {
        if(invoke_and_return_stop(func, GetPo(nIdx))) {
          return;
        }
      } else {
        if(invoke_and_return_stop(func, nIdx, GetPo(nIdx))) {
          return;
        }
      }
    }
  }

  template <bool fPos, bool fReverse, typename Func>
  inline void AndNetwork::ForEachPoDriver(const Func &func) const {
    if constexpr(fPos) {
      static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, int, bool>::value, "for each PO driver function format error");
    } else {
      static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each PO driver function format error");
    }
    for(int i = 0; i < GetNumPos(); i++) {
      int nIdx;
      if constexpr(fReverse) {
        nIdx = GetNumPos() - 1 - i;
      } else {
        nIdx = i;
      }
      int nPo = GetPo(nIdx);
      if constexpr(fPos) {
        if constexpr(is_invokable<Func, int, int>::value) {
          if(invoke_and_return_stop(func, nIdx, GetFanin(nPo, 0))) {
            return;
          }
        } else {
          if(invoke_and_return_stop(func, nIdx, GetFanin(nPo, 0), GetCompl(nPo, 0))) {
            return;
          }
        }
      } else {
        if constexpr(is_invokable<Func, int>::value) {
          if(invoke_and_return_stop(func, GetFanin(nPo, 0))) {
            return;
          }
        } else {
          if(invoke_and_return_stop(func, GetFanin(nPo, 0), GetCompl(nPo, 0))) {
            return;
          }
        }
      }
    }
  }

  template <bool fReverse, typename Func>
  inline void AndNetwork::ForEachInt(const Func &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, int>::value, "for each internal node function format error");
    static_assert(!(is_invokable<Func, int>::value && is_invokable<Func, int, int>::value), "for each internal node function must match exactly one callback format");
    auto fn = [&](int nId, int nIdx) {
      if constexpr(is_invokable<Func, int>::value) {
        return invoke_and_return_stop(func, nId);
      } else {
        return invoke_and_return_stop(func, nIdx, nId);
      }
    };
    if constexpr(fReverse) {
      int nIdx = GetNumInts() - 1;
      for(auto it = lInts_.rbegin(); it != lInts_.rend(); ++it) {
        if(fn(*it, nIdx)) {
          return;
        }
        nIdx--;
      }
    } else {
      int nIdx = 0;
      for(int nId : lInts_) {
        if(fn(nId, nIdx)) {
          return;
        }
        nIdx++;
      }
    }
  }

  template <bool fReverse, typename Func>
  inline void AndNetwork::ForEachPiInt(const Func &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, int>::value, "for each PI/internal node function format error");
    static_assert(!(is_invokable<Func, int>::value && is_invokable<Func, int, int>::value), "for each PI/internal node function must match exactly one callback format");
    auto fn = [&](int nId, int nIdx) {
      if constexpr(is_invokable<Func, int>::value) {
        return invoke_and_return_stop(func, nId);
      } else {
        return invoke_and_return_stop(func, nIdx, nId);
      }
    };
    if constexpr(fReverse) {
      int nIdx = GetNumPis() + GetNumInts() - 1;
      for(auto it = lInts_.rbegin(); it != lInts_.rend(); ++it) {
        if(fn(*it, nIdx)) {
          return;
        }
        nIdx--;
      }
      for(auto it = vPis_.rbegin(); it != vPis_.rend(); ++it) {
        if(fn(*it, nIdx)) {
          return;
        }
        nIdx--;
      }
    } else {
      int nIdx = 0;
      for(int nPi : vPis_) {
        if(fn(nPi, nIdx)) {
          return;
        }
        nIdx++;
      }
      for(int nId : lInts_) {
        if(fn(nId, nIdx)) {
          return;
        }
        nIdx++;
      }
    }
  }

  template <bool fIdx, bool fPi, bool fReverse, typename Func>
  inline void AndNetwork::ForEachFanin(int nId, const Func &func) const {
    if constexpr(fIdx) {
        static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, int, bool>::value, "for each fanin function format error");
    } else {
      static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each fanin function format error");
    }
    for(int i = 0; i < GetNumFanins(nId); i++) {
      int nIdx;
      if constexpr(fReverse) {
        nIdx = GetNumFanins(nId) - 1 - i;
      } else {
        nIdx = i;
      }
      int nFi = GetFanin(nId, nIdx);
      bool fCompl = GetCompl(nId, nIdx);
      if constexpr(!fPi) {
        if(IsPi(nFi)) {
          continue;
        }
      }
      if constexpr(fIdx) {
        if constexpr(is_invokable<Func, int, int>::value) {
          if(invoke_and_return_stop(func, nIdx, nFi)) {
            return;
          }
        } else {
          if(invoke_and_return_stop(func, nIdx, nFi, fCompl)) {
            return;
          }
        }
      } else {
        if constexpr(is_invokable<Func, int>::value) {
          if(invoke_and_return_stop(func, nFi)) {
            return;
          }
        } else {
          if(invoke_and_return_stop(func, nFi, fCompl)) {
            return;
          }
        }
      }
    }
  }

  template <bool fIdx, bool fPo, bool fReverse, typename Func>
  inline void AndNetwork::ForEachFanout(int nId, const Func &func) const {
    if constexpr(fIdx) {
      static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, int, bool>::value, "for each fanout function format error");
    } else {
      static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each fanout function format error");
    }
    int nRefs = vRefs_[nId];
    if(nRefs == 0) {
      return;
    }
    auto fn = [&](int nFo, int nIdx) {
      if constexpr(fIdx) {
        if constexpr(is_invokable<Func, int, int>::value) {
          return invoke_and_return_stop(func, nFo, nIdx);
        } else {
          return invoke_and_return_stop(func, nFo, nIdx, GetCompl(nFo, nIdx));
        }
      } else {
        if constexpr(is_invokable<Func, int>::value) {
          return invoke_and_return_stop(func, nFo);
        } else {
          return invoke_and_return_stop(func, nFo, GetCompl(nFo, nIdx));
        }
      }
    };
    if constexpr(fReverse) {
      for(auto it = vPos_.rbegin(); nRefs != 0 && it != vPos_.rend(); ++it) {
        if(GetFanin(*it, 0) == nId) {
          if constexpr(fPo) {
            if(fn(*it, 0)) {
              return;
            }
          }
          --nRefs;
        }
      }
      for(auto it = lInts_.rbegin(); nRefs != 0 && it != lInts_.rend(); ++it) {
        assert(*it != nId);
        int nIdx = FindFanin(*it, nId);
        if(nIdx != -1) {
          if(fn(*it, nIdx)) {
            return;
          }
          --nRefs;
        }
      }
    } else {
      if constexpr(!fPo) {
        for(auto it = vPos_.begin(); nRefs != 0 && it != vPos_.end(); ++it) {
          if(GetFanin(*it, 0) == nId) {
            --nRefs;
          }
        }
      }
      auto it = lInts_.begin();
      if(IsInt(nId)) {
        it = std::find(it, lInts_.end(), nId);
        assert(it != lInts_.end());
        ++it;
      }
      for(; nRefs != 0 && it != lInts_.end(); ++it) {
        assert(*it != nId);
        int nIdx = FindFanin(*it, nId);
        if(nIdx != -1) {
          if(fn(*it, nIdx)) {
            return;
          }
          --nRefs;
        }
      }
      if constexpr(fPo) {
        for(auto itPo = vPos_.begin(); nRefs != 0 && itPo != vPos_.end(); ++itPo) {
          if(GetFanin(*itPo, 0) == nId) {
            if(fn(*itPo, 0)) {
              return;
            }
            --nRefs;
          }
        }
      }
    }
    assert(nRefs == 0);
  }

  template <bool fPi, bool fGlobalStop, bool fTopo, bool fReverse, typename Func>
  inline void AndNetwork::ForEachTfi(int nId, const Func &func) {
    static_assert(is_invokable<Func, int>::value, "for each TFI function format error");
    if constexpr(fReverse) {
      static_assert(!returns_bool_v<Func, int>, "reverse TFI traversal does not support stop callbacks");
    }
    if(GetNumFanins(nId) == 0) {
      return;
    }
    StartTraversal();
    if constexpr(fTopo) {
      ForEachFanin<false, fPi, false>(nId, [&](int nFi) {
        vTrav_[nFi] = nTrav_;
      });
      auto it = lInts_.rbegin();
      if(IsInt(nId)) {
        it = std::find(it, lInts_.rend(), nId);
        assert(it != lInts_.rend());
        ++it;
      }
      ForEachTfiTopoInt<fPi, fGlobalStop, fReverse>(it, nTrav_, func);
    } else {
      if constexpr(fReverse) {
        std::vector<int> vTfi;
        ForEachTfiRec<fPi, fGlobalStop>(nId, [&](int nFi) {
          vTfi.push_back(nFi);
        });
        for(auto it = vTfi.rbegin(); it != vTfi.rend(); ++it) {
          func(*it);
        }
      } else {
        ForEachTfiRec<fPi, fGlobalStop>(nId, func);
      }
    }
    EndTraversal();
  }

  template <bool fPi, bool fGlobalStop, bool fTopo, bool fReverse, template <typename...> typename Container, typename... Ts, typename Func>
  inline void AndNetwork::ForEachTfiEnd(int nId, const Container<Ts...> &ends, const Func &func) {
    static_assert(is_invokable<Func, int>::value, "for each TFI-end function format error");
    if constexpr(fReverse) {
      static_assert(!returns_bool_v<Func, int>, "reverse TFI-end traversal does not support stop callbacks");
    }
    if(GetNumFanins(nId) == 0) {
      return;
    }
    if constexpr(fTopo) {
      unsigned nSkip = StartTraversal(2);
      for(int nEnd : ends) {
        vTrav_[nEnd] = nSkip;
      }
      ForEachFanin<false, fPi, false>(nId, [&](int nFi) {
        if(vTrav_[nFi] != nSkip) {
          vTrav_[nFi] = nTrav_;
        }
      });
      auto it = lInts_.rbegin();
      if(IsInt(nId)) {
        it = std::find(it, lInts_.rend(), nId);
        assert(it != lInts_.rend());
        ++it;
      }
      ForEachTfiTopoInt<fPi, fGlobalStop, fReverse>(it, nSkip, func);
    } else {
      StartTraversal();
      for(int nEnd : ends) {
        vTrav_[nEnd] = nTrav_;
      }
      if constexpr(fReverse) {
        std::vector<int> vTfi;
        ForEachTfiRec<fPi, fGlobalStop>(nId, [&](int nFi) {
          vTfi.push_back(nFi);
        });
        for(auto it = vTfi.rbegin(); it != vTfi.rend(); ++it) {
          func(*it);
        }
      } else {
        ForEachTfiRec<fPi, fGlobalStop>(nId, func);
      }
    }
    EndTraversal();
  }

  template <bool fPi, bool fGlobalStop, bool fTopo, bool fReverse, template <typename...> typename Container, typename... Ts, typename Func>
  inline void AndNetwork::ForEachTfis(const Container<Ts...> &ids, const Func &func) {
    static_assert(is_invokable<Func, int>::value, "for each TFIs topo function format error");
    if constexpr(fReverse) {
      static_assert(!returns_bool_v<Func, int>, "reverse TFIs topo traversal does not support stop callbacks");
    }
    StartTraversal();
    for(int nId : ids) {
      vTrav_[nId] = nTrav_;
    }
    auto it = lInts_.rbegin();
    while(it != lInts_.rend() && vTrav_[*it] != nTrav_) {
      ++it;
    }
    ForEachTfiTopoInt<fPi, fGlobalStop, fReverse>(it, nTrav_, func);
    EndTraversal();
  }

  template <bool fPo, bool fGlobalStop, bool fTopo, bool fReverse, typename Func>
  inline void AndNetwork::ForEachTfo(int nId, const Func &func) {
    static_assert(is_invokable<Func, int>::value, "for each TFO function format error");
    if constexpr(fReverse) {
      static_assert(!returns_bool_v<Func, int>, "reverse TFO traversal does not support stop callbacks");
    }
    if(GetNumFanouts(nId) == 0) {
      return;
    }
    StartTraversal();
    vTrav_[nId] = nTrav_;
    auto it = lInts_.begin();
    if(IsInt(nId)) {
      it = std::find(it, lInts_.end(), nId);
      assert(it != lInts_.end());
      ++it;
    }
    ForEachTfoInt<fPo, fGlobalStop, fReverse>(it, nTrav_, func);
    EndTraversal();
  }

  template <bool fPo, bool fGlobalStop, bool fTopo, bool fReverse, template <typename...> typename Container, typename... Ts, typename Func>
  inline void AndNetwork::ForEachTfoEnd(int nId, const Container<Ts...> &ends, const Func &func) {
    static_assert(is_invokable<Func, int>::value, "for each TFO-end function format error");
    if constexpr(fReverse) {
      static_assert(!returns_bool_v<Func, int>, "reverse TFO-end traversal does not support stop callbacks");
    }
    if(GetNumFanouts(nId) == 0) {
      return;
    }
    unsigned nSkip = StartTraversal(2);
    for(int nEnd : ends) {
      vTrav_[nEnd] = nSkip;
    }
    vTrav_[nId] = nTrav_;
    auto it = lInts_.begin();
    if(IsInt(nId)) {
      it = std::find(it, lInts_.end(), nId);
      assert(it != lInts_.end());
      ++it;
    }
    ForEachTfoInt<fPo, fGlobalStop, fReverse>(it, nSkip, func);
    EndTraversal();
  }

  template <bool fPo, bool fGlobalStop, bool fTopo, bool fReverse, template <typename...> typename Container, typename... Ts, typename Func>
  inline void AndNetwork::ForEachTfos(const Container<Ts...> &ids, const Func &func) {
    static_assert(is_invokable<Func, int>::value, "for each TFOs function format error");
    if constexpr(fReverse) {
      static_assert(!returns_bool_v<Func, int>, "reverse TFOs traversal does not support stop callbacks");
    }
    unsigned nSkip = StartTraversal(2);
    bool fHasNonInt = false;
    for(int nId : ids) {
      vTrav_[nId] = nTrav_;
      fHasNonInt |= !IsInt(nId);
    }
    auto it = lInts_.begin();
    if(!fHasNonInt) {
      while(it != lInts_.end() && vTrav_[*it] != nTrav_) {
        ++it;
      }
    }
    ForEachTfoInt<fPo, fGlobalStop, fReverse>(it, nSkip, func);
    EndTraversal();
  }

  // extraction

  template <template <typename...> typename Container, typename... Ts>
  inline std::unique_ptr<AndNetwork> AndNetwork::Extract(const Container<Ts...> &ids, const std::vector<int> &vInputs, const std::vector<int> &vOutputs) {
    auto pNtk = std::make_unique<AndNetwork>();
    pNtk->Reserve(int_size(vInputs) + int_size(ids) + int_size(vOutputs));
    std::map<int, int> m;
    m[GetConst0()] = pNtk->GetConst0();
    for(int nId : vInputs) {
      m[nId] = pNtk->AddPi();
    }
    StartTraversal();
    for(int nId : ids) {
      vTrav_[nId] = nTrav_;
    }
    ForEachInt([&](int nId) {
      if(vTrav_[nId] == nTrav_) {
        m[nId] = pNtk->CreateNode();
        pNtk->lInts_.push_back(m[nId]);
        pNtk->sInts_.insert(m[nId]);
        pNtk->vvFaninEdges_[m[nId]].resize(GetNumFanins(nId));
        ForEachFanin<true, true, false>(nId, [&](int nIdx, int nFi, bool fCompl) {
          assert(m.count(nFi));
          pNtk->vvFaninEdges_[m[nId]][nIdx] = pNtk->Node2Edge(m[nFi], fCompl);
          pNtk->vRefs_[m[nFi]]++;
        });
      }
    });
    EndTraversal();
    for(int nId : vOutputs) {
      assert(m.count(nId));
      pNtk->AddPo(m[nId], false);
    }
    return pNtk;
  }

  // actions

  inline void AndNetwork::Read(const AndNetwork &from) {
    Clear(true, false, false);
    Copy(from);
    Action action;
    action.type = READ;
    TakenAction(action);
  }

  template <typename Ntk, typename Reader>
  inline int AndNetwork::Read(const Ntk &from, const Reader &reader) {
    int r = 0;
    Clear(true, false, false);
    if constexpr(returns_int_v<Reader, const Ntk &, AndNetwork *>) {
      r = reader(from, this);
    } else {
      reader(from, this);
    }
    Action action;
    action.type = READ;
    TakenAction(action);
    return r;
  }
  
  inline void AndNetwork::RemoveFanin(int nId, int nIdx) {
    Action action;
    action.type = REMOVE_FANIN;
    action.id = nId;
    action.idx = nIdx;
    int nFi = GetFanin(nId, nIdx);
    bool fCompl = GetCompl(nId, nIdx);
    action.fi = nFi;
    action.c = fCompl;
    vRefs_[nFi]--;
    vvFaninEdges_[nId].erase(vvFaninEdges_[nId].begin() + nIdx);
    TakenAction(action);
  }

  inline void AndNetwork::RemoveUnused(int nId, bool fRecursive, bool fSweeping) {
    assert(vRefs_[nId] == 0);
    Action action;
    action.type = REMOVE_UNUSED;
    action.id = nId;
    ForEachFanin(nId, [&](int nFi) {
      action.vFanins.push_back(nFi);
      vRefs_[nFi]--;
    });
    vvFaninEdges_[nId].clear();
    if(!fSweeping) {
      auto it = std::find(lInts_.begin(), lInts_.end(), nId);
      assert(it != lInts_.end());
      lInts_.erase(it);
    }
    sInts_.erase(nId);
    TakenAction(action);
    if(fRecursive) {
      for(int nFi : action.vFanins) {
        if(vRefs_[nFi] == 0 && IsInt(nFi)) {
          RemoveUnused(nFi, fRecursive, fSweeping);
        }
      }
    }
  }

  inline void AndNetwork::RemoveBuffer(int nId) {
    assert(GetNumFanins(nId) == 1);
    assert(!fPropagating_ || fLockTrav_);
    int nFi = GetFanin(nId, 0);
    bool fCompl = GetCompl(nId, 0);
    if(nFi == GetConst0()) {
      RemoveConst(nId);
      return;
    }
    // remove if substitution would lead to duplication with the same polarity
    ForEachFanout<true, false, false>(nId, [&](int nFo, int nIdx, bool fFoCompl) {
      int nIdx2 = FindFanin(nFo, nFi);
      if(nIdx2 != -1 && GetCompl(nFo, nIdx2) == (fCompl ^ fFoCompl)) {
        RemoveFanin(nFo, nIdx);
        if(fPropagating_ && GetNumFanins(nFo) == 1) {
          vTrav_[nFo] = nTrav_;
        }
      }
    });
    Action action;
    action.type = REMOVE_BUFFER;
    action.id = nId;
    action.fi = nFi;
    action.c = fCompl;
    ForEachFanout<true, true, false>(nId, [&](int nFo, int nIdx, bool fFoCompl) {
      action.vFanouts.push_back(nFo);
      int nIdx2 = FindFanin(nFo, nFi);
      if(nIdx2 != -1) { // substitute with const-0 in case of duplication
        assert(GetCompl(nFo, nIdx2) != (fCompl ^ fFoCompl)); // of a different polarity
        vRefs_[GetConst0()]++;
        vvFaninEdges_[nFo][nIdx] = Node2Edge(GetConst0(), false);
        if(fPropagating_) {
          vTrav_[nFo] = nTrav_;
        }
      } else { // otherwise, substitute with fanin
        vvFaninEdges_[nFo][nIdx] = Node2Edge(nFi, fCompl ^ fFoCompl);
        vRefs_[nFi]++;
      }
    });
    vRefs_[nId] = 0;
    vRefs_[nFi]--;
    vvFaninEdges_[nId].clear();
    if(!fPropagating_) {
      auto it = std::find(lInts_.begin(), lInts_.end(), nId);
      assert(it != lInts_.end());
      lInts_.erase(it);
    }
    sInts_.erase(nId);
    TakenAction(action);
  }

  inline void AndNetwork::RemoveConst(int nId) {
    assert(GetNumFanins(nId) == 0 || FindFanin(nId, GetConst0()) != -1);
    assert(!fPropagating_ || fLockTrav_);
    bool fCompl = (GetNumFanins(nId) == 0);
    // just remove immediately if polarity is true but not PO
    ForEachFanout<true, false, false>(nId, [&](int nFo, int nIdx, bool fFoCompl) {
      if(fCompl ^ fFoCompl) {
        assert(!IsPo(nFo));
        RemoveFanin(nFo, nIdx);
        if(fPropagating_ && GetNumFanins(nFo) <= 1) {
          vTrav_[nFo] = nTrav_;
        }
      }
    });
    Action action;
    action.type = REMOVE_CONST;
    action.id = nId;
    // substitute with constant
    ForEachFanout<true, true, false>(nId, [&](int nFo, int nIdx, bool fFoCompl) {
      action.vFanouts.push_back(nFo);
      vRefs_[GetConst0()]++;
      vvFaninEdges_[nFo][nIdx] = Node2Edge(GetConst0(), fCompl ^ fFoCompl);
      if(fPropagating_) {
        vTrav_[nFo] = nTrav_;
      }
    });
    vRefs_[nId] = 0;
    ForEachFanin(nId, [&](int nFi) {
      vRefs_[nFi]--;
      action.vFanins.push_back(nFi);
    });
    vvFaninEdges_[nId].clear();
    if(!fPropagating_) {
      auto it = std::find(lInts_.begin(), lInts_.end(), nId);
      assert(it != lInts_.end());
      lInts_.erase(it);
    }
    sInts_.erase(nId);
    TakenAction(action);
  }

  inline void AndNetwork::AddFanin(int nId, int nFi, bool fCompl) {
    assert(FindFanin(nId, nFi) == -1); // no duplication
    assert(nFi != GetConst0() || !fCompl); // no const-1
    Action action;
    action.type = ADD_FANIN;
    action.id = nId;
    action.idx = GetNumFanins(nId);
    action.fi = nFi;
    action.c = fCompl;
    auto it = std::find(lInts_.begin(), lInts_.end(), nId);
    assert(it != lInts_.end());
    auto it2 = std::find(it, lInts_.end(), nFi);
    if(it2 != lInts_.end()) {
      lInts_.erase(it2);
      it2 = lInts_.insert(it, nFi);
      SortInts(it2);
    }
    vRefs_[nFi]++;
    vvFaninEdges_[nId].push_back(Node2Edge(nFi, fCompl));
    TakenAction(action);
  }

  inline bool AndNetwork::TrivialCollapse(int nId) {
    for(int nIdx = 0; nIdx < GetNumFanins(nId);) {
      int nFaninEdge = vvFaninEdges_[nId][nIdx];
      int nFi = Edge2Node(nFaninEdge);
      bool fCompl = EdgeIsCompl(nFaninEdge);
      if(!IsPi(nFi) && !fCompl && vRefs_[nFi] == 1) {
        Action action;
        action.type = TRIVIAL_COLLAPSE;
        action.id = nId;
        action.idx = nIdx;
        action.fi = nFi;
        action.c = fCompl;
        bool fConst0 = false;
        auto it = vvFaninEdges_[nId].begin() + nIdx;
        it = vvFaninEdges_[nId].erase(it);
        ForEachFanin<true, true, false>(nFi, [&](int nIdx2, int nFi2, bool fCompl2) {
          int nIdx3 = FindFanin(nId, nFi2);
          if(nIdx3 == -1) {
            // no duplication
            it = vvFaninEdges_[nId].insert(it, Node2Edge(nFi2, fCompl2));
            ++it;
            action.vFanins.push_back(nFi2);
            action.vIndices.push_back(nIdx2);
          } else if(fCompl2 != GetCompl(nId, nIdx3)) {
            // duplication with different polarity, add const-0
            vRefs_[nFi2]--;
            vRefs_[GetConst0()]++;
            it = vvFaninEdges_[nId].insert(it, Node2Edge(GetConst0(), false));
            ++it;
            action.vFanins.push_back(GetConst0());
            action.vIndices.push_back(nIdx2);
            fConst0 = true;
          } else {
            // duplication with the same polarity
            vRefs_[nFi2]--;
            nIdx = 0; // need to start over
          }
        });
        vRefs_[nFi] = 0;
        vvFaninEdges_[nFi].clear();
        auto itFi = std::find(lInts_.begin(), lInts_.end(), nFi);
        assert(itFi != lInts_.end());
        lInts_.erase(itFi);
        sInts_.erase(nFi);
        TakenAction(action);
        if(fConst0) {
          return true;
        }
      } else {
        nIdx++;
      }
    }
    return false;
  }
  
  inline bool AndNetwork::TrivialCollapse() {
    bool fConst0 = false;
    std::list<int> lInts = lInts_;
    for(auto it = lInts.rbegin(); it != lInts.rend(); ++it) {
      if(IsInt(*it)) {
        fConst0 |= TrivialCollapse(*it);
      }
    }
    return fConst0;
  }

  inline int AndNetwork::TrivialDecompose(int nId, int nFanins) {
    assert(GetNumFanins(nId) > 2);
    assert(nFanins > 1);
    assert(GetNumFanins(nId) > nFanins);
    Action action;
    action.type = TRIVIAL_DECOMPOSE;
    action.id = nId;
    action.idx = GetNumFanins(nId) - nFanins;
    int nNewFi = CreateNode();
    action.fi = nNewFi;
    for(int i = 0; i < nFanins; i++) {
      int nFaninEdge = vvFaninEdges_[nId].back();
      vvFaninEdges_[nId].pop_back();
      vvFaninEdges_[nNewFi].push_back(nFaninEdge);
      action.vFanins.push_back(Edge2Node(nFaninEdge));
    }
    vvFaninEdges_[nId].push_back(Node2Edge(nNewFi, false));
    vRefs_[nNewFi]++;
    auto it = std::find(lInts_.begin(), lInts_.end(), nId);
    assert(it != lInts_.end());
    lInts_.insert(it, nNewFi);
    sInts_.insert(nNewFi);
    TakenAction(action);
    return nNewFi;
  }

  inline void AndNetwork::TrivialDecompose(int nId) {
    while(GetNumFanins(nId) > 2) {
      Action action;
      action.type = TRIVIAL_DECOMPOSE;
      action.id = nId;
      action.idx = GetNumFanins(nId) - 2;
      int nNewFi = CreateNode();
      action.fi = nNewFi;
      int nFaninEdge1 = vvFaninEdges_[nId].back();
      vvFaninEdges_[nId].pop_back();
      int nFaninEdge0 = vvFaninEdges_[nId].back();
      vvFaninEdges_[nId].pop_back();
      vvFaninEdges_[nNewFi].push_back(nFaninEdge0);
      action.vFanins.push_back(Edge2Node(nFaninEdge0));
      vvFaninEdges_[nNewFi].push_back(nFaninEdge1);
      action.vFanins.push_back(Edge2Node(nFaninEdge1));
      vvFaninEdges_[nId].push_back(Node2Edge(nNewFi, false));
      vRefs_[nNewFi]++;
      auto it = std::find(lInts_.begin(), lInts_.end(), nId);
      assert(it != lInts_.end());
      lInts_.insert(it, nNewFi);
      sInts_.insert(nNewFi);
      TakenAction(action);
    }
  }

  inline void AndNetwork::SortFanins(int nId, const std::vector<int> &vIndices) {
    assert(vIndices.size() == vvFaninEdges_[nId].size());
    std::vector<int> vFaninEdges = vvFaninEdges_[nId];
    vvFaninEdges_[nId].clear();
    for(int nIdx : vIndices) {
      vvFaninEdges_[nId].push_back(vFaninEdges[nIdx]);
    }
    if(vFaninEdges == vvFaninEdges_[nId]) {
      return;
    }
    Action action;
    action.type = SORT_FANINS;
    action.id = nId;
    action.vIndices = vIndices;
    TakenAction(action);
  }

  template <typename Func>
  inline void AndNetwork::SortFanins(int nId, const Func &comp) {
    static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, bool, int, bool>::value, "fanin cost function format error");
    std::vector<int> vFaninEdges = vvFaninEdges_[nId];
    std::sort(vvFaninEdges_[nId].begin(), vvFaninEdges_[nId].end(), [&](int i, int j) {
      if constexpr(is_invokable<Func, int, int>::value) {
        return comp(Edge2Node(i), Edge2Node(j));
      } else {
        return comp(Edge2Node(i), EdgeIsCompl(i), Edge2Node(j), EdgeIsCompl(j));
      }
    });
    if(vFaninEdges == vvFaninEdges_[nId]) {
      return;
    }
    Action action;
    action.type = SORT_FANINS;
    action.id = nId;
    assert(check_int_size(vFaninEdges));
    for(int nFaninEdge : vvFaninEdges_[nId]) {
      auto it = std::find(vFaninEdges.begin(), vFaninEdges.end(), nFaninEdge);
      assert(it != vFaninEdges.end());
      action.vIndices.push_back(int_distance(vFaninEdges.begin(), it));
    }
    TakenAction(action);
  }

  inline std::pair<std::vector<int>, std::vector<bool>> AndNetwork::Insert(AndNetwork *pNtk, const std::vector<int> &vInputs, const std::vector<bool> &vCompls, const std::vector<int> &vOutputs) {
    Reserve(nNodes_ + pNtk->GetNumInts());
    std::map<int, std::pair<int, bool>> m;
    m[pNtk->GetConst0()] = std::make_pair(GetConst0(), false);
    assert(pNtk->GetNumPis() == int_size(vInputs));
    assert(vInputs.size() == vCompls.size());
    for(int i = 0; i < pNtk->GetNumPis(); i++) {
      assert(IsInt(vInputs[i]) || IsPi(vInputs[i]));
      m[pNtk->GetPi(i)] = std::make_pair(vInputs[i], vCompls[i]);
    }
    pNtk->ForEachInt([&](int nId) {
      int nId2 = CreateNode();
      lInts_.push_back(nId2);
      sInts_.insert(nId2);
      vvFaninEdges_[nId2].resize(pNtk->GetNumFanins(nId));
      pNtk->ForEachFanin<true, true, false>(nId, [&](int nIdx, int nFi, bool fCompl) {
        assert(m.count(nFi));
        vvFaninEdges_[nId2][nIdx] = Node2Edge(m[nFi].first, fCompl ^ m[nFi].second);
        vRefs_[m[nFi].first]++;
      });
      m[nId] = std::make_pair(nId2, false);
    });
    assert(pNtk->GetNumPos() == int_size(vOutputs));
    std::vector<int> vNewOutputs(pNtk->GetNumPos());
    std::vector<bool> vNewCompls(pNtk->GetNumPos());
    for(int i = 0; i < pNtk->GetNumPos(); i++) {
      int nId = vOutputs[i];
      int nPo = pNtk->GetPo(i);
      assert(m.count(pNtk->GetFanin(nPo, 0)));
      int nFi = m[pNtk->GetFanin(nPo, 0)].first;
      bool fCompl = pNtk->GetCompl(nPo, 0) ^ m[pNtk->GetFanin(nPo, 0)].second;
      assert(nId != nFi);
      vNewOutputs[i] = nFi;
      vNewCompls[i] = fCompl;
      // remove if substitution would lead to duplication with the same polarity
      ForEachFanout<true, false, false>(nId, [&](int nFo, int nIdx, bool fFoCompl) {
        int nIdx2 = FindFanin(nFo, nFi);
        if(nIdx2 != -1 && GetCompl(nFo, nIdx2) == (fCompl ^ fFoCompl)) {
          RemoveFanin(nFo, nIdx);
        }
      });
      ForEachFanout<true, true, false>(nId, [&](int nFo, int nIdx, bool fFoCompl) {
        int nIdx2 = FindFanin(nFo, nFi);
        if(nIdx2 != -1) { // substitute with const-0 in case of duplication
          assert(GetCompl(nFo, nIdx2) != (fCompl ^ fFoCompl)); // of a different polarity
          vRefs_[GetConst0()]++;
          vvFaninEdges_[nFo][nIdx] = Node2Edge(GetConst0(), false);
        } else { // otherwise, substitute with fanin
          vvFaninEdges_[nFo][nIdx] = Node2Edge(nFi, fCompl ^ fFoCompl);
          vRefs_[nFi]++;
          auto it = std::find(lInts_.begin(), lInts_.end(), nId);
          assert(it != lInts_.end());
          auto it2 = std::find(it, lInts_.end(), nFi);
          if(it2 != lInts_.end()) {
            lInts_.erase(it2);
            it2 = lInts_.insert(it, nFi);
            SortInts(it2);
          }
        }
      });
      vRefs_[nId] = 0;
    }
    Action action;
    action.type = INSERT;
    action.vFanins = vInputs;
    action.vFanouts = vOutputs;
    TakenAction(action);
    for(int nId : vOutputs) {
      RemoveUnused(nId, true);
    }
    return std::make_pair(std::move(vNewOutputs), std::move(vNewCompls));
  }

  // cleanup
  
  inline void AndNetwork::Propagate(int nId) {
    StartTraversal();
    auto it = lInts_.begin();
    if(nId == -1) {
      ForEachInt([&](int nId) {
        if(GetNumFanins(nId) <= 1 || FindFanin(nId, GetConst0()) != -1) {
          vTrav_[nId] = nTrav_;
        }
      });
      while(it != lInts_.end() && vTrav_[*it] != nTrav_) {
        ++it;
      }
    } else {
      vTrav_[nId] = nTrav_;
      it = std::find(lInts_.begin(), lInts_.end(), nId);
      assert(it != lInts_.end());
    }
    fPropagating_ = true;
    while(it != lInts_.end()) {
      if(vTrav_[*it] == nTrav_) {
        if(GetNumFanins(*it) == 1) {
          RemoveBuffer(*it);
        } else {
          RemoveConst(*it);
        }
        it = lInts_.erase(it);
      } else {
        ++it;
      }
    }
    fPropagating_ = false;
    EndTraversal();
  }

  inline void AndNetwork::Sweep(bool fPropagate) {
    if(fPropagate) {
      Propagate();
    }
    for(auto it = lInts_.rbegin(); it != lInts_.rend();) {
      if(vRefs_[*it] == 0) {
        RemoveUnused(*it, false, true);
        it = std::list<int>::reverse_iterator(lInts_.erase(--it.base()));
      } else {
        ++it;
      }
    }
  }

  // save & load

  inline int AndNetwork::Save(int nSlot) {
    Action action;
    action.type = SAVE;
    if(nSlot < 0) {
      nSlot = int_size(vBackups_);
      vBackups_.emplace_back(*this);
      assert(check_int_size(vBackups_));
    } else {
      assert(nSlot < int_size(vBackups_));
      vBackups_[nSlot].Copy(*this);
    }
    action.idx = nSlot;
    TakenAction(action);
    return nSlot;
  }

  inline void AndNetwork::Load(int nSlot) {
    assert(nSlot >= 0);
    assert(nSlot < int_size(vBackups_));
    Action action;
    action.type = LOAD;
    action.idx = nSlot;
    Copy(vBackups_[nSlot]);
    TakenAction(action);
  }

  inline void AndNetwork::PopBack() {
    assert(!vBackups_.empty());
    Action action;
    action.type = POP_BACK;
    action.idx = int_size(vBackups_) - 1;
    vBackups_.pop_back();
    TakenAction(action);
  }
  
  // misc

  inline int AndNetwork::AddCallback(const Callback &callback) {
    vCallbacks_.push_back(callback);
    return int_size(vCallbacks_) - 1;
  }

  inline void AndNetwork::DeleteCallback(int nIndex) {
    vCallbacks_[nIndex] = [&](const Action &action) {
      (void)action;
    };
  }

  inline void AndNetwork::Print() const {
    std::cout << "inputs: " << vPis_ << std::endl;
    ForEachInt([&](int nId) {
      std::cout << "node " << nId << ": ";
      PrintComplementedEdges([&](const std::function<void(int, bool)> &func) {
        ForEachFanin(nId, func);
      });
      std::cout << " (ref = " << vRefs_[nId] << ")";
      std::cout << std::endl;
    });
    std::cout << "outputs: ";
    PrintComplementedEdges([&](const std::function<void(int, bool)> &func) {
      ForEachPoDriver(func);
    });
    std::cout << std::endl;
  }

  // helpers

  // TODO: reuse already allocated but dead nodes? or perform garbage collection?
  inline int AndNetwork::CreateNode() {
    assert(!check_int_max(nNodes_));
    vvFaninEdges_.emplace_back();
    vRefs_.push_back(0);
    return nNodes_++;
  }

  inline void AndNetwork::SortInts(std::list<int>::iterator it) {
    ForEachFanin(*it, [&](int nFi) {
      auto it2 = std::find(it, lInts_.end(), nFi);
      if(it2 != lInts_.end()) {
        lInts_.erase(it2);
        it2 = lInts_.insert(it, nFi);
        SortInts(it2);
      }
    });
  }

  inline unsigned AndNetwork::StartTraversal(int n) {
    assert(n > 0);
    assert(!fLockTrav_);
    fLockTrav_ = true;
    do {
      for(int i = 0; i < n; i++) {
        nTrav_++;
        if(nTrav_ == 0) {
          vTrav_.clear();
          break;
        }
      }
    } while(nTrav_ == 0);
    vTrav_.resize(nNodes_);
    return nTrav_ - n + 1;
  }

  inline void AndNetwork::EndTraversal() {
    assert(fLockTrav_);
    fLockTrav_ = false;
  }

  template <bool fPi, bool fGlobalStop, typename Func>
  inline bool AndNetwork::ForEachTfiRec(int nId, const Func &func) {
    bool fStop = false;
    if constexpr(fGlobalStop) {
        ForEachFanin<false, fPi, false>(nId, [&](int nFi) {
        if(vTrav_[nFi] == nTrav_) {
          return false;
        }
        vTrav_[nFi] = nTrav_;
        if(invoke_and_return_stop(func, nFi)) {
          fStop = true;
          return true;
        }
        fStop = ForEachTfiRec<fPi, fGlobalStop>(nFi, func);
        return fStop;
      });
    } else {
      ForEachFanin<false, fPi, false>(nId, [&](int nFi) {
        if(vTrav_[nFi] == nTrav_) {
          return;
        }
        vTrav_[nFi] = nTrav_;
        if(invoke_and_return_stop(func, nFi)) {
          return;
        }
        ForEachTfiRec<fPi, fGlobalStop>(nFi, func);
      });
    }
    return fStop;
  }

  template <bool fPi, bool fGlobalStop, bool fReverse, typename It, typename Func>
  inline void AndNetwork::ForEachTfiTopoInt(It it, unsigned nSkip, const Func &func) {
    std::vector<int> vTfi;
    bool fStop = false;
    for(; it != lInts_.rend(); ++it) {
      if(vTrav_[*it] != nTrav_) {
        continue;
      }
      if constexpr(fReverse) {
        vTfi.push_back(*it);
      } else {
        if(invoke_and_return_stop(func, *it)) {
          if constexpr(fGlobalStop) {
            fStop = true;
            break;
          }
          continue;
        }
      }
      ForEachFanin<false, fPi, false>(*it, [&](int nFi) {
        if(vTrav_[nFi] != nSkip) {
          vTrav_[nFi] = nTrav_;
        }
      });
    }
    if constexpr(fPi) {
      if(!fStop) {
        for(int nPi : vPis_) {
          if(vTrav_[nPi] == nTrav_) {
            if constexpr(fReverse) {
              vTfi.push_back(nPi);
            } else {
              if(invoke_and_return_stop(func, nPi)) {
                if constexpr(fGlobalStop) {
                  break;
                }
              }
            }
          }
        }
      }
    }
    if constexpr(fReverse) {
      for(auto itTfi = vTfi.rbegin(); itTfi != vTfi.rend(); ++itTfi) {
        func(*itTfi);
      }
    }
  }
  
  template <bool fPo, bool fGlobalStop, bool fReverse, typename It, typename Func>
  inline void AndNetwork::ForEachTfoInt(It it, unsigned nSkip, const Func &func) {
    std::vector<int> vTfo;
    bool fStop = false;
    for(; it != lInts_.end(); ++it) {
      if(vTrav_[*it] == nSkip) {
        continue;
      }
      if(vTrav_[*it] != nTrav_) {
        ForEachFanin(*it, [&](int nFi) {
          if(vTrav_[nFi] == nTrav_) {
            vTrav_[*it] = nTrav_;
            return true;
          }
          return false;
        });
      }
      if(vTrav_[*it] == nTrav_) {
        if constexpr(fReverse) {
          vTfo.push_back(*it);
        } else {
          if(invoke_and_return_stop(func, *it)) {
            if constexpr(fGlobalStop) {
              fStop = true;
              break;
            } else {
              vTrav_[*it] = 0;
            }
          }
        }
      }
    }
    if constexpr(fPo) {
      if(!fStop) {
        for(int nPo : vPos_) {
          if(vTrav_[nPo] != nSkip) {
            if(vTrav_[GetFanin(nPo, 0)] == nTrav_) {
              if constexpr(fReverse) {
                vTfo.push_back(nPo);
              } else {
                if(invoke_and_return_stop(func, nPo)) {
                  if constexpr(fGlobalStop) {
                    break;
                  }
                }
              }
            }
          }
        }
      }
    }
    if constexpr(fReverse) {
      for(auto itTfo = vTfo.rbegin(); itTfo != vTfo.rend(); ++itTfo) {
        func(*itTfo);
      }
    }
  }

  inline void AndNetwork::Copy(const AndNetwork &from) {
    nNodes_ = from.nNodes_;
    vPis_ = from.vPis_;
    vPos_ = from.vPos_;
    lInts_ = from.lInts_;
    sInts_ = from.sInts_;
    vvFaninEdges_ = from.vvFaninEdges_;
    vRefs_ = from.vRefs_;
  }

  inline void AndNetwork::TakenAction(const Action &action) const {
    for(const Callback &callback : vCallbacks_) {
      callback(action);
    }
  }

} // namespace boop

BOOP_HEADER_END
