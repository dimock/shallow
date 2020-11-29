#pragma once

#include <xcommon.h>
#include <xlist.h>
#include <Board.h>

namespace NEngine
{

template <class BOARD, class MOVE>
struct ChecksGenerator
{
  using MovesList = xlist<MOVE, NumOfFields>;

  ChecksGenerator(BOARD const& board) :
    board_(board)
  {}

  inline void add(int from, int to)
  {
    MOVE move{ from, to, Figure::TypeNone, history(board_.color(), from, to).score() };
    X_ASSERT(find(move), "ChecksGenerator. move exists");
    insert_sorted(moves_, move);
  }

  bool find(MOVE const& m)
  {
    for (auto i = moves_.begin(); i != moves_.end(); ++i) {
      if (*i == m)
        return true;
    }
    return false;
  }

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

  void generate()
  {
    BitMask visited{};
    const auto& color = board_.color();
    const auto ocolor = Figure::otherColor(color);
    auto const& fmgr = board_.fmgr();
    auto mask_all = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
    auto mask_all_inv = ~mask_all;
    auto const& oki_pos = board_.kingPos(ocolor);
    auto const pawns = fmgr.pawn_mask(color) & ~movesTable().promote(color);

    // pawns
    {
      auto pw_check_mask = movesTable().pawnCaps(ocolor, oki_pos) & mask_all_inv;
      for(; pw_check_mask;)
      {
        auto to = clear_lsb(pw_check_mask);
        const auto& tfield = board_.getField(to);
        if(tfield || to == board_.enpassant())
          continue;

        // usual moves
        auto pw_from = movesTable().pawnFrom(color, to) & pawns;
        visited |= pw_from;
        for(; pw_from;)
        {
          auto from = clear_lsb(pw_from);
          if(board_.is_something_between(from, to, mask_all_inv))
            continue;
          add(from, to);
          break;
        }
      }
    }

    // knights
    {
      auto const& knight_check_mask = movesTable().caps(Figure::TypeKnight, oki_pos);
      auto kn_mask = board_.fmgr().knight_mask(color);
      for(; kn_mask;)
      {
        auto from = clear_lsb(kn_mask);
        auto kn_moves = movesTable().caps(Figure::TypeKnight, from) & mask_all_inv;
        if(!board_.discoveredCheck(from, mask_all, color, oki_pos))
          kn_moves &= knight_check_mask;
        for(; kn_moves;)
        {
          auto to = clear_lsb(kn_moves);
          X_ASSERT(board_.getField(to), "field, from that we are going to check is occupied");
          add(from, to);
        }
      }
    }

    // king
    {
      auto const& ki_pos = board_.kingPos(color);
      auto all_but_king_mask = mask_all_inv | set_mask_bit(ki_pos);
      bool castle = false;
      // short castle
      if(board_.castling(color, 0) && (movesTable().castleMasks(color, 0) & mask_all) == 0ULL)
      {
        static int rook_positions[] = { 61, 5 };
        auto const& r_pos = rook_positions[board_.color()];
        X_ASSERT(color && ki_pos != 4 ||  !color && ki_pos != 60, "invalid king position for castle");
        X_ASSERT(color && board_.getField(7).type() != Figure::TypeRook
                  || !color && board_.getField(63).type() != Figure::TypeRook, "no rook for castling, but castle is possible");
        if((oki_pos&7) == (r_pos&7) && board_.is_nothing_between(r_pos, oki_pos, mask_all_inv))
        {
          add(ki_pos, ki_pos+2);
          castle = true;
        }
      }

      // long castle
      if(!castle && board_.castling(color, 1) && (movesTable().castleMasks(color, 1) & mask_all) == 0ULL)
      {
        static int rook_positions[] = { 59, 3 };
        auto const& r_pos = rook_positions[board_.color()];
        X_ASSERT(color && ki_pos != 4 ||  !color && ki_pos != 60, "invalid king position for castle");
        X_ASSERT(color && board_.getField(0).type() != Figure::TypeRook
                  || !color && board_.getField(56).type() != Figure::TypeRook, "no rook for castling, but castle is possible");
        if((oki_pos&7) == (r_pos&7) && board_.is_nothing_between(r_pos, oki_pos, mask_all_inv))
        {
          add(ki_pos, ki_pos-2);
          castle = true;
        }
      }

      if(!castle && board_.discoveredCheck(ki_pos, mask_all, color, oki_pos))
      {
        auto exclude = ~(betweenMasks().from(oki_pos, ki_pos) | movesTable().caps(Figure::TypeKing, oki_pos));
        auto ki_mask = movesTable().caps(Figure::TypeKing, ki_pos) & mask_all_inv & exclude;
        for(; ki_mask;)
        {
          auto to = clear_lsb(ki_mask);
          X_ASSERT(board_.getField(to), "king moves to occupied field");
          add(ki_pos, to);
        }
      }
    }

    auto bi_king = magic_ns::bishop_moves(oki_pos, mask_all);
    auto r_king = magic_ns::rook_moves(oki_pos, mask_all);

    // discovers bishop|queen attack
    {
      auto pw_mask = bi_king & pawns;
      X_ASSERT(pw_mask & visited, "pawn gives check and discovers too");
      auto r_mask = bi_king & fmgr.rook_mask(color);
      if(pw_mask | r_mask)
      {
        visited |= pw_mask;
        // all checking queens and bishops if exclude pawns and rooks
        auto bq_from = magic_ns::bishop_moves(oki_pos, mask_all & ~(pw_mask | r_mask)) & (fmgr.bishop_mask(color) | fmgr.queen_mask(color));
        for(; bq_from;)
        {
          auto p = clear_lsb(bq_from);
          auto const& btw_mask = betweenMasks().between(p, oki_pos);
          auto pwm = btw_mask & pw_mask;
          if(pwm)
          {
            auto from = clear_lsb(pwm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "pawn should discoved check");
            const auto* to = movesTable().pawn(color, from) + 2; // skip captures
            for(; *to >= 0 && !board_.getField(*to); ++to)
              add(from, *to);
          }
          auto rm = btw_mask & r_mask;
          visited |= rm;
          if(rm)
          {
            auto from = clear_lsb(rm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "rook should discover check");
            generateRooks(from, mask_all, mask_all_inv);
          }
        }
      }
    }

    // discovers rook|queen attack
    {
      auto pw_mask = r_king & pawns & ~visited;
      auto bi_mask = r_king & fmgr.bishop_mask(color);
      if(pw_mask | bi_mask)
      {
        auto rq_from = magic_ns::rook_moves(oki_pos, mask_all & ~(pw_mask | bi_mask)) & (fmgr.rook_mask(color) | fmgr.queen_mask(color));
        for(; rq_from;)
        {
          auto p = clear_lsb(rq_from);
          auto const& btw_mask = betweenMasks().between(oki_pos, p);
          auto pwm = btw_mask & pw_mask;
          if(pwm)
          {
            auto from = clear_lsb(pwm);
            // pawn on the same X as king
            if((from&7) == (oki_pos&7))
              continue;
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "pawn should discover check");
            const auto* to = movesTable().pawn(color, from) + 2; // skip captures
            for(; *to >= 0 && !board_.getField(*to); ++to)
              add(from, *to);
          }
          auto bm = btw_mask & bi_mask;
          visited |= bm;
          if(bm)
          {
            auto from = clear_lsb(bm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "bishop should discover check");
            generateBishops(from, mask_all, mask_all_inv);
          }
        }
      }
    }

    // don't change visited any more
    visited = ~visited;

    // remaining direct attacks
    // queen
    {
      auto q_mask = fmgr.queen_mask(color);
      for(; q_mask;)
      {
        auto from = clear_lsb(q_mask);
        auto q_moves = magic_ns::queen_moves(from, mask_all) & mask_all_inv & (r_king | bi_king);
        for(; q_moves;)
        {
          auto to = clear_lsb(q_moves);
          X_ASSERT(board_.getField(to), "queen goes to occupied field");
          add(from, to);
        }
      }
    }
    // bishop
    {
      auto bi_mask = fmgr.bishop_mask(color) & visited;
      for(; bi_mask;)
      {
        auto from = clear_lsb(bi_mask);
        auto bi_moves = magic_ns::bishop_moves(from, mask_all) & mask_all_inv & bi_king;
        for(; bi_moves;)
        {
          auto to = clear_lsb(bi_moves);
          X_ASSERT(board_.getField(to), "bishop goes to occupied field");
          add(from, to);
        }
      }
    }
    // rook
    {
      auto r_mask = fmgr.rook_mask(color) & visited;
      for(; r_mask;)
      {
        auto from = clear_lsb(r_mask);
        auto rr = magic_ns::rook_moves(from, mask_all);
        auto r_moves = rr & mask_all_inv & r_king;
        for(; r_moves;)
        {
          auto to = clear_lsb(r_moves);
          X_ASSERT(board_.getField(to), "rook goes to occupied field");
          add(from, to);
        }
      }
    }

    iter_ = moves_.begin();
  }

  inline void generateBishops(int from, BitMask const& mask_all, BitMask const& mask_all_inv)
  {
    auto bmask = magic_ns::bishop_moves(from, mask_all) & mask_all_inv;
    for (; bmask;)
    {
      auto to = clear_lsb(bmask);
      add(from, to);
    }
  }

  inline void generateRooks(int from, BitMask const& mask_all, BitMask const& mask_all_inv)
  {
    auto rmask = magic_ns::rook_moves(from, mask_all) & mask_all_inv;
    for (; rmask;)
    {
      auto to = clear_lsb(rmask);
      add(from, to);
    }
  }

  BOARD const& board_;
  MovesList moves_;
  typename MovesList::iterator iter_;
}; // ChecksGenerator

} // NEngine