/*************************************************************
MoveMaker.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <Board.h>
#include <FigureDirs.h>

namespace NEngine
{

void Board::makeMove(const Move & move)
{
  X_ASSERT(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  halfmovesCounter_++;

  auto& undo = lastUndo();

  // save general data
  undo.data_ = data_;
  // save Zobrist keys
  undo.zcode_ = fmgr_.hashCode();
  undo.zcode_kpw_ = fmgr_.kpwnCode();
  // clear flags
  undo.mflags_ = 0;
  // clear eaten type
  undo.eaten_type_ = 0;
  // save move
  undo.move_ = move;
  // save PSQ eval
  undo.psq32_ = fmgr_.eval32();
  // set state
  data_.state_ = Ok;

  Figure::Color ocolor = Figure::otherColor(color());

  Field & ffrom = getField(move.from());
  Field & fto = getField(move.to());

  // castle
  auto castle_k = castling(color(), 0);
  auto castle_q = castling(color(), 1);
  static int castle_rook_pos[2][2] = { { 63, 7 }, { 56, 0 } }; // 0 - short, 1 - long
  if(ffrom.type() == Figure::TypeKing)
  {
    auto d = move.to() - move.from();
    if((2 == d || -2 == d))
    {
      undo.mflags_ |= UndoInfo::Castle | UndoInfo::Irreversible;

      // don't do castling under check
      X_ASSERT(underCheck(), "can not castle under check");

      d >>= 1;
      int rook_to = move.from() + d;
      int rook_from = move.from() + ((d>>1) ^ 3);//d < 0 ? undo.from_ - 4 : undo.from_ + 3

      Field & rf_field = getField(rook_from);
      X_ASSERT(rf_field.type() != Figure::TypeRook || rf_field.color() != color(), "no rook for castle");
      X_ASSERT(!(rook_from & 3) && getField(rook_from+1), "long castle is impossible");

      rf_field.clear();
      fmgr_.move(color(), Figure::TypeRook, rook_from, rook_to);
      auto& field_rook_to = getField(rook_to);
      X_ASSERT(field_rook_to, "field that rook is going to undo to while castling is occupied");
      field_rook_to.set(color(), Figure::TypeRook);
    }
    // hash castle and change flags
    if(castle_k)
    {
      clear_castling(color(), 0);
      fmgr_.hashCastling(color(), 0);
    }
    if(castle_q)
    {
      clear_castling(color(), 1);
      fmgr_.hashCastling(color(), 1);
    }
    king_pos_[color()] = move.to();
  }
  // is castling still possible?
  else if((castle_k || castle_q) && ffrom.type() == Figure::TypeRook)
  {
    // short castle
    if(castle_k && (move.from() == castle_rook_pos[0][color()]))
    {
      clear_castling(color(), 0);
      fmgr_.hashCastling(color(), 0);
    }
    // long castle
    if(castle_q && (move.from() == castle_rook_pos[1][color()]))
    {
      clear_castling(color(), 1);
      fmgr_.hashCastling(color(), 1);
    }
  }

  // remove captured figure
  if(fto)
  {
    X_ASSERT(fto.color() == color(), "invalid color of captured figure");

    // eat rook of another side
    if(castling() && fto.type() == Figure::TypeRook)
    {
      if(move.to() == castle_rook_pos[0][ocolor] && castling(ocolor, 0))
      {
        clear_castling(ocolor, 0);
        fmgr_.hashCastling(ocolor, 0);
      }
      else if(move.to() == castle_rook_pos[1][ocolor] && castling(ocolor, 1))
      {
        clear_castling(ocolor, 1);
        fmgr_.hashCastling(ocolor, 1);
      }
    }

    undo.eaten_type_ = fto.type();
    fmgr_.decr(fto.color(), fto.type(), move.to());
    fto.clear();
    undo.mflags_ |= UndoInfo::Capture | UndoInfo::Irreversible;
  }
  else if(Figure::TypePawn == ffrom.type() && move.to() != 0 && move.to() == enpassant())
  {
    int ep_pos = enpassantPos();
    auto& epfield = getField(ep_pos);
    X_ASSERT(epfield.color() != ocolor || epfield.type() != Figure::TypePawn, "en-passant pawn is invalid");
    fmgr_.decr(epfield.color(), epfield.type(), ep_pos);
    epfield.clear();
    undo.mflags_ |= UndoInfo::Capture | UndoInfo::Irreversible | UndoInfo::EnPassant;
  }

  // clear en-passant hash code
  if(enpassant() > 0)
    fmgr_.hashEnPassant(enpassant(), ocolor);

  data_.en_passant_ = -1;

  // 'from' -> 'to'
  if(move.new_type() == 0)
  {
    // en-passant
    if(Figure::TypePawn == ffrom.type() && !undo.capture())
    {
      int dy = move.from() - move.to();
      if(dy == 16 || dy == -16)
      {
        data_.en_passant_ = (move.to() + move.from()) >> 1;
        fmgr_.hashEnPassant(data_.en_passant_, color());
      }
      undo.mflags_ |= UndoInfo::Irreversible;
    }
    // usual movement
    fmgr_.move(ffrom.color(), ffrom.type(), move.from(), move.to());
    fto.set(color(), ffrom.type());
  }
  else
  {
    X_ASSERT(Figure::TypePawn != ffrom.type(), "non-pawn move but with promotion");
    // pawn promotion
    fmgr_.decr(ffrom.color(), ffrom.type(), move.from());
    fto.set(color(), (Figure::Type)move.new_type());
    fmgr_.incr(color(), (Figure::Type)move.new_type(), move.to());
    undo.mflags_ |= UndoInfo::Irreversible;
  }

  // clear field, figure moved from
  ffrom.clear();

  if(undo.irreversible())
  {
    data_.fiftyMovesCount_ = 0;
    data_.repsCounter_ = 1;
  }
  else
  {
    data_.fiftyMovesCount_++;
  }

  movesCounter_ += ocolor;

  // add hash color key
  fmgr_.hashColor();
  setColor(ocolor);

  verifyChessDraw(undo.irreversible());

  detectCheck(move);

  X_ASSERT(isAttacked(Figure::otherColor(color()), kingPos(color())) && !underCheck(), "check isn't detected");
  X_ASSERT(!isAttacked(Figure::otherColor(color()), kingPos(color())) && underCheck(), "detected check, that doesn't exist");
  X_ASSERT(isAttacked(color(), kingPos(Figure::otherColor(color()))), "our king is under check after undo");
}

void Board::unmakeMove(const Move& move)
{
  X_ASSERT(halfmovesCounter_ <= 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  auto& undo = lastUndo();

  halfmovesCounter_--;

  Figure::Color ocolor = color();

  data_ = undo.data_;
  fmgr_.resoreEval(undo.psq32_);

  movesCounter_ -= ocolor;

  auto& ffrom = getField(move.from());
  auto& fto   = getField(move.to());

  // restore figure
  if(move.new_type() != 0)
  {
    // restore old type
    fmgr_.u_decr(fto.color(), fto.type(), move.to());
    fmgr_.u_incr(fto.color(), Figure::TypePawn, move.from());
    ffrom.set(color(), Figure::TypePawn);
  }
  else
  {
    fmgr_.u_move(fto.color(), fto.type(), move.to(), move.from());
    ffrom.set(fto.color(), fto.type());
    if(ffrom.type() == Figure::TypeKing)
      king_pos_[color()] = move.from();
  }
  fto.clear();

  // restore en-passant
  if(undo.is_enpassant())
  {
    X_ASSERT(undo.enpassant() <= 0 || undo.enpassant() != move.to() || Figure::TypePawn != ffrom.type(), "en-passant error");
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    epfield.set(ocolor, Figure::TypePawn);
    fmgr_.u_incr(epfield.color(), epfield.type(), ep_pos);
  }

  // restore eaten figure
  if(undo.eaten_type_ > 0)
  {
    fto.set(ocolor, (Figure::Type)undo.eaten_type_);
    fmgr_.u_incr(fto.color(), fto.type(), move.to());
  }

  // restore rook after castling
  if(undo.castle())
  {
    X_ASSERT(ffrom.type() != Figure::TypeKing, "castle but no king move");
    auto d = move.to() - move.from();
    X_ASSERT(!(2 == d || -2 == d), "invalid king move while castle");
    d >>= 1;
    int rook_to = move.from() + d;
    int rook_from = move.from() + ((d>>1) ^ 3);//d < 0 ? undo.from_ - 4 : undo.from_ + 3

    auto& rfrom = getField(rook_from);
    auto& rto   = getField(rook_to);

    rto.clear();
    rfrom.set(color(), Figure::TypeRook);
    fmgr_.u_move(color(), Figure::TypeRook, rook_to, rook_from);
  }

  fmgr_.restoreHash(undo.zcode_);
  fmgr_.restoreKpwnCode(undo.zcode_kpw_);
}

void Board::makeNullMove()
{
  X_ASSERT(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  halfmovesCounter_++;

  auto& undo = lastUndo();

  // save general data
  undo.data_ = data_;
  // save Zobrist keys
  undo.zcode_ = fmgr_.hashCode();
  undo.zcode_kpw_ = fmgr_.kpwnCode();

  X_ASSERT(data_.state_ != Ok, "try null-move on invalid state");

  auto ocolor = Figure::otherColor(color());

  // clear en-passant hash code
  if(data_.en_passant_ >= 0)
    fmgr_.hashEnPassant(enpassant(), ocolor);

  data_.en_passant_ = -1;

  fmgr_.hashColor();
  setColor(ocolor);
}

void Board::unmakeNullMove()
{
  X_ASSERT(halfmovesCounter_ <= 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  auto& undo = lastUndo();

  halfmovesCounter_--;

  auto ocolor = color();

  data_ = undo.data_;

  fmgr_.restoreHash(undo.zcode_);
  fmgr_.restoreKpwnCode(undo.zcode_kpw_);
}

} // NEngine
