/*************************************************************
  CheckDetector.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Board.h>
#include <fpos.h>
#include <FigureDirs.h>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
void Board::detectCheck(const UndoInfo & undo)
{
  checkingNum_ = 0;
  const Figure::Color color = color_;
  Figure::Color ocolor = Figure::otherColor(color_);
  const Field & ffrom = getField(undo.from_);
  const Field & fto = getField(undo.to_);

  int d = undo.to_ - undo.from_;
  if ( fto.type() == Figure::TypeKing && ((2 == d || -2 == d)) ) // castle with check
  {
    X_ASSERT( !undo.castle_, "castle flag wasn't set" );

    d >>= 1;
    int rook_to = undo.from_ + d;
    if ( isAttackedBy(color, ocolor, Figure::TypeRook, rook_to) )
    {
      checking_[checkingNum_++] = rook_to;
      state_ |= UnderCheck;
    }

    // no more checking figures
    return;
  }
  else
  {
    const BitMask & king_mask = fmgr_.king_mask(color);
    if ( fto.type() == Figure::TypePawn )
    {
      const BitMask & pw_mask = movesTable().pawnCaps_o(ocolor, undo.to_);
      if ( pw_mask & king_mask )
        checking_[checkingNum_++] = undo.to_;
    }
    else if ( fto.type() == Figure::TypeKnight )
    {
      const BitMask & kn_mask = movesTable().caps(Figure::TypeKnight, undo.to_);
      if ( kn_mask & king_mask )
        checking_[checkingNum_++] = undo.to_;
    }
    else if ( isAttackedBy(color, ocolor, fto.type(), undo.to_) )
      checking_[checkingNum_++] = undo.to_;
  }

  int king_pos = kingPos(color);
  BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);
  BitMask mask_all = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite);

  // check through en-passant field
  int ep_checking = -1;
  if ( undo.en_passant_ == undo.to_ && fto.type() == Figure::TypePawn )
  {
    static int pw_delta[2] = { -8, 8 };
    int ep_pos = undo.en_passant_ + pw_delta[color]; // color == color of captured pawn

    // 1. through en-passant pawn field
    ep_checking = findDiscovered(ep_pos, ocolor, mask_all, brq_mask, king_pos);
    if ( ep_checking >= 0 )
      checking_[checkingNum_++] = ep_checking;
  }

  // 2. through undo.from_ field
  int apos = findDiscovered(undo.from_, ocolor, mask_all, brq_mask, king_pos);
  // exclude figures, that's made current move and checking through en-passant field
  if ( apos >= 0 && apos != undo.to_ && apos != ep_checking )
    checking_[checkingNum_++] = apos;

  X_ASSERT( checkingNum_ > 2, "more than 2 figures give check" );
  X_ASSERT( checkingNum_ > 1 && checking_[0] == checking_[1], "wrong double check detected" );

  if ( checkingNum_ > 0 )
    state_ |= UnderCheck;
}

//////////////////////////////////////////////////////////////////////////
/// is field 'pos' attacked by given color?
bool Board::fieldAttacked(const Figure::Color c, int8 pos, const BitMask & mask_all_inv) const
{
  Figure::Color ocolor = Figure::otherColor(c);

  {
    // knights
    const BitMask & n_caps = movesTable().caps(Figure::TypeKnight, pos);
    const BitMask & knight_msk = fmgr_.knight_mask(c);
    if ( n_caps & knight_msk )
      return true;

    // pawns
    const BitMask & p_caps = movesTable().pawnCaps_o(ocolor, pos);
    const BitMask & pawn_msk = fmgr_.pawn_mask_o(c);
    if ( p_caps & pawn_msk )
      return true;

    // king
    const BitMask & k_caps = movesTable().caps(Figure::TypeKing, pos);
    const BitMask & king_msk = fmgr_.king_mask(c);
    if ( k_caps & king_msk )
      return true;
  }

  // all long-range figures
  const BitMask & q_caps = movesTable().caps(Figure::TypeQueen, pos);
  BitMask mask_brq = fmgr_.bishop_mask(c) | fmgr_.rook_mask(c) | fmgr_.queen_mask(c);
  mask_brq &= q_caps;

  // do we have at least 1 attacking figure
  if ( mask_brq )
  {
    const BitMask & black = fmgr_.mask(Figure::ColorBlack);
    const BitMask & white = fmgr_.mask(Figure::ColorWhite);

    // rooks
    const BitMask & r_caps = movesTable().caps(Figure::TypeRook, pos);
    BitMask rook_msk = fmgr_.rook_mask(c) & r_caps;
    for ( ; rook_msk; )
    {
      int n = clear_lsb(rook_msk);

      X_ASSERT( (unsigned)n > 63, "invalid bit found in attack detector" );
      X_ASSERT( !getField(n) || getField(n).type() != Figure::TypeRook, "no figure but mask bit is 1" );
      X_ASSERT( getField(n).color() != c, "invalid figure color in attack detector" );

      if ( is_nothing_between(n, pos, mask_all_inv) )
        return true;
    }

    // bishops
    const BitMask & b_caps = movesTable().caps(Figure::TypeBishop, pos);
    BitMask bishop_msk = fmgr_.bishop_mask(c) & b_caps;
    for ( ; bishop_msk; )
    {
      int n = clear_lsb(bishop_msk);

      X_ASSERT( (unsigned)n > 63, "invalid bit found in attack detector" );
      X_ASSERT( !getField(n) || getField(n).type() != Figure::TypeBishop, "no figure but mask bit is 1" );
      X_ASSERT( getField(n).color() != c, "invalid figure color in attack detector" );

      if ( is_nothing_between(n, pos, mask_all_inv) )
        return true;
    }

    // queens
    BitMask queen_msk = fmgr_.queen_mask(c) & q_caps;
    for ( ; queen_msk; )
    {
      int n = clear_lsb(queen_msk);

      X_ASSERT( (unsigned)n > 63, "invalid bit found in attack detector" );
      X_ASSERT( !getField(n) || getField(n).type() != Figure::TypeQueen, "no figure but mask bit is 1" );
      X_ASSERT( getField(n).color() != c, "invalid figure color in attack detector" );

      if ( is_nothing_between(n, pos, mask_all_inv) )
        return true;
    }
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////
// returns number of checking figures.
// very slow. used only for initial validation
int Board::findCheckingFigures(Figure::Color ocolor, int ki_pos)
{
  checkingNum_ = 0;
  for (int type = Figure::TypePawn; type < Figure::TypeKing; ++type)
  {
      BitMask mask = fmgr_.type_mask((Figure::Type)type, ocolor);
      for ( ; mask; )
      {
        int n = clear_lsb(mask);
        const Field & field = getField(n);

        X_ASSERT( field.color() != ocolor || field.type() != type, "invalid figures mask in check detector" );

        int dir = figureDir().dir(field.type(), ocolor, n, ki_pos);
        if ( (dir < 0) || (Figure::TypePawn == type && (2 == dir || 3 == dir)) )
          continue;

        X_ASSERT( Figure::TypeKing == type, "king checks king" );

        if ( Figure::TypePawn == type || Figure::TypeKnight == type )
        {
          if ( checkingNum_ > 1 )
            return ++checkingNum_;

          checking_[checkingNum_++] = n;
          continue;
        }

        FPos dp = deltaPosCounter().getDeltaPos(n, ki_pos);

        X_ASSERT( FPos(0, 0) == dp, "invalid attacked position" );

        FPos p = FPosIndexer::get(ki_pos) + dp;
        const FPos & figp = FPosIndexer::get(n);
        bool have_figure = false;
        for ( ; p != figp; p += dp)
        {
          const Field & field = getField(p.index());
          if ( field )
          {
            have_figure = true;
            break;
          }
        }

        if ( !have_figure )
        {
          if ( checkingNum_ > 1 )
            return ++checkingNum_;

          checking_[checkingNum_++] = n;
        }
      }
  }

  return checkingNum_;
}

} // NEngine
