/*************************************************************
xoptions.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include "xoptions.h"

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
#ifdef __ANDROID__
      "64",
#else
      "256",
#endif
      {},
    }
    ,
    {
      "Threads",
      "spin",
      "1",
      "4",
      "1",
      {},
    }
  };
}

} // NEngine
