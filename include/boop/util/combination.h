#pragma once

#include <functional>
#include <vector>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

inline bool for_each_combination_rec(
    std::vector<int> &v, int n, int k,
    std::function<bool(std::vector<int> const &)> const &func) {
  if (k == 0) {
    return func(v);
  }
  for (int i = v.back() + 1; i < n - k + 1; i++) {
    v.push_back(i);
    if (for_each_combination_rec(v, n, k - 1, func)) {
      return true;
    }
    v.pop_back();
  }
  return false;
}

static inline void for_each_combination(
    int n, int k, std::function<bool(std::vector<int> const &)> const &func) {
  std::vector<int> v;
  v.reserve(k);
  for_each_combination_rec(v, n, k, func);
}

} // namespace boop

BOOP_HEADER_END
