#pragma once

#include <algorithm>
#include <random>
#include <bitset>
#include <climits>

#include "boop/util/util.h"
#include "boop/network/types.h"
#include "boop/rrr/types.h"
#include "boop/rrr/simulator/vec_ops.h"

BOOP_HEADER_START

namespace boop::rrr {

  template <typename Ntk>
  class Simulator {
  public:
    struct Parameter {
      int nVerbose = 0;
      int nWords = 16;
      bool fSave = true;
      bool fKeepStimuli = true;
    };

    // lifecycle
    Simulator();
    Simulator(const Parameter &par);
    void AssignNetwork(Ntk *pNtk, bool fReuse);
    void SetPrintLine(std::function<void(const std::string &)> fnPrintLine);

    // checks
    bool IsExhaustive() const;
    bool CheckRedundancy(int nId, int nIdx);
    bool CheckFeasibility(int nId, int nFi, bool fCompl);

    // cex
    void AddCex(const std::vector<VarValue> &vCex);

    // summary
    void ResetSummary();
    Summary<int> GetStatsSummary() const;
    Summary<Duration> GetTimesSummary() const;

  private:
    // aliases
    using Word = unsigned long long;
    static constexpr int WordWidth = CHAR_BIT * sizeof(Word);
    using itr = std::vector<Word>::iterator;
    using citr = std::vector<Word>::const_iterator;
    static constexpr Word one = 0xffffffffffffffffull;
    static constexpr Word vars[] = {0xaaaaaaaaaaaaaaaaull,
                                    0xccccccccccccccccull,
                                    0xf0f0f0f0f0f0f0f0ull,
                                    0xff00ff00ff00ff00ull,
                                    0xffff0000ffff0000ull,
                                    0xffffffff00000000ull};
    
    Ntk *pNtk_;
    const Parameter par_;
    int nWords_;

    bool fGenerated_;
    bool fInitialized_;
    bool fExhaustive_;
    int nTarget_;
    std::vector<Word> vValues_;
    std::vector<Word> vValuesInv_;
    std::vector<Word> vCare_;
    std::vector<Word> vTmp_;

    unsigned nTrav_;
    std::vector<unsigned> vTrav_;
    
    int nPivot_;
    std::vector<Word> vAssignedStimuli_;

    bool fUpdate_;
    std::set<int> sUpdates_;
    
    std::vector<Simulator> vBackups_;

    int nCex_;
    int nDiscarded_;
    int nPackedCountOld_;
    std::vector<int> vPackedCount_;
    std::vector<int> vPackedCountEvicted_;
    Duration durationSimulation_;
    Duration durationCare_;

    std::function<void(const std::string &)> fnPrintLine_;

    // print
    template <typename... Args>
    void Print(int nVerboseLevel, Args &&...args);
    void PrintBits(int nVerboseLevel, int nWords, std::vector<Word>::const_iterator it);

    // callback
    void ActionCallback(const Action &action);

    // topology
    unsigned StartTraversal(int n = 1);

    // simulation
    void SimulateNode(std::vector<Word> &v, int nId);
    bool ResimulateNode(std::vector<Word> &v, int nId);
    void SimulateOneWordNode(std::vector<Word> &v, int nId, int nOffset);
    void Simulate();
    void Resimulate();
    void SimulateOneWord(int nOffset);

    // generate stimuli
    void GenerateRandomStimuli();
    void GenerateExhaustiveStimuli();

    // compute care
    void ComputeCare(int nId);

    // preparation
    void Initialize();

    // save & load
    void Save(int nSlot);
    void Load(int nSlot);
    void PopBack();
  };

  // lifecycle

  template <typename Ntk>
  Simulator<Ntk>::Simulator()
    : pNtk_(nullptr),
      par_({0, 0, false, true}),
      nWords_(0),
      fGenerated_(false),
      fInitialized_(false),
      fExhaustive_(false),
      nTarget_(-1),
      nTrav_(0),
      nPivot_(0),
      fUpdate_(false) {
    ResetSummary();
  }
  
 template <typename Ntk>
  Simulator<Ntk>::Simulator(const Parameter &par)
    : pNtk_(nullptr),
      par_(par),
      nWords_(par.nWords),
      fGenerated_(false),
      fInitialized_(false),
      fExhaustive_(false),
      nTarget_(-1),
      nTrav_(0),
      nPivot_(0),
      fUpdate_(false) {
    ResetSummary();
  }

  template <typename Ntk>
  void Simulator<Ntk>::AssignNetwork(Ntk *pNtk, bool fReuse) {
    if(!fReuse) {
      fGenerated_ = false;
    }
    fInitialized_ = false;
    nTarget_ = -1;
    fUpdate_ = false;
    sUpdates_.clear();
    pNtk_ = pNtk;
    pNtk_->AddCallback([this](const Action &action) {
      ActionCallback(action);
    });
  }

  template <typename Ntk>
  void Simulator<Ntk>::SetPrintLine(std::function<void(const std::string &)> fnPrintLine) {
    fnPrintLine_ = std::move(fnPrintLine);
  }

  // checks

  template <typename Ntk>
  bool Simulator<Ntk>::IsExhaustive() const {
    return fExhaustive_;
  }
  
  template <typename Ntk>
  bool Simulator<Ntk>::CheckRedundancy(int nId, int nIdx) {
    if(!fInitialized_) {
      Initialize();
    }
    ComputeCare(nId);
    switch(pNtk_->GetNodeType(nId)) {
    case AND: {
      auto it = vCare_.begin();
      pNtk_->template ForEachFanin<true, true, false>(nId, [&](int nIdx2, int nFi, bool fCompl) {
        if(nIdx == nIdx2) {
          return;
        }
        vec_ops::And(nWords_, vTmp_.begin(), it, vValues_.begin() + nFi * nWords_, false, fCompl);
        it = vTmp_.begin();
      });
      int nFi = pNtk_->GetFanin(nId, nIdx);
      bool fCompl = pNtk_->GetCompl(nId, nIdx);
      vec_ops::And(nWords_, vTmp_.begin(), it, vValues_.begin() + nFi * nWords_, false, !fCompl);
      return vec_ops::IsZero(nWords_, vTmp_.begin(), false);
    }
    default:
      assert(0);
    }
    return false;
  }

  template <typename Ntk>
  bool Simulator<Ntk>::CheckFeasibility(int nId, int nFi, bool fCompl) {
    if(!fInitialized_) {
      Initialize();
    }
    ComputeCare(nId);
    switch(pNtk_->GetNodeType(nId)) {
    case AND: {
      auto it = vCare_.begin();
      pNtk_->ForEachFanin(nId, [&](int nFi2, bool fCompl2) {
        vec_ops::And(nWords_, vTmp_.begin(), it, vValues_.begin() + nFi2 * nWords_, false, fCompl2);
        it = vTmp_.begin();
      });
      vec_ops::And(nWords_, vTmp_.begin(), it, vValues_.begin() + nFi * nWords_, false, !fCompl);
      return vec_ops::IsZero(nWords_, vTmp_.begin(), false);
    }
    default:
      assert(0);
    }
    return false;
  }
  
  // cex

  template <typename Ntk>
  void Simulator<Ntk>::AddCex(const std::vector<VarValue> &vCex) {
    if(par_.nVerbose) {
      std::stringstream ss;
      for(VarValue c : vCex) {
        ss << GetVarValueChar(c);
      }
      Print(0, "cex:", ss.str());
    }
    // record care PI indices
    assert(int_size(vCex) == pNtk_->GetNumPis());
    std::vector<int> vCarePiIdxs;
    for(int nIdx = 0; nIdx < pNtk_->GetNumPis(); nIdx++) {
      if(vCex[nIdx] == rrrTRUE || vCex[nIdx] == rrrFALSE) {
        vCarePiIdxs.push_back(nIdx);
      }
    }
    assert(!vCarePiIdxs.empty());
    // find compatible word
    int nWord = 0;
    std::vector<Word> vCompatibleBits(1);
    auto it = vCompatibleBits.begin();
    for(; nWord < nWords_; nWord++) {
      vec_ops::Fill(1, it);
      for(int nIdx : vCarePiIdxs) {
        int nId = pNtk_->GetPi(nIdx);
        bool fCompl = (vCex[nIdx] == rrrFALSE);
        auto itX = vValues_.begin() + nId * nWords_ + nWord;
        auto itY = vAssignedStimuli_.begin() + nIdx * nWords_ + nWord;
        vec_ops::And(1, vTmp_.begin(), itX, itY, !fCompl, false);
        vec_ops::And(1, it, it, vTmp_.begin(), false, true);
        if(vec_ops::IsZero(1, it, false)) {
          break;
        }
      }
      if(!vec_ops::IsZero(1, it, false)) {
        break;
      }
    }
    // find compatible bit
    int nBit;
    if(nWord < nWords_) {
      assert(!vec_ops::IsZero(1, it, false));
      nBit = 0;
      while(!((*it >> nBit) & 1)) {
        nBit++;
      }
      Print(0, "fusing into stimulus word", nWord, "bit", nBit);
      vPackedCount_[nWord * WordWidth + nBit]++;
    } else {
      // no bits are compatible, so reset at pivot
      nWord = nPivot_ / WordWidth;
      nBit = nPivot_ % WordWidth;
      Print(0, "resetting stimulus word", nWord, "bit", nBit);
      if(vPackedCount_[nWord * WordWidth + nBit]) {
        // this can be zero only when stats has been reset
        vPackedCountEvicted_.push_back(vPackedCount_[nWord * WordWidth + nBit]);
      }
      vPackedCount_[nWord * WordWidth + nBit] = 1;
      Word mask = Word{1} << nBit;
      for(int nIdx = 0; nIdx < pNtk_->GetNumPis(); nIdx++) {
        vAssignedStimuli_[nIdx * nWords_ + nWord] &= ~mask;
      }
      nPivot_++;
      if(nPivot_ == WordWidth * nWords_) {
        nPivot_ = 0;
      }
    }
    // update stimulus
    for(int nIdx : vCarePiIdxs) {
      int nId = pNtk_->GetPi(nIdx);
      Word mask = Word{1} << nBit;
      if(vCex[nIdx] == rrrTRUE) {
        vValues_[nId * nWords_ + nWord] |= mask;
      } else {
        assert(vCex[nIdx] == rrrFALSE);
        vValues_[nId * nWords_ + nWord] &= ~mask;
      }
      vAssignedStimuli_[nIdx * nWords_ + nWord] |= mask;
      Print(1, "node", nId);
      PrintBits(2, 1, vValues_.begin() + nId * nWords_ + nWord);
      Print(1, "asgn", nId);
      PrintBits(2, 1, vAssignedStimuli_.begin() + nIdx * nWords_ + nWord);
    }
    // simulate
    SimulateOneWord(nWord);
    // recompute care with new stimulus
    if(nTarget_ != -1 && !pNtk_->IsPoDriver(nTarget_)) {
      TimePoint timeStart = GetCurrentTime();
      Print(0, "recomputing careset of", nTarget_);
      vValuesInv_.resize(nWords_ * pNtk_->GetNumNodes());
      StartTraversal();
      vec_ops::Copy(1, vValuesInv_.begin() + nTarget_ * nWords_ + nWord, vValues_.begin() + nTarget_ * nWords_ + nWord, true);
      vTrav_[nTarget_] = nTrav_;
      pNtk_->template ForEachTfo<false, true, true, false>(nTarget_, [&](int nId) {
        auto itX = vValuesInv_.end();
        auto itY = vValuesInv_.begin() + nId * nWords_ + nWord;
        bool fComplX = false;
        switch(pNtk_->GetNodeType(nId)) {
        case AND:
          pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
            if(itX == vValuesInv_.end()) {
              if(vTrav_[nFi] != nTrav_) {
                itX = vValues_.begin() + nFi * nWords_ + nWord;
              } else {
                itX = vValuesInv_.begin() + nFi * nWords_ + nWord;
              }
              fComplX = fCompl;
            } else {
              if(vTrav_[nFi] != nTrav_) {
                vec_ops::And(1, itY, itX, vValues_.begin() + nFi * nWords_ + nWord, fComplX, fCompl);
              } else {
                vec_ops::And(1, itY, itX, vValuesInv_.begin() + nFi * nWords_ + nWord, fComplX, fCompl);
              }
              itX = itY;
              fComplX = false;
            }
          });
          if(itX == vValuesInv_.end()) {
            vec_ops::Fill(1, itY);
          } else if(itX != itY) {
            vec_ops::Copy(1, itY, itX, fComplX);
          }
          break;
        default:
          assert(0);
        }
        vTrav_[nId] = nTrav_;
        Print(1, "node", nId);
        PrintBits(2, 1, vValuesInv_.begin() + nId * nWords_ + nWord);
      });
      vec_ops::Clear(1, vCare_.begin() + nWord);
      pNtk_->ForEachPoDriver([&](int nFi) {
        assert(nFi != nTarget_);
        if(vTrav_[nFi] == nTrav_) { // skip unaffected POs
          vCare_[nWord] |= (vValues_[nFi * nWords_ + nWord] ^ vValuesInv_[nFi * nWords_ + nWord]);
        }
      });
      Print(1, "care", nTarget_);
      PrintBits(2, 1, vCare_.begin() + nWord);
      durationCare_ += GetDuration(timeStart, GetCurrentTime());
    }    
    nCex_++;
  }

  // summary
  
  template <typename Ntk>
  void Simulator<Ntk>::ResetSummary() {
    nCex_ = 0;
    nDiscarded_ = 0;
    nPackedCountOld_ = 0;
    for(int nCount : vPackedCount_) {
      if(nCount) {
        nPackedCountOld_++;
      }
    }
    vPackedCountEvicted_.clear();
    durationSimulation_ = 0;
    durationCare_ = 0;
  }
  
  template <typename Ntk>
  Summary<int> Simulator<Ntk>::GetStatsSummary() const {
    Summary<int> summary;
    summary.emplace_back("sim cex", nCex_);
    if(!par_.fKeepStimuli) {
      summary.emplace_back("sim discarded cex", nDiscarded_);
    }
    int nPackedCount = int_size(vPackedCountEvicted_) - nPackedCountOld_;
    for(int nCount : vPackedCount_) {
      if(nCount) {
        nPackedCount++;
      }
    }
    summary.emplace_back("sim packed pattern", nPackedCount);
    summary.emplace_back("sim evicted pattern", int_size(vPackedCountEvicted_));
    return summary;
  }

  template <typename Ntk>
  Summary<Duration> Simulator<Ntk>::GetTimesSummary() const {
    Summary<Duration> summary;
    summary.emplace_back("sim simulation", durationSimulation_);
    summary.emplace_back("sim care computation", durationCare_);
    return summary;
  }
  
  // print

  template <typename Ntk>
  template <typename... Args>
  void Simulator<Ntk>::Print(int nVerboseLevel, Args &&...args) {
    if(fnPrintLine_ && par_.nVerbose > nVerboseLevel) {
      std::stringstream ss;
      for(int i = 0; i < nVerboseLevel; i++) {
        ss << "\t";
      }
      PrintNext(ss, std::forward<Args>(args)...);
      fnPrintLine_(ss.str());
    }
  }

  template <typename Ntk>
  void Simulator<Ntk>::PrintBits(int nVerboseLevel, int nWords, std::vector<Word>::const_iterator it) {
    if(par_.nVerbose > nVerboseLevel) {
      std::stringstream ss = vec_ops::GetStringStream(nWords, it);
      std::string line;
      while(std::getline(ss, line)) {
        Print(nVerboseLevel, line);
      }
    }
  }
  
  // callback
  
  template <typename Ntk>
  void Simulator<Ntk>::ActionCallback(const Action &action) {
    switch(action.type) {
    case REMOVE_FANIN:
      assert(fInitialized_);
      if(action.id == nTarget_) {
        fUpdate_ = true;
      } else {
        sUpdates_.insert(action.id);
      }
      break;
    case REMOVE_UNUSED:
      break;
    case REMOVE_BUFFER:
    case REMOVE_CONST:
      if(fInitialized_) {
        if(action.id == nTarget_) {
          if(fUpdate_) {
            for(int nFo : action.vFanouts) {
              sUpdates_.insert(nFo);
            }
            fUpdate_ = false;
          }
          nTarget_ = -1;
        } else {
          if(sUpdates_.count(action.id)) {
            sUpdates_.erase(action.id);
            for(int nFo : action.vFanouts) {
              sUpdates_.insert(nFo);
            }
          }
        }
      }
      break;
    case ADD_FANIN:
      assert(fInitialized_);
      if(action.id == nTarget_) {
        fUpdate_ = true;
      } else {
        sUpdates_.insert(action.id);
      }
      break;
    case TRIVIAL_COLLAPSE:
      break;
    case TRIVIAL_DECOMPOSE:
      if(fInitialized_) {
        vValues_.resize(nWords_ * pNtk_->GetNumNodes());
        SimulateNode(vValues_, action.fi);
        // time of this simulation is not measured for simplicity
      }
      break;
    case SORT_FANINS:
      break;
    case READ:
      fInitialized_ = false;
      break;
    case SAVE:
      if(par_.fSave) {
        Save(action.idx);
      }
      break;
    case LOAD:
      if(par_.fSave) {
        Load(action.idx);
      } else {
        fInitialized_ = false;
      }
      break;
    case POP_BACK:
      if(par_.fSave) {
        PopBack();
      }
      break;
    default:
      assert(0);
    }
  }
  
  // topology
  
  template <typename Ntk>
  unsigned Simulator<Ntk>::StartTraversal(int n) {
    do {
      for(int i = 0; i < n; i++) {
        nTrav_++;
        if(nTrav_ == 0) {
          vTrav_.clear();
          break;
        }
      }
    } while(nTrav_ == 0);
    vTrav_.resize(pNtk_->GetNumNodes());
    return nTrav_ - n + 1;
  }

  // simulation
  
  template <typename Ntk>
  void Simulator<Ntk>::SimulateNode(std::vector<Word> &v, int nId) {
    auto itX = v.end();
    auto itY = v.begin() + nId * nWords_;
    bool fComplX = false;
    switch(pNtk_->GetNodeType(nId)) {
    case AND:
      pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
        if(itX == v.end()) {
          itX = v.begin() + nFi * nWords_;
          fComplX = fCompl;
        } else {
          vec_ops::And(nWords_, itY, itX, v.begin() + nFi * nWords_, fComplX, fCompl);
          itX = itY;
          fComplX = false;
        }
      });
      if(itX == v.end()) {
        vec_ops::Fill(nWords_, itY);
      } else if(itX != itY) {
        vec_ops::Copy(nWords_, itY, itX, fComplX);
      }
      break;
    default:
      assert(0);
    }
  }

  template <typename Ntk>
  bool Simulator<Ntk>::ResimulateNode(std::vector<Word> &v, int nId) {
    auto itX = v.end();
    bool fComplX = false;
    switch(pNtk_->GetNodeType(nId)) {
    case AND:
      pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
        if(itX == v.end()) {
          itX = v.begin() + nFi * nWords_;
          fComplX = fCompl;
        } else {
          vec_ops::And(nWords_, vTmp_.begin(), itX, v.begin() + nFi * nWords_, fComplX, fCompl);
          itX = vTmp_.begin();
          fComplX = false;
        }
      });
      if(itX == v.end()) {
        vec_ops::Fill(nWords_, vTmp_.begin());
        itX = vTmp_.begin();
        fComplX = false;
      }
      break;
    default:
      assert(0);
    }
    auto itY = v.begin() + nId * nWords_;
    if(vec_ops::IsEq(nWords_, itY, itX, fComplX)) {
      return false;
    }
    vec_ops::Copy(nWords_, itY, itX, fComplX);
    return true;
  }

  template <typename Ntk>
  void Simulator<Ntk>::SimulateOneWordNode(std::vector<Word> &v, int nId, int nOffset) {
    auto itX = v.end();
    auto itY = v.begin() + nId * nWords_ + nOffset;
    bool fComplX = false;
    switch(pNtk_->GetNodeType(nId)) {
    case AND:
      pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
        if(itX == v.end()) {
          itX = v.begin() + nFi * nWords_ + nOffset;
          fComplX = fCompl;
        } else {
          vec_ops::And(1, itY, itX, v.begin() + nFi * nWords_ + nOffset, fComplX, fCompl);
          itX = itY;
          fComplX = false;
        }
      });
      if(itX == v.end()) {
        vec_ops::Fill(1, itY);
      } else if(itX != itY) {
        vec_ops::Copy(1, itY, itX, fComplX);
      }
      break;
    default:
      assert(0);
    }
  }

  template <typename Ntk>
  void Simulator<Ntk>::Simulate() {
    TimePoint timeStart = GetCurrentTime();
    Print(0, "simulating");
    pNtk_->ForEachInt([&](int nId) {
      SimulateNode(vValues_, nId);
      Print(1, "simulating", "node", nId);
      PrintBits(2, nWords_, vValues_.begin() + nId * nWords_);
    });
    durationSimulation_ += GetDuration(timeStart, GetCurrentTime());
  }

  template <typename Ntk>
  void Simulator<Ntk>::Resimulate() {
    TimePoint timeStart = GetCurrentTime();
    Print(0, "resimulating");
    pNtk_->template ForEachTfos<false, false, true, false>(sUpdates_, [&](int nId) {
      bool fUpdated = ResimulateNode(vValues_, nId);
      Print(1, "resimulating", "node", nId);
      PrintBits(2, nWords_, vValues_.begin() + nId * nWords_);
      return !fUpdated;
    });
    /* alternative version that updates entire TFO
    pNtk_->template ForEachTfos<false, true, true, false>(sUpdates_, [&](int nId) {
      SimulateNode(vValues_, nId);
      Print(1, "resimulating", "node", nId);
      PrintBits(2, nWords_, vValues_.begin() + nId * nWords_);
      }
    });
    */
    durationSimulation_ += GetDuration(timeStart, GetCurrentTime());
  }

  template <typename Ntk>
  void Simulator<Ntk>::SimulateOneWord(int nOffset) {
    TimePoint timeStart = GetCurrentTime();
    Print(0, "simulating word", nOffset);
    pNtk_->ForEachInt([&](int nId) {
      SimulateOneWordNode(vValues_, nId, nOffset);
      Print(1, "simulating word", nOffset, "node", nId);
      PrintBits(2, 1, vValues_.begin() + nId * nWords_ + nOffset);
    });
    durationSimulation_ += GetDuration(timeStart, GetCurrentTime());
  }

  // generate stimuli

  template <typename Ntk>
  void Simulator<Ntk>::GenerateRandomStimuli() {
    Print(0, "generating random stimuli");
    vValues_.resize(nWords_ * pNtk_->GetNumNodes());
    std::mt19937_64 rng;
    pNtk_->ForEachPi([&](int nId) {
      for(int i = 0; i < nWords_; i++) {
        vValues_[nId * nWords_ + i] = rng();
      }
      Print(1, "node", nId);
      PrintBits(2, nWords_, vValues_.begin() + nId * nWords_);
    });
    fExhaustive_ = false;
  }

  template <typename Ntk>
  void Simulator<Ntk>::GenerateExhaustiveStimuli() {
    Print(0, "generating exhaustive stimuli");
    if(pNtk_->GetNumPis() <= 6) {
      nWords_ = 1;
    } else {
      nWords_ = 1 << (pNtk_->GetNumPis() - 6);
    }
    assert(nWords_ <= par_.nWords);
    vValues_.resize(nWords_ * pNtk_->GetNumNodes());
    pNtk_->ForEachPi([&](int nIdx, int nId) {
      auto it = vValues_.begin() + nId * nWords_;
      if(nIdx < 6) {
        for(int i = 0; i < nWords_; i++, ++it) {
          *it = vars[nIdx];
        }
      } else {
        const int nBlock = 1 << (nIdx - 6);
        for(int i = 0; i < nWords_;) {
          for(int j = 0; j < nBlock; i++, j++, ++it) {
            *it = 0;
          }
          for(int j = 0; j < nBlock; i++, j++, ++it) {
            *it = one;
          }
        }
      }
      Print(1, "node", nId);
      PrintBits(2, nWords_, vValues_.begin() + nId * nWords_);
    });
    fExhaustive_ = true;
  }

  // compute care
  
  template <typename Ntk>
  void Simulator<Ntk>::ComputeCare(int nId) {
    if(sUpdates_.empty() && nId == nTarget_) {
      return;
    }
    if(fUpdate_) {
      sUpdates_.insert(nTarget_);
      fUpdate_ = false;
    }
    if(!sUpdates_.empty()) {
      Resimulate();
      sUpdates_.clear();
    }
    nTarget_ = nId;
    TimePoint timeStart = GetCurrentTime();
    Print(0, "computing careset of", nTarget_);
    if(pNtk_->IsPoDriver(nTarget_)) {
      vec_ops::Fill(nWords_, vCare_.begin());
      Print(1, "care", nTarget_);
      PrintBits(2, nWords_, vCare_.begin());
      durationCare_ += GetDuration(timeStart, GetCurrentTime());
      return;
    }
    // TFO computation
    vValuesInv_.resize(nWords_ * pNtk_->GetNumNodes());
    StartTraversal();
    vec_ops::Copy(nWords_, vValuesInv_.begin() + nTarget_ * nWords_, vValues_.begin() + nTarget_ * nWords_, true);
    vTrav_[nTarget_] = nTrav_;
    pNtk_->template ForEachTfo<false, true, true, false>(nTarget_, [&](int nId) {
      auto itX = vValuesInv_.end();
      auto itY = vValuesInv_.begin() + nId * nWords_;
      bool fComplX = false;
      switch(pNtk_->GetNodeType(nId)) {
      case AND:
        pNtk_->ForEachFanin(nId, [&](int nFi, bool fCompl) {
          if(itX == vValuesInv_.end()) {
            if(vTrav_[nFi] != nTrav_) {
              itX = vValues_.begin() + nFi * nWords_;
            } else {
              itX = vValuesInv_.begin() + nFi * nWords_;
            }
            fComplX = fCompl;
          } else {
            if(vTrav_[nFi] != nTrav_) {
              vec_ops::And(nWords_, itY, itX, vValues_.begin() + nFi * nWords_, fComplX, fCompl);
            } else {
              vec_ops::And(nWords_, itY, itX, vValuesInv_.begin() + nFi * nWords_, fComplX, fCompl);
            }
            itX = itY;
            fComplX = false;
          }
        });
        if(itX == vValuesInv_.end()) {
          vec_ops::Fill(nWords_, itY);
        } else if(itX != itY) {
          vec_ops::Copy(nWords_, itY, itX, fComplX);
        }
        break;
      default:
        assert(0);
      }
      vTrav_[nId] = nTrav_;
      Print(1, "node", nId);
      PrintBits(2, nWords_, vValuesInv_.begin() + nId * nWords_);
    });
    // compute care
    vec_ops::Clear(nWords_, vCare_.begin());
    pNtk_->ForEachPoDriver([&](int nFi) {
      assert(nFi != nTarget_);
      if(vTrav_[nFi] == nTrav_) { // skip unaffected POs
        for(int i = 0; i < nWords_; i++) {
          vCare_[i] |= (vValues_[nFi * nWords_ + i] ^ vValuesInv_[nFi * nWords_ + i]);
        }
      }
    });
    Print(1, "care", nTarget_);
    PrintBits(2, nWords_, vCare_.begin());
    durationCare_ += GetDuration(timeStart, GetCurrentTime());
  }

  // preparation

  template <typename Ntk>
  void Simulator<Ntk>::Initialize() {
    if(!fGenerated_) {
      nWords_ = par_.nWords;
      if(nWords_ == 0 ||
         pNtk_->GetNumPis() > 36 ||
         (pNtk_->GetNumPis() > 6 && (1 << (pNtk_->GetNumPis() - 6)) > nWords_)) {
        GenerateRandomStimuli();
      } else {
        GenerateExhaustiveStimuli();
      }
      vCare_.resize(nWords_);
      vTmp_.resize(nWords_);
      nPivot_ = 0;
      vAssignedStimuli_.clear();
      vAssignedStimuli_.resize(nWords_ * pNtk_->GetNumPis());
      for(int nCount : vPackedCount_) {
        if(nCount) {
          vPackedCountEvicted_.push_back(nCount);
        }
      }
      vPackedCount_.clear();
      vPackedCount_.resize(nWords_ * WordWidth);
      fGenerated_ = true;
    } else {
      // use same nWords_ as we are reusing patterns even if par_.nWords changed
      vValues_.resize(nWords_ * pNtk_->GetNumNodes());
    }
    nTarget_ = -1;
    fUpdate_ = false;
    sUpdates_.clear();
    Simulate();
    fInitialized_ = true;
  }
  
  // save & load

 template <typename Ntk>
  void Simulator<Ntk>::Save(int nSlot) {
    assert(nSlot >= 0);
    assert(!check_int_max(nSlot)); // TODO: revisit this after network save behavior is decided
    if(nSlot >= int_size(vBackups_)) {
      vBackups_.resize(nSlot + 1);
    }
    vBackups_[nSlot].nWords_ = nWords_;
    vBackups_[nSlot].fGenerated_ = fGenerated_;
    vBackups_[nSlot].fInitialized_ = fInitialized_;
    vBackups_[nSlot].fExhaustive_ = fExhaustive_;
    if(fInitialized_) {
      // resimulate before saving values to avoid resimulation after each load
      if(sUpdates_.empty()) {
        vBackups_[nSlot].nTarget_ = nTarget_;
        vBackups_[nSlot].vCare_ = vCare_;
      } else {
        vBackups_[nSlot].nTarget_ = -1;
        vBackups_[nSlot].vCare_ = vCare_;
      }
      if(fUpdate_) {
        sUpdates_.insert(nTarget_);
        fUpdate_ = false;
      }
      if(!sUpdates_.empty()) {
        Resimulate();
        sUpdates_.clear();
      }
      // assigned to -1 if we resimulate but care set still needs updating
      nTarget_ = vBackups_[nSlot].nTarget_;
    }
    if(fGenerated_) {
      vBackups_[nSlot].vValues_ = vValues_;
      vBackups_[nSlot].nPivot_ = nPivot_;
      vBackups_[nSlot].vAssignedStimuli_ = vAssignedStimuli_;
      if(!par_.fKeepStimuli) {
        vBackups_[nSlot].nCex_ = nCex_;
        vBackups_[nSlot].nPackedCountOld_ = nPackedCountOld_;
        vBackups_[nSlot].vPackedCount_ = vPackedCount_;
        vBackups_[nSlot].vPackedCountEvicted_ = vPackedCountEvicted_;
      }
    }
  }

  template <typename Ntk>
  void Simulator<Ntk>::Load(int nSlot) {
    assert(nSlot >= 0);
    assert(nSlot < int_size(vBackups_));
    fUpdate_ = false;
    sUpdates_.clear();
    fInitialized_ = vBackups_[nSlot].fInitialized_;
    if(fInitialized_) {
      nTarget_ = vBackups_[nSlot].nTarget_;
      vCare_ = vBackups_[nSlot].vCare_;
    } else {
      nTarget_ = -1;
    }
    fGenerated_ = vBackups_[nSlot].fGenerated_;
    if(!fGenerated_) {
      return;
    }
    if(!par_.fKeepStimuli) {
      nWords_ = vBackups_[nSlot].nWords_;
      fExhaustive_ = vBackups_[nSlot].fExhaustive_;
      vValues_ = vBackups_[nSlot].vValues_;
      nPivot_ = vBackups_[nSlot].nPivot_;
      vAssignedStimuli_ = vBackups_[nSlot].vAssignedStimuli_;
      nDiscarded_ += nCex_ - vBackups_[nSlot].nCex_;
      nCex_ = vBackups_[nSlot].nCex_;
      nPackedCountOld_ = vBackups_[nSlot].nPackedCountOld_;
      vPackedCount_ = vBackups_[nSlot].vPackedCount_;
      vPackedCountEvicted_ = vBackups_[nSlot].vPackedCountEvicted_;
      vTmp_.resize(nWords_);
      return;
    }
    assert(nWords_ == vBackups_[nSlot].nWords_);
    std::vector<int> vOffsets;
    for(int i = 0; i < nWords_; i++) {
      bool fDifferent = false;
      pNtk_->ForEachPi([&](int nId) { // TODO: break maybe
        if(vBackups_[nSlot].vValues_[nId * vBackups_[nSlot].nWords_ + i] != vValues_[nId * nWords_ + i]) {
          fDifferent = true;
        }
      });
      if(fDifferent) {
        vOffsets.push_back(i);
      }
    }
    if(vOffsets.empty()) {
      nTarget_ = vBackups_[nSlot].nTarget_;
      vValues_ = vBackups_[nSlot].vValues_;
    } else {
      nTarget_ = -1;
      std::vector<std::vector<Word>> vInputStimuli(pNtk_->GetNumPis());
      pNtk_->ForEachPi([&](int nIdx, int nId) {
        vInputStimuli[nIdx].resize(nWords_);
        vec_ops::Copy(nWords_, vInputStimuli[nIdx].begin(), vValues_.begin() + nId * nWords_, false);
      });
      vValues_ = vBackups_[nSlot].vValues_;
      pNtk_->ForEachPi([&](int nIdx, int nId) {
        vec_ops::Copy(nWords_, vValues_.begin() + nId * nWords_, vInputStimuli[nIdx].begin(), false);
      });
      for(int nOffset : vOffsets) {
        SimulateOneWord(nOffset);
      }
    }
    // TODO: better to make network class clara out all backups on read unless fReuse (also on assign network?)
  }

  template <typename Ntk>
  void Simulator<Ntk>::PopBack() {
    vBackups_.pop_back();
  }

} // namespace boop::rrr

BOOP_HEADER_END
