#pragma once

#include <functional>
#include <type_traits>
#include <utility>

BOOP_HEADER_START

namespace boop {

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

} // namespace boop

BOOP_HEADER_END
