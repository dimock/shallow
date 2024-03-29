#pragma once

#include "xcommon.h"
#include "xlist.h"
#include "Board.h"
#include "History.h"
#include "xalgorithm.h"

namespace NEngine
{

template <class BOARD, class MOVE>
struct UsualGenerator
{
  using MovesList = xlist<MOVE, BOARD::MovesMax>;
  
  UsualGenerator(BOARD const& board) :
    board_(board)
  {}


  MOVE* next()
  {
    while(iter_ != moves_.end())
    {
      auto* move = &*iter_;
      ++iter_;
      if(board_.validateMove(*move))
        return move;
    }
    return nullptr;
  }

  inline void add(int from, int to)
  {
    insert_sorted(moves_, MOVE{ from, to, Figure::TypeNone, history(board_.color(), from, to).score() });
  }
 
  inline void generateBishops(Figure::Color color, BitMask const mask_all, BitMask const mask_all_inv)
  {
    auto bmask = board_.fmgr().bishop_mask(color);
    for (; bmask;)
    {
      auto from = clear_lsb(bmask);
      auto mask = magic_ns::bishop_moves(from, mask_all) & mask_all_inv;
      for (; mask;)
      {
        auto to = clear_lsb(mask);
        add(from, to);
      }
    }
  }

  inline void generateRooks(Figure::Color color, BitMask const mask_all, BitMask const mask_all_inv)
  {
    auto rmask = board_.fmgr().rook_mask(color);
    for (; rmask;)
    {
      auto from = clear_lsb(rmask);
      auto mask = magic_ns::rook_moves(from, mask_all) & mask_all_inv;
      for (; mask;)
      {
        auto to = clear_lsb(mask);
        add(from, to);
      }
    }
  }

  inline void generateQueens(Figure::Color color, BitMask const mask_all, BitMask const mask_all_inv)
  {
    auto qmask = board_.fmgr().queen_mask(color);
    for (; qmask;)
    {
      auto from = clear_lsb(qmask);
      auto mask = magic_ns::queen_moves(from, mask_all) & mask_all_inv;
      for (; mask;)
      {
        auto to = clear_lsb(mask);
        add(from, to);
      }
    }
  }

  inline void generate()
  {
    const auto color = board_.color();
    const auto ocolor = Figure::otherColor(color);
    auto const& fmgr = board_.fmgr();
    auto mask_all = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
    auto mask_all_inv = ~mask_all;

    // pawns movements except promotions cause it's already generated by caps generator
    auto pw_mask = fmgr.pawn_mask(color) & ~movesTable().promote(color);
    if (color) {
      pw_mask <<= 8;
      pw_mask &= 0x00ffffffffffff00 & mask_all_inv;
      auto pw_mask0 = (pw_mask >> 8) & 0xff00;
      pw_mask0 <<= 16;
      pw_mask0 &= mask_all_inv;
      for (; pw_mask;)
      {
        auto to = clear_lsb(pw_mask);
        add(to - 8, to);
      }
      for (; pw_mask0;)
      {
        auto to = clear_lsb(pw_mask0);
        add(to - 16, to);
      }
    }
    else {
      pw_mask >>= 8;
      pw_mask &= 0x00ffffffffffff00 & mask_all_inv;
      auto pw_mask0 = (pw_mask << 8) & 0x00ff000000000000;
      pw_mask0 >>= 16;
      pw_mask0 &= mask_all_inv;
      for (; pw_mask;)
      {
        auto to = clear_lsb(pw_mask);
        add(to + 8, to);
      }
      for (; pw_mask0;)
      {
        auto to = clear_lsb(pw_mask0);
        add(to + 16, to);
      }
    }

    // knights movements
    auto kn_mask = fmgr.knight_mask(color);
    for(; kn_mask;)
    {
      auto kn_pos = clear_lsb(kn_mask);
      auto kn_caps = movesTable().caps(Figure::TypeKnight, kn_pos) & mask_all_inv;

      for(; kn_caps;)
      {
        auto to = clear_lsb(kn_caps);
        X_ASSERT(board_.getField(to), "try to generate capture");
        add(kn_pos, to);
      }
    }
    
    // bishops, rooks and queens movements
    generateBishops(color, mask_all, mask_all_inv);
    generateRooks(color, mask_all, mask_all_inv);
    generateQueens(color, mask_all, mask_all_inv);

    // kings movements
    auto const ki_pos  = board_.kingPos(color);
    auto const oki_pos = board_.kingPos(ocolor);
    auto ki_mask = movesTable().caps(Figure::TypeKing, ki_pos) & mask_all_inv & ~movesTable().caps(Figure::TypeKing, oki_pos);
    if(ki_mask)
    {
      // short castle
      if(board_.castling(color, 0) && (movesTable().castleMasks(color, 0) & mask_all) == 0ULL)
      {
        X_ASSERT(color && ki_pos != 4 ||  !color && ki_pos != 60, "invalid king position for castle");
        add(ki_pos, ki_pos+2);
      }

      // long castle
      if(board_.castling(color, 1) && (movesTable().castleMasks(color, 1) & mask_all) == 0ULL)
      {
        X_ASSERT(color && ki_pos != 4 ||  !color && ki_pos != 60, "invalid king position for castle");
        add(ki_pos, ki_pos-2);
      }

      for(; ki_mask;)
      {
        auto to = clear_lsb(ki_mask);
        X_ASSERT(board_.getField(to), "try to capture by king");
        add(ki_pos, to);
      }
    }

    iter_ = moves_.begin();
  }

  BOARD const& board_;
  MovesList moves_;
  typename MovesList::iterator iter_;
};

} // NEngine