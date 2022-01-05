/*************************************************************
  shallow.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include "iostream"
#include "xprotocol.h"
#include "magicbb.h"
#include "xtests.h"
#include "xlist.h"
#include "xalgorithm.h"
#include "iomanip"


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
  NEngine::EvalCoefficients::initialize();

  std::cout.setf(std::ios_base::unitbuf);
  NShallow::xProtocolMgr xpr;

#ifndef __ANDROID__

#if(!defined(NDEBUG) && !defined(PROCESS_MOVES_SEQ))
  if (argn > 1)
  {
#ifdef SEE_TEST_EPD
    NEngine::testSee(argv[1]);
#else
    NEngine::evaluateFen(argv[1], argn > 2 ? argv[2] : "");
#endif // DO_SEE_TEST
    return 0;
  }
#endif

#ifdef PROCESS_MOVES_SEQ
  if (argn > 1)
  {
    NEngine::analyzeFen(argv[1], argn > 2 ? argv[2] : "", argn > 3 ? argv[3] : "");
    return 0;
  }
#endif

#endif // !__ANDROID__

  main_loop(xpr);

  return 0;
}

