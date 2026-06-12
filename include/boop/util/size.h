#pragma once

#include <cassert>
#include <iterator>
#include <limits>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

  template <template <typename...> typename Container, typename... Ts>
  static inline int int_size(Container<Ts...> const &c) {
    assert(c.size() <= (typename Container<Ts...>::size_type)std::numeric_limits<int>::max());
    return c.size();
  }

  template <template <typename...> typename Container, typename... Ts>
  static inline bool check_int_size(Container<Ts...> const &c) {
    return c.size() <= (typename Container<Ts...>::size_type)std::numeric_limits<int>::max();
  }

  static inline bool check_int_max(int i) {
    return i == std::numeric_limits<int>::max();
  }

  template <typename Iterator>
  static inline int int_distance(Iterator begin, Iterator it) {
    typename std::iterator_traits<Iterator>::difference_type d = std::distance(begin, it);
    assert(d <= (typename std::iterator_traits<Iterator>::difference_type)std::numeric_limits<int>::max());
    assert(d >= (typename std::iterator_traits<Iterator>::difference_type)std::numeric_limits<int>::lowest());
    return d;
  }
  
} // namespace boop

BOOP_HEADER_END
