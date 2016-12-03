/*************************************************************
  Board.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Board.h>
#include <FigureDirs.h>
#include <MovesGenerator.h>
#include <Evaluator.h>
#include <xindex.h>
#include <Helpers.h>

namespace NEngine
{

Board::Board()
{
  clear();
}

void Board::clear()
{
  fmgr_.clear();

  can_win_[0] = can_win_[1] = true;
  en_passant_ = -1;
  state_ = Invalid;
  color_ = Figure::ColorBlack;
  fiftyMovesCount_ = 0;
  movesCounter_ = 1;
  halfmovesCounter_ = 0;
  repsCounter_ = 0;
  castling_ = 0;
  checkingNum_ = 0;

  for (int i = 0; i < NumOfFields; ++i)
    fields_[i].clear();
}

bool Board::initEmpty(Figure::Color color)
{
  clear();
  color_ = color;
  return true;
}

bool Board::canBeReduced() const
{
  const UndoInfo & undo = undoInfoRev(0);

  History & hist = MovesGenerator::history(undo.from_, undo.to_);

  return  ((hist.good()<<2) <= hist.bad()) &&
    !(undo.capture_ || undo.new_type_ > 0 || undo.threat_ || undo.castle_ || underCheck());
}

bool Board::isDangerPawn(Move & move) const
{
  const Field & ffrom = getField(move.from_);
  if ( ffrom.type() != Figure::TypePawn )
    return false;

  if ( move.capture_ || move.new_type_ > 0 )
    return true;

  Figure::Color  color = color_;
  Figure::Color ocolor = Figure::otherColor(color);

  // attacking
  const uint64 & p_caps = movesTable().pawnCaps(ffrom.color(), move.to_);
  const uint64 & o_mask = fmgr_.mask(ocolor);
  if ( p_caps & o_mask )
    return true;

  //// becomes passed
  //const uint64 & pmsk = fmgr_.pawn_mask(color);
  const uint64 & opmsk = fmgr_.pawn_mask(ocolor);
  const uint64 & passmsk = pawnMasks().mask_passed(color, move.to_);
  //const uint64 & blckmsk = pawnMasks().mask_blocked(color, move.to_);

  if ( !(opmsk & passmsk) )//&& !(pmsk & blckmsk) )
		return true;

  if ( !move.seen_ && see(move) >= 0 )
		move.see_good_ = 1;

	move.seen_ = 1;

  return move.see_good_;
}

bool Board::isDangerQueen(const Move & move) const
{
  const Field & ffrom = getField(move.from_);
  if ( ffrom.type() != Figure::TypeQueen )
    return false;

  if ( move.capture_ )
    return true;

  Figure::Color  color = color_;
  Figure::Color ocolor = Figure::otherColor(color);

  int oki_pos = kingPos(ocolor);
  int dist = distanceCounter().getDistance(oki_pos, move.to_);
  if ( dist > 2 )
    return false;

  BitMask mask_all = fmgr().mask(Figure::ColorBlack) | fmgr().mask(Figure::ColorWhite);
  BitMask oki_caps = movesTable().caps(Figure::TypeKing, oki_pos);
  BitMask q_caps = movesTable().caps(Figure::TypeQueen, move.to_);
  BitMask attacked_mask = (oki_caps & q_caps) & ~mask_all;
  BitMask ki_moves = oki_caps & ~(mask_all | attacked_mask);
  int movesN = pop_count(ki_moves);

  return attacked_mask && movesN <= 3;
}

bool Board::isKnightFork(const Move & move) const
{
	const Field & ffrom = getField(move.from_);
	if ( ffrom.type() != Figure::TypeKnight )
		return false;

	if ( move.capture_ )
		return false;

	Figure::Color  color = color_;
	Figure::Color ocolor = Figure::otherColor(color);

	const BitMask & kn_caps = movesTable().caps(Figure::TypeKnight, move.to_);
	BitMask op_mask = fmgr().rook_mask(ocolor) | fmgr().queen_mask(ocolor);
	op_mask &= kn_caps;

	return op_mask != 0 && !one_bit_set(op_mask);
}

bool Board::isKnightForkAfter(const Move & move) const
{
	const Field & fto = getField(move.to_);
	if ( fto.type() != Figure::TypeKnight )
		return false;

	const BitMask & kn_caps = movesTable().caps(Figure::TypeKnight, move.to_);
	BitMask op_mask = fmgr().rook_mask(color_) | fmgr().queen_mask(color_);
	op_mask &= kn_caps;

	return op_mask != 0 && !one_bit_set(op_mask);
}

bool Board::isDoublePawnAttack(const Move & move) const
{
	const Field & fto = getField(move.to_);
	if ( fto.type() != Figure::TypePawn )
		return false;

	Figure::Color ocolor = Figure::otherColor(color_);

	const BitMask & pw_caps = movesTable().pawnCaps(ocolor, move.to_);
	BitMask op_mask = fmgr().knight_mask(color_) | fmgr().bishop_mask(color_) | fmgr().rook_mask(color_) | fmgr().queen_mask(color_);
	op_mask &= pw_caps;

	return op_mask != 0 && !one_bit_set(op_mask);
}

bool Board::isBishopAttack(const Move & move) const
{
	const Field & fto = getField(move.to_);
	if ( fto.type() != Figure::TypeBishop )
		return false;

	const BitMask & bi_caps = movesTable().caps(Figure::TypeBishop, move.to_);
	BitMask op_mask = fmgr().rook_mask(color_) | fmgr().queen_mask(color_);
	if ( !(op_mask & bi_caps) )
		return false;


	BitMask all_mask = fmgr().mask(Figure::ColorWhite) | fmgr().mask(Figure::ColorBlack);
	all_mask ^= op_mask;
	BitMask inv_mask_all = ~all_mask;
	op_mask &= bi_caps;
	const BitMask & oki_mask = fmgr().king_mask(color_);
	int oki_pos = _lsb64(oki_mask);

	int attackedN = 0;

	for ( ; op_mask;)
	{
		int p = clear_lsb(op_mask);
		if ( is_something_between(move.to_, p, inv_mask_all) )
			continue;

		// 2 attacked figures found
		if ( ++attackedN > 1 )
			return true;

		// may be it's pinned
		BitMask mask_from = betweenMasks().from(move.to_, p);
		mask_from &= oki_mask;
		if ( !mask_from )
			continue;

		if ( is_nothing_between(move.to_, oki_pos, inv_mask_all) )
			return true;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////
/// verification of move/unmove methods; use it for debug only
bool Board::operator != (const Board & other) const
{
  const char * buf0 = reinterpret_cast<const char*>(this);
  const char * buf1 = reinterpret_cast<const char*>(&other);

  for (int i = 0; i < sizeof(Board); ++i)
  {
    if ( buf0[i] != buf1[i] )
      return true;
  }

  return false;
}

bool Board::verifyCastling(const Figure::Color c, int t) const
{
  if ( c && getField(4).type() != Figure::TypeKing )
    return false;

  if ( !c && getField(60).type() != Figure::TypeKing )
    return false;

  if ( c && t == 0 && getField(7).type() != Figure::TypeRook )
    return false;

  if ( c && t == 1 && getField(0).type() != Figure::TypeRook )
    return false;

  if ( !c && t == 0 && getField(63).type() != Figure::TypeRook )
    return false;

  if ( !c && t == 1 && getField(56).type() != Figure::TypeRook )
    return false;

  return true;
}

//////////////////////////////////////////////////////////////////////////
bool Board::invalidate()
{
  Figure::Color ocolor = Figure::otherColor(color_);

  int ki_pos = kingPos(color_);
  int oki_pos = kingPos(ocolor);

  state_ = Ok;

  if ( isAttacked(color_, oki_pos) )
  {
    state_ = Invalid;
    return false;
  }

  if ( isAttacked(ocolor, ki_pos) )
    state_ |= UnderCheck;

  verifyChessDraw();

  if ( drawState() )
    return true;

  int cnum = findCheckingFigures(ocolor, ki_pos);
  if ( cnum > 2 )
  {
    state_ = Invalid;
    return false;
  }
  else if ( cnum > 0 )
  {
    state_ |= UnderCheck;
  }

  verifyState();

  return true;
}

// verify is there draw or mat
void Board::verifyState()
{
  if ( matState() || drawState() )
    return;

#ifndef NDEBUG
  Board board0(*this);
#endif

  MovesGenerator mg(*this);
  bool found = false;
  for ( ; !found; )
  {
    auto* m = mg.move();
    if ( !m )
      break;

    if ( validateMove(*m) )
      found = true;
  }

  if ( !found )
  {
    setNoMoves();

    // update move's state because it is last one
    if ( halfmovesCounter_ > 0 )
    {
      UndoInfo & undo = undoInfo(halfmovesCounter_-1);
      undo.state_ = state_;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
bool Board::addFigure(const Figure::Color color, const Figure::Type type, int pos)
{
  if ( !type || (unsigned)pos > 63 )
    return false;

  Field & field = getField(pos);
  if ( field )
    return false;

  fields_[pos].set(color, type);
  fmgr_.incr(color, type, pos);

  return false;
}

//////////////////////////////////////////////////////////////////////////
void Board::verifyMasks() const
{
  for (int c = 0; c < 2; ++c)
  {
    BitMask pawn_mask = 0ULL;
    //BitMask pawn_mask_t = 0ULL;
    BitMask knight_mask = 0ULL;
    BitMask bishop_mask = 0ULL;
    BitMask rook_mask = 0ULL;
    BitMask queen_mask = 0ULL;
    BitMask king_mask = 0ULL;
    BitMask all_mask = 0ULL;

    Figure::Color color = (Figure::Color)c;
    for (int p = 0; p < NumOfFields; ++p)
    {
      const Field & field = getField(p);
      if ( !field || field.color() != color )
        continue;

      all_mask |= set_mask_bit(p);

      switch ( field.type() )
      {
      case Figure::TypePawn:
        {
          //pawn_mask_t |= set_mask_bit(Index(p).transp());
          pawn_mask |= set_mask_bit(p);
        }
        break;

      case Figure::TypeKnight:
        knight_mask |= set_mask_bit(p);
        break;

      case Figure::TypeBishop:
        bishop_mask |= set_mask_bit(p);
        break;

      case Figure::TypeRook:
        rook_mask |= set_mask_bit(p);
        break;

      case Figure::TypeQueen:
        queen_mask |= set_mask_bit(p);
        break;

      case Figure::TypeKing:
        king_mask |= set_mask_bit(p);
        break;
      }
    }

    X_ASSERT( pawn_mask != fmgr_.pawn_mask(color), "pawn mask invalid" );
    //X_ASSERT( pawn_mask_t != fmgr_.pawn_mask_t(color), "pawn mask invalid" );
    X_ASSERT( knight_mask != fmgr_.knight_mask(color), "knight mask invalid" );
    X_ASSERT( bishop_mask != fmgr_.bishop_mask(color), "bishop mask invalid" );
    X_ASSERT( rook_mask != fmgr_.rook_mask(color), "rook mask invalid" );
    X_ASSERT( queen_mask != fmgr_.queen_mask(color), "queen mask invalid" );
    X_ASSERT( king_mask != fmgr_.king_mask(color), "king mask invalid" );
    X_ASSERT( all_mask != fmgr_.mask(color), "invalid all figures mask" );

  }
}

} // NEngine
