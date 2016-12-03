/*************************************************************
  shallow.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <xprotocol.h>
#include <magicbb.h>
#include <xlist.h>
#include <list>

int main(int, char *)
{
#ifdef _MSC_VER
  NEngine::init_popcount_ptr();
#endif

  NEngine::magic_ns::initialize();

  std::cout.setf(std::ios_base::unitbuf);
  NShallow::xProtocolMgr xpr;
  for(; xpr.doCmd(););

	return 0;
}

