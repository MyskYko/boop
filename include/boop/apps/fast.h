#pragma once

#include <cassert>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

template <typename Ntk> void RunFast(Ntk *pNtk) {
  assert(pNtk != nullptr);
  pNtk->TrivialCollapse();
  pNtk->Sweep();
  pNtk->BalancedDecompose();
}

} // namespace boop

BOOP_HEADER_END
