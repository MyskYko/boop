#pragma once

#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <set>
#include <string>
#include <vector>

#include "boop/config.h"

BOOP_HEADER_START

namespace boop {

// warning

static inline void print_warning(std::string const &message) {
  std::cerr << "[w] " << message << std::endl;
}

// containers

template <typename T>
std::ostream &operator<<(std::ostream &os, std::vector<T> const &v) {
  std::string strDelim;
  os << "[";
  for (T const &e : v) {
    os << strDelim << e;
    strDelim = ", ";
  }
  os << "]";
  return os;
}

template <typename T>
std::ostream &operator<<(std::ostream &os, std::set<T> const &s) {
  std::string strDelim;
  os << "{";
  for (T const &e : s) {
    os << strDelim << e;
    strDelim = ", ";
  }
  os << "}";
  return os;
}

// complemented edges

static inline void print_complemented_edges(
    std::function<void(std::function<void(int, bool)> const &)> const
        &forEachEdge) {
  std::string strDelim;
  std::cout << "[";
  forEachEdge([&](int nId, bool fCompl) {
    std::cout << strDelim << (fCompl ? "!" : "") << nId;
    strDelim = ", ";
  });
  std::cout << "]";
}

// print next

struct SW {
  int nWidth = 0;
  bool fLeft = false;
};

struct NS {}; // no space

struct PrintFormat {
  static constexpr int nIntWidth = 4;
  static constexpr bool fDoubleScientific = true;
  static constexpr int nDoubleScientificPrecision =
      std::numeric_limits<double>::max_digits10;
  static constexpr int nDoubleFixedWidth = 8;
  static constexpr int nDoubleFixedPrecision = 2;
};

template <typename T> void print_next(std::ostream &os, T t);
template <typename T, typename... Args>
void print_next(std::ostream &os, T t, Args... args);

static inline void print_next(std::ostream &os, int t) {
  os << std::setw(PrintFormat::nIntWidth) << t;
}

template <typename... Args>
static inline void print_next(std::ostream &os, int t, Args... args) {
  os << std::setw(PrintFormat::nIntWidth) << t << " ";
  print_next(os, args...);
}

static inline void print_next(std::ostream &os, bool arg) { os << arg; }

template <typename... Args>
static inline void print_next(std::ostream &os, bool arg, Args... args) {
  if (arg) {
    os << "!";
  } else {
    os << " ";
  }
  print_next(os, args...);
}

static inline void print_next(std::ostream &os, double t) {
  if constexpr (PrintFormat::fDoubleScientific) {
    os << std::scientific
       << std::setprecision(PrintFormat::nDoubleScientificPrecision) << t;
  } else {
    os << std::fixed << std::setprecision(PrintFormat::nDoubleFixedPrecision)
       << std::setw(PrintFormat::nDoubleFixedWidth) << t;
  }
}

template <typename... Args>
static inline void print_next(std::ostream &os, double t, Args... args) {
  if constexpr (PrintFormat::fDoubleScientific) {
    os << std::scientific
       << std::setprecision(PrintFormat::nDoubleScientificPrecision) << t
       << " ";
  } else {
    os << std::fixed << std::setprecision(PrintFormat::nDoubleFixedPrecision)
       << std::setw(PrintFormat::nDoubleFixedWidth) << t << " ";
  }
  print_next(os, args...);
}

template <typename T>
static inline void print_next(std::ostream &os, SW sw, T arg) {
  if (sw.fLeft) {
    os << std::left;
  }
  os << std::setw(sw.nWidth) << arg;
  if (sw.fLeft) {
    os << std::right;
  }
}

template <typename T, typename... Args>
static inline void print_next(std::ostream &os, SW sw, T arg, Args... args) {
  if (sw.fLeft) {
    os << std::left;
  }
  os << std::setw(sw.nWidth) << arg << " ";
  if (sw.fLeft) {
    os << std::right;
  }
  print_next(os, args...);
}

template <typename T, typename... Args>
static inline void print_next(std::ostream &os, NS ns, T arg, Args... args) {
  (void)ns;
  os << arg;
  print_next(os, args...);
}

template <typename T>
static inline void print_next(std::ostream &os, std::vector<T> const &arg) {
  os << "[ ";
  for (T const &e : arg) {
    print_next(os, e);
    os << " ";
  }
  os << "]";
}

template <typename T, typename... Args>
static inline void print_next(std::ostream &os, std::vector<T> const &arg,
                              Args... args) {
  os << "[ ";
  for (T const &e : arg) {
    print_next(os, e);
    os << " ";
  }
  os << "] ";
  print_next(os, args...);
}

template <typename T>
static inline void print_next(std::ostream &os, std::set<T> const &arg) {
  os << "{ ";
  for (T const &e : arg) {
    print_next(os, e);
    os << " ";
  }
  os << "}";
}

template <typename T, typename... Args>
static inline void print_next(std::ostream &os, std::set<T> const &arg,
                              Args... args) {
  os << "{ ";
  for (T const &e : arg) {
    print_next(os, e);
    os << " ";
  }
  os << "} ";
  print_next(os, args...);
}

template <typename T> void print_next(std::ostream &os, T t) { os << t; }

template <typename T, typename... Args>
void print_next(std::ostream &os, T t, Args... args) {
  os << t << " ";
  print_next(os, args...);
}

} // namespace boop

BOOP_HEADER_END
