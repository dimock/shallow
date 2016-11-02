/*************************************************************
xInput.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <iostream>
#include <string>

#ifdef _MSC_VER
#include <windows.h>
#elif (defined __GNUC__)
#endif

namespace NShallow
{

class xInput
{
public:
  xInput();

  std::string peekInput();
  std::string getInput();

private:
  bool peek();
  std::string readInput();

#ifdef _MSC_VER
  HANDLE hinput_;
  bool   in_pipe_;
#elif (defined __GNUC__)
#endif

  std::istream& is_;
};

} // NShallow
