/*************************************************************
  CapsGenerator.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <MovesGenerator.h>
#include <MovesTable.h>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
CapsGenerator::CapsGenerator(const Move & hcap, Board & board) :
  MovesGeneratorBase(board),
  hcap_(hcap),
  thresholdType_(Figure::TypeNone)
{
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();

  const Figure::Color color = board_.color_;
  const Figure::Color ocolor = Figure::otherColor(color);
  mask_all_ = board_.fmgr().mask(Figure::ColorWhite) | board_.fmgr().mask(Figure::ColorBlack);
  mask_brq_ = board_.fmgr().bishop_mask(color) | board_.fmgr().rook_mask(color) | board_.fmgr().queen_mask(color);
  oking_pos_ = board_.kingPos(ocolor);
}

CapsGenerator::CapsGenerator(Board & board) :
  MovesGeneratorBase(board),
  thresholdType_(Figure::TypeNone)
{
  hcap_.clear();

  const Figure::Color & color = board_.color_;
  const Figure::Color ocolor = Figure::otherColor(color);
  mask_all_ = board_.fmgr().mask(Figure::ColorWhite) | board_.fmgr().mask(Figure::ColorBlack);
  mask_brq_ = board_.fmgr().bishop_mask(color) | board_.fmgr().rook_mask(color) | board_.fmgr().queen_mask(color);
  oking_pos_ = board_.kingPos(ocolor);
}

void CapsGenerator::restart()
{
  if ( !numOfMoves_ )
    return;

  for (int i = 0; i < numOfMoves_; ++i)
    moves_[i].alreadyDone_ = 0;
}

int CapsGenerator::generate(const Move & hcap, Figure::Type thresholdType)
{
  hcap_ = hcap;
  thresholdType_ = thresholdType;

  if ( numOfMoves_ > 0 ) // already generated
    return numOfMoves_;

  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();

  return numOfMoves_;
}

int CapsGenerator::generate()
{
  int m = 0;

  const Figure::Color color = board_.color_;
  const Figure::Color ocolor = Figure::otherColor(color);

  BitMask oppenent_mask = board_.fmgr_.mask(ocolor) ^ board_.fmgr_.king_mask(ocolor);
  const BitMask & black = board_.fmgr_.mask(Figure::ColorBlack);
  const BitMask & white = board_.fmgr_.mask(Figure::ColorWhite);
  BitMask mask_all = white | black;
  int ki_pos = board_.kingPos(color);
  int oki_pos = board_.kingPos(ocolor);

  // generate pawn promotions
  const BitMask & pawn_msk = board_.fmgr_.pawn_mask_o(color);
  {
    static int pw_delta[] = { -8, +8 };
    BitMask promo_msk = movesTable().promote_o(color);
    promo_msk &= pawn_msk;

    for ( ; promo_msk; )
    {
      int from = clear_lsb(promo_msk);

      X_ASSERT( (unsigned)from > 63, "invalid promoted pawn's position" );

      const Field & field = board_.getField(from);

      X_ASSERT( !field || field.color() != color || field.type() != Figure::TypePawn, "there is no pawn to promote" );

      int to = from + pw_delta[color];

      X_ASSERT( (unsigned)to > 63, "pawn tries to go to invalid field" );

      if ( board_.getField(to) )
        continue;

      add(m, from, to, Figure::TypeQueen, false);

      // add promotion to checking knight
      if ( figureDir().dir(Figure::TypeKnight, color, to, oki_pos) >= 0 )
        add(m, from, to, Figure::TypeKnight, false);
    }
  }

  // firstly check if we have at least 1 attacking pawn
  bool pawns_eat = false;

  BitMask pawn_eat_msk = 0;
  if ( color )
    pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
  else
    pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);

  pawns_eat = (pawn_eat_msk & oppenent_mask) != 0;

  if ( !pawns_eat && board_.en_passant_ >= 0 )
    pawns_eat = (pawn_eat_msk & set_mask_bit(board_.en_passant_)) != 0;

  // generate captures

  // 1. Pawns
  if ( pawns_eat )
  {
    BitMask pw_mask = board_.fmgr().pawn_mask_o(color);

    for ( ; pw_mask; )
    {
      int pw_pos = clear_lsb(pw_mask);

      BitMask p_caps = movesTable().pawnCaps_o(color, pw_pos) & oppenent_mask;

      for ( ; p_caps; )
      {
        X_ASSERT( !pawns_eat, "have pawns capture, but not detected by mask" );

        int to = clear_lsb(p_caps);

        bool promotion = to > 55 || to < 8; // 1st || last line

        X_ASSERT( (unsigned)to > 63, "invalid pawn's capture position" );

        const Field & field = board_.getField(to);
        if ( !field || field.color() != ocolor )
          continue;

        add(m, pw_pos, to, promotion ? Figure::TypeQueen : Figure::TypeNone, true);

        // add promotion to checking knight
        if ( promotion && figureDir().dir(Figure::TypeKnight, color, to, oki_pos) >= 0 )
          add(m, pw_pos, to, Figure::TypeKnight, true);
      }
    }

    if ( board_.en_passant_ >= 0 )
    {
      X_ASSERT( board_.getField(board_.enpassantPos()).type() != Figure::TypePawn || board_.getField(board_.enpassantPos()).color() != ocolor, "there is no en passant pawn" );
      BitMask ep_mask = movesTable().pawnCaps_o(ocolor, board_.en_passant_) & board_.fmgr().pawn_mask_o(color);

      for ( ; ep_mask; )
      {
        int from = clear_lsb(ep_mask);
        add(m, from, board_.en_passant_, Figure::TypeNone, true);
      }
    }
  }

  // 2. Knights
  BitMask kn_mask = board_.fmgr().knight_mask(color);
  for ( ; kn_mask; )
  {
    int kn_pos = clear_lsb(kn_mask);

    // don't need to verify capture possibility by mask
    BitMask f_caps = movesTable().caps(Figure::TypeKnight, kn_pos) & oppenent_mask;
    for ( ; f_caps; )
    {
      int to = clear_lsb(f_caps);

      X_ASSERT( (unsigned)to > 63, "invalid field index while capture" );

      const Field & field = board_.getField(to);

      X_ASSERT( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

      add(m, kn_pos, to, Figure::TypeNone, true);
    }
  }

  // 3. Bishops + Rooks + Queens
  for (int type = Figure::TypeBishop; type <= Figure::TypeQueen; ++type)
  {
    BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, color);
    for ( ; fg_mask; )
    {
      int from = clear_lsb(fg_mask);

      BitMask f_caps = movesTable().caps((Figure::Type)type, from) & oppenent_mask;
      for ( ; f_caps; )
      {
        int8 to = _lsb64(f_caps);
        int pos = board_.find_first_index(from, to, mask_all);
        if ( set_mask_bit(pos) & oppenent_mask )
          add(m, from, pos, Figure::TypeNone, true);

        f_caps &= ~betweenMasks().from(from, to);
      }
    }
  }

  // 4. King
  {
    // don't need to verify capture possibility by mask
    BitMask f_caps = movesTable().caps(Figure::TypeKing, ki_pos) & oppenent_mask;
    for ( ; f_caps; )
    {
      int to = clear_lsb(f_caps);

      X_ASSERT( (unsigned)to > 63, "invalid field index while capture" );

      const Field & field = board_.getField(to);

      X_ASSERT( !field || field.color() != ocolor, "there is no opponent's figure on capturing field" );

      add(m, ki_pos, to, Figure::TypeNone, true);
    }
  }

  return m;
}

//////////////////////////////////////////////////////////////////////////
bool CapsGenerator::expressCheck(Move & move) const
{
  const Field & ffield = board_.getField(move.from_);

  const Figure::Color color = board_.color_;
  const Figure::Color ocolor = Figure::otherColor(color);

  const BitMask mask_all = mask_all_ | set_mask_bit(move.to_);
  if ( board_.discoveredCheck(move.from_, color, mask_all, mask_brq_, oking_pos_) )
  {
    move.discoveredCheck_ = 1;
    return true;
  }

  if ( ffield.type() == Figure::TypeKing )
    return false;

  const BitMask & oki_mask = board_.fmgr().king_mask(ocolor);

  X_ASSERT( !ffield, "no moved figure" );

  if ( ffield.type() == Figure::TypePawn )
  {
    const BitMask & pw_caps = movesTable().pawnCaps_o(color, move.to_);
    if ( pw_caps & oki_mask )
      return true;

    // en-passant discovered check
    if ( board_.en_passant_ == move.to_ )
    {
      int ep_pos = board_.enpassantPos();
      X_ASSERT( board_.getField(ep_pos).type() != Figure::TypePawn || board_.getField(ep_pos).color() != ocolor, "no pawn for en-passant capture" );

      if ( board_.discoveredCheck(ep_pos, color, mask_all ^ set_mask_bit(ep_pos), mask_brq_, oking_pos_) )
      {
        move.discoveredCheck_ = 1;
        return true;
      }
    }

    // promotion to knight was generated with check only
    if ( move.new_type_ == Figure::TypeKnight )
      return true;

    if ( move.new_type_ == Figure::TypeQueen )
    {
      if ( figureDir().dir(Figure::TypeQueen, ffield.color(), move.to_, oking_pos_) < 0 )
        return false;

      return board_.is_nothing_between(move.to_, oking_pos_, ~mask_all_);
    }

    return false;
  }

  if ( ffield.type() == Figure::TypeKnight )
  {
    const BitMask & kn_caps = movesTable().caps(Figure::TypeKnight, move.to_);
    return (kn_caps & oki_mask) != 0;
  }

  X_ASSERT( ffield.type() < Figure::TypeBishop || ffield.type() > Figure::TypeQueen, "wrong attacking figure type" );

  // at the last bishop + rook + queen
  if ( figureDir().dir(ffield.type(), ffield.color(), move.to_, oking_pos_) < 0 )
    return false;

  return board_.is_nothing_between(move.to_, oking_pos_, ~mask_all_);
}

} // NEngine
