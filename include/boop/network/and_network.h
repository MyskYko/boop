#pragma once

#include <utility>
#include <functional>
#include <set>
#include <vector>
#include <list>
#include <map>
#include <algorithm>

#include "util/util.h"

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
    void Read(const AndNetwork &from);
    template <typename Ntk, typename Reader>
    int Read(const Ntk &from, const Reader &reader);

    // network properties
    bool UseComplementedEdges() const;
    bool HasMultipleNodeTypes() const;
    int GetNumNodes() const; // number of allocated nodes (max id + 1)
    int GetNumPis() const;
    int GetNumInts() const;
    int GetNumPos() const;
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
    NodeType GetNodeType(int nId) const;
    bool IsPoDriver(int nId) const;
    int GetPiIndex(int nId) const;
    int GetIntIndex(int nId) const;
    int GetPoIndex(int nId) const;
    int GetNumFanins(int nId) const;
    int GetNumFanouts(int nId) const;
    int GetFanin(int nId, int nIdx) const;
    bool GetCompl(int nId, int nIdx) const;
    int FindFanin(int nId, int nFi) const;
    bool IsReconvergent(int nId);
    std::vector<int> GetNeighbors(int nId, bool fPis, int nHops);
    template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
    bool IsReachable(const Container<Ts...> &srcs, const Container2<Ts2...> &dsts);
    template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
    std::vector<int> GetInners(const Container<Ts...> &srcs, const Container2<Ts2...> &dsts);
    std::set<int> GetExtendedFanins(int nId);

    // network traversal
    template <bool fReverse = false, typename Func> void ForEachPi(const Func &func) const;
    template <bool fReverse = false, typename Func> void ForEachPo(const Func &func) const;
    template <bool fReverse = false, typename Func> void ForEachPoDriver(const Func &func) const;
    template <bool fReverse = false, typename Func> void ForEachInt(const Func &func) const;
    template <bool fReverse = false, typename Func> void ForEachPiInt(const Func &func) const;
    template <bool fReverse = false, typename Func> void ForEachFanin(int nId, const Func &func) const;
    template <bool fReverse = false, typename Func> void ForEachFanout(int nId, bool fPos, const Func &func) const;
    template <bool fReverse = false, typename Func> void ForEachTfi(int nId, bool fPis, const Func &func);
    template <bool fReverse = false, typename Func> void ForEachTfo(int nId, bool fPos, const Func &func);
    template <bool fReverse = false, template <typename...> typename Container, typename... Ts, typename Func>
    void ForEachTfiEnd(int nId, const Container<Ts...> &ends, const Func &func);
    template <bool fReverse = false, template <typename...> typename Container, typename... Ts, typename Func>
    void ForEachTfoEnd(int nId, const Container<Ts...> &ends, const Func &func);
    template <bool fReverse = false, template <typename...> typename Container, typename... Ts, typename Func>
    void ForEachTfis(const Container<Ts...> &ids, bool fPis, const Func &func);
    template <bool fReverse = false, template <typename...> typename Container, typename... Ts, typename Func>
    void ForEachTfos(const Container<Ts...> &ids, bool fPos, const Func &func);

    // extraction
    template <template <typename...> typename Container, typename... Ts>
    std::unique_ptr<AndNetwork> Extract(const Container<Ts...> &ids, const std::vector<int> &vInputs, const std::vector<int> &vOutputs);

    // actions
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
    
    // network cleanup
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
    unsigned iTrav_;
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
    unsigned StartTraversal(unsigned n = 1);
    void EndTraversal();
    void ForEachTfiRec(int nId, const std::function<void(int)> &fn);  // TODO: may fix this
    void Copy(const AndNetwork &from);
    void TakenAction(const Action &action) const;
  };

  // lifecycle
  
  inline AndNetwork::AndNetwork()
    : nNodes_(0),
      fLockTrav_(false),
      iTrav_(0),
      fPropagating_(false) {
    // add constant node
    vvFaninEdges_.emplace_back();
    vRefs_.push_back(0);
    nNodes_++;
  }

  inline AndNetwork::AndNetwork(const AndNetwork &other)
    : fLockTrav_(false),
      iTrav_(0),
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
      iTrav_ = 0;
      vTrav_.clear();
      fPropagating_ = false;
      // add constant node
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
  
  // TODO: from here
  
  inline int AndNetwork::AddAnd(int nId0, int nId1, bool fCompl0, bool fCompl1) {
    assert(nId0 < nNodes_);
    assert(nId1 < nNodes_);
    // TODO: it is a philosophical question whether to allow dangling nodes or not
    assert(nId0 != nId1);
    lInts_.push_back(nNodes_);
    sInts_.insert(nNodes_);
    vRefs_[nId0]++;
    vRefs_[nId1]++;
    vvFaninEdges_.emplace_back(std::initializer_list<int>({Node2Edge(nId0, fCompl0), Node2Edge(nId1, fCompl1)}));
    vRefs_.push_back(0);
    assert(!check_int_max(nNodes_));
    return nNodes_++;
  }

  inline int AndNetwork::AddAnd(std::vector<int> const &vFanins, std::vector<bool> const &vCompls) {
    lInts.push_back(nNodes);
    sInts.insert(nNodes);
    assert(vFanins.size() == vCompls.size());
    vvFaninEdges.emplace_back(vFanins.size());
    for(int i = 0; i < int_size(vFanins); i++) {
      assert(vFanins[i] < nNodes);
      vRefs[vFanins[i]]++;
      vvFaninEdges[nNodes][i] = Node2Edge(vFanins[i], vCompls[i]);
    }
    vRefs.push_back(0);
    assert(!check_int_max(nNodes));
    return nNodes++;
  }

  inline int AndNetwork::AddPo(int id, bool c) {
    assert(id < nNodes);
    vPos.push_back(nNodes);
    vRefs[id]++;
    vvFaninEdges.emplace_back(std::initializer_list<int>({Node2Edge(id, c)}));
    vRefs.push_back(0);
    assert(!check_int_max(nNodes));
    return nNodes++;
  }

  inline void AndNetwork::ChangePiOrder(std::vector<int> const &vOrder) {
    assert(vOrder.size() == vPis.size());
    std::vector<int> vPisNew(vPis.size());
    for(int idx = 0; idx < int_size(vPis); idx++) {
      int old_idx = vOrder[idx];
      vPisNew[idx] = vPis[old_idx];
    }
    vPis = vPisNew;
  }

  inline void AndNetwork::Read(AndNetwork const &from) {
    Clear(true, false, false);
    Copy(from);
    Action action;
    action.type = READ;
    TakenAction(action);
  }

  template <typename Ntk, typename Reader>
  int AndNetwork::Read(Ntk const &from, Reader const &reader) {
    int r = 0;
    Clear(true, false, false);
    if constexpr(returns_int_v<Reader, Ntk const &, AndNetwork *>) {
      r = reader(from, this);
    } else {
      reader(from, this);
    }
    Action action;
    action.type = READ;
    TakenAction(action);
    return r;
  }
  
  /* }}} */
  
  /* {{{ Private functions */

  inline int AndNetwork::CreateNode() {
    // TODO: reuse already allocated but dead nodes? or perform garbage collection?
    vvFaninEdges.emplace_back();
    vRefs.push_back(0);
    assert(!check_int_max(nNodes));
    return nNodes++;
  }

  inline void AndNetwork::SortInts(itr it) {
    ForEachFanin(*it, [&](int fi) {
      itr it2 = std::find(it, lInts.end(), fi);
      if(it2 != lInts.end()) {
        lInts.erase(it2);
        it2 = lInts.insert(it, fi);
        SortInts(it2);
      }
    });
  }

  inline unsigned AndNetwork::StartTraversal(unsigned n) {
    assert(n > 0);
    assert(!fLockTrav);
    fLockTrav = true;
    do {
      for(int i = 0; i < n; i++) {
        iTrav++;
        if(iTrav == 0) {
          vTrav.clear();
          break;
        }
      }
    } while(iTrav == 0);
    vTrav.resize(nNodes);
    return iTrav - n + 1;
  }
  
  inline void AndNetwork::EndTraversal() {
    assert(fLockTrav);
    fLockTrav = false;
  }

  inline void AndNetwork::ForEachTfiRec(int id, std::function<void(int)> const &func) {
    for(int fi_edge: vvFaninEdges[id]) {
      int fi = Edge2Node(fi_edge);
      if(vTrav[fi] == iTrav) {
        continue;
      }
      func(fi);
      vTrav[fi] = iTrav;
      ForEachTfiRec(fi, func);
    }
  }

  inline void AndNetwork::Copy(AndNetwork const &from) {
    nNodes       = from.nNodes;
    vPis         = from.vPis;
    vPos         = from.vPos;
    lInts        = from.lInts;
    sInts        = from.sInts;
    vvFaninEdges = from.vvFaninEdges;
    vRefs        = from.vRefs;
    pPat         = from.pPat;
    pCond        = from.pCond;
  }

  inline void AndNetwork::TakenAction(Action const &action) const {
    for(Callback const &callback: vCallbacks) {
      callback(action);
    }
  }

  /* }}} */

  /* {{{ Constructors */

  
  /* }}} */

  
  /* {{{ Network properties */
  
  inline bool AndNetwork::UseComplementedEdges() const {
    return true;
  }
  
  inline bool AndNetwork::HasMultipleNodeTypes() const {
    return false;
  }
  
  inline int AndNetwork::GetNumNodes() const {
    return nNodes;
  }

  inline int AndNetwork::GetNumPis() const {
    return int_size(vPis);
  }
  
  inline int AndNetwork::GetNumInts() const {
    return int_size(lInts);
  }
  
  inline int AndNetwork::GetNumPos() const {
    return int_size(vPos);
  }

  inline int AndNetwork::GetNumLevels() const {
    int nMaxLevel = 0;
    std::vector<int> vLevels(nNodes);
    for(int id: lInts) {
      for(int fi_edge: vvFaninEdges[id]) {
        int fi = Edge2Node(fi_edge);
        if(vLevels[id] < vLevels[fi]) {
          vLevels[id] = vLevels[fi];
        }
      }
      vLevels[id] += 1;
      if(nMaxLevel < vLevels[id]) {
        nMaxLevel = vLevels[id];
      }
    }
    return nMaxLevel;
  }

  inline int AndNetwork::GetConst0() const {
    return 0;
  }
  
  inline int AndNetwork::GetPi(int idx) const {
    return vPis[idx];
  }

  inline int AndNetwork::GetPo(int idx) const {
    return vPos[idx];
  }

  inline std::vector<int> AndNetwork::GetPis() const {
    return vPis;
  }

  inline std::vector<int> AndNetwork::GetInts() const {
    return std::vector<int>(lInts.begin(), lInts.end());
  }
  
  inline std::vector<int> AndNetwork::GetPisInts() const {
    std::vector<int> vPisInts = vPis;
    vPisInts.insert(vPisInts.end(), lInts.begin(), lInts.end());
    return vPisInts;
  }
  
  inline std::vector<int> AndNetwork::GetPos() const {
    return vPos;
  }
  
  /* }}} */

  /* {{{ Node properties */
  
  inline bool AndNetwork::IsPi(int id) const {
    return GetNumFanins(id) == 0 && std::find(vPis.begin(), vPis.end(), id) != vPis.end();
  }

  inline bool AndNetwork::IsInt(int id) const {
    return sInts.count(id);
  }

  inline bool AndNetwork::IsPo(int id) const {
    return GetNumFanouts(id) == 0 && std::find(vPos.begin(), vPos.end(), id) != vPos.end();
  }
  
  inline NodeType AndNetwork::GetNodeType(int id) const {
    if(IsPi(id)) {
      return PI;
    }
    if(IsPo(id)) {
      return PO;
    }
    return AND;
  }

  inline bool AndNetwork::IsPoDriver(int id) const {
    for(int po: vPos) {
      if(GetFanin(po, 0) == id) {
        return true;
      }
    }
    return false;
  }

  inline int AndNetwork::GetPiIndex(int id) const {
    assert(IsPi(id));
    assert(check_int_size(vPis));
    std::vector<int>::const_iterator it = std::find(vPis.begin(), vPis.end(), id);
    assert(it != vPis.end());
    return std::distance(vPis.begin(), it);
  }
  
  inline int AndNetwork::GetIntIndex(int id) const {
    assert(check_int_size(lInts));
    int index = 0;
    citr it = lInts.begin();
    for(; it != lInts.end(); it++) {
      if(*it == id) {
        break;
      }
      index++;
    }
    assert(it != lInts.end());
    return index;
  }

  inline int AndNetwork::GetPoIndex(int id) const {
    assert(IsPo(id));
    assert(check_int_size(vPos));
    std::vector<int>::const_iterator it = std::find(vPos.begin(), vPos.end(), id);
    assert(it != vPos.end());
    return std::distance(vPos.begin(), it);
  }
  
  inline int AndNetwork::GetNumFanins(int id) const {
    return int_size(vvFaninEdges[id]);
  }

  inline int AndNetwork::GetNumFanouts(int id) const {
    return vRefs[id];
  }

  inline int AndNetwork::GetFanin(int id, int idx) const {
    return Edge2Node(vvFaninEdges[id][idx]);
  }

  inline bool AndNetwork::GetCompl(int id, int idx) const {
    return EdgeIsCompl(vvFaninEdges[id][idx]);
  }

  inline int AndNetwork::FindFanin(int id, int fi) const {
    for(int idx = 0; idx < GetNumFanins(id); idx++) {
      if(GetFanin(id, idx) == fi) {
        return idx;
      }
    }
    return -1;
  }
  
  inline bool AndNetwork::IsReconvergent(int id) {
    if(GetNumFanouts(id) <= 1) {
      return false;
    }
    unsigned iTravStart = StartTraversal(GetNumFanouts(id));
    int idx = 0;
    ForEachFanout(id, false, [&](int fo) {
      vTrav[fo] = iTravStart + idx;
      idx++;
    });
    if(idx <= 1) {
      // less than two fanouts excluding POs
      EndTraversal();
      return false;
    }
    citr it = lInts.begin();
    while(vTrav[*it] < iTravStart && it != lInts.end()) {
      it++;
    }
    it++;
    for(; it != lInts.end(); it++) {
      for(int fi_edge: vvFaninEdges[*it]) {
        int fi = Edge2Node(fi_edge);
        if(vTrav[fi] >= iTravStart) {
          if(vTrav[*it] >= iTravStart && vTrav[*it] != vTrav[fi]) {
            EndTraversal();
            return true;
          }
          vTrav[*it] = vTrav[fi];
        }
      }
    }
    EndTraversal();
    return false;
  }

  inline std::vector<int> AndNetwork::GetNeighbors(int id, bool fPis, int nHops) {
    StartTraversal();
    vTrav[id] = iTrav;
    std::vector<int> vPrevs, vNexts;
    vNexts.push_back(id);
    for(int i = 0; i < nHops; i++) {
      vPrevs.swap(vNexts);
      for(int id: vPrevs) {
        ForEachFanin(id, [&](int fi) {
          if(vTrav[fi] != iTrav) {
            vNexts.push_back(fi);
            vTrav[fi] = iTrav;
          }
        });
        ForEachFanout(id, false, [&](int fo) {
          if(vTrav[fo] != iTrav) {
            vNexts.push_back(fo);
            vTrav[fo] = iTrav;
          }
        });
      }
      vPrevs.clear();
    }
    vTrav[id] = 0;
    std::vector<int> v;
    if(fPis) {
      ForEachPiInt([&](int id) {
        if(vTrav[id] == iTrav) {
          v.push_back(id);
        }
      });
    } else {
      ForEachInt([&](int id) {
        if(vTrav[id] == iTrav) {
          v.push_back(id);
        }
      });
    }
    EndTraversal();
    return v;
  }

  template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
  inline bool AndNetwork::IsReachable(Container<Ts...> const &srcs, Container2<Ts2...> const &dsts) {
    if(srcs.empty() || dsts.empty()) {
      return false;
    }
    // mark destinations
    unsigned iTravStart = StartTraversal(2);
    for(int id: dsts) {
      vTrav[id] = iTravStart;
    }
    // mark sources
    for(int id: srcs) {
      if(vTrav[id] == iTravStart) {
        EndTraversal();
        return true;
      }
      vTrav[id] = iTrav;
    }
    // find the first source
    citr it = lInts.begin();
    while(vTrav[*it] != iTrav && it != lInts.end()) {
      it++;
    }
    // check if sources are reachable to destinations
    for(; it != lInts.end(); it++) {
      if(vTrav[*it] == iTrav) {
        continue;
      }
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTrav) {
          if(vTrav[*it] == iTravStart) {
            EndTraversal();
            return true;
          }
          vTrav[*it] = iTrav;
          break;
        }
      }
    }
    for(int po: vPos) {
      if(vTrav[po] == iTrav) {
        continue;
      }
      if(vTrav[GetFanin(po, 0)] == iTrav) {
        if(vTrav[po] == iTravStart) {
          EndTraversal();
          return true;
        }
        vTrav[po] = iTrav;
      }
    }
    EndTraversal();
    return false;
  }

  template <template <typename...> typename Container, typename... Ts, template <typename...> typename Container2, typename... Ts2>
  inline std::vector<int> AndNetwork::GetInners(Container<Ts...> const &srcs, Container2<Ts2...> const &dsts) {
    // this includes sources and destinations that are connected
    if(srcs.empty() || dsts.empty()) {
      return std::vector<int>();
    }
    unsigned iTravStart = StartTraversal(4);
    unsigned iDst = iTravStart;
    unsigned iTfo = iTravStart + 1;
    unsigned iInner = iTravStart + 2;
    // mark destinations (to prevent nodes between destinations to sources being included)
    for(int id: dsts) {
      vTrav[id] = iDst;
    }
    // mark TFOs of sources until destinations, which will be marekd as inner
    for(int id: srcs) {
      if(vTrav[id] == iDst) {
        vTrav[id] = iInner;
      } else {
        vTrav[id] = iTfo;
      }
    }
    citr it = lInts.begin();
    while(vTrav[*it] != iTfo && it != lInts.end()) {
      it++;
    }
    for(; it != lInts.end(); it++) {
      if(vTrav[*it] >= iTfo) { // TFO or inner
        continue;
      }
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTfo) {
          if(vTrav[*it] == iDst) {
            vTrav[*it] = iInner;
          } else {
            vTrav[*it] = iTfo;
          }
          break;
        }
      }
    }
    // traverse TFIs of connected destinations
    std::vector<int> vInners;
    for(int id: dsts) {
      if(vTrav[id] == iInner) {
        vInners.push_back(id);
        vTrav[id] = iTrav;
        ForEachTfiRec(id, [&](int fi) {
          if(vTrav[fi] == iTfo || vTrav[fi] == iInner) {
            vInners.push_back(fi);
          }
        });
      }
    }
    EndTraversal();
    return vInners;
  }

  inline std::set<int> AndNetwork::GetExtendedFanins(int id) {
    // go to the root of trivially collapsable nodes
    while(GetNumFanouts(id) == 1) {
      int id_new = -1;
      ForEachFanout(id, false, [&](int fo, bool c) {
        if(!c) {
          id_new = fo;
        }
      });
      if(id_new != -1) {
        id = id_new;
      } else {
        break;
      }
    }
    // emulate trivial collapse
    std::vector<int> vFaninEdges = vvFaninEdges[id];
    for(int idx = 0; idx < int_size(vFaninEdges);) {
      int fi_edge = vFaninEdges[idx];
      int fi = Edge2Node(fi_edge);
      bool c = EdgeIsCompl(fi_edge);
      if(!IsPi(fi) && !c && vRefs[fi] == 1) {
        std::vector<int>::iterator it = vFaninEdges.begin() + idx;
        it = vFaninEdges.erase(it);
        vFaninEdges.insert(it, vvFaninEdges[fi].begin(), vvFaninEdges[fi].end());
      } else {
        idx++;
      }
    }
    // create set
    std::set<int> sFanins;
    for(int fi_edge: vFaninEdges) {
      sFanins.insert(Edge2Node(fi_edge));
    }
    return sFanins;
  }

  /* }}} */

  /* {{{ Network traversal */

  inline void AndNetwork::ForEachPi(std::function<void(int)> const &func) const {
    for(int pi: vPis) {
      func(pi);
    }
  }

  inline void AndNetwork::ForEachPiIdx(std::function<void(int, int)> const &func) const {
    for(int idx = 0; idx < GetNumPis(); idx++) {
      func(idx, GetPi(idx));
    }
  }

  inline void AndNetwork::ForEachInt(std::function<void(int)> const &func) const {
    for(int id: lInts) {
      func(id);
    }
  }

  inline void AndNetwork::ForEachIntReverse(std::function<void(int)> const &func) const {
    for(critr it = lInts.rbegin(); it != lInts.rend(); it++) {
      func(*it);
    }
  }
  
  inline void AndNetwork::ForEachIntStop(std::function<bool(int)> const &func) const {
    for(int id: lInts) {
      if(func(id)) {
        break;
      }
    }
  }

  inline void AndNetwork::ForEachPiInt(std::function<void(int)> const &func) const {
    for(int pi: vPis) {
      func(pi);
    }
    for(int id: lInts) {
      func(id);
    }
  }
  
  inline void AndNetwork::ForEachPiIntStop(std::function<bool(int)> const &func) const {
    for(int pi: vPis) {
      if(func(pi)) {
        return;
      }
    }
    for(int id: lInts) {
      if(func(id)) {
        return;
      }
    }
  }
  
  inline void AndNetwork::ForEachPo(std::function<void(int)> const &func) const {
    for(int po: vPos) {
      func(po);
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachPoDriver(Func const &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each edge function format error");
    for(int po: vPos) {
      if constexpr(is_invokable<Func, int>::value) {
        func(GetFanin(po, 0));
      } else if constexpr(is_invokable<Func, int, bool>::value) {
        func(GetFanin(po, 0), GetCompl(po, 0));
      }
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachPoDriverIdx(Func const &func) const {
    static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, int, bool>::value, "for each edge function format error");
    for(int idx = 0; idx < GetNumPos(); idx++) {
      if constexpr(is_invokable<Func, int, int>::value) {
        func(idx, GetFanin(vPos[idx], 0));
      } else if constexpr(is_invokable<Func, int, int, bool>::value) {
        func(idx, GetFanin(vPos[idx], 0), GetCompl(vPos[idx], 0));
      }
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachPoDriverStop(Func const &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each edge function format error");
    for(int po: vPos) {
      if constexpr(is_invokable<Func, int>::value) {
        if(func(GetFanin(po, 0))) {
          break;
        }
      } else if constexpr(is_invokable<Func, int, bool>::value) {
        if(func(GetFanin(po, 0), GetCompl(po, 0))) {
          break;
        }
      }
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachFanin(int id, Func const &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each edge function format error");
    for(int fi_edge: vvFaninEdges[id]) {
      if constexpr(is_invokable<Func, int>::value) {
        func(Edge2Node(fi_edge));
      } else if constexpr(is_invokable<Func, int, bool>::value) {
        func(Edge2Node(fi_edge), EdgeIsCompl(fi_edge));
      }
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachFaninIdx(int id, Func const &func) const {
    static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, int, bool>::value, "for each edge function format error");
    for(int idx = 0; idx < GetNumFanins(id); idx++) {
      if constexpr(is_invokable<Func, int, int>::value) {
        func(idx, GetFanin(id, idx));
      } else if constexpr(is_invokable<Func, int, int, bool>::value) {
        func(idx, GetFanin(id, idx), GetCompl(id, idx));
      }
    }
  }
  
  template <typename Func>
  inline void AndNetwork::ForEachFaninReverse(int id, Func const &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each edge function format error");
    for(std::vector<int>::const_reverse_iterator it = vvFaninEdges[id].rbegin(); it != vvFaninEdges[id].rend(); it++) {
      if constexpr(is_invokable<Func, int>::value) {
        func(Edge2Node(*it));
      } else if constexpr(is_invokable<Func, int, bool>::value) {
        func(Edge2Node(*it), EdgeIsCompl(*it));
      }
    }
  }

  template <typename Func>
  inline void AndNetwork::ForEachFanout(int id, bool fPos, Func const &func) const {
    static_assert(is_invokable<Func, int>::value || is_invokable<Func, int, bool>::value, "for each edge function format error");
    if(vRefs[id] == 0) {
      return;
    }
    citr it = lInts.begin();
    if(IsInt(id)) {
      it = std::find(it, lInts.end(), id);
      assert(it != lInts.end());
      it++;
    }
    int nRefs = vRefs[id];
    for(; nRefs != 0 && it != lInts.end(); it++) {
      int idx = FindFanin(*it, id);
      if(idx >= 0) {
        if constexpr(is_invokable<Func, int>::value) {
          func(*it);
        } else if constexpr(is_invokable<Func, int, bool>::value) {
          func(*it, GetCompl(*it, idx));
        }
        nRefs--;
      }
    }
    if(fPos && nRefs != 0) {
      for(int po: vPos) {
        if(GetFanin(po, 0) == id) {
          if constexpr(is_invokable<Func, int>::value) {
            func(po);
          } else if constexpr(is_invokable<Func, int, bool>::value) {
            func(po, GetCompl(po, 0));
          }
          nRefs--;
          if(nRefs == 0) {
            break;
          }
        }
      }
    }
    assert(!fPos || nRefs == 0);
  }

  template <typename Func>
  inline void AndNetwork::ForEachFanoutRidx(int id, bool fPos, Func const &func) const {
    static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, bool, int>::value, "for each edge function format error");
    if(vRefs[id] == 0) {
      return;
    }
    citr it = lInts.begin();
    if(IsInt(id)) {
      it = std::find(it, lInts.end(), id);
      assert(it != lInts.end());
      it++;
    }
    int nRefs = vRefs[id];
    for(; nRefs != 0 && it != lInts.end(); it++) {
      int idx = FindFanin(*it, id);
      if(idx >= 0) {
        if constexpr(is_invokable<Func, int, int>::value) {
          func(*it, idx);
        } else if constexpr(is_invokable<Func, int, bool, int>::value) {
          func(*it, GetCompl(*it, idx), idx);
        }
        nRefs--;
      }
    }
    if(fPos && nRefs != 0) {
      for(int po: vPos) {
        if(GetFanin(po, 0) == id) {
          if constexpr(is_invokable<Func, int, int>::value) {
            func(po, 0);
          } else if constexpr(is_invokable<Func, int, bool, int>::value) {
            func(po, GetCompl(po, 0), 0);
          }
          nRefs--;
          if(nRefs == 0) {
            break;
          }
        }
      }
    }
    assert(!fPos || nRefs == 0);
  }

  inline void AndNetwork::ForEachTfi(int id, bool fPis, std::function<void(int)> const &func) {
    // this does not include id itself
    StartTraversal();
    if(!fPis) {
      for(int pi: vPis) {
        vTrav[pi] = iTrav;
      }
    }
    ForEachTfiRec(id, func);
    EndTraversal();
  }

  template <template <typename...> typename Container, typename... Ts>
  inline void AndNetwork::ForEachTfiEnd(int id, Container<Ts...> const &ends, std::function<void(int)> const &func) {
    // this does not include id itself
    StartTraversal();
    for(int end: ends) {
      vTrav[end] = iTrav;
    }
    ForEachTfiRec(id, func);
    EndTraversal();
  }

  inline void AndNetwork::ForEachTfiUpdate(int id, bool fPis, std::function<bool(int)> const &func) {
    if(GetNumFanins(id) == 0) {
      return;
    }
    StartTraversal();
    for(int fi_edge: vvFaninEdges[id]) {
      vTrav[Edge2Node(fi_edge)] = iTrav;
    }
    critr it = std::find(lInts.rbegin(), lInts.rend(), id);
    assert(it != lInts.rend());
    it++;
    for(; it != lInts.rend(); it++) {
      if(vTrav[*it] == iTrav) {
        if(func(*it)) {
          for(int fi_edge: vvFaninEdges[*it]) {
            vTrav[Edge2Node(fi_edge)] = iTrav;
          }
        }
      }
    }
    if(fPis) {
      for(int pi: vPis) {
        if(vTrav[pi] == iTrav) {
          func(pi);
        }
      }
    }
    EndTraversal();
  }

  template <template <typename...> typename Container, typename... Ts>
  inline void AndNetwork::ForEachTfisUpdate(Container<Ts...> const &ids, bool fPis, std::function<bool(int)> const &func) {
    // this includes ids themselves
    StartTraversal();
    for(int id: ids) {
      vTrav[id] = iTrav;
    }
    critr it = lInts.rbegin();
    while(vTrav[*it] != iTrav && it != lInts.rend()) {
      it++;
    }
    for(; it != lInts.rend(); it++) {
      if(vTrav[*it] == iTrav) {
        if(func(*it)) {
          for(int fi_edge: vvFaninEdges[*it]) {
            vTrav[Edge2Node(fi_edge)] = iTrav;
          }
        }
      }
    }
    if(fPis) {
      for(int pi: vPis) {
        if(vTrav[pi] == iTrav) {
          func(pi);
        }
      }
    }
    EndTraversal();
  }

  inline void AndNetwork::ForEachTfo(int id, bool fPos, std::function<void(int)> const &func) {
    // this does not include id itself
    if(vRefs[id] == 0) {
      return;
    }
    StartTraversal();
    vTrav[id] = iTrav;
    citr it = std::find(lInts.begin(), lInts.end(), id);
    assert(it != lInts.end());
    it++;
    for(; it != lInts.end(); it++) {
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTrav) {
          func(*it);
          vTrav[*it] = iTrav;
          break;
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[GetFanin(po, 0)] == iTrav) {
          func(po);
          vTrav[po] = iTrav;
        }
      }
    }
    EndTraversal();
  }

  inline void AndNetwork::ForEachTfoReverse(int id, bool fPos, std::function<void(int)> const &func) {
    // this does not include id itself
    if(vRefs[id] == 0) {
      return;
    }
    StartTraversal();
    vTrav[id] = iTrav;
    citr it = std::find(lInts.begin(), lInts.end(), id);
    assert(it != lInts.end());
    it++;
    for(; it != lInts.end(); it++) {
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTrav) {
          vTrav[*it] = iTrav;
          break;
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[GetFanin(po, 0)] == iTrav) {
          vTrav[po] = iTrav;
        }
      }
    }
    EndTraversal(); // release here so func can call IsReconvergent
    unsigned iTravTfo = iTrav;
    if(fPos) {
      // use reverse order even for POs
      for(std::vector<int>::const_reverse_iterator it = vPos.rbegin(); it != vPos.rend(); it++) {
        assert(vTrav[*it] <= iTravTfo); // make sure func does not touch vTrav of preceding nodes
        if(vTrav[*it] == iTravTfo) {
          func(*it);
        }
      }
    }
    for(critr it = lInts.rbegin(); *it != id; it++) {
      assert(vTrav[*it] <= iTravTfo); // make sure func does not touch vTrav of preceding nodes
      if(vTrav[*it] == iTravTfo) {
        func(*it);
      }
    }
  }

  inline void AndNetwork::ForEachTfoUpdate(int id, bool fPos, std::function<bool(int)> const &func) {
    // this does not include id itself
    if(vRefs[id] == 0) {
      return;
    }
    StartTraversal();
    vTrav[id] = iTrav;
    citr it = std::find(lInts.begin(), lInts.end(), id);
    assert(it != lInts.end());
    it++;
    for(; it != lInts.end(); it++) {
      for(int fi_edge: vvFaninEdges[*it]) {
        if(vTrav[Edge2Node(fi_edge)] == iTrav) {
          if(func(*it)) {
            vTrav[*it] = iTrav;
          }
          break;
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[GetFanin(po, 0)] == iTrav) {
          if(func(po)) {
            vTrav[po] = iTrav;
          }
        }
      }
    }
    EndTraversal();
  }

  template <template <typename...> typename Container, typename... Ts>
  inline void AndNetwork::ForEachTfos(Container<Ts...> const &ids, bool fPos, std::function<void(int)> const &func) {
    // this includes ids themselves
    StartTraversal();
    for(int id: ids) {
      vTrav[id] = iTrav;
    }
    citr it = lInts.begin();
    while(vTrav[*it] != iTrav && it != lInts.end()) {
      it++;
    }
    for(; it != lInts.end(); it++) {
      if(vTrav[*it] == iTrav) {
        func(*it);
      } else {
        for(int fi_edge: vvFaninEdges[*it]) {
          if(vTrav[Edge2Node(fi_edge)] == iTrav) {
            func(*it);
            vTrav[*it] = iTrav;
            break;
          }
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[po] == iTrav || vTrav[GetFanin(po, 0)] == iTrav) {
          func(po);
          vTrav[po] = iTrav;
        }
      }
    }
    EndTraversal();
  }
  
  template <template <typename...> typename Container, typename... Ts>
  inline void AndNetwork::ForEachTfosUpdate(Container<Ts...> const &ids, bool fPos, std::function<bool(int)> const &func) {
    // this includes ids themselves
    StartTraversal();
    for(int id: ids) {
      vTrav[id] = iTrav;
    }
    citr it = lInts.begin();
    while(vTrav[*it] != iTrav && it != lInts.end()) {
      it++;
    }
    for(; it != lInts.end(); it++) {
      if(vTrav[*it] == iTrav) {
        if(!func(*it)) {
          vTrav[*it] = 0;
        }
      } else {
        for(int fi_edge: vvFaninEdges[*it]) {
          if(vTrav[Edge2Node(fi_edge)] == iTrav) {
            if(func(*it)) {
              vTrav[*it] = iTrav;
            }
            break;
          }
        }
      }
    }
    if(fPos) {
      for(int po: vPos) {
        if(vTrav[po] == iTrav) {
          if(!func(po)) {
            vTrav[po] = 0;
          }
        } else if(vTrav[GetFanin(po, 0)] == iTrav) {
          if(func(po)) {
            vTrav[po] = iTrav;
          }
        }
      }
    }
    EndTraversal();
  }

  /* }}} */

  /* {{{ Extraction */

  template <template <typename...> typename Container, typename... Ts>
  inline AndNetwork *AndNetwork::Extract(Container<Ts...> const &ids, std::vector<int> const &vInputs, std::vector<int> const &vOutputs) {
    AndNetwork *pNtk = new AndNetwork;
    pNtk->Reserve(int_size(vInputs) + int_size(ids) + int_size(vOutputs));
    std::map<int, int> m;
    m[GetConst0()] = pNtk->GetConst0();
    for(int id: vInputs) {
      m[id] = pNtk->AddPi();
    }
    StartTraversal();
    for(int id: ids) {
      vTrav[id] = iTrav;
    }
    ForEachInt([&](int id) {
      if(vTrav[id] == iTrav) {
        m[id] = pNtk->CreateNode();
        pNtk->lInts.push_back(m[id]);
        pNtk->sInts.insert(m[id]);
        pNtk->vvFaninEdges[m[id]].resize(GetNumFanins(id));
        ForEachFaninIdx(id, [&](int idx, int fi, bool c) {
          assert(m.count(fi));
          pNtk->vvFaninEdges[m[id]][idx] = pNtk->Node2Edge(m[fi], c);
          pNtk->vRefs[m[fi]]++;
        });
      }
    });
    EndTraversal();
    for(int id: vOutputs) {
      assert(m.count(id));
      pNtk->AddPo(m[id], false);
    }
    return pNtk;
  }
  
  /* }}} */
  
  /* {{{ Actions */
  
  inline void AndNetwork::RemoveFanin(int id, int idx) {
    Action action;
    action.type = REMOVE_FANIN;
    action.id = id;
    action.idx = idx;
    int fi = GetFanin(id, idx);
    bool c = GetCompl(id, idx);
    action.fi = fi;
    action.c = c;
    vRefs[fi]--;
    vvFaninEdges[id].erase(vvFaninEdges[id].begin() + idx);
    TakenAction(action);
  }

  inline void AndNetwork::RemoveUnused(int id, bool fRecursive, bool fSweeping) {
    assert(vRefs[id] == 0);
    Action action;
    action.type = REMOVE_UNUSED;
    action.id = id;
    ForEachFanin(id, [&](int fi) {
      action.vFanins.push_back(fi);
      vRefs[fi]--;
    });
    vvFaninEdges[id].clear();
    if(!fSweeping) {
      itr it = std::find(lInts.begin(), lInts.end(), id);
      lInts.erase(it);
    }
    sInts.erase(id);
    TakenAction(action);
    if(fRecursive) {
      for(int fi: action.vFanins) {
        if(vRefs[fi] == 0 && IsInt(fi)) {
          RemoveUnused(fi, fRecursive, fSweeping);
        }
      }
    }
  }

  inline void AndNetwork::RemoveBuffer(int id) {
    assert(GetNumFanins(id) == 1);
    assert(!fPropagating || fLockTrav);
    int fi = GetFanin(id, 0);
    bool c = GetCompl(id, 0);
    // check if it is buffering constant
    if(fi == GetConst0()) {
      RemoveConst(id);
      return;
    }
    // remove if substitution would lead to duplication with the same polarity
    ForEachFanoutRidx(id, false, [&](int fo, bool foc, int idx) {
      int idx2 = FindFanin(fo, fi);
      if(idx2 != -1 && GetCompl(fo, idx2) == (c ^ foc)) {
        RemoveFanin(fo, idx);
        if(fPropagating && GetNumFanins(fo) == 1) {
          vTrav[fo] = iTrav;
        }
      }
    });
    // substitute node with fanin or const-0
    Action action;
    action.type = REMOVE_BUFFER;
    action.id = id;
    action.fi = fi;
    action.c = c;
    ForEachFanoutRidx(id, true, [&](int fo, bool foc, int idx) {
      action.vFanouts.push_back(fo);
      int idx2 = FindFanin(fo, fi);
      if(idx2 != -1) { // substitute with const-0 in case of duplication
        assert(GetCompl(fo, idx2) != (c ^ foc)); // of a different polarity
        vRefs[GetConst0()]++;
        vvFaninEdges[fo][idx] = Node2Edge(GetConst0(), 0);
        if(fPropagating) {
          vTrav[fo] = iTrav;
        }
      } else { // otherwise, substitute with fanin
        vvFaninEdges[fo][idx] = Node2Edge(fi, c ^ foc);
        vRefs[fi]++;
      }
    });
    // remove node
    vRefs[id] = 0;
    vRefs[fi]--;
    vvFaninEdges[id].clear();
    if(!fPropagating) {
      itr it = std::find(lInts.begin(), lInts.end(), id);
      lInts.erase(it);
    }
    sInts.erase(id);
    TakenAction(action);
  }

  inline void AndNetwork::RemoveConst(int id) {
    assert(GetNumFanins(id) == 0 || FindFanin(id, GetConst0()) != -1);
    assert(!fPropagating || fLockTrav);
    bool c = (GetNumFanins(id) == 0);
    // just remove immediately if polarity is true but not PO
    ForEachFanoutRidx(id, false, [&](int fo, bool foc, int idx) {
      if(c ^ foc) {
        assert(!IsPo(fo));
        RemoveFanin(fo, idx);
        if(fPropagating && GetNumFanins(fo) <= 1) {
          vTrav[fo] = iTrav;
        }
      }
    });
    // substitute with constant
    Action action;
    action.type = REMOVE_CONST;
    action.id = id;
    ForEachFanoutRidx(id, true, [&](int fo, bool foc, int idx) {
      action.vFanouts.push_back(fo);
      vRefs[GetConst0()]++;
      vvFaninEdges[fo][idx] = Node2Edge(GetConst0(), c ^ foc);
      if(fPropagating) {
        vTrav[fo] = iTrav;
      }
    });
    // remove node
    vRefs[id] = 0;
    ForEachFanin(id, [&](int fi) {
      vRefs[fi]--;
      action.vFanins.push_back(fi);
    });
    vvFaninEdges[id].clear();
    if(!fPropagating) {
      itr it = std::find(lInts.begin(), lInts.end(), id);
      lInts.erase(it);
    }
    sInts.erase(id);
    TakenAction(action);
  }

  inline void AndNetwork::AddFanin(int id, int fi, bool c) {
    assert(FindFanin(id, fi) == -1); // no duplication
    assert(fi != GetConst0() || !c); // no const-1
    Action action;
    action.type = ADD_FANIN;
    action.id = id;
    action.idx = GetNumFanins(id);
    action.fi = fi;
    action.c = c;
    itr it = std::find(lInts.begin(), lInts.end(), id);
    itr it2 = std::find(it, lInts.end(), fi);
    if(it2 != lInts.end()) {
      lInts.erase(it2);
      it2 = lInts.insert(it, fi);
      SortInts(it2);
    }
    vRefs[fi]++;
    vvFaninEdges[id].push_back(Node2Edge(fi, c));
    TakenAction(action);
  }

  inline bool AndNetwork::TrivialCollapse(int id) {
    for(int idx = 0; idx < GetNumFanins(id);) {
      int fi_edge = vvFaninEdges[id][idx];
      int fi = Edge2Node(fi_edge);
      bool c = EdgeIsCompl(fi_edge);
      if(!IsPi(fi) && !c && vRefs[fi] == 1) {
        Action action;
        action.type = TRIVIAL_COLLAPSE;
        action.id = id;
        action.idx = idx;
        action.fi = fi;
        action.c = c;
        bool fConst0 = false;
        std::vector<int>::iterator it = vvFaninEdges[id].begin() + idx;
        it = vvFaninEdges[id].erase(it);
        ForEachFaninIdx(fi, [&](int idx2, int fi2, bool c2) {
          int idx3 = FindFanin(id, fi2);
          if(idx3 == -1) {
            // no duplication
            it = vvFaninEdges[id].insert(it, Node2Edge(fi2, c2));
            it++;
            action.vFanins.push_back(fi2);
            action.vIndices.push_back(idx2);
          } else if(c2 != GetCompl(id, idx3)) {
            // duplication with differnt polarity, add const-0
            vRefs[fi2]--;
            vRefs[GetConst0()]++;
            it = vvFaninEdges[id].insert(it, Node2Edge(GetConst0(), 0));
            it++;
            action.vFanins.push_back(GetConst0());
            action.vIndices.push_back(idx2);
            fConst0 = true;
          } else {
            // duplication with the same polarity
            vRefs[fi2]--;
            idx = 0; // need to start over
          }
        });
        // remove collapsed fanin
        vRefs[fi] = 0;
        vvFaninEdges[fi].clear();
        lInts.erase(std::find(lInts.begin(), lInts.end(), fi));
        sInts.erase(fi);
        TakenAction(action);
        if(fConst0) {
          return true;
        }
      } else {
        idx++;
      }
    }
    return false;
  }
  
  inline bool AndNetwork::TrivialCollapse() {
    bool fConst0 = false;
    std::list<int> lInts_ = lInts;
    for(critr it = lInts_.rbegin(); it != lInts_.rend(); it++) {
      if(IsInt(*it)) {
        fConst0 |= TrivialCollapse(*it);
      }
    }
    return fConst0;
  }

  inline int AndNetwork::TrivialDecompose(int id, int nFanins) {
    assert(GetNumFanins(id) > 2);
    assert(nFanins > 1);
    assert(GetNumFanins(id) > nFanins);
    Action action;
    action.type = TRIVIAL_DECOMPOSE;
    action.id = id;
    action.idx = GetNumFanins(id) - nFanins;
    int new_fi = CreateNode();
    action.fi = new_fi;
    for(int i = 0; i < nFanins; i++) {
      int fi_edge = vvFaninEdges[id].back();
      vvFaninEdges[id].pop_back();
      vvFaninEdges[new_fi].push_back(fi_edge);
      action.vFanins.push_back(Edge2Node(fi_edge));
    }
    vvFaninEdges[id].push_back(Node2Edge(new_fi, false));
    vRefs[new_fi]++;
    itr it = std::find(lInts.begin(), lInts.end(), id);
    lInts.insert(it, new_fi);
    sInts.insert(new_fi);
    TakenAction(action);
    return new_fi;
  }

  inline void AndNetwork::TrivialDecompose(int id) {
    while(GetNumFanins(id) > 2) {
      Action action;
      action.type = TRIVIAL_DECOMPOSE;
      action.id = id;
      action.idx = GetNumFanins(id) - 2;
      int new_fi = CreateNode();
      action.fi = new_fi;
      int fi_edge1 = vvFaninEdges[id].back();
      vvFaninEdges[id].pop_back();
      int fi_edge0 = vvFaninEdges[id].back();
      vvFaninEdges[id].pop_back();
      vvFaninEdges[new_fi].push_back(fi_edge0);
      action.vFanins.push_back(Edge2Node(fi_edge0));
      vvFaninEdges[new_fi].push_back(fi_edge1);
      action.vFanins.push_back(Edge2Node(fi_edge1));
      vvFaninEdges[id].push_back(Node2Edge(new_fi, false));
      vRefs[new_fi]++;
      itr it = std::find(lInts.begin(), lInts.end(), id);
      lInts.insert(it, new_fi);
      sInts.insert(new_fi);
      TakenAction(action);
    }
  }

  inline void AndNetwork::SortFanins(int id, std::vector<int> const &vIndices) {
    assert(vIndices.size() == vvFaninEdges[id].size());
    std::vector<int> vFaninEdges = vvFaninEdges[id];
    vvFaninEdges[id].clear();
    for(int idx: vIndices) {
      vvFaninEdges[id].push_back(vFaninEdges[idx]);
    }
    if(vFaninEdges == vvFaninEdges[id]) {
      return;
    }
    Action action;
    action.type = SORT_FANINS;
    action.id = id;
    action.vIndices = vIndices;
    TakenAction(action);
  }

  template <typename Func>
  inline void AndNetwork::SortFanins(int id, Func const &comp) {
    static_assert(is_invokable<Func, int, int>::value || is_invokable<Func, int, bool, int, bool>::value, "fanin cost function format error");
    std::vector<int> vFaninEdges = vvFaninEdges[id];
    std::sort(vvFaninEdges[id].begin(), vvFaninEdges[id].end(), [&](int i, int j) {
      if constexpr(is_invokable<Func, int, int>::value) {
        return comp(Edge2Node(i), Edge2Node(j));
      } else if constexpr(is_invokable<Func, int, bool, int, bool>::value) {
        return comp(Edge2Node(i), EdgeIsCompl(i), Edge2Node(j), EdgeIsCompl(j));
      }
    });
    if(vFaninEdges == vvFaninEdges[id]) {
      return;
    }
    Action action;
    action.type = SORT_FANINS;
    action.id = id;
    assert(check_int_size(vFaninEdges));
    for(int fanin_edge: vvFaninEdges[id]) {
      std::vector<int>::const_iterator it = std::find(vFaninEdges.begin(), vFaninEdges.end(), fanin_edge);
      action.vIndices.push_back(std::distance(vFaninEdges.cbegin(), it));
    }
    TakenAction(action);
  }

  inline std::pair<std::vector<int>, std::vector<bool>> AndNetwork::Insert(AndNetwork *pNtk, std::vector<int> const &vInputs, std::vector<bool> const &vCompls, std::vector<int> const &vOutputs) {
    Reserve(nNodes + pNtk->GetNumInts());
    std::map<int, std::pair<int, bool>> m;
    m[pNtk->GetConst0()] = std::make_pair(GetConst0(), false);
    assert(pNtk->GetNumPis() == int_size(vInputs));
    assert(vInputs.size() == vCompls.size());
    for(int i = 0; i < pNtk->GetNumPis(); i++) {
      assert(IsInt(vInputs[i]) || IsPi(vInputs[i]));
      m[pNtk->GetPi(i)] = std::make_pair(vInputs[i], vCompls[i]);
    }
    pNtk->ForEachInt([&](int id) {
      int id2 = CreateNode();
      lInts.push_back(id2);
      sInts.insert(id2);
      vvFaninEdges[id2].resize(pNtk->GetNumFanins(id));
      pNtk->ForEachFaninIdx(id, [&](int idx, int fi, bool c) {
        assert(m.count(fi));
        vvFaninEdges[id2][idx] = Node2Edge(m[fi].first, c ^ m[fi].second);
        vRefs[m[fi].first]++;
      });
      m[id] = std::make_pair(id2, false);
    });
    assert(pNtk->GetNumPos() == int_size(vOutputs));
    std::vector<int> vNewOutputs(pNtk->GetNumPos());
    std::vector<bool> vNewCompls(pNtk->GetNumPos());
    for(int i = 0; i < pNtk->GetNumPos(); i++) {
      int id = vOutputs[i];
      int po = pNtk->GetPo(i);
      assert(m.count(pNtk->GetFanin(po, 0)));
      int fi = m[pNtk->GetFanin(po, 0)].first;
      bool c = pNtk->GetCompl(po, 0) ^ m[pNtk->GetFanin(po, 0)].second;
      assert(id != fi);
      vNewOutputs[i] = fi;
      vNewCompls[i] = c;
      // remove if substitution would lead to duplication with the same polarity
      ForEachFanoutRidx(id, false, [&](int fo, bool foc, int idx) {
        int idx2 = FindFanin(fo, fi);
        if(idx2 != -1 && GetCompl(fo, idx2) == (c ^ foc)) {
          RemoveFanin(fo, idx);
        }
      });
      ForEachFanoutRidx(id, true, [&](int fo, bool foc, int idx) {
        int idx2 = FindFanin(fo, fi);
        if(idx2 != -1) { // substitute with const-0 in case of duplication
          assert(GetCompl(fo, idx2) != (c ^ foc)); // of a different polarity
          vRefs[GetConst0()]++;
          vvFaninEdges[fo][idx] = Node2Edge(GetConst0(), 0);
        } else { // otherwise, substitute with fanin
          vvFaninEdges[fo][idx] = Node2Edge(fi, c ^ foc);
          vRefs[fi]++;
          // sort internal nodes
          itr it = std::find(lInts.begin(), lInts.end(), id);
          itr it2 = std::find(it, lInts.end(), fi);
          if(it2 != lInts.end()) {
            lInts.erase(it2);
            it2 = lInts.insert(it, fi);
            SortInts(it2);
          }
        }
      });
      vRefs[id] = 0;
    }
    Action action;
    action.type = INSERT;
    action.vFanins = vInputs;
    action.vFanouts = vOutputs;
    TakenAction(action);
    for(int id: vOutputs) {
      RemoveUnused(id, true);
    }
    return std::make_pair(std::move(vNewOutputs), std::move(vNewCompls));
  }

  /* }}} */

  /* {{{ Network cleanup */
  
  inline void AndNetwork::Propagate(int id) {
    StartTraversal();
    itr it;
    if(id == -1) {
      ForEachInt([&](int id) {
        if(GetNumFanins(id) <= 1 || FindFanin(id, GetConst0()) != -1) {
          vTrav[id] = iTrav;
        }
      });
      it = lInts.begin();
      while(vTrav[*it] != iTrav && it != lInts.end()) {
        it++;
      }
    } else {
      vTrav[id] = iTrav;
      it = std::find(lInts.begin(), lInts.end(), id);
    }
    fPropagating = true;
    while(it != lInts.end()) {
      if(vTrav[*it] == iTrav) {
        if(GetNumFanins(*it) == 1) {
          RemoveBuffer(*it);
        } else {
          RemoveConst(*it);
        }
        it = lInts.erase(it);
      } else {
        it++;
      }
    }
    fPropagating = false;
    EndTraversal();
  }

  inline void AndNetwork::Sweep(bool fPropagate) {
    if(fPropagate) {
      Propagate();
    }
    for(ritr it = lInts.rbegin(); it != lInts.rend();) {
      if(vRefs[*it] == 0) {
        RemoveUnused(*it, false, true);
        it = ritr(lInts.erase(--it.base()));
      } else {
        it++;
      }
    }
  }
  
  /* }}} */

  /* {{{ Save & load */

  inline int AndNetwork::Save(int slot) {
    Action action;
    action.type = SAVE;
    if(slot < 0) {
      slot = int_size(vBackups);
      vBackups.emplace_back(*this);
      assert(check_int_size(vBackups));
    } else {
      assert(slot < int_size(vBackups));
      vBackups[slot].Copy(*this);
    }
    action.idx = slot;
    TakenAction(action);
    return slot;
  }

  inline void AndNetwork::Load(int slot) {
    assert(slot >= 0);
    assert(slot < int_size(vBackups));
    Action action;
    action.type = LOAD;
    action.idx = slot;
    Copy(vBackups[slot]);
    TakenAction(action);
  }

  inline void AndNetwork::PopBack() {
    assert(!vBackups.empty());
    Action action;
    action.type = POP_BACK;
    action.idx = int_size(vBackups) - 1;
    vBackups.pop_back();
    TakenAction(action);
  }

  /* }}} */
  
  /* {{{ Misc */

  inline int AndNetwork::AddCallback(Callback const &callback) {
    vCallbacks.push_back(callback);
    return int_size(vCallbacks) - 1;
  }

  inline void AndNetwork::DeleteCallback(int index) {
    vCallbacks[index] = [&](Action const &action) {
      (void)action;
    };
  }

  inline void AndNetwork::RegisterPattern(Pattern *pPat_) {
    pPat = pPat_;
  }

  inline Pattern *AndNetwork::GetPattern() {
    return pPat;
  }

  inline void AndNetwork::RegisterCond(AndNetwork *pCond_) {
    pCond = pCond_;
  }

  inline AndNetwork *AndNetwork::GetCond() {
    return pCond;
  }
  
  inline void AndNetwork::Print() const {
    std::cout << "inputs: " << vPis << std::endl;
    ForEachInt([&](int id) {
      std::cout << "node " << id << ": ";
      PrintComplementedEdges(std::bind(&AndNetwork::ForEachFanin<std::function<void(int, bool)>>, this, id, std::placeholders::_1));
      std::cout << " (ref = " << vRefs[id] << ")";
      std::cout << std::endl;
    });
    std::cout << "outputs: ";
    PrintComplementedEdges(std::bind(&AndNetwork::ForEachPoDriver<std::function<void(int, bool)>>, this, std::placeholders::_1));
    std::cout << std::endl;
  }

  /* }}} */

} // namespace boop

BOOP_HEADER_END
