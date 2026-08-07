#pragma once

#include <algorithm>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

#include "boop/config.h"
#include "boop/util/size.h"

BOOP_HEADER_START

namespace boop {

class CellLibrary {
public:
  enum PinPhase { UNKNOWN, INV, NONINV };

  struct TimingArc {
    PinPhase phase;
    double dRiseBlockDelay;
    double dRiseFanoutDelay;
    double dFallBlockDelay;
    double dFallFanoutDelay;
  };

  // initialization
  void Clear();
  void Reserve(int nReserve);
  int AddCell(std::string strName, std::vector<std::string> vInputNames,
              std::vector<double> vInputLoads,
              std::vector<std::string> vOutputNames,
              std::vector<double> vMaxLoads,
              std::vector<std::string> vOutputFunctions,
              std::vector<double> vOutputAreas,
              std::vector<std::vector<TimingArc>> vvTimingArcs);

  // library properties
  int GetNumCells() const;

  // cell properties
  int GetNumInputs(int nCell) const;
  int GetNumOutputs(int nCell) const;
  const std::string &GetCellName(int nCell) const;
  const std::string &GetInputName(int nCell, int nInput) const;
  const std::string &GetOutputName(int nCell, int nOutput) const;
  const std::string &GetOutputFunction(int nCell, int nOutput) const;
  double GetInputLoad(int nCell, int nInput) const;
  double GetOutputArea(int nCell, int nOutput) const;
  double GetMaxLoad(int nCell, int nOutput) const;
  const TimingArc &GetTimingArc(int nCell, int nOutput, int nInput) const;

private:
  struct Cell {
    std::string strName;
    std::vector<std::string> vInputNames;
    std::vector<double> vInputLoads;
    std::vector<std::string> vOutputNames;
    std::vector<double> vMaxLoads;
    std::vector<std::string> vOutputFunctions;
    std::vector<double> vOutputAreas;
    std::vector<std::vector<TimingArc>> vvTimingArcs;
  };

  std::vector<Cell> vCells_;
};

// initialization

inline void CellLibrary::Clear() { vCells_.clear(); }

inline void CellLibrary::Reserve(int nReserve) { vCells_.reserve(nReserve); }

inline int CellLibrary::AddCell(
    std::string strName, std::vector<std::string> vInputNames,
    std::vector<double> vInputLoads, std::vector<std::string> vOutputNames,
    std::vector<double> vMaxLoads, std::vector<std::string> vOutputFunctions,
    std::vector<double> vOutputAreas,
    std::vector<std::vector<TimingArc>> vvTimingArcs) {
  assert(!vOutputFunctions.empty());
  assert(int_size(vInputLoads) == int_size(vInputNames));
  assert(int_size(vOutputNames) == int_size(vOutputFunctions));
  assert(int_size(vMaxLoads) == int_size(vOutputFunctions));
  assert(int_size(vOutputAreas) == int_size(vOutputFunctions));
  assert(int_size(vvTimingArcs) == int_size(vOutputFunctions));
  for (const std::vector<TimingArc> &vArcs : vvTimingArcs) {
    assert(int_size(vArcs) == int_size(vInputNames));
  }
  assert(std::none_of(vCells_.begin(), vCells_.end(), [&](const Cell &cell) {
    return cell.strName == strName;
  }));
  int nCell = int_size(vCells_);
  vCells_.push_back(Cell{std::move(strName), std::move(vInputNames),
                         std::move(vInputLoads), std::move(vOutputNames),
                         std::move(vMaxLoads), std::move(vOutputFunctions),
                         std::move(vOutputAreas), std::move(vvTimingArcs)});
  return nCell;
}

// library properties

inline int CellLibrary::GetNumCells() const { return int_size(vCells_); }

// cell properties

inline int CellLibrary::GetNumInputs(int nCell) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  return int_size(vCells_[nCell].vInputNames);
}

inline int CellLibrary::GetNumOutputs(int nCell) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  return int_size(vCells_[nCell].vOutputNames);
}

inline const std::string &CellLibrary::GetCellName(int nCell) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  return vCells_[nCell].strName;
}

inline const std::string &CellLibrary::GetInputName(int nCell,
                                                    int nInput) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  assert(nInput >= 0 && nInput < GetNumInputs(nCell));
  assert(int_size(vCells_[nCell].vInputNames) == GetNumInputs(nCell));
  return vCells_[nCell].vInputNames[nInput];
}

inline const std::string &CellLibrary::GetOutputName(int nCell,
                                                     int nOutput) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  assert(nOutput >= 0 && nOutput < GetNumOutputs(nCell));
  assert(int_size(vCells_[nCell].vOutputNames) == GetNumOutputs(nCell));
  return vCells_[nCell].vOutputNames[nOutput];
}

inline const std::string &CellLibrary::GetOutputFunction(int nCell,
                                                         int nOutput) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  assert(nOutput >= 0 && nOutput < GetNumOutputs(nCell));
  assert(int_size(vCells_[nCell].vOutputFunctions) == GetNumOutputs(nCell));
  return vCells_[nCell].vOutputFunctions[nOutput];
}

inline double CellLibrary::GetInputLoad(int nCell, int nInput) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  assert(nInput >= 0 && nInput < GetNumInputs(nCell));
  assert(int_size(vCells_[nCell].vInputLoads) == GetNumInputs(nCell));
  return vCells_[nCell].vInputLoads[nInput];
}

inline double CellLibrary::GetOutputArea(int nCell, int nOutput) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  assert(nOutput >= 0 && nOutput < GetNumOutputs(nCell));
  assert(int_size(vCells_[nCell].vOutputAreas) == GetNumOutputs(nCell));
  return vCells_[nCell].vOutputAreas[nOutput];
}

inline double CellLibrary::GetMaxLoad(int nCell, int nOutput) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  assert(nOutput >= 0 && nOutput < GetNumOutputs(nCell));
  assert(int_size(vCells_[nCell].vMaxLoads) == GetNumOutputs(nCell));
  return vCells_[nCell].vMaxLoads[nOutput];
}

inline const CellLibrary::TimingArc &
CellLibrary::GetTimingArc(int nCell, int nOutput, int nInput) const {
  assert(nCell >= 0 && nCell < GetNumCells());
  assert(nOutput >= 0 && nOutput < GetNumOutputs(nCell));
  assert(nInput >= 0 && nInput < GetNumInputs(nCell));
  assert(int_size(vCells_[nCell].vvTimingArcs) == GetNumOutputs(nCell));
  assert(int_size(vCells_[nCell].vvTimingArcs[nOutput]) == GetNumInputs(nCell));
  return vCells_[nCell].vvTimingArcs[nOutput][nInput];
}

} // namespace boop

BOOP_HEADER_END
