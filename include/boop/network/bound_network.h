#pragma once

#include <algorithm>
#include <cassert>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>

#include "boop/config.h"
#include "boop/library/cell_library.h"
#include "boop/network/types.h"
#include "boop/util/functional.h"
#include "boop/util/size.h"

BOOP_HEADER_START

namespace boop {

class BoundNetwork {
public:
  struct Ref {
    int nId;
    int nInput;
  };

  // lifecycle
  explicit BoundNetwork(const CellLibrary *pLibrary);
  BoundNetwork(const BoundNetwork &other);

  // initialization (should not be called after optimization has started)
  void Clear(bool fClearNetwork = true, bool fClearBackups = true);
  void Reserve(int nReserve);
  int AddPi();
  int AddCell(int nCell, const std::vector<int> &vFanins);
  int AddBlackBox(const std::vector<int> &vFanins, int nOutputs);
  int AddPo(int nId);
  void SetDontTouch(int nId, bool fValue = true);
  void SetDontShare(int nId, bool fValue = true);

  // network properties
  const CellLibrary *GetLibrary() const;
  bool UseComplementedEdges() const;
  bool HasMultipleNodeTypes() const;
  int GetNumNodes() const;
  int GetNumPis() const;
  int GetNumInts() const;
  int GetNumPos() const;
  int GetNumInstances() const;
  int GetNumCells() const;
  int GetNumBlackBoxes() const;
  int GetConst0() const;
  int GetConst1() const;

  // node properties
  bool IsPi(int nId) const;
  bool IsInt(int nId) const;
  bool IsPo(int nId) const;
  NodeType GetNodeType(int nId) const;
  bool IsConst(int nId) const;
  bool IsCell(int nId) const;
  bool IsBlackBox(int nId) const;
  int GetNumFanins(int nId) const;
  int GetNumFanouts(int nId) const;
  int GetFanin(int nId, int nInput) const;
  int GetCell(int nId) const;
  int GetInstance(int nId) const;
  int GetNumOutputs(int nId) const;
  int GetOutputIndex(int nId) const;
  int GetOutput(int nId, int nOutputIndex) const;
  bool IsDontTouch(int nId) const;
  bool IsDontShare(int nId) const;

  // network traversal
  template <bool fReverse = false, typename Func>
  void ForEachPi(const Func &func) const;
  template <bool fReverse = false, typename Func>
  void ForEachPo(const Func &func) const;
  template <bool fReverse = false, typename Func>
  void ForEachInt(const Func &func) const;
  template <bool fReverse = false, typename Func>
  void ForEachInstance(const Func &func) const;
  template <bool fIdx = false, bool fReverse = false, typename Func>
  void ForEachFanin(int nId, const Func &func) const;
  template <bool fIdx = false, bool fReverse = false, typename Func>
  void ForEachFanout(int nId, const Func &func) const;
  template <bool fReverse = false, typename Func>
  void ForEachOutput(int nId, const Func &func) const;

  // actions
  void Read(const BoundNetwork &from);
  template <typename Ntk, typename Reader>
  int Read(const Ntk &from, const Reader &reader);
  void Substitute(int nOld, int nNew);
  void Substitute(int nOld, int nNew, Ref ref);
  void RemoveUnused(int nInstance); // remove instance

  // save & load
  int Save(int nSlot = -1);
  void Load(int nSlot);
  void PopBack();

private:
  // network data
  const CellLibrary *pLibrary_;
  int nNodes_; // number of allocated nodes
  std::vector<int> vPis_;
  std::vector<int> vPos_;
  std::vector<int> vInts_;        // internal nodes in topological order
  std::set<int> sInts_;           // internal nodes as a set
  std::vector<int> vNodeTypes_;
  std::vector<int> vCells_;       // library cell ids
  std::vector<int> vInstances_;   // representative node id of instance
  std::vector<int> vOutputIndices_;
  std::vector<std::vector<int>> vvFanins_;
  std::vector<std::vector<Ref>> vvFanouts_;
  std::vector<std::vector<int>> vvOtherOutputs_;
  std::vector<bool> vDontTouches_;
  std::vector<bool> vDontShares_;

  // backups
  std::vector<BoundNetwork> vBackups_;

  // helpers
  int CreateNode(NodeType type);
  int CreateBoundInstance(NodeType type, int nCell,
                          const std::vector<int> &vFanins, int nOutputs);
  void AddFanout(int nId, Ref ref);
  void DeleteFanout(int nId, Ref ref);
  void Copy(const BoundNetwork &from);
};

// lifecycle

inline BoundNetwork::BoundNetwork(const CellLibrary *pLibrary)
    : pLibrary_(pLibrary), nNodes_(0) {
  assert(pLibrary_ != nullptr);
  Clear();
}

inline BoundNetwork::BoundNetwork(const BoundNetwork &other) { Copy(other); }

// initialization

inline void BoundNetwork::Clear(bool fClearNetwork, bool fClearBackups) {
  if (fClearNetwork) {
    nNodes_ = 0;
    vPis_.clear();
    vPos_.clear();
    vInts_.clear();
    sInts_.clear();
    vNodeTypes_.clear();
    vCells_.clear();
    vInstances_.clear();
    vOutputIndices_.clear();
    vvFanins_.clear();
    vvFanouts_.clear();
    vvOtherOutputs_.clear();
    vDontTouches_.clear();
    vDontShares_.clear();
    CreateNode(CONST);
    CreateNode(CONST);
  }
  if (fClearBackups) {
    vBackups_.clear();
  }
}

inline void BoundNetwork::Reserve(int nReserve) {
  vPis_.reserve(nReserve);
  vPos_.reserve(nReserve);
  vInts_.reserve(nReserve);
  vNodeTypes_.reserve(nReserve);
  vCells_.reserve(nReserve);
  vInstances_.reserve(nReserve);
  vOutputIndices_.reserve(nReserve);
  vvFanins_.reserve(nReserve);
  vvFanouts_.reserve(nReserve);
  vvOtherOutputs_.reserve(nReserve);
  vDontTouches_.reserve(nReserve);
  vDontShares_.reserve(nReserve);
}

inline int BoundNetwork::AddPi() {
  int nId = CreateNode(PI);
  vPis_.push_back(nId);
  return nId;
}

inline int BoundNetwork::AddCell(int nCell,
                                 const std::vector<int> &vFanins) {
  assert(nCell >= 0 && nCell < pLibrary_->GetNumCells());
  assert(int_size(vFanins) == pLibrary_->GetNumInputs(nCell));
  return CreateBoundInstance(CELL, nCell, vFanins,
                             pLibrary_->GetNumOutputs(nCell));
}

inline int BoundNetwork::AddBlackBox(const std::vector<int> &vFanins,
                                     int nOutputs) {
  return CreateBoundInstance(BLACK_BOX, -1, vFanins, nOutputs);
}

inline int BoundNetwork::AddPo(int nFanin) {
  assert(nFanin >= 0 && nFanin < GetNumNodes());
  assert(!IsPo(nFanin));
  int nId = CreateNode(PO);
  vvFanins_[nId].push_back(nFanin);
  AddFanout(nFanin, Ref{nId, 0});
  vPos_.push_back(nId);
  return nId;
}

inline void BoundNetwork::SetDontTouch(int nId, bool fValue) {
  assert(nId >= 0 && nId < GetNumNodes());
  vDontTouches_[nId] = fValue;
}

inline void BoundNetwork::SetDontShare(int nId, bool fValue) {
  assert(nId >= 0 && nId < GetNumNodes());
  vDontShares_[nId] = fValue;
}

// network properties

inline const CellLibrary *BoundNetwork::GetLibrary() const { return pLibrary_; }

inline bool BoundNetwork::UseComplementedEdges() const { return false; }

inline bool BoundNetwork::HasMultipleNodeTypes() const { return true; }

inline int BoundNetwork::GetNumNodes() const { return nNodes_; }

inline int BoundNetwork::GetNumPis() const { return int_size(vPis_); }

inline int BoundNetwork::GetNumInts() const { return int_size(vInts_); }

inline int BoundNetwork::GetNumPos() const { return int_size(vPos_); }

inline int BoundNetwork::GetNumInstances() const {
  int nInstances = 0;
  for (int nId : vInts_) {
    if (vInstances_[nId] == nId) {
      ++nInstances;
    }
  }
  return nInstances;
}

inline int BoundNetwork::GetNumCells() const {
  int nCells = 0;
  for (int nId : vInts_) {
    if (IsCell(nId) && vInstances_[nId] == nId) {
      ++nCells;
    }
  }
  return nCells;
}

inline int BoundNetwork::GetNumBlackBoxes() const {
  int nBlackBoxes = 0;
  for (int nId : vInts_) {
    if (IsBlackBox(nId) && vInstances_[nId] == nId) {
      ++nBlackBoxes;
    }
  }
  return nBlackBoxes;
}

inline int BoundNetwork::GetConst0() const { return 0; }

inline int BoundNetwork::GetConst1() const { return 1; }

// node properties

inline bool BoundNetwork::IsPi(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return vNodeTypes_[nId] == PI;
}

inline bool BoundNetwork::IsInt(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return sInts_.count(nId);
}

inline bool BoundNetwork::IsPo(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return vNodeTypes_[nId] == PO;
}

inline NodeType BoundNetwork::GetNodeType(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return static_cast<NodeType>(vNodeTypes_[nId]);
}

inline bool BoundNetwork::IsConst(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return vNodeTypes_[nId] == CONST;
}

inline bool BoundNetwork::IsCell(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return vNodeTypes_[nId] == CELL;
}

inline bool BoundNetwork::IsBlackBox(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return vNodeTypes_[nId] == BLACK_BOX;
}

inline int BoundNetwork::GetNumFanins(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return int_size(vvFanins_[nId]);
}

inline int BoundNetwork::GetNumFanouts(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  assert(!IsPo(nId));
  return int_size(vvFanouts_[nId]);
}

inline int BoundNetwork::GetFanin(int nId, int nInput) const {
  assert(nId >= 0 && nId < GetNumNodes());
  assert(nInput >= 0 && nInput < GetNumFanins(nId));
  return vvFanins_[nId][nInput];
}

inline int BoundNetwork::GetCell(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  assert(IsCell(nId));
  return vCells_[nId];
}

inline int BoundNetwork::GetInstance(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  assert(IsInt(nId));
  return vInstances_[nId];
}

inline int BoundNetwork::GetNumOutputs(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  assert(vInstances_[nId] == nId);
  return int_size(vvOtherOutputs_[nId]) + 1;
}

inline int BoundNetwork::GetOutputIndex(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  assert(IsInt(nId));
  return vOutputIndices_[nId];
}

inline int BoundNetwork::GetOutput(int nId, int nOutputIndex) const {
  assert(nId >= 0 && nId < GetNumNodes());
  int nInstance = GetInstance(nId);
  if (nOutputIndex == 0) {
    return nInstance;
  }
  assert(nOutputIndex > 0 && nOutputIndex < GetNumOutputs(nInstance));
  return vvOtherOutputs_[nInstance][nOutputIndex - 1];
}

inline bool BoundNetwork::IsDontTouch(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return vDontTouches_[nId];
}

inline bool BoundNetwork::IsDontShare(int nId) const {
  assert(nId >= 0 && nId < GetNumNodes());
  return vDontShares_[nId];
}

// network traversal

template <bool fReverse, typename Func>
inline void BoundNetwork::ForEachPi(const Func &func) const {
  static_assert(std::is_invocable_v<Func, int>,
                "for each PI function format error");
  if constexpr (fReverse) {
    for (auto it = vPis_.rbegin(); it != vPis_.rend(); ++it) {
      func(*it);
    }
  } else {
    for (int nId : vPis_) {
      func(nId);
    }
  }
}

template <bool fReverse, typename Func>
inline void BoundNetwork::ForEachPo(const Func &func) const {
  static_assert(std::is_invocable_v<Func, int>,
                "for each PO function format error");
  if constexpr (fReverse) {
    for (auto it = vPos_.rbegin(); it != vPos_.rend(); ++it) {
      func(*it);
    }
  } else {
    for (int nId : vPos_) {
      func(nId);
    }
  }
}

template <bool fReverse, typename Func>
inline void BoundNetwork::ForEachInt(const Func &func) const {
  static_assert(std::is_invocable_v<Func, int>,
                "for each internal node function format error");
  if constexpr (fReverse) {
    for (auto it = vInts_.rbegin(); it != vInts_.rend(); ++it) {
      func(*it);
    }
  } else {
    for (int nId : vInts_) {
      func(nId);
    }
  }
}

template <bool fReverse, typename Func>
inline void BoundNetwork::ForEachInstance(const Func &func) const {
  static_assert(std::is_invocable_v<Func, int>,
                "for each instance function format error");
  if constexpr (fReverse) {
    for (auto it = vInts_.rbegin(); it != vInts_.rend(); ++it) {
      if (vInstances_[*it] == *it) {
        func(*it);
      }
    }
  } else {
    for (int nId : vInts_) {
      if (vInstances_[nId] == nId) {
        func(nId);
      }
    }
  }
}

template <bool fIdx, bool fReverse, typename Func>
inline void BoundNetwork::ForEachFanin(int nId, const Func &func) const {
  if constexpr (fIdx) {
    static_assert(std::is_invocable_v<Func, int, int>,
                  "for each fanin function format error");
  } else {
    static_assert(std::is_invocable_v<Func, int>,
                  "for each fanin function format error");
  }
  assert(nId >= 0 && nId < GetNumNodes());
  const std::vector<int> &vFanins = vvFanins_[nId];
  if constexpr (fReverse) {
    for (int i = int_size(vFanins) - 1; i >= 0; --i) {
      if constexpr (fIdx) {
        func(vFanins[i], i);
      } else {
        func(vFanins[i]);
      }
    }
  } else {
    for (int i = 0; i < int_size(vFanins); ++i) {
      if constexpr (fIdx) {
        func(vFanins[i], i);
      } else {
        func(vFanins[i]);
      }
    }
  }
}

template <bool fIdx, bool fReverse, typename Func>
inline void BoundNetwork::ForEachFanout(int nId, const Func &func) const {
  if constexpr (fIdx) {
    static_assert(std::is_invocable_v<Func, Ref, int>,
                  "for each fanout function format error");
  } else {
    static_assert(std::is_invocable_v<Func, Ref>,
                  "for each fanout function format error");
  }
  assert(nId >= 0 && nId < GetNumNodes());
  assert(!IsPo(nId));
  const std::vector<Ref> &vFanouts = vvFanouts_[nId];
  if constexpr (fReverse) {
    for (int i = int_size(vFanouts) - 1; i >= 0; --i) {
      if constexpr (fIdx) {
        func(vFanouts[i], i);
      } else {
        func(vFanouts[i]);
      }
    }
  } else {
    for (int i = 0; i < int_size(vFanouts); ++i) {
      if constexpr (fIdx) {
        func(vFanouts[i], i);
      } else {
        func(vFanouts[i]);
      }
    }
  }
}

template <bool fReverse, typename Func>
inline void BoundNetwork::ForEachOutput(int nId, const Func &func) const {
  static_assert(std::is_invocable_v<Func, int>,
                "for each output function format error");
  assert(nId >= 0 && nId < GetNumNodes());
  int nInstance = GetInstance(nId);
  const std::vector<int> &vOtherOutputs = vvOtherOutputs_[nInstance];
  if (vOtherOutputs.empty()) {
    func(nInstance);
    return;
  }
  if constexpr (fReverse) {
    for (auto it = vOtherOutputs.rbegin(); it != vOtherOutputs.rend(); ++it) {
      func(*it);
    }
    func(nInstance);
  } else {
    func(nInstance);
    for (int nOtherOutput : vOtherOutputs) {
      func(nOtherOutput);
    }
  }
}

// actions

inline void BoundNetwork::Read(const BoundNetwork &from) {
  Clear(true, false);
  Copy(from);
}

template <typename Ntk, typename Reader>
inline int BoundNetwork::Read(const Ntk &from, const Reader &reader) {
  int nResult = 0;
  Clear(true, false);
  if constexpr (returns_int_v<Reader, const Ntk &, BoundNetwork *>) {
    nResult = reader(from, this);
  } else {
    reader(from, this);
  }
  return nResult;
}

inline void BoundNetwork::Substitute(int nOld, int nNew) {
  assert(nOld >= 0 && nOld < GetNumNodes());
  assert(!IsPo(nOld));
  assert(nNew >= 0 && nNew < GetNumNodes());
  assert(!IsPo(nNew));
  std::vector<Ref> vFanouts = vvFanouts_[nOld];
  for (Ref ref : vFanouts) {
    Substitute(nOld, nNew, ref);
  }
}

inline void BoundNetwork::Substitute(int nOld, int nNew, Ref ref) {
  assert(nOld >= 0 && nOld < GetNumNodes());
  assert(!IsPo(nOld));
  assert(nNew >= 0 && nNew < GetNumNodes());
  assert(!IsPo(nNew));
  assert(ref.nId >= 0 && ref.nId < GetNumNodes());
  assert(ref.nInput >= 0 && ref.nInput < GetNumFanins(ref.nId));
  assert(vvFanins_[ref.nId][ref.nInput] == nOld);
  DeleteFanout(nOld, ref);
  vvFanins_[ref.nId][ref.nInput] = nNew;
  AddFanout(nNew, ref);
}

inline void BoundNetwork::RemoveUnused(int nInstance) {
  assert(nInstance >= 0 && nInstance < GetNumNodes());
  assert(!IsConst(nInstance));
  assert(!IsPi(nInstance));
  assert(!IsPo(nInstance));
  assert(nInstance == GetInstance(nInstance));
  auto remove_output = [&](int nId) {
    for (int nInput = 0; nInput < GetNumFanins(nId); ++nInput) {
      DeleteFanout(vvFanins_[nId][nInput], Ref{nId, nInput});
    }
    vvFanins_[nId].clear();
    vvOtherOutputs_[nId].clear();
    auto it = std::find(vInts_.begin(), vInts_.end(), nId);
    assert(it != vInts_.end());
    vInts_.erase(it);
    sInts_.erase(nId);
  };
  for (int nId : vvOtherOutputs_[nInstance]) {
    assert(vvFanouts_[nId].empty());
  }
  assert(vvFanouts_[nInstance].empty());
  std::vector<int> vOtherOutputs = std::move(vvOtherOutputs_[nInstance]);
  remove_output(nInstance);
  for (int nId : vOtherOutputs) {
    remove_output(nId);
  }
}

// save & load

inline int BoundNetwork::Save(int nSlot) {
  if (nSlot < 0) {
    nSlot = int_size(vBackups_);
    vBackups_.push_back(*this);
  } else {
    assert(nSlot >= 0 && nSlot < int_size(vBackups_));
    vBackups_[nSlot].Copy(*this);
  }
  vBackups_[nSlot].vBackups_.clear();
  return nSlot;
}

inline void BoundNetwork::Load(int nSlot) {
  assert(nSlot >= 0 && nSlot < int_size(vBackups_));
  Copy(vBackups_[nSlot]);
}

inline void BoundNetwork::PopBack() {
  assert(!vBackups_.empty());
  vBackups_.pop_back();
}

// helpers

inline int BoundNetwork::CreateNode(NodeType type) {
  int nId = nNodes_++;
  vNodeTypes_.push_back(type);
  vCells_.push_back(-1);
  vInstances_.push_back(nId);
  vOutputIndices_.push_back(0);
  vvFanins_.emplace_back();
  vvFanouts_.emplace_back();
  vvOtherOutputs_.emplace_back();
  vDontTouches_.push_back(false);
  vDontShares_.push_back(false);
  return nId;
}

inline int BoundNetwork::CreateBoundInstance(NodeType type, int nCell,
                                             const std::vector<int> &vFanins,
                                             int nOutputs) {
  assert(type == CELL || type == BLACK_BOX);
  assert(nOutputs > 0);
  for (int nFanin : vFanins) {
    assert(nFanin >= 0 && nFanin < GetNumNodes());
    assert(!IsPo(nFanin));
  }
  int nInstance = CreateNode(type);
  vCells_[nInstance] = nCell;
  vvFanins_[nInstance] = vFanins;
  vInts_.push_back(nInstance);
  sInts_.insert(nInstance);
  for (int nInput = 0; nInput < int_size(vFanins); ++nInput) {
    AddFanout(vFanins[nInput], Ref{nInstance, nInput});
  }
  if (nOutputs == 1) {
    return nInstance;
  }
  vvOtherOutputs_[nInstance].reserve(nOutputs - 1);
  for (int i = 1; i < nOutputs; ++i) {
    int nId = CreateNode(type);
    vCells_[nId] = nCell;
    vInstances_[nId] = nInstance;
    vOutputIndices_[nId] = i;
    vvFanins_[nId] = vFanins;
    vvOtherOutputs_[nInstance].push_back(nId);
    vInts_.push_back(nId);
    sInts_.insert(nId);
    for (int nInput = 0; nInput < int_size(vFanins); ++nInput) {
      AddFanout(vFanins[nInput], Ref{nId, nInput});
    }
  }
  return nInstance;
}

inline void BoundNetwork::AddFanout(int nId, Ref ref) {
  assert(nId >= 0 && nId < GetNumNodes());
  assert(!IsPo(nId));
  assert(ref.nId >= 0 && ref.nId < GetNumNodes());
  vvFanouts_[nId].push_back(ref);
}

inline void BoundNetwork::DeleteFanout(int nId, Ref ref) {
  assert(nId >= 0 && nId < GetNumNodes());
  assert(!IsPo(nId));
  auto &vFanouts = vvFanouts_[nId];
  auto it = std::find_if(vFanouts.begin(), vFanouts.end(), [&](Ref fanout) {
    return fanout.nId == ref.nId && fanout.nInput == ref.nInput;
  });
  assert(it != vFanouts.end());
  vFanouts.erase(it);
}

inline void BoundNetwork::Copy(const BoundNetwork &from) {
  pLibrary_ = from.pLibrary_;
  nNodes_ = from.nNodes_;
  vPis_ = from.vPis_;
  vPos_ = from.vPos_;
  vInts_ = from.vInts_;
  sInts_ = from.sInts_;
  vNodeTypes_ = from.vNodeTypes_;
  vCells_ = from.vCells_;
  vInstances_ = from.vInstances_;
  vOutputIndices_ = from.vOutputIndices_;
  vvFanins_ = from.vvFanins_;
  vvFanouts_ = from.vvFanouts_;
  vvOtherOutputs_ = from.vvOtherOutputs_;
  vDontTouches_ = from.vDontTouches_;
  vDontShares_ = from.vDontShares_;
}

} // namespace boop

BOOP_HEADER_END
