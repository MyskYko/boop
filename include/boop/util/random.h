#pragma once

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

class SimpleRNG {
  static constexpr unsigned NUMBER1 = 3716960521u;
  static constexpr unsigned NUMBER2 = 2174103536u;
  unsigned m_z, m_w;

public:
  SimpleRNG() : m_z(NUMBER1), m_w(NUMBER2) {}

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
    return ((unsigned long long)(*this)() << 32) |
           ((unsigned long long)(*this)() << 0);
  }
};

} // namespace boop

BOOP_HEADER_END
