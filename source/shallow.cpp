/*************************************************************
  shallow.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <xprotocol.h>
#include <magicbb.h>
#include <xtests.h>


int main(int argn, char *argv[])
{
  NEngine::init_popcount_ptr();
  NEngine::magic_ns::initialize();

  std::cout.setf(std::ios_base::unitbuf);
  NShallow::xProtocolMgr xpr;
 
  if(argn > 1)
  {
    NEngine::optimizeFen(argv[1]);
    return 0;
  }

  for(; xpr.doCmd(););

  //std::cout << "xcounter = " << NEngine::Engine::xcounter_ << std::endl;
  //std::cout << "moves made = " << NEngine::XCounter::count_ << std::endl;

	return 0;
}

