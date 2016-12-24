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
    //NEngine::optimizeFen(argv[1]);
    NEngine::testFen(
      argv[1],
      [](size_t, NEngine::xEPD& e)
      {
        NShallow::Processor proc;
        NEngine::Evaluator eval;
        eval.initialize(&e.board_, nullptr, &proc.getEvals());
        auto score = eval(-NEngine::Figure::MatScore, NEngine::Figure::MatScore);
        std::cout << score << std::endl;
      },
      [](std::string const& err_str) 
      {
        std::cout << "Error: " << err_str << std::endl;
      });
    return 0;
  }

  for(; xpr.doCmd(););

	return 0;
}

