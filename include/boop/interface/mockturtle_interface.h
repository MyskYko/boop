#pragma once

#include <string>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

template <typename Ntk, typename Rng>
extern std::string MockturtlePerformLocal(Ntk *pNtk, Rng &rng);

} // namespace boop

BOOP_HEADER_END
