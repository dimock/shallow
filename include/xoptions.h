#pragma once
/*************************************************************
xoptions.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <string>
#include <vector>

namespace NEngine
{

struct xOptions
{
  // in Mb
  int   hash_size_{256};
  bool  use_nullmove_{ true };
};

struct xOptionInfo
{
  std::string name;
  std::string type;
  std::string min_val;
  std::string max_val;
  std::string def_val;
  std::vector<std::string> vars;
};

std::vector<xOptionInfo> all_options();

} // NEngine