#include "boop/interface/mockturtle_interface.h"

#include <random>

#include "boop/network/and_network.h"

#ifdef BOOP_USE_MOCKTURTLE

#include <cassert>
#include <map>
#include <vector>

#include <mockturtle/algorithms/aig_resub.hpp>
#include <mockturtle/algorithms/cleanup.hpp>
#include <mockturtle/algorithms/cut_rewriting.hpp>
#include <mockturtle/algorithms/node_resynthesis/bidecomposition.hpp>
#include <mockturtle/algorithms/node_resynthesis/dsd.hpp>
#include <mockturtle/algorithms/node_resynthesis/xag_npn.hpp>
#include <mockturtle/algorithms/refactoring.hpp>
#include <mockturtle/algorithms/resubstitution.hpp>
#include <mockturtle/algorithms/sim_resub.hpp>
#include <mockturtle/algorithms/window_rewriting.hpp>
#include <mockturtle/networks/aig.hpp>
#include <mockturtle/views/depth_view.hpp>
#include <mockturtle/views/fanout_view.hpp>
#include <mockturtle/views/topo_view.hpp>

BOOP_IMPL_START

namespace boop {

namespace mockturtle_interface {

using mockturtle::aig_network;
using mockturtle::aig_resubstitution;
using mockturtle::aig_resubstitution2;
using mockturtle::bidecomposition_resynthesis;
using mockturtle::cleanup_dangling;
using mockturtle::cut_rewriting;
using mockturtle::cut_rewriting_params;
using mockturtle::depth_view;
using mockturtle::dsd_resynthesis;
using mockturtle::fanout_view;
using mockturtle::refactoring;
using mockturtle::refactoring_params;
using mockturtle::resubstitution_params;
using mockturtle::sim_resubstitution;
using mockturtle::topo_view;
using mockturtle::window_rewriting;
using mockturtle::window_rewriting_params;
using mockturtle::xag_npn_resynthesis;

template <typename Rng> std::string Rewrite(aig_network &aig, Rng &rng) {
  std::string strLog;
  switch (rng() % 2) {
  case 0: {
    strLog = "rw";
    xag_npn_resynthesis<aig_network> resynNpn;
    cut_rewriting_params paramsRewrite;
    paramsRewrite.cut_enumeration_ps.cut_size = 4;
    paramsRewrite.allow_zero_gain = rng() & 1;
    paramsRewrite.use_dont_cares = rng() & 1;
    paramsRewrite.preserve_depth = rng() & 1;
    if (paramsRewrite.allow_zero_gain) {
      strLog += " -z";
    }
    if (paramsRewrite.use_dont_cares) {
      strLog += " -d";
    }
    if (paramsRewrite.preserve_depth) {
      strLog += " -l";
    }
    cut_rewriting(aig, resynNpn, paramsRewrite);
    break;
  }
  case 1: {
    strLog = "wrw";
    window_rewriting_params paramsWindow;
    strLog += " -K " + std::to_string(paramsWindow.cut_size);
    strLog += " -L " + std::to_string(paramsWindow.num_levels);
    paramsWindow.filter_cyclic_substitutions = true;
    window_rewriting(aig, paramsWindow);
    break;
  }
  }
  aig = cleanup_dangling(aig);
  return strLog;
}

template <typename Rng> std::string Refactor(aig_network &aig, Rng &rng) {
  refactoring_params paramsRefactor;
  paramsRefactor.allow_zero_gain = rng() & 1;
  std::string strLog = " -K " + std::to_string(paramsRefactor.max_pis);
  if (paramsRefactor.allow_zero_gain) {
    strLog += " -z";
  }
  strLog = "rf-dsd" + strLog;
  bidecomposition_resynthesis<aig_network> resynFallback;
  dsd_resynthesis<aig_network, decltype(resynFallback)> resynDsd(resynFallback);
  refactoring(aig, resynDsd, paramsRefactor);
  aig = cleanup_dangling(aig);
  return strLog;
}

template <typename Rng> std::string Resubstitute(aig_network &aig, Rng &rng) {
  resubstitution_params paramsResub;
  paramsResub.max_pis = 6 + (rng() % 7);
  paramsResub.max_inserts = rng() % 3;
  paramsResub.window_size = 12 + (rng() % 3);
  paramsResub.preserve_depth = rng() & 1;
  std::string strLog = " -K " + std::to_string(paramsResub.max_pis);
  strLog += " -N " + std::to_string(paramsResub.max_inserts);
  switch (rng() % 3) {
  case 0:
    strLog = "rs" + strLog;
    aig_resubstitution(aig, paramsResub);
    break;
  case 1: {
    strLog = "wrs" + strLog;
    strLog += " -W " + std::to_string(paramsResub.window_size);
    if (paramsResub.preserve_depth) {
      strLog += " -l";
    }
    depth_view viewDepth{aig};
    fanout_view viewFanout{viewDepth};
    aig_resubstitution2(viewFanout, paramsResub);
    break;
  }
  case 2:
    strLog = "srs" + strLog;
    sim_resubstitution(aig, paramsResub);
    break;
  }
  aig = cleanup_dangling(aig);
  return strLog;
}

template <typename Ntk>
void MockturtleReader(const aig_network &aig, Ntk *pNtk) {
  topo_view viewAig{aig};
  std::map<aig_network::node, int> mNodes;
  pNtk->Reserve(viewAig.size());
  auto sigConst0 = viewAig.get_constant(false);
  auto nodeConst0 = viewAig.get_node(sigConst0);
  assert(!viewAig.constant_value(nodeConst0));
  mNodes[nodeConst0] = pNtk->GetConst0();
  viewAig.foreach_pi([&](auto node) { mNodes[node] = pNtk->AddPi(); });
  viewAig.foreach_gate([&](auto node) {
    std::vector<int> vFanins;
    std::vector<bool> vCompls;
    viewAig.foreach_fanin(node, [&](auto sig) {
      auto nodeFanin = viewAig.get_node(sig);
      vFanins.push_back(mNodes[nodeFanin]);
      vCompls.push_back(viewAig.is_complemented(sig));
    });
    mNodes[node] = pNtk->AddAnd(vFanins, vCompls);
  });
  viewAig.foreach_po([&](auto sig) {
    auto nodeDriver = viewAig.get_node(sig);
    pNtk->AddPo(mNodes[nodeDriver], viewAig.is_complemented(sig));
  });
}

template <typename Ntk> aig_network *CreateMockturtle(Ntk *pNtk) {
  aig_network *pAig = new aig_network;
  std::vector<aig_network::signal> vSignals(pNtk->GetNumNodes());
  vSignals[0] = pAig->get_constant(false);
  pNtk->ForEachPi([&](int nId) { vSignals[nId] = pAig->create_pi(); });
  pNtk->ForEachInt([&](int nId) {
    assert(pNtk->GetNodeType(nId) == AND);
    auto sig = pAig->get_constant(true);
    pNtk->ForEachFanin(nId, [&](int nFi, bool fCompl) {
      auto sigFanin = fCompl ? pAig->create_not(vSignals[nFi]) : vSignals[nFi];
      sig = (sig == pAig->get_constant(true)) ? sigFanin
                                              : pAig->create_and(sig, sigFanin);
    });
    vSignals[nId] = sig;
  });
  pNtk->ForEachPoDriver([&](int nFi, bool fCompl) {
    auto sig = fCompl ? pAig->create_not(vSignals[nFi]) : vSignals[nFi];
    pAig->create_po(sig);
  });
  return pAig;
}

template <typename Ntk, typename Rng>
std::string PerformLocal(Ntk *pNtk, Rng &rng) {
  aig_network *pAig = CreateMockturtle(pNtk);
  std::string strLog;
  switch (rng() % 3) {
  case 0:
    strLog = Rewrite(*pAig, rng);
    break;
  case 1:
    strLog = Refactor(*pAig, rng);
    break;
  case 2:
    strLog = Resubstitute(*pAig, rng);
    break;
  }
  pNtk->Read(*pAig, MockturtleReader<Ntk>);
  delete pAig;
  return strLog;
}

} // namespace mockturtle_interface

template <typename Ntk, typename Rng>
std::string MockturtlePerformLocal(Ntk *pNtk, Rng &rng) {
  return mockturtle_interface::PerformLocal(pNtk, rng);
}

template std::string
MockturtlePerformLocal<AndNetwork, std::mt19937>(AndNetwork *pNtk,
                                                 std::mt19937 &rng);

} // namespace boop

BOOP_IMPL_END

#else

BOOP_IMPL_START

namespace boop {

template <typename Ntk, typename Rng>
std::string MockturtlePerformLocal(Ntk *pNtk, Rng &rng) {
  (void)pNtk;
  (void)rng;
  return "mockturtle is disabled";
}

template std::string
MockturtlePerformLocal<AndNetwork, std::mt19937>(AndNetwork *pNtk,
                                                 std::mt19937 &rng);

} // namespace boop

BOOP_IMPL_END

#endif
