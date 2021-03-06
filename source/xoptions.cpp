/*************************************************************
xoptions.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xoptions.h>

namespace NEngine
{

std::vector<xOptionInfo> all_options()
{
  return
  {
    {
      "Hash",
      "spin",
      "1",
      "1024",
      "256",
      {},
    },
    {
      "Threads",
      "spin",
      "1",
      "8",
      "1",
      {},
    }
  };
}

} // NEngine
