#include "boop/interface/rinox_interface.h"
#include "boop/util/size.h"

#ifdef BOOP_USE_RINOX
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

#include <rinox/libraries/libraries.hpp>
#include <rinox/network/network.hpp>
#include <rinox/opto/algorithms/resynthesize.hpp>

BOOP_IMPL_START

namespace boop {

namespace {

mockturtle::phase_type ConvertPinPhase(CellLibrary::PinPhase phase) {
  switch (phase) {
  case CellLibrary::UNKNOWN:
    return mockturtle::phase_type::UNKNOWN;
  case CellLibrary::INV:
    return mockturtle::phase_type::INV;
  case CellLibrary::NONINV:
    return mockturtle::phase_type::NONINV;
  }
  assert(false);
  return mockturtle::phase_type::UNKNOWN;
}

mockturtle::pin MakeMockturtlePin(const CellLibrary *pLibrary, int nCell,
                                  int nOutput, int nInput) {
  const CellLibrary::TimingArc &arc =
      pLibrary->GetTimingArc(nCell, nOutput, nInput);
  return mockturtle::pin{pLibrary->GetInputName(nCell, nInput),
                         ConvertPinPhase(arc.phase),
                         pLibrary->GetInputLoad(nCell, nInput),
                         pLibrary->GetMaxLoad(nCell, nOutput),
                         arc.dRiseBlockDelay,
                         arc.dRiseFanoutDelay,
                         arc.dFallBlockDelay,
                         arc.dFallFanoutDelay};
}

std::vector<mockturtle::pin> BuildMockturtlePins(const CellLibrary *pLibrary,
                                                 int nCell, int nOutput) {
  std::vector<mockturtle::pin> vPins;
  vPins.reserve(pLibrary->GetNumInputs(nCell));
  for (int i = 0; i < pLibrary->GetNumInputs(nCell); ++i) {
    vPins.push_back(MakeMockturtlePin(pLibrary, nCell, nOutput, i));
  }
  return vPins;
}

std::vector<std::string> GetInputNames(const CellLibrary *pLibrary, int nCell) {
  std::vector<std::string> vInputNames;
  vInputNames.reserve(pLibrary->GetNumInputs(nCell));
  for (int i = 0; i < pLibrary->GetNumInputs(nCell); ++i) {
    vInputNames.push_back(pLibrary->GetInputName(nCell, i));
  }
  return vInputNames;
}

std::vector<mockturtle::gate> BuildMockturtleGatesFromLibrary(
    const CellLibrary *pLibrary,
    std::vector<std::vector<unsigned>> &vvBindingIds,
    std::vector<int> &vCellIdsByBindingId) {
  std::vector<mockturtle::gate> vGates;
  vvBindingIds.resize(pLibrary->GetNumCells());
  for (int nCell = 0; nCell < pLibrary->GetNumCells(); ++nCell) {
    const int nInputs = pLibrary->GetNumInputs(nCell);
    const int nOutputs = pLibrary->GetNumOutputs(nCell);
    const std::vector<std::string> vInputNames = GetInputNames(pLibrary, nCell);
    for (int i = 0; i < nOutputs; ++i) {
      const unsigned nBindingId = static_cast<unsigned>(int_size(vGates));
      const std::string &strFunction = pLibrary->GetOutputFunction(nCell, i);
      kitty::dynamic_truth_table tt{static_cast<uint32_t>(nInputs)};
      bool fParsed = kitty::create_from_formula(tt, strFunction, vInputNames);
      assert(fParsed);
      (void)fParsed;
      vGates.push_back(mockturtle::gate{
          nBindingId,
          pLibrary->GetCellName(nCell),
          strFunction,
          static_cast<uint32_t>(nInputs),
          tt,
          pLibrary->GetOutputArea(nCell, i),
          BuildMockturtlePins(pLibrary, nCell, i),
          pLibrary->GetOutputName(nCell, i)});
      vvBindingIds[nCell].push_back(nBindingId);
      vCellIdsByBindingId.push_back(nCell);
    }
  }
  return vGates;
}

template <typename RinoxNetwork>
RinoxNetwork *CreateRinox(BoundNetwork *pNtk,
                          const std::vector<mockturtle::gate> &vGates,
                          const std::vector<std::vector<unsigned>>
                              &vvBindingIds) {
  RinoxNetwork *pNtkRinox = new RinoxNetwork(vGates);
  using signal = typename RinoxNetwork::signal;
  std::vector<signal> vSignals(pNtk->GetNumNodes());
  vSignals[0] = pNtkRinox->get_constant(false);
  vSignals[1] = pNtkRinox->get_constant(true);
  pNtk->ForEachPi([&](int nId) {
    vSignals[nId] = pNtkRinox->create_pi();
  });
  pNtk->ForEachInstance([&](int nInstance) {
    assert(pNtk->IsCell(nInstance));
    const int nCell = pNtk->GetCell(nInstance);
    std::vector<signal> vChildren;
    vChildren.reserve(pNtk->GetNumFanins(nInstance));
    pNtk->ForEachFanin(nInstance, [&](int nId) {
      vChildren.push_back(vSignals[nId]);
    });
    signal sig = pNtkRinox->create_node(vChildren, vvBindingIds[nCell]);
    for (int i = 0; i < pNtk->GetNumOutputs(nInstance); ++i) {
      const int nId = pNtk->GetOutput(nInstance, i);
      vSignals[nId] = signal{sig.index, static_cast<uint64_t>(i)};
    }
  });
  pNtk->ForEachPo([&](int nPo) {
    const int nId = pNtk->GetFanin(nPo, 0);
    pNtkRinox->create_po(vSignals[nId]);
  });
  return pNtkRinox;
}

template <typename RinoxNetwork>
void RinoxReader(const RinoxNetwork &ntkSrc, BoundNetwork *pNtk,
                 const std::vector<int> &vCellIdsByBindingId) {
  const CellLibrary *pLibrary = pNtk->GetLibrary();
  std::vector<std::vector<int>> vSignals(ntkSrc.size());
  vSignals[0] = {pNtk->GetConst0()};
  vSignals[1] = {pNtk->GetConst1()};
  ntkSrc.foreach_pi([&](auto nPi) {
    vSignals[nPi] = {pNtk->AddPi()};
  });
  ntkSrc.foreach_gate([&](auto nGate) {
    if (ntkSrc.is_dead(nGate)) {
      return;
    }
    std::vector<int> vFanins;
    ntkSrc.foreach_fanin(nGate, [&](auto sig) {
      vFanins.push_back(vSignals[sig.index][sig.output]);
    });
    std::vector<unsigned> vBindingIds = ntkSrc.get_binding_ids(nGate);
    const int nCell = vCellIdsByBindingId[vBindingIds.front()];
    const int nInstance = pNtk->AddCell(nCell, vFanins);
    vSignals[nGate].reserve(pLibrary->GetNumOutputs(nCell));
    for (int i = 0; i < pLibrary->GetNumOutputs(nCell); ++i) {
      vSignals[nGate].push_back(pNtk->GetOutput(nInstance, i));
    }
  });
  ntkSrc.foreach_po([&](auto sig) {
    pNtk->AddPo(vSignals[sig.index][sig.output]);
  });
}

template <typename RinoxNetwork>
void RunRinoxOptimization(RinoxNetwork &ntk,
                          const std::vector<mockturtle::gate> &vGates) {
  rinox::libraries::augmented_library<
      rinox::network::design_type_t::CELL_BASED>
      augmentedLibrary(vGates);
  using Params = rinox::opto::algorithms::default_resynthesis_params<12>;
  using Database = rinox::databases::mapped_database<RinoxNetwork,
                                                     Params::max_num_vars>;
  Database database(augmentedLibrary);
  Params params;
  params.try_rewire = true;
  rinox::opto::algorithms::area_resynthesize<RinoxNetwork, Params>(
      ntk, &database, params);
}

} // namespace

void RinoxExecute(BoundNetwork *pNtk) {
  const CellLibrary *pLibrary = pNtk->GetLibrary();
  pNtk->ForEachInt([&](int nId) {
    assert(!pNtk->IsDontTouch(nId));
    assert(!pNtk->IsDontShare(nId));
  });
  pNtk->ForEachPo([&](int nId) {
    assert(!pNtk->IsDontTouch(nId));
    assert(!pNtk->IsDontShare(nId));
  });
  for (int nCell = 0; nCell < pLibrary->GetNumCells(); ++nCell) {
    assert(pLibrary->GetNumOutputs(nCell) <= 4);
  }
  std::vector<std::vector<unsigned>> vvBindingIds;
  std::vector<int> vCellIdsByBindingId;
  std::vector<mockturtle::gate> vGates = BuildMockturtleGatesFromLibrary(
      pLibrary, vvBindingIds, vCellIdsByBindingId);
  using RinoxNetwork =
      rinox::network::bound_network<rinox::network::design_type_t::CELL_BASED,
                                    4>;
  RinoxNetwork *pNtkRinox =
      CreateRinox<RinoxNetwork>(pNtk, vGates, vvBindingIds);
  RunRinoxOptimization(*pNtkRinox, vGates);
  pNtk->Read(*pNtkRinox, [&](const RinoxNetwork &ntkSrc, BoundNetwork *pNtkDst) {
    RinoxReader(ntkSrc, pNtkDst, vCellIdsByBindingId);
  });
  delete pNtkRinox;
}

} // namespace boop

BOOP_IMPL_END

#else

BOOP_IMPL_START

namespace boop {

void RinoxExecute(BoundNetwork *pNtk) {
  (void)pNtk;
}

} // namespace boop

BOOP_IMPL_END

#endif
