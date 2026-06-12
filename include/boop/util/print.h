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
  
  static inline void PrintWarning(std::string const &message) {
    std::cerr << "[w] " << message << std::endl;
  }

  // containers
  
  template <typename T>
  std::ostream &operator<<(std::ostream &os, std::vector<T> const &v) {
    std::string delim;
    os << "[";
    for(T const &e: v) {
      os << delim << e;
      delim = ", ";
    }
    os << "]";
    return os;
  }
  
  template <typename T>
  std::ostream &operator<<(std::ostream &os, std::set<T> const &s) {
    std::string delim;
    os << "{";
    for(T const &e: s) {
      os << delim << e;
      delim = ", ";
    }
    os << "}";
    return os;
  }

  // complemented edges

  static inline void PrintComplementedEdges(std::function<void(std::function<void(int, bool)> const &)> const &forEachEdge) {
    std::string delim;
    std::cout << "[";
    forEachEdge([&] (int id, bool c) {
      std::cout << delim << (c? "!": "") << id;
      delim = ", ";
    });
    std::cout << "]";
  }

  // print next

  struct SW {
    int width = 0;
    bool left = false;
  };

  struct NS {}; // no space

  struct PrintFormat {
    static constexpr int int_width = 4;
    static constexpr bool double_scientific = true;
    static constexpr int double_scientific_precision = std::numeric_limits<double>::max_digits10;
    static constexpr int double_fixed_width = 8;
    static constexpr int double_fixed_precision = 2;
  };

  template <typename T>
  void PrintNext(std::ostream &os, T t);
  template <typename T, typename... Args>
  void PrintNext(std::ostream &os, T t, Args... args);
  
  static inline void PrintNext(std::ostream &os, int t) {
    os << std::setw(PrintFormat::int_width) << t;
  }

  template <typename... Args>
  static inline void PrintNext(std::ostream &os, int t, Args... args) {
    os << std::setw(PrintFormat::int_width) << t << " ";
    PrintNext(os, args...);
  }

  static inline void PrintNext(std::ostream &os, bool arg) {
    os << arg;
  }
  
  template <typename... Args>
  static inline void PrintNext(std::ostream &os, bool arg, Args... args) {
    if(arg) {
      os << "!";
    } else {
      os << " ";
    }
    PrintNext(os, args...);
  }
  
  static inline void PrintNext(std::ostream &os, double t) {
    if constexpr (PrintFormat::double_scientific) {
      os << std::scientific << std::setprecision(PrintFormat::double_scientific_precision) << t;
    } else {
      os << std::fixed << std::setprecision(PrintFormat::double_fixed_precision) << std::setw(PrintFormat::double_fixed_width) << t;
    }
  }

  template <typename... Args>
  static inline void PrintNext(std::ostream &os, double t, Args... args) {
    if constexpr (PrintFormat::double_scientific) {
      os << std::scientific << std::setprecision(PrintFormat::double_scientific_precision) << t << " ";
    } else {
      os << std::fixed << std::setprecision(PrintFormat::double_fixed_precision) << std::setw(PrintFormat::double_fixed_width) << t << " ";
    }
    PrintNext(os, args...);
  }

  template <typename T>
  static inline void PrintNext(std::ostream &os, SW sw, T arg) {
    if(sw.left) {
      os << std::left;
    }
    os << std::setw(sw.width) << arg;
    if(sw.left) {
      os << std::right;
    }
  }

  template <typename T, typename... Args>
  static inline void PrintNext(std::ostream &os, SW sw, T arg, Args... args) {
    if(sw.left) {
      os << std::left;
    }
    os << std::setw(sw.width) << arg << " ";
    if(sw.left) {
      os << std::right;
    }
    PrintNext(os, args...);
  }

  template <typename T, typename... Args>
  static inline void PrintNext(std::ostream &os, NS ns, T arg, Args... args) {
    (void)ns;
    os << arg;
    PrintNext(os, args...);
  }
  
  template <typename T>
  static inline void PrintNext(std::ostream &os, std::vector<T> const &arg) {
    os << "[ ";
    for(T const &e: arg) {
      PrintNext(os, e);
      os << " ";
    }
    os << "]";
  }

  template <typename T, typename... Args>
  static inline void PrintNext(std::ostream &os, std::vector<T> const &arg, Args... args) {
    os << "[ ";
    for(T const &e: arg) {
      PrintNext(os, e);
      os << " ";
    }
    os << "] ";
    PrintNext(os, args...);
  }

  template <typename T>
  static inline void PrintNext(std::ostream &os, std::set<T> const &arg) {
    os << "{ ";
    for(T const &e: arg) {
      PrintNext(os, e);
      os << " ";
    }
    os << "}";
  }

  template <typename T, typename... Args>
  static inline void PrintNext(std::ostream &os, std::set<T> const &arg, Args... args) {
    os << "{ ";
    for(T const &e: arg) {
      PrintNext(os, e);
      os << " ";
    }
    os << "} ";
    PrintNext(os, args...);
  }
  
  template <typename T>
  void PrintNext(std::ostream &os, T t) {
    os << t;
  }

  template <typename T, typename... Args>
  void PrintNext(std::ostream &os, T t, Args... args) {
    os << t << " ";
    PrintNext(os, args...);
  }  
  
} // namespace boop

BOOP_HEADER_END
