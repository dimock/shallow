#pragma once

#include <xcommon.h>
#include <xlist.h>
#include <Board.h>
#include <History.h>
#include <xalgorithm.h>

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


    //for(;;)
    //{
    //  auto it = std::max_element(moves_.begin(), moves_.end());
    //  if(it == moves_.end())
    //    break;
    //  auto* move = &*it;
    //  moves_.erase(it);
    //  if(board_.validateMove(*move))
    //    return move;
    //}
    return nullptr;
  }

  inline void add(int from, int to)
  {
    //moves_.emplace_back(from, to);
    //moves_.back().sort_value = history(board_.color(), from, to).score();
    insert_sorted(moves_, MOVE{ from, to, Figure::TypeNone, history(board_.color(), from, to).score() });
  }

  inline void generate(int type, Figure::Color color, BitMask const& mask_all_inv)
  {
    auto fg_mask = board_.fmgr().type_mask((Figure::Type)type, color);
    for(; fg_mask;)
    {
      auto fg_pos = clear_lsb(fg_mask);
      const auto* table = movesTable().move(type-Figure::TypeBishop, fg_pos);
      for(; *table; ++table)
      {
        const auto* packed = reinterpret_cast<const int8*>(table);
        auto count = packed[0];
        auto delta = packed[1];
        auto p = fg_pos;
        for(; count; --count)
        {
          p += delta;
          const auto& field = board_.getField(p);
          if(field)
            break;
          add(fg_pos, p);
        }
      }
    }
  }

  inline void generate()
  {
    const auto& color = board_.color();
    const auto ocolor = Figure::otherColor(color);
    auto const& fmgr = board_.fmgr();
    auto mask_all = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
    auto mask_all_inv = ~mask_all;

    // pawns movements except promotions cause it's already generated by caps generator
    auto pw_mask = fmgr.pawn_mask(color) & ~movesTable().promote(color);
    for(; pw_mask;)
    {
      auto pw_pos = clear_lsb(pw_mask);
      const auto* table = movesTable().pawn(color, pw_pos) + 2; // skip captures
      for(; *table >= 0 && !board_.getField(*table); ++table)
        add(pw_pos, *table);
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
    generate(Figure::TypeBishop, color, mask_all_inv);
    generate(Figure::TypeRook, color, mask_all_inv);
    generate(Figure::TypeQueen, color, mask_all_inv);

    // kings movements
    auto const& ki_pos  = board_.kingPos(color);
    auto const& oki_pos = board_.kingPos(ocolor);
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