/*************************************************************
  MovesGenerator.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Board.h>
#include <xlist.h>
#include <algorithm>
#include <MovesTable.h>
#include <fstream>
#include <CapsGenerator.h>
#include <ChecksGenerator.h>
#include <EscapeGenerator.h>
#include <UsualGenerator.h>
#include <engine.h>

namespace NEngine
{

template <class BOARD, class MOVE, int NUM = BOARD::MovesMax>
xlist<MOVE, NUM> generate(BOARD const& board)
{
  xlist<MOVE, NUM> moves;
  const auto& color = board.color();
  const auto ocolor = Figure::otherColor(color);
  auto const& fmgr = board.fmgr();

  if(!board.underCheck() || !board.doubleCheck())
  {
    // pawns movements
    if(auto pw_mask = fmgr.pawn_mask(color))
    {
      for(; pw_mask;)
      {
        auto pw_pos = clear_lsb(pw_mask);
        const auto* table = movesTable().pawn(color, pw_pos);
        for(int i = 0; i < 2; ++i, ++table)
        {
          if(*table < 0)
            continue;
          const auto& field = board.getField(*table);
          bool capture = false;
          if((field && field.color() == ocolor) ||
             (board.enpassant() > 0 && *table == board.enpassant()))
          {
            capture = true;
          }
          if(!capture)
            continue;
          bool promotion = *table > 55 || *table < 8;
          MOVE move{ pw_pos, *table, Figure::TypeNone };
          // promotion
          if(*table > 55 || *table < 8)
          {
            moves.emplace_back(pw_pos, *table, Figure::TypeQueen);
            moves.emplace_back(pw_pos, *table, Figure::TypeRook);
            moves.emplace_back(pw_pos, *table, Figure::TypeBishop);
            moves.emplace_back(pw_pos, *table, Figure::TypeKnight);
          }
          else
          {
            moves.emplace_back(pw_pos, *table);
          }
        }

        for(; *table >= 0 && !board.getField(*table); ++table)
        {
          // promotion
          if(*table > 55 || *table < 8)
          {
            moves.emplace_back(pw_pos, *table, Figure::TypeQueen);
            moves.emplace_back(pw_pos, *table, Figure::TypeRook);
            moves.emplace_back(pw_pos, *table, Figure::TypeBishop);
            moves.emplace_back(pw_pos, *table, Figure::TypeKnight);
          }
          else
          {
            moves.emplace_back(pw_pos, *table);
          }
        }
      }
    }

    // knights movements
    if(fmgr.knight_mask(color))
    {
      auto kn_mask = fmgr.knight_mask(color);
      for(; kn_mask;)
      {
        auto kn_pos = clear_lsb(kn_mask);
        const auto* table = movesTable().knight(kn_pos);
        for(; *table >= 0; ++table)
        {
          const auto& field = board.getField(*table);
          bool capture = false;
          if(field)
          {
            if(field.color() == color)
              continue;
            capture = true;
          }
          moves.emplace_back(kn_pos, *table);
        }
      }
    }

    // bishops, rooks and queens movements
    for(int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      auto fg_mask = fmgr.type_mask((Figure::Type)type, color);
      for(; fg_mask;)
      {
        auto fg_pos = clear_lsb(fg_mask);
        const auto* table = movesTable().move(type-Figure::TypeBishop, fg_pos);
        for(; *table; ++table)
        {
          const auto* packed = reinterpret_cast<const int8*>(table);
          auto count = packed[0];
          auto const& delta = packed[1];
          auto p = fg_pos;
          bool capture = false;
          for(; count && !capture; --count)
          {
            p += delta;
            const auto & field = board.getField(p);
            if(field)
            {
              if(field.color() == color)
                break;
              capture = true;
            }
            moves.emplace_back(fg_pos, p);
          }
        }
      }
    }
  }

  // kings movements
  {
    auto ki_mask = fmgr.king_mask(color);
    X_ASSERT(ki_mask == 0, "invalid position - no king");
    auto ki_pos = clear_lsb(ki_mask);
    const auto* table = movesTable().king(ki_pos);
    for(; *table >= 0; ++table)
    {
      const auto & field = board.getField(*table);
      bool capture = false;
      if(field)
      {
        if(field.color() == color)
          continue;
        capture = true;
      }
      moves.emplace_back(ki_pos, *table);
    }

    if(!board.underCheck())
    {
      // short castle
      if(board.castling(board.color(), 0) && !board.getField(ki_pos+2) && !board.getField(ki_pos+1))
        moves.emplace_back(ki_pos, ki_pos+2);

      // long castle
      if(board.castling(board.color(), 1) && !board.getField(ki_pos-2) && !board.getField(ki_pos-1) && !board.getField(ki_pos-3))
        moves.emplace_back(ki_pos, ki_pos-2);
    }
  }

  return moves;
}

template <class BOARD, class MOVE>
struct FastGenerator
{
  using MovesList  = xlist<MOVE, NumOfFields>;
  using MovesListU = xlist<MOVE, 128>;

  enum Order { oEscape, oHash, oGenCaps, oCaps, oKiller, oGenUsual, oUsual, oWeak, oWeakUsual } order_{ oEscape };

  FastGenerator(BOARD const& board, MOVE const& hmove, MOVE const& killer) :
    board_(board),
    cg_(board), ug_(board), eg_(board, hmove, killer),
    hmove_(hmove),
    killer_(killer)
  {
    if(killer_ == hmove_)
      killer_ = MOVE{ true };
    if(!board.underCheck())
      order_ = oHash;
  }

  MOVE* next()
  {
    if(order_ == oEscape)
    {
      return eg_.next_see();
    }
    if(order_ == oHash)
    {
      order_ = oGenCaps;
      if(hmove_)
      {
        hmove_.set_ok();
        return &hmove_;
      }
    }
    if(order_ == oGenCaps)
    {
      cg_.generateCaps();
      order_ = oCaps;
    }
    if(order_ == oCaps)
    {
      while(auto* move = cg_.next())
      {
        if(*move == hmove_)
          continue;
        if(board_.see(*move, 0))
        {
          move->set_ok();
          return move;
        }
        weak_.push_back(*move);
      }
      order_ = killer_ ? oKiller : oGenUsual;
    }
    if(order_ == oKiller)
    {
      X_ASSERT(!killer_, "try to make do non-exsisting killer");
      order_ = oGenUsual;
      bool capture = killer_.new_type() || board_.getField(killer_.to())
        || (killer_.to() > 0 && board_.enpassant() == killer_.to() && board_.getField(killer_.from()).type() == Figure::TypePawn);
      if(!capture && board_.possibleMove(killer_) && board_.validateMove(killer_))
      {
        X_ASSERT(!board_.moveExists(killer_), "non-existing killer");
        killer_.set_ok();
        return &killer_;
      }
      X_ASSERT(!capture && board_.moveExists(killer_), "killer was not detected as valid move");
    }
    if(order_ == oGenUsual)
    {
      ug_.generate();
      order_ = oUsual;
    }
    if(order_ == oUsual)
    {
      while(auto* move = ug_.next())
      {
        if(*move == hmove_ || *move == killer_)
          continue;
        if(board_.see(*move, 0))
        {
          move->set_ok();
          return move;
        }
        weakUsual_.push_back(*move);
      }
      order_ = oWeak;
      iter_ = weak_.begin();
    }
    if(order_ == oWeak)
    {
      while(iter_ != weak_.end())
      {
        auto* move = &*iter_;
        X_ASSERT(*move == hmove_, "do move from hash second time");
        ++iter_;
        X_ASSERT(!board_.validateMove(*move), "invalid weak move");
        return move;
      }
      order_ = oWeakUsual;
      iteru_ = weakUsual_.begin();
    }
    if(order_ == oWeakUsual)
    {
      while(iteru_ != weakUsual_.end())
      {
        auto* move = &*iteru_;
        X_ASSERT(*move == hmove_, "do move from hash second time");
        ++iteru_;
        X_ASSERT(!board_.validateMove(*move), "invalid weak move");
        return move;
      }
    }
    return nullptr;
  }

  CapsGenerator<BOARD, MOVE> cg_;
  UsualGenerator<BOARD, MOVE> ug_;
  EscapeGenerator<BOARD, MOVE> eg_;
  MovesList  weak_;
  MovesListU weakUsual_;
  typename MovesList::iterator iter_;
  typename MovesListU::iterator iteru_;
  BOARD const& board_;
  MOVE hmove_;
  MOVE killer_;
};

template <class BOARD, class MOVE>
struct TacticalGenerator
{
  using MovesList = xlist<MOVE, BOARD::MovesMax>;

  enum Order { oEscape, oHash, oGenCaps, oCaps, oGenChecks, oChecks } order_{ oEscape };

  TacticalGenerator(BOARD const& board, MOVE const& hmove, int depth, bool fpr) :
    board_(board),
    cg_(board), ckg_(board), eg_(board, hmove),
    hmove_(hmove),
    depth_(depth),
    fpr_{ fpr }
  {
    if(!board.underCheck())
      order_ = oHash;
  }

  MOVE* next()
  {
    if(order_ == oEscape)
    {
      return eg_.next();
    }
    if(order_ == oHash)
    {
      order_ = oGenCaps;
      if(hmove_)
      {
        hmove_.set_ok();
        return &hmove_;
      }
    }
    if(order_ == oGenCaps)
    {
      cg_.generateCaps();
      order_ = oCaps;
    }
    if(order_ == oCaps)
    {
      while(auto* move = cg_.next())
      {
        if(*move == hmove_)
          continue;
        return move;
      }
      order_ = oGenChecks;
    }
    if(order_ == oGenChecks)
    {
      if (depth_ >= 0)
        ckg_.generate();
      else
        ckg_.generateOne(false);
      order_ = oChecks;
    }
    if(order_ == oChecks)
    {
      while(auto* move = ckg_.next())
      {
        if(*move == hmove_)
          continue;
        return move;
      }
    }
    return nullptr;
  }

  CapsGenerator<BOARD, MOVE> cg_;
  ChecksGenerator<BOARD, MOVE> ckg_;
  EscapeGenerator<BOARD, MOVE> eg_;
  BOARD const& board_;
  MOVE hmove_;
  int depth_{};
  bool fpr_{};
};

} // NEngine
