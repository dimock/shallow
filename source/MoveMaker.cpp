/*************************************************************
MoveMaker.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <Board.h>
#include <FigureDirs.h>

namespace NEngine
{

// physically possible
bool Board::possibleMove(const Move & move) const
{
  X_ASSERT( (unsigned)move.to_ > 63 || (unsigned)move.from_ > 63, "invalid move given" );

  const Field & fto = getField(move.to_);
  if ( fto && (fto.color() == color_ || fto.type() == Figure::TypeKing) )
    return false;

  const Field & ffrom = getField(move.from_);
  if ( !ffrom || ffrom.color() != color_ )
    return false;

  // only kings move is possible
  if ( checkingNum_ > 1 && ffrom.type() != Figure::TypeKing )
    return false;

  if ( ffrom.type() != Figure::TypePawn && move.new_type_ > 0 )
    return false;

  int dir = figureDir().dir(ffrom.type(), color_, move.from_, move.to_);
  if ( dir < 0 )
    return false;

  // castle
  if ( ffrom.type() == Figure::TypeKing && (8 == dir || 9 == dir) )
  {
    if ( checkingNum_ )
      return false;

    if ( !castling(color_, dir-8) ) // 8 means short castling, 9 - long
      return false;

    if ( color_ && move.from_ != 4 || !color_ && move.from_ != 60 )
      return false;

    int d = (move.to_ - move.from_) >> 1; // +1 short, -1 long
    if ( move.capture_ || getField(move.to_) || getField(move.from_ + d) )
      return false;

    // long castling - field right to rook (or left to king) is occupied
    if ( dir == 9 && getField(move.to_-1) )
      return false;
  }

  switch ( ffrom.type() )
  {
  case Figure::TypePawn:
    {
      int8 y = move.to_ >> 3;
      if ( ((7 == y || 0 == y) && !move.new_type_) || (0 != y && 7 != y && move.new_type_) )
        return false;

      X_ASSERT( move.new_type_ < 0 || move.new_type_ > Figure::TypeQueen, "invalid promotion piece" );

      if ( fto || move.to_ == en_passant_ )
      {
        return (0 == dir || 1 == dir);
      }
      else
      {
        int8 to3 = move.from_ + ((move.to_-move.from_) >> 1);
        X_ASSERT( (uint8)to3 > 63 || 3 == dir && to3 != move.from_ + (move.to_ > move.from_ ? 8 : -8), "pawn goes to invalid field" );
        return (2 == dir) || (3 == dir && !getField(to3));
      }
    }
    break;

  case Figure::TypeBishop:
  case Figure::TypeRook:
  case Figure::TypeQueen:
    {
      const BitMask & mask  = betweenMasks().between(move.from_, move.to_);
      const BitMask & black = fmgr_.mask(Figure::ColorBlack);
      const BitMask & white = fmgr_.mask(Figure::ColorWhite);

      bool ok = (mask & ~(black | white)) == mask;
      return ok;
    }
    break;
  }

  return true;
}

#ifdef VALIDATE_VALIDATOR
// verify move by rules
bool Board::validateMove2(const Move & move) const
#else
bool Board::validateMove(const Move & move) const
#endif
{
  if ( drawState() || matState() )
    return false;

  if ( !move ) // null-move
    return true;

  const Field & ffrom = getField(move.from_);

  Figure::Color ocolor = Figure::otherColor(color_);

  // validate under check
  if ( checkingNum_ )
  {
#ifdef NDEBUG
    if ( move.checkVerified_ )
      return true;
#endif

    {
      if ( Figure::TypeKing == ffrom.type() )
      {
        X_ASSERT((move.from_&7)-(move.to_&7) > 1 || (move.from_&7)-(move.to_&7) < -1, "try to castle under check");

        return !isAttacked(ocolor, move.to_, move.from_);
      }
      else if ( checkingNum_ > 1 )
        return false;

      X_ASSERT( (unsigned)checking_[0] >= NumOfFields, "invalid checking figure" );

      // regular capture
      if ( move.to_ == checking_[0] )
      {
        // maybe moving figure discovers check
        BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
        mask_all |= set_mask_bit(move.to_);

        BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);
        brq_mask &= ~set_mask_bit(move.to_);

        int ki_pos = kingPos(color_);

        return !discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos);
      }

      // en-passant capture. we have to check the direction from king to en-passant pawn
      if ( move.to_ == en_passant_ && Figure::TypePawn == ffrom.type() )
      {
        int ep_pos = enpassantPos();
        if ( ep_pos == checking_[0] )
        {
          BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
          mask_all |= set_mask_bit(move.to_);
          mask_all ^= set_mask_bit(move.from_);
          mask_all &= ~set_mask_bit(ep_pos);

          BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);

          int ki_pos = kingPos(color_);

          // through removed en-passant pawn's field
          if ( discoveredCheck(ep_pos, ocolor, mask_all, brq_mask, ki_pos) )
            return false;

          // through moved pawn's field
          return !discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos);
        }
      }

      // moving figure covers king
      {
        const Field & cfield = getField(checking_[0]);
        X_ASSERT( Figure::TypeKing == cfield.type(), "king is attacking king" );
        X_ASSERT( !cfield, "king is attacked by non existing figure" );

        // Pawn and Knight could be only removed to escape from check
        if ( Figure::TypeKnight == cfield.type() || Figure::TypePawn == cfield.type() )
          return false;

        int ki_pos = kingPos(color_);
        const BitMask & protect_king_msk = betweenMasks().between(ki_pos, checking_[0]);
        if ( (protect_king_msk & set_mask_bit(move.to_)) == 0 )
          return false;

        BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
        mask_all |= set_mask_bit(move.to_);

        BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);
        brq_mask &= ~set_mask_bit(move.to_);

        return !discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos);
      }
    }

    X_ASSERT(move.checkVerified_, "check verificaton in escape generator failed");

    return false;
  }

  // without check
  if ( ffrom.type() == Figure::TypeKing  )
  {
    // castling
    int d = move.to_ - move.from_;
    if ( (2 == d || -2 == d) )
    {
      X_ASSERT( !castling(color_, d > 0 ? 0 : 1), "castling impossible" );
      X_ASSERT( !verifyCastling(color_, d > 0 ? 0 : 1), "castling flag is invalid" );

      // don't do castling under check
      X_ASSERT( checkingNum_ > 0, "can not castle under check" );
      X_ASSERT( move.capture_, "can't capture while castling" );
      X_ASSERT( !(move.from_ == 4 && color_ || move.from_ == 60 && !color_), "kings position is wrong" );
      X_ASSERT( getField(move.to_), "king position after castle is occupied" );

      d >>= 1;

      // verify if there is suitable rook for castling
      int rook_from = move.from_ + ((d>>1) ^ 3); //d < 0 ? move.from_ - 4 : move.from_ + 3

      X_ASSERT( (rook_from != 0 && color_ && d < 0) || (rook_from != 7 && color_ && d > 0), "invalid castle rook position" );
      X_ASSERT( (rook_from != 56 && !color_ && d < 0) || (rook_from != 63 && !color_ && d > 0), "invalid castle rook position" );

      int rook_to = move.from_ + d;

      X_ASSERT( getField(rook_to), "field, that rook is going to move to, is occupied" );

      X_ASSERT ( !(rook_from & 3) && getField(rook_from+1), "long castling impossible" );

      if ( isAttacked(ocolor, move.to_) || isAttacked(ocolor, rook_to) )
        return false;

      return true;
    }
    else
    {
      // other king's movements - don't put it under check
      return !isAttacked(ocolor, move.to_, move.from_);
    }
  }
  else
  {
    BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
    BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);
    int ki_pos = kingPos(color_);
    BitMask to_mask = set_mask_bit(move.to_);
    mask_all |= to_mask;
    brq_mask &= ~to_mask;

    if ( discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos) )
      return false;

    // en-passant check
    if ( move.to_ == en_passant_ && Figure::TypePawn == ffrom.type() )
    {
      int ep_pos = enpassantPos();
      BitMask mask_all_ep = (mask_all ^ set_mask_bit(move.from_)) | set_mask_bit(en_passant_);
      return !discoveredCheck(ep_pos, ocolor, mask_all_ep, brq_mask, ki_pos);
    }

    return true;
  }
}

void Board::makeMove(const Move & mv)
{
  if ( !mv )
  {
    makeNullMove();
    return;
  }

  X_ASSERT(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  halfmovesCounter_++;

  UndoInfo & undo = lastUndo();
  undo.clearUndo();
  undo = mv;

  // store Zobrist key and state
  undo.old_state_ = state_;
  undo.zcode_old_ = fmgr_.hashCode();
  undo.zcode_pawn_ = fmgr_.pawnCode();

  // store checking info
  undo.checking_figs_ = checking_figs_;
  undo.checkingNum_ = checkingNum_;

  undo.castling_ = castling_;

  state_ = Ok;

  const Figure::Color & color = color_;
  Figure::Color ocolor = Figure::otherColor(color);

  //// store masks - don't undo it to save time
  //undo.mask_[0] = fmgr_.mask(Figure::ColorBlack);
  //undo.mask_[1] = fmgr_.mask(Figure::ColorWhite);

  Field & ffrom = getField(undo.from_);
  Field & fto   = getField(undo.to_);

  // castle
  bool castle_k = castling(color, 0);
  bool castle_q = castling(color, 1);
  bool castle_kk = castle_k, castle_qq = castle_q;
  static int castle_rook_pos[2][2] = { {63, 7}, {56, 0} }; // 0 - short, 1 - long

  if ( ffrom.type() == Figure::TypeKing )
  {
    int d = undo.to_ - undo.from_;
    if ( (2 == d || -2 == d) )
    {
      undo.castle_ = true;

      // don't do castling under check
      X_ASSERT( checkingNum_ > 0, "can not castle under check" );

      d >>= 1;
      int rook_to = undo.from_ + d;
      int rook_from = undo.from_ + ((d>>1) ^ 3);//d < 0 ? undo.from_ - 4 : undo.from_ + 3

      Field & rf_field = getField(rook_from);
      X_ASSERT( rf_field.type() != Figure::TypeRook || rf_field.color() != color_, "no rook for castle" );
      X_ASSERT( !(rook_from & 3) && getField(rook_from+1), "long castle is impossible" );

      rf_field.clear();
      fmgr_.move(color_, Figure::TypeRook, rook_from, rook_to);
      Field & field_rook_to  = getField(rook_to);
      X_ASSERT( field_rook_to, "field that rook is going to undo to while castling is occupied" );
      field_rook_to.set(color_, Figure::TypeRook);
    }

    castle_kk = false;
    castle_qq = false;
  }
  // castling of current color is still possible
  else if ( (castle_k || castle_q) && (ffrom.type() == Figure::TypeRook) )
  {
    // short castle
    castle_kk = undo.from_ != castle_rook_pos[0][color];

    // long castle
    castle_qq = undo.from_ != castle_rook_pos[1][color];
  }
  
  // eat rook of another side
  if ( castling() && fto.type() == Figure::TypeRook )
  {
    bool ocastle_k = castling(ocolor, 0);
    bool ocastle_q = castling(ocolor, 1);

    if ( ocastle_k && undo.to_ == castle_rook_pos[0][ocolor] )
    {
      clear_castling(ocolor, 0);
      fmgr_.hashCastling(ocolor, 0);
    }
    else if ( ocastle_q && undo.to_ == castle_rook_pos[1][ocolor] )
    {
      clear_castling(ocolor, 1);
      fmgr_.hashCastling(ocolor, 1);
    }
  }

  // hash castle and change flags
  if ( castle_k && !castle_kk )
  {
    clear_castling(color_, 0);
    fmgr_.hashCastling(color_, 0);
  }

  if ( castle_q && !castle_qq )
  {
    clear_castling(color_, 1);
    fmgr_.hashCastling(color_, 1);
  }

  // remove captured figure
  if ( fto )
  {
    X_ASSERT(fto.color() == color_, "invalid color of captured figure" );

    undo.eaten_type_ = fto.type();
    fmgr_.decr(fto.color(), fto.type(), undo.to_);
    fto.clear();
    X_ASSERT( !undo.capture_, "capture flag isn't set" );
  }
  else if ( undo.to_ == en_passant_ && Figure::TypePawn == ffrom.type() )
  {
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    X_ASSERT( epfield.color() != ocolor || epfield.type() != Figure::TypePawn, "en-passant pawn is invalid" );
    fmgr_.decr(epfield.color(), epfield.type(), ep_pos);
    epfield.clear();
    X_ASSERT( !undo.capture_, "en-passant isn't detected as capture" );
  }
  else
  {
    X_ASSERT(undo.capture_, "capture flag set but no figure to eat");
  }

  // clear en-passant hash code
  if ( en_passant_ >= 0 )
    fmgr_.hashEnPassant(en_passant_, ocolor);

  // save en-passant field
  undo.en_passant_ = en_passant_;
  en_passant_ = -1;


  // en-passant
  if ( Figure::TypePawn == ffrom.type() )
  {
    int dir = figureDir().dir(ffrom.type(), ffrom.color(), undo.from_, undo.to_);
    if ( 3 == dir )
    {
      en_passant_ = (undo.to_ + undo.from_) >> 1;
      fmgr_.hashEnPassant(en_passant_, color_);
    }
  }

  // undo figure 'from' -> 'to'
  if ( undo.new_type_ > 0 )
  {
    // pawn promotion
    fmgr_.decr(ffrom.color(), ffrom.type(), undo.from_);
    fto.set(color_, (Figure::Type)undo.new_type_);
    fmgr_.incr(fto.color(), fto.type(), undo.to_);
  }
  else
  {
    // usual movement
    fmgr_.move(ffrom.color(), ffrom.type(), undo.from_, undo.to_);
    fto.set(color_, ffrom.type());
  }

  // clear field, figure moved from
  ffrom.clear();

  // update counters
  undo.fifty_moves_  = fiftyMovesCount_;
  undo.reps_counter_ = repsCounter_;

  if ( Figure::TypePawn == fto.type() || undo.capture_ || undo.new_type_ > 0 )
  {
    undo.irreversible_ = true;
    fiftyMovesCount_ = 0;
    repsCounter_ = 0;
  }
  else
  {
    undo.irreversible_ = false;
    fiftyMovesCount_++;
  }

  movesCounter_ += ~color_ & 1;

  // add hash color key
  fmgr_.hashColor();
  color_ = ocolor;

  // put new hash code to detect threefold repetition
  undo.zcode_ = fmgr_.hashCode();

  undo.can_win_[0] = can_win_[0];
  undo.can_win_[1] = can_win_[1];

  verifyChessDraw();

  detectCheck(undo);

  undo.state_ = state_;

  X_ASSERT( isAttacked(Figure::otherColor(color_), kingPos(color_)) && !underCheck(), "check isn't detected" );
  X_ASSERT( !isAttacked(Figure::otherColor(color_), kingPos(color_)) && underCheck(), "detected check, that doesn't exist" );
  X_ASSERT( isAttacked(color_, kingPos(Figure::otherColor(color_))), "our king is under check after undo" );
}

void Board::unmakeMove()
{
  X_ASSERT(halfmovesCounter_ <= 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  UndoInfo & undo = lastUndo();

  if ( !undo )
  {
    unmakeNullMove();
    return;
  }

  halfmovesCounter_--;

  color_ = Figure::otherColor(color_);

  // store checking info
  checking_figs_ = undo.checking_figs_;
  checkingNum_ = undo.checkingNum_;

  // restore castling flags
  castling_ = undo.castling_;

  Figure::Color ocolor = Figure::otherColor((Figure::Color)color_);

  can_win_[0] = undo.can_win_[0];
  can_win_[1] = undo.can_win_[1];

  movesCounter_ -= ~color_ & 1;

  Field & ffrom = getField(undo.from_);
  Field & fto = getField(undo.to_);

  // restore figure
  if ( undo.new_type_ > 0 )
  {
    // restore old type
    fmgr_.u_decr(fto.color(), fto.type(), undo.to_);
    fmgr_.u_incr(fto.color(), Figure::TypePawn, undo.from_);
    ffrom.set(color_, Figure::TypePawn);
  }
  else
  {
    fmgr_.u_move(fto.color(), fto.type(), undo.to_, undo.from_);
    ffrom.set(fto.color(), fto.type());
  }
  fto.clear();

  // restore en-passant
  en_passant_ = undo.en_passant_;
  if ( en_passant_ == undo.to_ && Figure::TypePawn == ffrom.type() )
  {
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    epfield.set(ocolor, Figure::TypePawn);
    fmgr_.u_incr(epfield.color(), epfield.type(), ep_pos);
  }

  // restore eaten figure
  if ( undo.eaten_type_ > 0 )
  {
    fto.set(ocolor, (Figure::Type)undo.eaten_type_);
    fmgr_.u_incr(fto.color(), fto.type(), undo.to_);
  }

  // restore rook after castling
  if ( ffrom.type() == Figure::TypeKing )
  {
    int d = undo.to_ - undo.from_;
    if ( (2 == d || -2 == d) )
    {
      d >>= 1;
      int rook_to = undo.from_ + d;
      int rook_from = undo.from_ + ((d>>1) ^ 3);//d < 0 ? undo.from_ - 4 : undo.from_ + 3

      Field & rfrom = getField(rook_from);
      Field & rto = getField(rook_to);

      rto.clear();
      rfrom.set(color_, Figure::TypeRook);
      fmgr_.u_move(color_, Figure::TypeRook, rook_to, rook_from);
    }
  }

  //// restore figures masks
  //fmgr_.restoreMasks(undo.mask_);

  fiftyMovesCount_ = undo.fifty_moves_;
  repsCounter_ = undo.reps_counter_;

  // restore hash code and state
  state_ = (State)undo.old_state_;
  fmgr_.restoreHash(undo.zcode_old_);
  fmgr_.restorePawnCode(undo.zcode_pawn_);
}

bool Board::verifyChessDraw()
{
  if ( (fiftyMovesCount_ == 100 && !underCheck()) || (fiftyMovesCount_ > 100) )
  {
    state_ |= Draw50Moves;
    return true;
  }

  can_win_[0] = ( fmgr_.pawns(Figure::ColorBlack) || fmgr_.rooks(Figure::ColorBlack) || fmgr_.queens(Figure::ColorBlack) ) ||
    ( fmgr_.knights(Figure::ColorBlack) + fmgr_.bishops(Figure::ColorBlack) > 1 );

  can_win_[1] = ( fmgr_.pawns(Figure::ColorWhite) || fmgr_.rooks(Figure::ColorWhite) || fmgr_.queens(Figure::ColorWhite) ) ||
    ( fmgr_.knights(Figure::ColorWhite) + fmgr_.bishops(Figure::ColorWhite) > 1 );

  if ( !can_win_[0] && !can_win_[1] )
  {
    if ( (fmgr_.knights(Figure::ColorWhite) + fmgr_.bishops(Figure::ColorWhite) +
          fmgr_.knights(Figure::ColorBlack) + fmgr_.bishops(Figure::ColorBlack)) == 1 )
    {
      state_ |= DrawInsuf;
      return true;
    }
  }

  int reps = countReps();

  if ( reps > repsCounter_ )
    repsCounter_ = reps;

  if ( reps >= 3 )
  {
    state_ |= DrawReps;
    return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////
// NULL MOVE
void Board::makeNullMove()
{
  X_ASSERT(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  halfmovesCounter_++;

  UndoInfo & undo = lastUndo();
  undo.clear();
  undo.clearUndo();

  undo.zcode_old_ = hashCode();
  undo.zcode_pawn_ = pawnCode();
  undo.state_ = state_;

  if ( en_passant_ >= 0 )
  {
    fmgr_.hashEnPassant(en_passant_, color_);
    undo.en_passant_ = en_passant_;
    en_passant_ = -1;
  }
  else
    undo.en_passant_ = -1;

  color_ = Figure::otherColor(color_);
  fmgr_.hashColor();
  undo.zcode_ = hashCode();
}

void Board::unmakeNullMove()
{
  X_ASSERT(halfmovesCounter_ <= 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  UndoInfo & undo = lastUndo();
  halfmovesCounter_--;

  color_ = Figure::otherColor(color_);
  en_passant_ = undo.en_passant_;
  fmgr_.restoreHash(undo.zcode_old_);
  fmgr_.restorePawnCode(undo.zcode_pawn_);
  state_ = (State)undo.state_;
}


#ifdef VALIDATE_VALIDATOR
bool Board::validateMove(const Move & mv) const
{
  bool ok = validateMove2(mv);
  Board board1(*this);
  bool valid = board1.validateValidator(mv);
  char fen[256];
  toFEN(fen);
  if ( ok != valid )
    validateMove2(mv);
  X_ASSERT( ok != valid, "validateMove() failed" );
  return ok;
}

bool Board::validateValidator(const Move & mv)
{
  halfmovesCounter_++;

  UndoInfo & undo = lastUndo();
  undo.clearUndo();
  undo = mv;

  // store Zobrist key and state
  undo.old_state_ = state_;
  undo.zcode_old_ = fmgr_.hashCode();
  undo.zcode_pawn_ = fmgr_.pawnCode();

  // store checking info
  undo.checking_figs_ = checking_figs_;
  undo.checkingNum_ = checkingNum_;

  undo.castling_ = castling_;

  state_ = Ok;

  const Figure::Color & color = color_;
  Figure::Color ocolor = Figure::otherColor(color);

  // store masks - don't undo it to save time
  undo.mask_[0] = fmgr_.mask(Figure::ColorBlack);
  undo.mask_[1] = fmgr_.mask(Figure::ColorWhite);

  Field & ffrom = getField(undo.from_);
  Field & fto   = getField(undo.to_);

  // castle
  bool castle_k = castling(color, 0);
  bool castle_q = castling(color, 1);
  bool castle_kk = castle_k, castle_qq = castle_q;
  static int castle_rook_pos[2][2] = { {63, 7}, {56, 0} }; // 0 - short, 1 - long

  if ( ffrom.type() == Figure::TypeKing )
  {
    int d = undo.to_ - undo.from_;
    if ( (2 == d || -2 == d) )
    {
      undo.castle_ = true;

      // don't do castling under check
      if ( checkingNum_ > 0 )
        return false;

      d >>= 1;
      int rook_to = undo.from_ + d;
      int rook_from = undo.from_ + ((d>>1) ^ 3);//d < 0 ? undo.from_ - 4 : undo.from_ + 3

      Field & rf_field = getField(rook_from);
      X_ASSERT( rf_field.type() != Figure::TypeRook || rf_field.color() != color_, "no rook for castle" );
      X_ASSERT( !(rook_from & 3) && getField(rook_from+1), "long castle is impossible" );

      rf_field.clear();
      fmgr_.move(color_, Figure::TypeRook, rook_from, rook_to);
      Field & field_rook_to  = getField(rook_to);
      X_ASSERT( field_rook_to, "field that rook is going to undo to while castling is occupied" );
      field_rook_to.set(color_, Figure::TypeRook);

      if ( isAttacked(ocolor, rook_to) || isAttacked(ocolor, undo.to_) )
        return false;
    }

    castle_kk = false;
    castle_qq = false;
  }
  // castling of current color is still possible
  else if ( (castle_k || castle_q) && (ffrom.type() == Figure::TypeRook) )
  {
    // short castle
    castle_kk = undo.from_ != castle_rook_pos[0][color];

    // long castle
    castle_qq = undo.from_ != castle_rook_pos[1][color];
  }

  // eat rook of another side
  if ( castling() && fto.type() == Figure::TypeRook )
  {
    bool ocastle_k = castling(ocolor, 0);
    bool ocastle_q = castling(ocolor, 1);

    if ( ocastle_k && undo.to_ == castle_rook_pos[0][ocolor] )
    {
      clear_castling(ocolor, 0);
      fmgr_.hashCastling(ocolor, 0);
    }
    else if ( ocastle_q && undo.to_ == castle_rook_pos[1][ocolor] )
    {
      clear_castling(ocolor, 1);
      fmgr_.hashCastling(ocolor, 1);
    }
  }

  // hash castle and change flags
  if ( castle_k && !castle_kk )
  {
    clear_castling(color_, 0);
    fmgr_.hashCastling(color_, 0);
  }

  if ( castle_q && !castle_qq )
  {
    clear_castling(color_, 1);
    fmgr_.hashCastling(color_, 1);
  }

  // remove captured figure
  if ( fto )
  {
    X_ASSERT(fto.color() == color_, "invalid color of captured figure" );

    undo.eaten_type_ = fto.type();
    fmgr_.decr(fto.color(), fto.type(), undo.to_);
    fto.clear();
    X_ASSERT( !undo.capture_, "capture flag isn't set" );
  }
  else if ( undo.to_ == en_passant_ && Figure::TypePawn == ffrom.type() )
  {
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    X_ASSERT( epfield.color() != ocolor || epfield.type() != Figure::TypePawn, "en-passant pawn is invalid" );
    fmgr_.decr(epfield.color(), epfield.type(), ep_pos);
    epfield.clear();
    X_ASSERT( !undo.capture_, "en-passant isn't detected as capture" );
  }
  else
  {
    X_ASSERT(undo.capture_, "capture flag set but no figure to eat");
  }

  // clear en-passant hash code
  if ( en_passant_ >= 0 )
    fmgr_.hashEnPassant(en_passant_, ocolor);

  // save en-passant field
  undo.en_passant_ = en_passant_;
  en_passant_ = -1;


  // en-passant
  if ( Figure::TypePawn == ffrom.type() )
  {
    int dir = figureDir().dir(ffrom.type(), ffrom.color(), undo.from_, undo.to_);
    if ( 3 == dir )
    {
      en_passant_ = (undo.to_ + undo.from_) >> 1;
      fmgr_.hashEnPassant(en_passant_, color_);
    }
  }

  // undo figure 'from' -> 'to'
  if ( undo.new_type_ > 0 )
  {
    // pawn promotion
    fmgr_.decr(ffrom.color(), ffrom.type(), undo.from_);
    fto.set(color_, (Figure::Type)undo.new_type_);
    fmgr_.incr(fto.color(), fto.type(), undo.to_);
  }
  else
  {
    // usual movement
    fmgr_.move(ffrom.color(), ffrom.type(), undo.from_, undo.to_);
    fto.set(color_, ffrom.type());
  }

  // clear field, figure moved from
  ffrom.clear();

  // update counters
  undo.fifty_moves_  = fiftyMovesCount_;
  undo.reps_counter_ = repsCounter_;

  if ( Figure::TypePawn == fto.type() || undo.capture_ || undo.new_type_ > 0 )
  {
    undo.irreversible_ = true;
    fiftyMovesCount_ = 0;
    repsCounter_ = 0;
  }
  else
  {
    undo.irreversible_ = false;
    fiftyMovesCount_++;
  }

  movesCounter_ += ~color_ & 1;

  // add hash color key
  fmgr_.hashColor();
  color_ = ocolor;

  // put new hash code to detect threefold repetition
  undo.zcode_ = fmgr_.hashCode();

  undo.can_win_[0] = can_win_[0];
  undo.can_win_[1] = can_win_[1];

  verifyChessDraw();

  detectCheck(undo);

  undo.state_ = state_;

  X_ASSERT( isAttacked(Figure::otherColor(color_), kingPos(color_)) && !underCheck(), "check isn't detected" );
  X_ASSERT( !isAttacked(Figure::otherColor(color_), kingPos(color_)) && underCheck(), "detected check, that doesn't exist" );
  if ( isAttacked(color_, kingPos(Figure::otherColor(color_))) )
    return false;

  return true;
}
#endif

int Board::calculateReps(const Move & move) const
{
  const Field & ffrom = getField(move.from_);
  const Field & fto   = getField(move.to_);

  // capture or pawn movement
  if ( move.capture_ || ffrom.type() == Figure::TypePawn )
    return 0;

  // castle
  if ( ffrom.type() == Figure::TypeKing && castling(color_) )
      return 0;

  // 1st rook movement
  if ( ffrom.type() == Figure::TypeRook )
  {
      if ( ( color_ && (castling_K() && move.from_ ==  0 || castling_Q() && move.from_ ==  7)) ||
           (!color_ && (castling_k() && move.from_ == 56 || castling_q() && move.from_ == 63)) )
      {
        return 0;
      }
  }

  // calculate new hash value
  BitMask zcode = hashCode();
  Figure::Color ocolor = Figure::otherColor(color_);

  if ( en_passant_ > 0 )
  {
    const BitMask & enpassantCode = fmgr().enpassantCode(en_passant_, ocolor);
    zcode ^= enpassantCode;
  }

  // movement
  {
    const BitMask & uc0 = fmgr().code(color_, ffrom.type(), move.from_);
    const BitMask & uc1 = fmgr().code(color_, ffrom.type(), move.to_);
    zcode ^= uc0;
    zcode ^= uc1;
  }

  // color
  zcode ^= fmgr().colorCode();

  return countReps(2, zcode);
}

int Board::countReps() const
{
  return countReps(3, hashCode());
}

} // NEngine
