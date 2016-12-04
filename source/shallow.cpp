/*************************************************************
  shallow.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <xprotocol.h>
#include <magicbb.h>
#include <xtests.h>

int main(int argn, char *argv[])
{
#ifdef _MSC_VER
  NEngine::init_popcount_ptr();
#endif

  NEngine::magic_ns::initialize();

  std::cout.setf(std::ios_base::unitbuf);
  NShallow::xProtocolMgr xpr;
 
  if(argn > 1)
  {
    NEngine::testFen(argv[1], [](NEngine::Board& board, NEngine::Move& move)
    {
      std::cout << NEngine::toFEN(board)
                << std::endl
                << NEngine::printSAN(board, move)
                << std::endl
                << "see: "<< board.see(move)
                << std::endl;
    });
    return 0;
  }

  for(; xpr.doCmd(););

	return 0;
}

