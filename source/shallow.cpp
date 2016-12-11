/*************************************************************
  shallow.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <xprotocol.h>
#include <magicbb.h>
#include <xtests.h>

void see_perf_test(std::string const& fname)
{
    int N = 1000000;
    auto see_old_cbk = [N](size_t i, NEngine::Board& board, NEngine::Move& move)
    {
      int x = 0;
      for(int n = 0; n < N; ++n)
      {
        x += board.see_old(move);
      }
      move.seen_ = x != 0;
    };
    auto see_new_cbk = [N](size_t i, NEngine::Board& board, NEngine::Move& move)
    {
      int x = 0;
      for(int n = 0; n < N; ++n)
      {
        x += board.see(move);
      }
      move.seen_ = x != 0;
    };
    NEngine::testFen(fname,
                     see_new_cbk,
                     [](std::string const& err)
                     {
                       std::cout << "error: " << err << std::endl;
                     });
}

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
    //NEngine::testSee(argv[1]);
    see_perf_test(argv[1]);
    return 0;
  }

  for(; xpr.doCmd(););

  //std::cout << "see failed: " << NEngine::Board::see_failed_ << std::endl;

	return 0;
}

