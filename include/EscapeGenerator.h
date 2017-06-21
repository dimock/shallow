#pragma once

#include <xcommon.h>
#include <xlist.h>
#include <Board.h>
#include <xalgorithm.h>

namespace NEngine
{

template <class BOARD, class MOVE>
struct EscapeGenerator
{
  using MovesList = xlist<MOVE, NumOfFields>;

  enum Order { oGenCaps, oCaps, oGenUsual, oUsual, oWeak } order_{};

  EscapeGenerator(BOARD const& board) :
    board_(board)
  {
    ocolor = Figure::otherColor(board_.color());
    const auto& black = board_.fmgr().mask(Figure::ColorBlack);
    const auto& white = board_.fmgr().mask(Figure::ColorWhite);
    mask_all = white | black;
  }

  inline void add_caps(int from, int to, Figure::Type new_type)
  {
//    caps_.emplace_back(from, to, new_type);
    insert_sorted(caps_, MOVE{ from, to, new_type, board_.sortValueOfCap(from, to, new_type) });
  }

  inline void add_usual(int from, int to)
  {
//    usual_.emplace_back(from, to);
    insert_sorted(usual_, MOVE{ from, to, Figure::TypeNone, history(board_.color(), from, to).score() });
  }

  inline void generateCaps()
  {
    const auto& color = board_.color();
    protect_king_msk_ = betweenMasks().between(board_.kingPos(color), board_.checking());
    auto const& ch_pos = board_.checking();
    auto const& fmgr = board_.fmgr();

    X_ASSERT((unsigned)board_.checking() > 63, "there is no checking figure");

    // 1st - pawns
    const auto& pawn_msk = fmgr.pawn_mask(color);
    const auto& opawn_caps = movesTable().pawnCaps(ocolor, ch_pos);
    auto eat_msk = pawn_msk & opawn_caps;

    bool promotion = ch_pos > 55 || ch_pos < 8; // 1st || last line
    for(; eat_msk;)
    {
      auto n = clear_lsb(eat_msk);

      const auto& fpawn = board_.getField(n);
      X_ASSERT(!fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != color, "no pawn on field we are going to do capture from");

      if(promotion)
      {
        add_caps(n, ch_pos, Figure::TypeQueen);

        // only checking knight
        if((movesTable().caps(Figure::TypeKnight, ch_pos) & fmgr.king_mask(ocolor)))
          add_caps(n, ch_pos, Figure::TypeKnight);
      }
      else
        add_caps(n, ch_pos, Figure::TypeNone);
    }

    if(board_.enpassant() > 0)
    {
      auto ep_pos = board_.enpassantPos();
      if((ch_pos == ep_pos) || (protect_king_msk_ & set_mask_bit(board_.enpassant())))
      {
        X_ASSERT(!board_.getField(ep_pos) || board_.getField(ep_pos).type() != Figure::TypePawn, "en-passant pown doesnt exist");
        X_ASSERT(board_.getField(ch_pos).type() != Figure::TypePawn, "en-passant pawn is on checking position but checking figure type is not pawn");
        const auto& opawn_caps_ep = movesTable().pawnCaps(ocolor, board_.enpassant());
        auto eat_msk_ep = pawn_msk & opawn_caps_ep;
        for(; eat_msk_ep;)
        {
          auto n = clear_lsb(eat_msk_ep);

          const auto& fpawn = board_.getField(n);
          X_ASSERT(!fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != color, "no pawn on field we are going to do capture from");

          add_caps(n, board_.enpassant(), Figure::TypeNone);
        }
      }
    }

    // Pawns promotions
    auto pw_mask = board_.fmgr().pawn_mask(color) & movesTable().promote(color);
    for(; pw_mask;)
    {
      auto pw_pos = clear_msb(pw_mask);

      // +2 - skip captures
      const auto* table = movesTable().pawn(color, pw_pos) + 2;
      for(; *table >= 0 && !board_.getField(*table); ++table)
      {
        if((protect_king_msk_ & set_mask_bit(*table)) == 0)
          continue;

        X_ASSERT(*table <= 55 && *table >= 8, "not a promotion move");
        add_caps(pw_pos, *table, Figure::TypeQueen);

        // add promotion to knight only if it gives check
        if(movesTable().caps(Figure::TypeKnight, ch_pos) & fmgr.king_mask(ocolor))
          add_caps(pw_pos, *table, Figure::TypeKnight);
      }
    }

    // 2nd - knight's captures
    {
      const auto& knight_caps = movesTable().caps(Figure::TypeKnight, ch_pos);
      const auto& knight_msk = fmgr.knight_mask(color);
      auto eat_msk = knight_msk & knight_caps;
      for(; eat_msk;)
      {
        auto n = clear_lsb(eat_msk);

        const auto& fknight = board_.getField(n);
        X_ASSERT(!fknight || fknight.type() != Figure::TypeKnight || fknight.color() != color, "no knight on field we are going to do capture from");

        add_caps(n, ch_pos, Figure::TypeNone);
      }
    }

    // 3rd - bishops, rooks and queens
    {
      auto const& bi_moves = magic_ns::bishop_moves(ch_pos, mask_all);
      auto bi_mask = bi_moves & fmgr.bishop_mask(color);
      for(; bi_mask;)
      {
        auto n = clear_lsb(bi_mask);

        const auto& field = board_.getField(n);
        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypeBishop, "no bishop on field we are going to do capture from");

        add_caps(n, ch_pos, Figure::TypeNone);
      }
      auto const& r_moves = magic_ns::rook_moves(ch_pos, mask_all);
      auto r_mask = r_moves & fmgr.rook_mask(color);
      for(; r_mask;)
      {
        auto n = clear_lsb(r_mask);

        const auto& field = board_.getField(n);
        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypeRook, "no rook on field we are going to do capture from");

        add_caps(n, ch_pos, Figure::TypeNone);
      }

      auto q_mask = (bi_moves | r_moves) & fmgr.queen_mask(color);
      for(; q_mask;)
      {
        auto n = clear_lsb(q_mask);

        const auto& field = board_.getField(n);
        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypeQueen, "no queen on field we are going to do capture from");

        add_caps(n, ch_pos, Figure::TypeNone);
      }
    }
  }

  // now try to protect king - put something between it and checking figure
  inline void generateUsual()
  {
    // checking figure position and type
    auto const& ch_type = board_.getField(board_.checking()).type();
    X_ASSERT(!ch_type, "there is no checking figure");
    X_ASSERT(ch_type == Figure::TypeKing, "king is attacking king");

    if(Figure::TypePawn == ch_type || Figure::TypeKnight == ch_type || !protect_king_msk_)
      return;

    auto const& color = board_.color();
    auto const& fmgr = board_.fmgr();

    // 1. Pawns. exclude promotions and captures
    auto pw_mask = fmgr.pawn_mask(color) & ~movesTable().promote(color);
    for(; pw_mask;)
    {
      auto pw_pos = clear_msb(pw_mask);

      // +2 - skip captures
      const auto* table = movesTable().pawn(color, pw_pos) + 2;
      for(; *table >= 0 && !board_.getField(*table); ++table)
      {
        if((protect_king_msk_ & set_mask_bit(*table)) == 0)
          continue;

        X_ASSERT(*table > 55 || *table < 8, "pawn move should not be a promotion");
        add_usual(pw_pos, *table);
      }
    }


    // 2. Knights
    auto kn_mask = fmgr.knight_mask(color);
    for(; kn_mask;)
    {
      auto kn_pos = clear_msb(kn_mask);
      const auto& knight_msk = movesTable().caps(Figure::TypeKnight, kn_pos);
      auto msk_protect = protect_king_msk_ & knight_msk;
      for(; msk_protect;)
      {
        auto n = clear_lsb(msk_protect);

        const auto& field = board_.getField(n);
        X_ASSERT(field, "there is something between king and checking figure");

        add_usual(kn_pos, n);
      }
    }

    // 3. Bishops
    {
      auto bi_mask = fmgr.bishop_mask(color);
      for(; bi_mask;)
      {
        auto from = clear_lsb(bi_mask);
        auto bi_moves = magic_ns::bishop_moves(from, mask_all);
        auto bi_protect = protect_king_msk_ & bi_moves;
        for(; bi_protect;)
        {
          auto to = clear_lsb(bi_protect);

          const auto& field = board_.getField(to);
          X_ASSERT(field, "there is something between king and checking figure");

          add_usual(from, to);
        }
      }
    }

    // 4. Rooks
    {
      auto r_mask = fmgr.rook_mask(color);
      for(; r_mask;)
      {
        auto from = clear_lsb(r_mask);
        auto const& r_moves = magic_ns::rook_moves(from, mask_all);
        auto r_protect = protect_king_msk_ & r_moves;
        for(; r_protect;)
        {
          auto to = clear_lsb(r_protect);

          const auto& field = board_.getField(to);
          X_ASSERT(field, "there is something between king and checking figure");

          add_usual(from, to);
        }
      }
    }

    // 5. Queens
    {
      auto q_mask = fmgr.queen_mask(color);
      for(; q_mask;)
      {
        auto from = clear_lsb(q_mask);
        auto q_moves = magic_ns::queen_moves(from, mask_all);
        auto q_protect = protect_king_msk_ & q_moves;
        for(; q_protect;)
        {
          auto to = clear_lsb(q_protect);

          const auto& field = board_.getField(to);
          X_ASSERT(field, "there is something between king and checking figure");

          add_usual(from, to);
        }
      }
    }
  }

  inline void generateKingCaps()
  {
    auto const& color = board_.color();
    const auto& o_mask = board_.fmgr().mask(ocolor);

    // captures
    auto ki_mask = movesTable().caps(Figure::TypeKing, board_.kingPos(color)) & o_mask;
    for(; ki_mask;)
    {
      auto to = clear_lsb(ki_mask);

      auto const& field = board_.getField(to);
      X_ASSERT(!field || field.color() == color, "escape generator: try to put king to occupied field");
      add_caps(board_.kingPos(color), to, Figure::TypeNone);
    }
  }

  inline void generateKingUsual()
  {
    auto const& color = board_.color();
    const auto& mask = board_.fmgr().mask(color);
    const auto& o_mask = board_.fmgr().mask(ocolor);

    // usual moves
    auto ki_mask = movesTable().caps(Figure::TypeKing, board_.kingPos(color))
      & ~(mask_all | movesTable().caps(Figure::TypeKing, board_.kingPos(ocolor)));
    for(; ki_mask;)
    {
      auto to = clear_lsb(ki_mask);

      X_ASSERT(board_.getField(to), "escape generator: try to put king to occupied field");
      add_usual(board_.kingPos(color), to);
    }
  }

  MOVE* next()
  {
    if(order_ == oGenCaps)
    {
      if(!board_.doubleCheck())
        generateCaps();
      generateKingCaps();
      order_ = oCaps;
      iter_ = caps_.begin();
    }
    if(order_ == oCaps)
    {
      while(iter_ != caps_.end())
      {
        auto* move = &*iter_;
        ++iter_;
        if(board_.validateMove(*move))
          return move;
      }
      order_ = oGenUsual;
    }
    if(order_ == oGenUsual)
    {
      if(!board_.doubleCheck())
        generateUsual();
      generateKingUsual();
      order_ = oUsual;
      iter_ = usual_.begin();
    }
    if(order_ == oUsual)
    {
      while(iter_ != usual_.end())
      {
        auto* move = &*iter_;
        ++iter_;
        if(board_.validateMove(*move))
          return move;
      }
    }
    return nullptr;
  }

  MOVE* next_see()
  {
    if(order_ == oGenCaps)
    {
      if(!board_.doubleCheck())
        generateCaps();
      generateKingCaps();
      order_ = oCaps;
      iter_ = caps_.begin();
    }
    if(order_ == oCaps)
    {
      while(iter_ != caps_.end())
      {
        auto* move = &*iter_;
        ++iter_;
        if(board_.validateMove(*move))
        {
          if(board_.see(*move, 0))
          {
            move->set_ok();
            return move;
          }
          weak_.push_back(*move);
        }
      }
      order_ = oGenUsual;
    }
    if(order_ == oGenUsual)
    {
      if(!board_.doubleCheck())
        generateUsual();
      generateKingUsual();
      order_ = oUsual;
      iter_ = usual_.begin();
    }
    if(order_ == oUsual)
    {
      while(iter_ != usual_.end())
      {
        auto* move = &*iter_;
        ++iter_;
        if(board_.validateMove(*move))
          return move;
      }
      order_ = oWeak;
      iter_ = weak_.begin();
    }
    if(order_ == oWeak)
    {
      while(iter_ != weak_.end())
      {
        auto* move = &*iter_;
        ++iter_;
        X_ASSERT(!board_.validateMove(*move), "invalid weak move");
        return move;
      }
    }
    return nullptr;
  }

  BOARD const& board_;
  MovesList caps_;
  MovesList usual_;
  MovesList weak_;
  typename MovesList::iterator iter_;
  BitMask protect_king_msk_;
  BitMask mask_all;
  Figure::Color ocolor;
}; // EscapeGenerator

} // NEngine
