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
    NEngine::testSee(argv[1]);
    //auto see_old_cbk = [](size_t i, NEngine::Board& board, NEngine::Move& move)
    //{
    //  auto color = board.getColor();
    //  auto ocolor = NEngine::Figure::otherColor(color);
    //  uint64 all_mask_inv = ~(board.fmgr().mask(NEngine::Figure::ColorBlack) | board.fmgr().mask(NEngine::Figure::ColorWhite));
    //  auto brq_mask = board.fmgr().bishop_mask(color) | board.fmgr().rook_mask(color) | board.fmgr().queen_mask(color);
    //  brq_mask &= ~NEngine::set_mask_bit(move.to_);
    //  auto ki_pos = board.kingPos(ocolor);
    //  int x = 0;
    //  for(int n = 0; n < 100000000; ++n)
    //  {
    //    x += board.see_check(ocolor, move.from_, move.to_, ki_pos, all_mask_inv, brq_mask);
    //  }
    //  move.seen_ = x != 0;
    //};
    //auto see_new_cbk = [](size_t i, NEngine::Board& board, NEngine::Move& move)
    //{
    //  auto color = board.getColor();
    //  auto ocolor = NEngine::Figure::otherColor(color);
    //  uint64 all_mask_inv = ~(board.fmgr().mask(NEngine::Figure::ColorBlack) | board.fmgr().mask(NEngine::Figure::ColorWhite));
    //  auto ki_pos = board.kingPos(ocolor);
    //  int x = 0;
    //  for(int n = 0; n < 100000000; ++n)
    //  {
    //    x += board.see_check_mbb(color, move.from_, move.to_, ki_pos, all_mask_inv);
    //  }
    //  move.seen_ = x != 0;
    //};
    //NEngine::testFen(argv[1],
    //                 see_new_cbk,
    //                 [](std::string const& err)
    //                 {
    //                   std::cout << "error: " << err << std::endl;
    //                 });
    return 0;
  }

  for(; xpr.doCmd(););

	return 0;
}

