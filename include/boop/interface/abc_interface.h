#pragma once

#include <string>

// todo: accommodate how abc is compiled (parent or child) or not compiled together

void Abc_Start();
void Abc_Stop();

template <typename Ntk>
void Abc9Execute(Ntk *pNtk, const std::string &command);

#ifndef BOOP_USE_ABC

inline void Abc_Start() {
}

inline void Abc_Stop() {
}

template <typename Ntk>
inline void Abc9Execute(Ntk *pNtk, const std::string &command) {
  (void)pNtk;
  (void)command;
}

#endif
