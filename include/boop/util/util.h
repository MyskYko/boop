#pragma once

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <set>
#include <string>
#include <functional>
#include <limits>
#include <type_traits>
#include <bit>
#include <cassert>

#include "boop/util/types.h"
#include "boop/util/time.h"
#include "boop/util/print.h"

BOOP_HEADER_START

namespace boop {

  //

  constexpr int clog2(int n) {
    assert(n > 0);
    if(n <= 1) {
      return 0;
    }
    return std::bit_width(n - 1);
  }
  
  constexpr int pow2_ceil(int n) {
    assert(n >= 0);
    if(n == 0) {
      return 0;
    }
    return std::bit_ceil(n);
  }

  /* {{{ Invocable */

#if defined(__cpp_lib_is_invocable)
  template <typename Fn, typename... Args>
  using is_invokable = std::is_invocable<Fn, Args...>;
#else
  template <typename Fn, typename... Args>
  struct is_invokable: std::is_constructible<std::function<void(Args...)>, std::reference_wrapper<typename std::remove_reference<Fn>::type>> {};
#endif

#if defined(__cpp_lib_is_invocable)
  template <typename Fn, typename... Args>
  using invoke_return_t = std::invoke_result_t<Fn, Args...>;
#else
  template <typename Fn, typename... Args>
  struct invoke_return {
  private:
    template <typename F, typename... A>
    static auto test(int) -> decltype(std::declval<F>()(std::declval<A>()...));
    template <typename, typename...>
    static void test(...);
  public:
    using type = decltype(test<Fn, Args...>(0));
  };
  template <typename Fn, typename... Args>
  using invoke_return_t = typename invoke_return<Fn, Args...>::type;
#endif

#if defined(__cpp_lib_is_invocable)
  template <typename Fn, typename... Args>
  constexpr bool returns_int_v = std::is_same_v<invoke_return_t<Fn, Args...>, int>;
#else
  template <typename Fn, typename... Args>
  struct returns_int: std::is_same<invoke_return_t<Fn, Args...>, int> {};
  template <typename Fn, typename... Args>
  constexpr bool returns_int_v = returns_int<Fn, Args...>::value;
#endif

#if defined(__cpp_lib_is_invocable)
  template <typename Fn, typename... Args>
  constexpr bool returns_bool_v = std::is_same_v<invoke_return_t<Fn, Args...>, bool>;
#else
  template <typename Fn, typename... Args>
  struct returns_bool: std::is_same<invoke_return_t<Fn, Args...>, bool> {};
  template <typename Fn, typename... Args>
  constexpr bool returns_bool_v = returns_bool<Fn, Args...>::value;
#endif

  template <typename Func, typename... Args>
  static inline bool invoke_and_return_stop(const Func &func, Args &&...args) {
    if constexpr(returns_bool_v<Func, Args...>) {
      return func(std::forward<Args>(args)...);
    } else {
      func(std::forward<Args>(args)...);
      return false;
    }
  }
  
  /* }}} */

  /* {{{ Int size */
  
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
  
  /* }}} */

  /* {{{ Combination */

  inline bool ForEachCombinationRec(std::vector<int> &v, int n, int k, std::function<bool(std::vector<int> const &)> const &func) {
    if(k == 0) {
      return func(v);
    }
    for(int i = v.back() + 1; i < n - k + 1; i++) {
      v.push_back(i);
      if(ForEachCombinationRec(v, n, k-1, func)) {
        return true;
      }
      v.pop_back();
    }
    return false;
  }
  
  static inline void ForEachCombination(int n, int k, std::function<bool(std::vector<int> const &)> const &func) {
    std::vector<int> v;
    v.reserve(k);
    ForEachCombinationRec(v, n, k, func);
  }
  
  /* }}} */

  /* {{{ Random */

  class SimpleRNG {
    static constexpr unsigned NUMBER1 = 3716960521u;
    static constexpr unsigned NUMBER2 = 2174103536u;
    unsigned m_z, m_w;
    
  public:
    SimpleRNG() :
      m_z(NUMBER1),
      m_w(NUMBER2) {
    }
    
    unsigned operator()() {
      m_z = 36969 * (m_z & 65535) + (m_z >> 16);
      m_w = 18000 * (m_w & 65535) + (m_w >> 16);
      return (m_z << 16) + m_w;
    }

    void Reset() {
      m_z = NUMBER1;
      m_w = NUMBER2;
    }

    unsigned long long W() {
      return ((unsigned long long)(*this)() << 32) | ((unsigned long long)(*this)() << 0);
    }
  };
  
  /* }}} */

} // namespace boop

BOOP_HEADER_END
