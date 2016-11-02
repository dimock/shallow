/*************************************************************
  MovesGenerator.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <MovesGenerator.h>
#include <MovesTable.h>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
EscapeGenerator::EscapeGenerator(Board & board) :
  MovesGeneratorBase(board), takeHash_(0), movesCount_(0), fake_(0), weakN_(0),do_weak_(0)
{
  weak_[0].clear();
  hmove_.clear();
}

EscapeGenerator::EscapeGenerator(const Move & hmove, Board & board) :
  MovesGeneratorBase(board), hmove_(hmove), takeHash_(0), movesCount_(0), fake_(0), weakN_(0),do_weak_(0)
{
  weak_[0].clear();
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
  movesCount_ = numOfMoves_ + takeHash_;
}

void EscapeGenerator::restart()
{
  if ( !numOfMoves_ )
    return;

  weak_[0].clear();
  weakN_ = 0;

  for (int i = 0; i < numOfMoves_; ++i)
    moves_[i].alreadyDone_ = 0;
}

void EscapeGenerator::generate(const Move & hmove)
{
  if ( movesCount_ > 0 )
    return;

  hmove_ = hmove;
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
  movesCount_ = numOfMoves_ + takeHash_;
}

int EscapeGenerator::generate()
{
  if ( board_.checkingNum_ == 1 )
    return generateUsual();
  else
    return generateKingonly(numOfMoves_);
}

int EscapeGenerator::generateUsual()
{
  int m = numOfMoves_;
  const Figure::Color color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);

  X_ASSERT( (unsigned)board_.checking_[0] > 63, "there is no checking figure" );

  // checking figure position and type
  const int8 & ch_pos = board_.checking_[0];
  Figure::Type ch_type = board_.getField(ch_pos).type();

  X_ASSERT( !ch_type, "there is no checking figure" );
  X_ASSERT( ch_type == Figure::TypeKing, "king is attacking king" );

  int ki_pos = board_.kingPos(color);
  const uint64 & black = board_.fmgr_.mask(Figure::ColorBlack);
  const uint64 & white = board_.fmgr_.mask(Figure::ColorWhite);
  const uint64 & pawn_msk = board_.fmgr_.pawn_mask_o(board_.color_);
  BitMask mask_all = white | black;
  uint64 mask_all_inv = ~mask_all;
  BitMask brq_mask = board_.fmgr_.bishop_mask(ocolor) | board_.fmgr_.rook_mask(ocolor) | board_.fmgr_.queen_mask(ocolor);

  static int pw_delta[] = { -8, 8 };

  // check if there is at least 1 pawns capture
  bool pawns_eat = false;
  bool ep_capture = false;

  {
    uint64 pawn_eat_msk = 0;
    if ( board_.color_ )
      pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
    else
      pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);

    // mask of attacking figure
    uint64 checking_fig_mask = set_mask_bit(ch_pos);

    pawns_eat = (pawn_eat_msk & checking_fig_mask) != 0;

    if ( ch_type == Figure::TypePawn && board_.en_passant_ >= 0 )
    {
      int ep_pos = board_.enpassantPos();
      X_ASSERT( !board_.getField(ep_pos), "en-passant pown doesnt exist" );

      ep_capture = (ch_pos == ep_pos) && (pawn_eat_msk & set_mask_bit(board_.en_passant_));
    }

    pawns_eat = pawns_eat || ep_capture;
  }

  // 1st - get pawn's captures
  if ( pawns_eat )
  {
    // en-passant
    if ( ep_capture )
    {
      int8 ep_pos = board_.enpassantPos();
      const uint64 & opawn_caps_ep = board_.g_movesTable->pawnCaps_o(ocolor, board_.en_passant_);
      uint64 eat_msk_ep = pawn_msk & opawn_caps_ep;

      for ( ; eat_msk_ep; )
      {
        int n = clear_lsb(eat_msk_ep);

        const Field & fpawn = board_.getField(n);
        X_ASSERT( !fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != board_.color_, "no pawn on field we are going to do capture from" );

        BitMask mask_all_ep = mask_all ^ set_mask_bit(n);
        mask_all |= set_mask_bit(board_.en_passant_);

        if ( !board_.discoveredCheck(n, ocolor, mask_all, brq_mask, ki_pos) &&
             !board_.discoveredCheck(ep_pos, ocolor, mask_all_ep, brq_mask, ki_pos) )
        {
          add(m, n, board_.en_passant_, Figure::TypeNone, true);
        }
      }
    }


    const uint64 & opawn_caps = board_.g_movesTable->pawnCaps_o(ocolor, ch_pos);
    uint64 eat_msk = pawn_msk & opawn_caps;

    bool promotion = ch_pos > 55 || ch_pos < 8; // 1st || last line

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & fpawn = board_.getField(n);

      X_ASSERT( !fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != board_.color_, "no pawn on field we are going to do capture from" );

      if ( !board_.discoveredCheck(n, ocolor, mask_all | set_mask_bit(ch_pos), brq_mask & ~set_mask_bit(ch_pos), ki_pos) )
      {
        add(m, n, ch_pos, promotion ? Figure::TypeQueen : Figure::TypeNone, true);
        if ( promotion )
        {
          if ( (board_.g_movesTable->caps(Figure::TypeKnight, ch_pos) & board_.fmgr_.king_mask(ocolor)) )
            add(m, n, ch_pos, Figure::TypeKnight, true);
        }
      }
    }
  }

  // 2nd - knight's captures
  {
    const uint64 & knight_caps = board_.g_movesTable->caps(Figure::TypeKnight, ch_pos);
    const uint64 & knight_msk = board_.fmgr_.knight_mask(color);
    uint64 eat_msk = knight_msk & knight_caps;

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & fknight = board_.getField(n);

      X_ASSERT( !fknight || fknight.type() != Figure::TypeKnight || fknight.color() != board_.color_, "no knight on field we are going to do capture from" );

      if ( !board_.discoveredCheck(n, ocolor, mask_all | set_mask_bit(ch_pos), brq_mask & ~set_mask_bit(ch_pos), ki_pos) )
        add(m, n, ch_pos, Figure::TypeNone, true);
    }
  }

  // 3rd - bishops, rooks and queens
  {
    const uint64 & queen_caps = board_.g_movesTable->caps(Figure::TypeQueen, ch_pos);
    uint64 brq_msk = board_.fmgr_.bishop_mask(color) | board_.fmgr_.rook_mask(color) | board_.fmgr_.queen_mask(color);
    uint64 eat_msk = brq_msk & queen_caps;

    for ( ; eat_msk; )
    {
      int n = clear_lsb(eat_msk);

      const Field & field = board_.getField(n);

      X_ASSERT( !field || field.color() != board_.color_, "no figure on field we are going to do capture from" );

      // can fig go to checking figure field
      int dir = board_.g_figureDir->dir(field.type(), board_.color_, n, ch_pos);
      if ( dir < 0 )
        continue;

      const uint64 & btw_msk = board_.g_betweenMasks->between(n, ch_pos);
      if ( (btw_msk & mask_all_inv) != btw_msk )
        continue;

      if ( !board_.discoveredCheck(n, ocolor, mask_all | set_mask_bit(ch_pos), brq_mask & ~set_mask_bit(ch_pos), ki_pos) )
        add(m, n, ch_pos, Figure::TypeNone, true);
    }
  }

  // now try to protect king - put something between it and checking figure
  const BitMask & protect_king_msk = board_.g_betweenMasks->between(board_.kingPos(color), ch_pos);

  if ( protect_king_msk && Figure::TypePawn != ch_type && Figure::TypeKnight != ch_type )
  {
    // 1. Pawns
    BitMask pw_mask = board_.fmgr().pawn_mask_o(color);
    for ( ; pw_mask; )
    {
      int pw_pos = clear_msb(pw_mask);

      if ( board_.discoveredCheck(pw_pos, ocolor, mask_all, brq_mask, ki_pos) )
        continue;

      // +2 - skip captures
      const int8 * table = board_.g_movesTable->pawn(color, pw_pos) + 2;

      for (; *table >= 0 && !board_.getField(*table); ++table)
      {
        if ( (protect_king_msk & set_mask_bit(*table)) == 0 )
          continue;

        bool promotion = *table > 55 || *table < 8;

        add(m, pw_pos, *table, promotion ? Figure::TypeQueen : Figure::TypeNone, false);
        if ( promotion )
        {
          // add promotion to knight only if it gives check and we don't lost it immediately
          Move nmove;
          nmove.set(pw_pos, *table, Figure::TypeKnight, false);

          if ( (board_.g_movesTable->caps(Figure::TypeKnight, ch_pos) & board_.fmgr_.king_mask(ocolor)) && board_.see(nmove) >= 0 )
            add(m, pw_pos, *table, Figure::TypeKnight, false);
        }
      }
    }


    // 2. Knights
    BitMask kn_mask = board_.fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int kn_pos = clear_msb(kn_mask);

      if ( board_.discoveredCheck(kn_pos, ocolor, mask_all, brq_mask, ki_pos) )
        continue;

      const uint64 & knight_msk = board_.g_movesTable->caps(Figure::TypeKnight, kn_pos);
      uint64 msk_protect = protect_king_msk & knight_msk;
      for ( ; msk_protect; )
      {
        int n = clear_lsb(msk_protect);

        const Field & field = board_.getField(n);

        X_ASSERT( field, "there is something between king and checking figure" );

        add(m, kn_pos, n, Figure::TypeNone, false);
      }
    }

    // 3. Bishops + Rooks + Queens
    for (int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, color);
      for ( ; fg_mask; )
      {
        int fg_pos = clear_msb(fg_mask);

        if ( board_.discoveredCheck(fg_pos, ocolor, mask_all, brq_mask, ki_pos) )
          continue;

        const uint64 & figure_msk = board_.g_movesTable->caps(type, fg_pos);
        uint64 msk_protect = protect_king_msk & figure_msk;

        for ( ; msk_protect; )
        {
          int n = clear_lsb(msk_protect);

          const Field & field = board_.getField(n);

          X_ASSERT( field, "there is something between king and checking figure" );
          X_ASSERT( board_.g_figureDir->dir((Figure::Type)type, color, fg_pos, n) < 0, "figure can't go to required field" );

          const uint64 & btw_msk = board_.g_betweenMasks->between(fg_pos, n);
          if ( (btw_msk & mask_all_inv) != btw_msk )
            continue;

          add(m, fg_pos, n, Figure::TypeNone, false);
        }
      }
    }
  }

  // at the last generate all king's movements
  m = generateKingonly(m);

  return m;
}

int EscapeGenerator::generateKingonly(int m)
{
  const Figure::Color color = board_.color_;
  Figure::Color ocolor = Figure::otherColor(color);

  int from = board_.kingPos(color);

  const BitMask & mask = board_.fmgr_.mask(color);
  const BitMask & o_mask = board_.fmgr_.mask(ocolor);

  // captures
  BitMask ki_mask = board_.g_movesTable->caps(Figure::TypeKing, from) & o_mask;
  for ( ; ki_mask; )
  {
    int to = clear_lsb(ki_mask);

    X_ASSERT( !board_.getField(to) || board_.getField(to).color() == color, "escape generator: try to put king to occupied field" );
    if ( !board_.isAttacked(ocolor, to, from) )
      add(m, from, to, Figure::TypeNone, true);
  }

  // other moves
  ki_mask = board_.g_movesTable->caps(Figure::TypeKing, from) & ~(mask | o_mask);
  for ( ; ki_mask; )
  {
    int to = clear_lsb(ki_mask);

    X_ASSERT( board_.getField(to), "escape generator: try to put king to occupied field" );
    if ( !board_.isAttacked(ocolor, to, from) )
      add(m, from, to, Figure::TypeNone, false);
  }

  return m;
}

} // NEngine
