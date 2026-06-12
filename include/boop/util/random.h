#pragma once

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

class SimpleRNG {
  static constexpr unsigned uDefaultZ = 3716960521u;
  static constexpr unsigned uDefaultW = 2174103536u;
  unsigned uZ_, uW_;

public:
  SimpleRNG() : uZ_(uDefaultZ), uW_(uDefaultW) {}

  unsigned operator()() {
    uZ_ = 36969 * (uZ_ & 65535) + (uZ_ >> 16);
    uW_ = 18000 * (uW_ & 65535) + (uW_ >> 16);
    return (uZ_ << 16) + uW_;
  }

  void Reset() {
    uZ_ = uDefaultZ;
    uW_ = uDefaultW;
  }

  unsigned long long W() {
    return ((unsigned long long)(*this)() << 32) |
           ((unsigned long long)(*this)() << 0);
  }
};

} // namespace boop

BOOP_HEADER_END
