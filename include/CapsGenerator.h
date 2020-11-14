#pragma once

#include <xcommon.h>
#include <xlist.h>
#include <Board.h>
#include <xalgorithm.h>

namespace NEngine
{

template <class BOARD, class MOVE>
struct CapsGenerator
{
  using MovesList = xlist<MOVE, NumOfFields>;

  CapsGenerator(BOARD const& board) :
    board_(board)
  {}

  inline void add(int from, int to, Figure::Type new_type)
  {
    insert_sorted(moves_, MOVE{ from, to, new_type, board_.sortValueOfCap(from, to, new_type) });
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

  inline void generateCaps()
  {
    const auto color = board_.color();
    const auto ocolor = Figure::otherColor(color);

    auto const& fmgr = board_.fmgr();
    auto opponent_mask = fmgr.mask(ocolor) ^ fmgr.king_mask(ocolor);
    const auto& black = fmgr.mask(Figure::ColorBlack);
    const auto& white = fmgr.mask(Figure::ColorWhite);
    auto mask_all = white | black;
    auto const& ki_pos  = board_.kingPos(color);
    auto const& oki_pos = board_.kingPos(ocolor);

    // generate pawn promotions
    const auto& pawn_msk = fmgr.pawn_mask(color);
    {
      static int pw_delta[] = { -8, +8 };
      auto promo_msk = movesTable().promote(color);
      promo_msk &= pawn_msk;

      for(; promo_msk;)
      {
        auto from = clear_lsb(promo_msk);

        X_ASSERT((unsigned)from > 63, "invalid promoted pawn's position");

        const auto& field = board_.getField(from);

        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypePawn, "there is no pawn to promote");

        auto to = from + pw_delta[color];

        X_ASSERT((unsigned)to > 63, "pawn tries to go to invalid field");

        if(board_.getField(to))
          continue;

        add(from, to, Figure::TypeQueen);

        // add promotion to checking knight
        if(figureDir().dir(Figure::TypeKnight, color, to, oki_pos) >= 0)
          add(from, to, Figure::TypeKnight);
      }
    }

    // firstly check if we have at least 1 attacking pawn
    bool pawns_eat = pawn_msk != 0ULL;
    if(pawns_eat)
    {
      BitMask pawn_eat_msk{};
      if(color)
        pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
      else
        pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
      pawns_eat = (pawn_eat_msk & opponent_mask) != 0ULL;
      if(!pawns_eat && board_.enpassant() > 0)
        pawns_eat = (pawn_eat_msk & set_mask_bit(board_.enpassant())) != 0ULL;
    }
    // generate captures

    // 1. Pawns
    if(pawns_eat)
    {
      auto pw_mask = fmgr.pawn_mask(color);
      auto promo_mask = pw_mask & movesTable().promote(color);
      pw_mask ^= promo_mask;
      for(; promo_mask;)
      {
        auto pw_pos = clear_lsb(promo_mask);
        auto p_caps = movesTable().pawnCaps(color, pw_pos) & opponent_mask;
        for(; p_caps;)
        {
          X_ASSERT(!pawns_eat, "have pawns capture, but not detected by mask");

          auto to = clear_lsb(p_caps);

          X_ASSERT(!(to > 55 || to < 8), "not a promotion"); // 1st || last line
          X_ASSERT((unsigned)to > 63, "invalid pawn's capture position");
          X_ASSERT(!board_.getField(to) || board_.getField(to).color() != ocolor, "no figure for pawns capture");
          X_ASSERT(board_.getField(to).type() == Figure::TypeKing || board_.getField(to).type() == Figure::TypePawn,
                   "captured figure should not be king|pawn");

          add(pw_pos, to, Figure::TypeQueen);

          // add promotion to checking knight
          if(figureDir().dir(Figure::TypeKnight, color, to, oki_pos) >= 0)
            add(pw_pos, to, Figure::TypeKnight);
        }
      }

      for(; pw_mask;)
      {
        auto pw_pos = clear_lsb(pw_mask);

        auto p_caps = movesTable().pawnCaps(color, pw_pos) & opponent_mask;
        for(; p_caps;)
        {
          X_ASSERT(!pawns_eat, "have pawns capture, but not detected by mask");

          auto to = clear_lsb(p_caps);

          X_ASSERT(to > 55 || to < 8, "promotion"); // 1st || last line
          X_ASSERT((unsigned)to > 63, "invalid pawn's capture position");
          X_ASSERT(!board_.getField(to) || board_.getField(to).color() != ocolor, "no figure for pawns capture");
          X_ASSERT(board_.getField(to).type() == Figure::TypeKing, "captured figure should not be king");

          add(pw_pos, to, Figure::TypeNone);
        }
      }

      if(board_.enpassant() > 0)
      {
        X_ASSERT(board_.getField(board_.enpassantPos()).type() != Figure::TypePawn || board_.getField(board_.enpassantPos()).color() != ocolor, "there is no en passant pawn");
        auto ep_mask = movesTable().pawnCaps(ocolor, board_.enpassant()) & fmgr.pawn_mask(color);

        for(; ep_mask;)
        {
          auto from = clear_lsb(ep_mask);
          add(from, board_.enpassant(), Figure::TypeNone);
        }
      }
    }

    // 2. Knights
    auto kn_mask = fmgr.knight_mask(color);
    for(; kn_mask;)
    {
      auto kn_pos = clear_lsb(kn_mask);

      // don't need to verify capture possibility by mask
      auto f_caps = movesTable().caps(Figure::TypeKnight, kn_pos) & opponent_mask;
      for(; f_caps;)
      {
        auto to = clear_lsb(f_caps);

        X_ASSERT((unsigned)to > 63, "invalid field index while capture");

        const auto& field = board_.getField(to);
        X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

        add(kn_pos, to, Figure::TypeNone);
      }
    }

    {
      auto bi_mask = fmgr.type_mask(Figure::TypeBishop, color);
      for(; bi_mask;)
      {
        auto from = clear_lsb(bi_mask);
        auto f_caps = magic_ns::bishop_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          auto to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const auto& field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    {
      auto r_mask = fmgr.type_mask(Figure::TypeRook, color);
      for(; r_mask;)
      {
        auto from = clear_lsb(r_mask);
        auto f_caps = magic_ns::rook_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          int8 to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const auto& field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    {
      auto q_mask = fmgr.type_mask(Figure::TypeQueen, color);
      for(; q_mask;)
      {
        auto from = clear_lsb(q_mask);
        auto f_caps = magic_ns::queen_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          int8 to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const auto& field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    // 4. King
    {
      // don't need to verify capture possibility by mask
      auto f_caps = movesTable().caps(Figure::TypeKing, ki_pos) & opponent_mask;
      for(; f_caps;)
      {
        auto to = clear_lsb(f_caps);

        X_ASSERT((unsigned)to > 63, "invalid field index while capture");

        const auto& field = board_.getField(to);

        X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

        add(ki_pos, to, Figure::TypeNone);
      }
    }

    iter_ = moves_.begin();
  }

  BOARD const& board_;
  MovesList moves_;
  typename MovesList::iterator iter_;
}; // CapsGenerator

} // NEngine