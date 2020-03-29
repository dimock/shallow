/*************************************************************
  shallow.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <xprotocol.h>
#include <magicbb.h>
#include <xtests.h>
#include <xlist.h>
#include <xalgorithm.h>
#include <iomanip>


#if ((defined _MSC_VER) && (defined USE_MINIDUMP))

#include <windows.h>
#include "minidump.h"
void main_loop(NShallow::xProtocolMgr & xpr)
{
  __try
  {
    for (; xpr.doCmd(); );

  }
  __except (TopLevelFilter(GetExceptionInformation()))
  {
    xpr.writeError();
  }
}
#else
void main_loop(NShallow::xProtocolMgr & xpr)
{
  for (; xpr.doCmd(); );
}
#endif

int main(int argn, char *argv[])
{
  NEngine::init_popcount_ptr();
  NEngine::magic_ns::initialize();

  std::cout.setf(std::ios_base::unitbuf);
  NShallow::xProtocolMgr xpr;

  //if (argn > 1)
  //{
  //  NEngine::testMovegen(argv[1]);
  //  return 0;
  //}

  main_loop(xpr);

	return 0;
}

