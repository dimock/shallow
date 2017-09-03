/*************************************************************
  Board.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Board.h>
#include <FigureDirs.h>
#include <Evaluator.h>
#include <xindex.h>
#include <Helpers.h>
#include <MovesGenerator.h>

namespace NEngine
{

bool Board::addFigure(const Figure::Color color, const Figure::Type type, int pos)
{
  if(!type || (unsigned)pos > 63)
    return false;

  auto& field = getField(pos);
  if(field)
    return false;

  field.set(color, type);
  fmgr_.incr(color, type, pos);

  if(type == Figure::TypeKing)
    king_pos_[color] = pos;

  return false;
}

// physically possible
bool Board::possibleMove(const Move & move) const
{
  X_ASSERT(move.to() > 63 || move.from() > 63, "invalid move given");

  const auto& fto = getField(move.to());
  if(fto && (fto.color() == color() || fto.type() == Figure::TypeKing))
    return false;

  const auto& ffrom = getField(move.from());
  if(!ffrom || ffrom.color() != color())
    return false;

  // only kings move is possible
  if(underCheck() && doubleCheck())
    return ffrom.type() == Figure::TypeKing;

  if(ffrom.type() != Figure::TypePawn && move.new_type() > 0)
    return false;

  int dir = figureDir().dir(ffrom.type(), color(), move.from(), move.to());
  if(dir < 0)
    return false;

  // castle
  if(ffrom.type() == Figure::TypeKing && (8 == dir || 9 == dir))
  {
    if(underCheck())
      return false;

    if(!castling(color(), dir-8)) // 8 means short castling, 9 - long
      return false;

    if(color() && move.from() != 4 || !color() && move.from() != 60)
      return false;

    int d = (move.to() - move.from()) >> 1; // +1 short, -1 long
    if(getField(move.to()) || getField(move.from() + d))
      return false;

    // long castling - field right to rook (or left to king) is occupied
    if(dir == 9 && getField(move.to()-1))
      return false;
  }

  switch(ffrom.type())
  {
  case Figure::TypePawn:
  {
    auto y = move.to() >> 3;
    if(((7 == y || 0 == y) && !move.new_type()) || (0 != y && 7 != y && move.new_type()))
      return false;
    X_ASSERT(move.new_type() < 0 || move.new_type() > Figure::TypeQueen, "invalid promotion piece");
    if(fto || move.to() > 0 && move.to() == enpassant())
    {
      return (0 == dir || 1 == dir);
    }
    else
    {
      auto to3 = move.from() + ((move.to()-move.from()) >> 1);
      X_ASSERT((uint8)to3 > 63 || 3 == dir && to3 != move.from() + (move.to() > move.from() ? 8 : -8), "pawn goes to invalid field");
      return (2 == dir) || (3 == dir && !getField(to3));
    }
  }
  break;

  case Figure::TypeBishop:
  case Figure::TypeRook:
  case Figure::TypeQueen:
  {
    const auto& mask = betweenMasks().between(move.from(), move.to());
    const auto& black = fmgr_.mask(Figure::ColorBlack);
    const auto& white = fmgr_.mask(Figure::ColorWhite);

    bool ok = (mask & ~(black | white)) == mask;
    return ok;
  }
  break;
  }

  return true;
}

bool Board::escapeMove(const Move& move) const
{
  X_ASSERT(!underCheck(), "do escape move but not under check");
  X_ASSERT(move.new_type() || getField(move.to())
           || (move.to() > 0 && enpassant() == move.to() && getField(move.from()).type() == Figure::TypePawn), "killer should not be capture");
  if(getField(move.from()).type() == Figure::TypeKing)
  {
    X_ASSERT(getField(move.to()) && getField(move.to()).color() == color(), "capture own figure");
    return true;
  }
  X_ASSERT(doubleCheck(), "double check but no-king move");
  auto const& mask = betweenMasks().between(kingPos(color()), checking());
  return (mask & set_mask_bit(move.to())) != 0ULL;
}

bool Board::moveExists(const Move& move) const
{
  bool found{ false };
  auto moves = generate<Board, Move>(*this);
  for(auto m : moves)
  {
    if(!validateMoveBruteforce(m))
      continue;
    if(m == move)
    {
      found = true;
      break;
    }
  }
  return found;
}

bool Board::hasMove() const
{
  auto moves = generate<Board, Move>(*this);
  for(auto m : moves)
  {
    if(!validateMoveBruteforce(m))
      continue;
    return true;
  }
  return false;
}

bool Board::hasReps(const Move & move) const
{
  const auto& ffrom = getField(move.from());
  const auto& fto = getField(move.to());

  // capture or pawn movement
  if(ffrom.type() == Figure::TypePawn || fto)
    return false;

  // castle
  if(ffrom.type() == Figure::TypeKing && castling(color()))
    return false;

  // 1st rook movement
  if(ffrom.type() == Figure::TypeRook)
  {
    if((color() && (castling_K() && move.from() ==  0 || castling_Q() && move.from() ==  7)) ||
        (!color() && (castling_k() && move.from() == 56 || castling_q() && move.from() == 63)))
    {
      return false;
    }
  }

  // calculate new hash value
  auto zcode = fmgr().hashCode();
  auto ocolor = Figure::otherColor(color());

  if(enpassant() > 0)
  {
    zcode ^= fmgr().enpassantCode(enpassant(), ocolor);
  }

  // movement
  {
    zcode ^= fmgr().code(color(), ffrom.type(), move.from());
    zcode ^= fmgr().code(color(), ffrom.type(), move.to());
  }

  // color
  zcode ^= fmgr().colorCode();

  {
    bool reps = false;
    auto i = halfmovesCounter_ - 1;
    auto stop = halfmovesCounter_ - data_.fiftyMovesCount_; // TODO: is it correct?
    for(; i >= stop && !reps; i -= 2)
    {
      if(undoInfo(i).zcode_ == zcode)
        reps++;
      if(undoInfo(i).irreversible())
        break;
    }
    // may be we forget to test initial position?
    if(!reps && halfmovesCounter_ > 0 && i == -1 && zcode == undoInfo(0).zcode_)
      return true;
    return reps;
  }
}


// for initialization only
bool Board::isCastlePossible(Figure::Color c, int t) const
{
  static const int king_position[] = { 60, 4 };
  static const int rook_position[2][2] = { { 63, 56 }, { 7, 0 } };
  if(kingPos(c) != king_position[c])
    return false;
  X_ASSERT((unsigned)t > 1, "invalid castle type");
  auto const& rfield = getField(rook_position[c][t]);
  return rfield && rfield.type() == Figure::TypeRook && rfield.color() == c;
}

void Board::clear()
{
  auto* undoStack = g_undoStack;
  *this = Board{};
  g_undoStack = undoStack;
}

void Board::setMovesCounter(int c)
{
  movesCounter_ = c;
  halfmovesCounter_ = (c - 1)*2 + !color();
  if(data_.fiftyMovesCount_ > halfmovesCounter_)
    data_.fiftyMovesCount_ = halfmovesCounter_;
}

bool Board::invalidate()
{
  X_ASSERT(data_.fiftyMovesCount_ > halfmovesCounter_, "invalid moves counters");

  Figure::Color ocolor = Figure::otherColor(color());

  int ki_pos  = kingPos(color());
  int oki_pos = kingPos(ocolor);

  data_.state_ = Ok;

  bool failed = isAttacked(color(), oki_pos);
  X_ASSERT(failed, "not moving king is under check");
  if(failed)
  {
    data_.state_ = Invalid;
    return false;
  }

  findCheckingFigures(ocolor, ki_pos);

  X_ASSERT(isAttacked(ocolor, ki_pos) && !underCheck(), "invalid check detection");

  verifyChessDraw(false);

  if(drawState())
    return true;

  verifyState();

  return true;
}

bool Board::fieldAttacked(const Figure::Color c, int8 pos, const BitMask & mask_all_inv) const
{
  auto ocolor = Figure::otherColor(c);

  {
    // knights
    const BitMask & n_caps = movesTable().caps(Figure::TypeKnight, pos);
    const BitMask & knight_msk = fmgr_.knight_mask(c);
    if(n_caps & knight_msk)
      return true;

    // pawns
    const BitMask & p_caps = movesTable().pawnCaps(ocolor, pos);
    const BitMask & pawn_msk = fmgr_.pawn_mask(c);
    if(p_caps & pawn_msk)
      return true;

    // king
    const BitMask & k_caps = movesTable().caps(Figure::TypeKing, pos);
    const BitMask & king_msk = fmgr_.king_mask(c);
    if(k_caps & king_msk)
      return true;
  }

  // all long-range figures
  const BitMask & q_caps = movesTable().caps(Figure::TypeQueen, pos);
  BitMask mask_brq = fmgr_.bishop_mask(c) | fmgr_.rook_mask(c) | fmgr_.queen_mask(c);
  mask_brq &= q_caps;

  // do we have at least 1 attacking figure
  if(mask_brq)
  {
    // rooks
    const BitMask & r_caps = movesTable().caps(Figure::TypeRook, pos);
    BitMask rook_msk = fmgr_.rook_mask(c) & r_caps;
    for(; rook_msk;)
    {
      int n = clear_lsb(rook_msk);

      X_ASSERT((unsigned)n > 63, "invalid bit found in attack detector");
      X_ASSERT(!getField(n) || getField(n).type() != Figure::TypeRook, "no figure but mask bit is 1");
      X_ASSERT(getField(n).color() != c, "invalid figure color in attack detector");

      if(is_nothing_between(n, pos, mask_all_inv))
        return true;
    }

    // bishops
    const BitMask & b_caps = movesTable().caps(Figure::TypeBishop, pos);
    BitMask bishop_msk = fmgr_.bishop_mask(c) & b_caps;
    for(; bishop_msk;)
    {
      int n = clear_lsb(bishop_msk);

      X_ASSERT((unsigned)n > 63, "invalid bit found in attack detector");
      X_ASSERT(!getField(n) || getField(n).type() != Figure::TypeBishop, "no figure but mask bit is 1");
      X_ASSERT(getField(n).color() != c, "invalid figure color in attack detector");

      if(is_nothing_between(n, pos, mask_all_inv))
        return true;
    }

    // queens
    BitMask queen_msk = fmgr_.queen_mask(c) & q_caps;
    for(; queen_msk;)
    {
      int n = clear_lsb(queen_msk);

      X_ASSERT((unsigned)n > 63, "invalid bit found in attack detector");
      X_ASSERT(!getField(n) || getField(n).type() != Figure::TypeQueen, "no figure but mask bit is 1");
      X_ASSERT(getField(n).color() != c, "invalid figure color in attack detector");

      if(is_nothing_between(n, pos, mask_all_inv))
        return true;
    }
  }

  return false;
}

void Board::findCheckingFigures(Figure::Color ocolor, int ki_pos)
{
  int count = 0;
  for(int type = Figure::TypePawn; type < Figure::TypeKing; ++type)
  {
    BitMask mask = fmgr_.type_mask((Figure::Type)type, ocolor);
    for(; mask;)
    {
      int n = clear_lsb(mask);
      const Field & field = getField(n);

      X_ASSERT(field.color() != ocolor || field.type() != type, "invalid figures mask in check detector");

      int dir = figureDir().dir(field.type(), ocolor, n, ki_pos);
      if((dir < 0) || (Figure::TypePawn == type && (2 == dir || 3 == dir)))
        continue;

      X_ASSERT(Figure::TypeKing == type, "king checks king");

      if(Figure::TypePawn == type || Figure::TypeKnight == type)
      {
        ++count;
        data_.checking_ = n;
        continue;
      }

      FPos dp = deltaPosCounter().getDeltaPos(n, ki_pos);

      X_ASSERT(FPos(0, 0) == dp, "invalid attacked position");

      FPos p = FPosIndexer::get(ki_pos) + dp;
      const FPos & figp = FPosIndexer::get(n);
      bool have_figure = false;
      for(; p != figp; p += dp)
      {
        const Field & field = getField(p.index());
        if(field)
        {
          have_figure = true;
          break;
        }
      }

      if(!have_figure)
      {
        ++count;
        data_.checking_ = n;
      }
    }
  }
  X_ASSERT(count > 2, "more than 2 checking figures");
  if(count > 0)
    data_.state_ |= State::UnderCheck;
  if(count > 1)
    data_.state_ |= State::DoubleCheck;
}

// verify is there draw or mat
void Board::verifyState()
{
  if(matState() || drawState())
    return;

  auto moves = generate<Board, Move>(*this);
  bool found = false;
  for(auto it = moves.begin(); it != moves.end() && !found; ++it)
  {
    auto const& move = *it;
    if(validateMoveBruteforce(move))
      found = true;
  }

  if(!found)
  {
    if(underCheck())
      data_.state_ |= State::ChessMat;
    else
      data_.state_ |= State::Stalemat;

    // update move's state because it is last one
    if(halfmovesCounter_ > 0)
    {
      auto& undo = undoInfo(halfmovesCounter_-1);
      undo.data_.state_ = data_.state_;
    }
  }
}

/// slow, but verify all the conditions
bool Board::validateMoveBruteforce(const Move & move) const
{
  if(drawState() || matState())
    return false;
  const auto & ffrom = getField(move.from());
  X_ASSERT(ffrom.type() != Figure::TypePawn && move.new_type() > 0, "not a pawn promotion");
  X_ASSERT(!ffrom || ffrom.color() != color(), "no moving figure or invalid color");
  auto ocolor = Figure::otherColor(color());
  X_ASSERT(move.to() == kingPos(ocolor), "try to capture king");
  X_ASSERT(getField(move.to()) && getField(move.to()).color() == color(), "try to capture own figure");
  auto mask_all = (fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack));
  X_ASSERT(underCheck() != fieldAttacked(ocolor, kingPos(color()), ~mask_all), "king under attack differs from check flag");
  X_ASSERT(fieldAttacked(color(), kingPos(ocolor), ~mask_all), "opponents king is under check");
  auto const& king_pos = kingPos(color());
  if(ffrom.type() == Figure::TypeKing)
  {
    X_ASSERT(move.from() != king_pos, "king position is invalid");
    X_ASSERT(underCheck() && std::abs(Index(move.from()).x()-Index(move.to()).x()) == 2, "try to caslte under check");
    X_ASSERT(std::abs(Index(move.from()).x()-Index(move.to()).x()) > 2 || std::abs(Index(move.from()).y()-Index(move.to()).y()) > 1, "invalid king move");
    X_ASSERT(std::abs(Index(move.from()).x()-Index(move.to()).x()) == 2 && std::abs(Index(move.from()).y()-Index(move.to()).y()) != 0, "invalid king move");
    auto mask_all_inv = ~(mask_all ^ set_mask_bit(move.from()));
    if(fieldAttacked(ocolor, move.to(), mask_all_inv))
      return false;
    if(underCheck())
      return true;
    if(std::abs(Index(move.from()).x()-Index(move.to()).x()) == 2)
    {
      int ctype = Index(move.from()).x() > Index(move.to()).x();
      X_ASSERT(!castling(color(), ctype), "castle is not allowed");
      X_ASSERT(!(move.from() == 4 && color() || move.from() == 60 && !color()), "kings position is wrong");
      X_ASSERT(color() && move.to() != 2 && move.to() != 6, "white castle final king position is wrong");
      X_ASSERT(!color() && move.to() != 58 && move.to() != 62, "black castle final king position is wrong");
      X_ASSERT(getField(move.to()), "king position after castle is occupied");
      int king_through_pos = color()
        ? (ctype ? 3 : 5)
        : (ctype ? 59 : 61);
      X_ASSERT(getField(king_through_pos), "field, that rook is going to move to, is occupied");
      int rook_through_pos = ctype
        ? (color() ? 1 : 57)
        : 0;
      X_ASSERT(ctype == 1 && getField(rook_through_pos), "long castling impossible, next to rook field is occupied");
      return !fieldAttacked(ocolor, king_through_pos, mask_all_inv);
    }
    return true;
  }
  X_ASSERT(underCheck() && doubleCheck(), "only king movements allowed");
  auto pw_mask = fmgr_.pawn_mask(ocolor) & ~set_mask_bit(move.to());
  // exclude pawn, captured by en-passant
  if(enpassant() > 0 && move.to() == enpassant() && Figure::TypePawn == ffrom.type())
  {
    auto ep_pos = enpassantPos();
    pw_mask &= ~set_mask_bit(ep_pos);
  }
  auto pw_caps = movesTable().pawnCaps(color(), king_pos);
  // king is still under pawn's check?
  if(pw_mask & pw_caps)
    return false;
  auto kn_mask = fmgr_.knight_mask(ocolor) & ~set_mask_bit(move.to());
  if(kn_mask & movesTable().caps(Figure::TypeKnight, king_pos))
    return false;
  X_ASSERT(fmgr_.king_mask(ocolor) & movesTable().caps(Figure::TypeKing, king_pos), "king attacks king");
  int en_pos = -1;
  if(enpassant() > 0 && enpassant() == move.to() && ffrom.type() == Figure::TypePawn)
  {
    en_pos = enpassantPos();
  }
  // bishops, rooks and queens movements
  for(int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
  {
    auto fg_mask = fmgr_.type_mask((Figure::Type)type, ocolor);
    for(; fg_mask;)
    {
      int fg_pos = clear_lsb(fg_mask);
      // recently captured figure
      if(fg_pos == move.to())
        continue;
      const uint16* table = movesTable().move(type-Figure::TypeBishop, fg_pos);
      for(; *table; ++table)
      {
        const auto* packed = reinterpret_cast<const int8*>(table);
        auto count = packed[0];
        auto const& delta = packed[1];
        auto p = fg_pos;
        for(; count; --count)
        {
          p += delta;
          // skip empty field
          if(p == move.from() || p == en_pos)
            continue;
          // stop on field occupied my recently moved fiure
          if(p == move.to())
            break;
          // capture my king
          if(p == king_pos)
            return false;
          X_ASSERT(getField(p) && getField(p).type() == Figure::TypeKing && getField(p).color() == color(), "king position is invalid");
          // field is occupied
          if(getField(p))
            break;
        }
      }
    }
  }
  return true;
}

bool Board::validateMove(const Move & move) const
{
  if(drawState() || matState())
    return false;

  X_ASSERT((unsigned)move.from() > 63 || (unsigned)move.to() > 63, "invalid move positions");
  const auto& ffrom = getField(move.from());
  auto ocolor = Figure::otherColor(color());
  X_ASSERT(move.to() == kingPos(ocolor), "try to capture king");
  auto mask_all = (fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack));
  if(Figure::TypeKing == ffrom.type())
  {
    X_ASSERT(underCheck() && ((move.from()&7)-(move.to()&7) > 1 || (move.from()&7)-(move.to()&7) < -1),
              "try to castle under check");

    mask_all ^= set_mask_bit(move.from());
    if(isAttacked(move.to(), mask_all, ocolor))
      return false;
    // no need to verify castle under check
    if(underCheck())
      return true;
    // castle
    auto d = move.to() - move.from();
    if(d == 2 || d == -2)
    {
      X_ASSERT(!castling(color(), d == 2 ? 0 : 1), "castle is not allowed");
      X_ASSERT(!(move.from() == 4 && color() || move.from() == 60 && !color()), "kings position is wrong");
      X_ASSERT(color() && move.to() != 2 && move.to() != 6, "white castle final king position is wrong");
      X_ASSERT(!color() && move.to() != 58 && move.to() != 62, "black castle final king position is wrong");
      X_ASSERT(getField(move.to()), "king position after castle is occupied");
      d >>= 1;
      // verify if there is suitable rook for castling
      int rook_from = move.from() + ((d>>1) ^ 3); //d < 0 ? move.from_ - 4 : move.from_ + 3
      int rook_to = move.from() + d;
      X_ASSERT((rook_from != 0 && color() && d < 0) || (rook_from != 7 && color() && d > 0), "invalid castle rook position");
      X_ASSERT((rook_from != 56 && !color() && d < 0) || (rook_from != 63 && !color() && d > 0), "invalid castle rook position");
      X_ASSERT(getField(rook_to), "field, that rook is going to move to, is occupied");
      X_ASSERT(!(rook_from & 3) && getField(rook_from+1), "long castling impossible");
      return !isAttacked(rook_to, mask_all, ocolor);
    }
    return true;
  }

  X_ASSERT(underCheck() && doubleCheck(), "2 checking figures - only king moves allowed");

  // moving figure discovers check?

  auto const& ki_pos = kingPos(color());
  if(enpassant() > 0 && move.to() == enpassant() && Figure::TypePawn == ffrom.type())
  {
    int ep_pos = enpassantPos();
    auto through = set_mask_bit(move.from()) | set_mask_bit(ep_pos);
    mask_all |= set_mask_bit(move.to());
    mask_all &= ~through;
    bool ok = !discoveredCheckEp(through, mask_all, ocolor, ki_pos);
    X_ASSERT(ok && isAttacked(ki_pos, mask_all, ~set_mask_bit(move.to()), ocolor), "king is attacked from somewhere after move");
    X_ASSERT(!ok && !isAttacked(ki_pos, mask_all, ~set_mask_bit(move.to()), ocolor), "invalid attack of my king detected after move");
    return ok;
  }
  bool ok = !discoveredCheckUsual(move.from(), move.to(), mask_all, ocolor, ki_pos);
  X_ASSERT(ok && isAttacked(ki_pos, (mask_all | set_mask_bit(move.to())) & ~set_mask_bit(move.from()), ~set_mask_bit(move.to()), ocolor),
            "king is attacked from somewhere after move");
  X_ASSERT(!ok && !isAttacked(ki_pos, (mask_all | set_mask_bit(move.to())) & ~set_mask_bit(move.from()), ~set_mask_bit(move.to()), ocolor),
            "invalid attack of my king detected after move");
  return ok;
}


bool Board::verifyCheckingFigure(int ch_pos, Figure::Color checking_color) const
{
  int count = 0;
  int checking[2] = {};
  auto king_color = Figure::otherColor(checking_color);
  auto const& king_pos = kingPos(king_color);
  for(int type = Figure::TypePawn; type < Figure::TypeKing; ++type)
  {
    BitMask mask = fmgr_.type_mask((Figure::Type)type, checking_color);
    for(; mask;)
    {
      int n = clear_lsb(mask);
      const Field & field = getField(n);
      X_ASSERT(field.color() != checking_color || field.type() != type, "invalid figures mask in check detector");
      int dir = figureDir().dir(field.type(), checking_color, n, king_pos);
      if((dir < 0) || (Figure::TypePawn == type && (2 == dir || 3 == dir)))
        continue;
      if(Figure::TypePawn == type || Figure::TypeKnight == type)
      {
        X_ASSERT(count > 1, "too much checking figures");
        checking[count++] = n;
        continue;
      }
      FPos dp = deltaPosCounter().getDeltaPos(n, king_pos);
      X_ASSERT(FPos(0, 0) == dp, "invalid attacked position");
      FPos p = FPosIndexer::get(king_pos) + dp;
      const FPos & figp = FPosIndexer::get(n);
      bool have_figure = false;
      for(; p != figp; p += dp)
      {
        const Field & field = getField(p.index());
        if(field)
        {
          have_figure = true;
          break;
        }
      }
      if(!have_figure)
      {
        X_ASSERT(count > 1, "too much checking figures");
        checking[count++] = n;
      }
    }
  }
  X_ASSERT(count > 2, "more than 2 checking figures");
  for(int i = 0; i < count; ++i)
  {
    if(checking[i] == ch_pos)
      return true;
  }
  return true;
}

void Board::detectCheck(Move const& move)
{
  data_.checking_ = 0;
  Figure::Color ocolor = Figure::otherColor(color());
  const Field & fto = getField(move.to());
  auto const& king_pos = kingPos(color());
  auto const& king_mask = fmgr_.king_mask(color());
  auto mask_all = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite);
  int epch{ -1 };
  // castle with check
  if(fto.type() == Figure::TypeKing)
  {
    int d = move.to() - move.from();
    if(2 == d || -2 == d)
    {
      d >>= 1;
      int rook_to = move.from() + d;
      if((rook_to&7) == (king_pos&7) && is_nothing_between(rook_to, king_pos, ~mask_all))
      {
        data_.checking_ = rook_to;
        data_.state_ |= UnderCheck;
      }
      X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
      // no more checking figures
      return;
    }
  }
  else if(fto.type() == Figure::TypePawn)
  {
    auto const& pw_mask = movesTable().pawnCaps(ocolor, move.to());
    if(pw_mask & king_mask)
    {
      data_.checking_ = move.to();
      data_.state_ |= UnderCheck;
      // usual pawn's move?
      if((move.from()&7) == (move.to()&7))
      {
        X_ASSERT(move.new_type(), "invalid promotion detected");
        X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
        // no more checking figures
        return;
      }
    }
    // en-passant
    else if(move.to() != 0 && lastUndo().enpassant() == move.to())
    {
      static int pw_delta[2] = { -8, 8 };
      // color == color of captured pawn
      int ep_pos = move.to() + pw_delta[color()];
      // through en-passant pawn field
      epch = data_.checking_ = findDiscoveredPtEp(ep_pos, mask_all, ocolor, king_pos);
      if(data_.checking_ < 64)
      {
        data_.state_ |= UnderCheck;
        X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
      }
    }
  }
  else if(fto.type() == Figure::TypeKnight)
  {
    const BitMask & kn_mask = movesTable().caps(Figure::TypeKnight, move.to());
    if(kn_mask & king_mask)
    {
      data_.state_ |= UnderCheck;
      data_.checking_ = move.to();
      X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
    }
  }
  else if(isAttackedBy(color(), ocolor, fto.type(), move.to(), king_pos, mask_all))
  {
    data_.state_ |= UnderCheck;
    data_.checking_ = move.to();
    X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
  }

  // through move.from field
  auto pt = findDiscoveredPt(move.from(), move.to(), mask_all, ocolor, king_pos);
  if(pt >= 64)
    return;
  if(pt == epch)
  {
    X_ASSERT(pt == move.to(), "invalid discovered checking figure position found");
    X_ASSERT(!underCheck(), "check was not detected");
    return;
  }
  X_ASSERT(!verifyCheckingFigure(pt, ocolor), "invalid checking figure found");
  if(underCheck())
  {
    data_.state_ |= DoubleCheck;
    return;
  }
  data_.state_ |= UnderCheck;
  data_.checking_ = pt;
}

void Board::verifyChessDraw(bool irreversibleLast)
{
  if((data_.fiftyMovesCount_ == 100 && !underCheck()) || (data_.fiftyMovesCount_ > 100))
  {
    data_.state_ |= Draw50Moves;
    return;
  }

  if(!fmgr_.pawns(Figure::ColorBlack) && !fmgr_.pawns(Figure::ColorWhite))
  {
    if((fmgr_.rooks(Figure::ColorBlack) + fmgr_.queens(Figure::ColorBlack)
      + fmgr_.rooks(Figure::ColorWhite) + fmgr_.queens(Figure::ColorWhite) == 0)
      && (fmgr_.knights(Figure::ColorBlack) + fmgr_.bishops(Figure::ColorBlack)) < 2
      && (fmgr_.knights(Figure::ColorWhite) + fmgr_.bishops(Figure::ColorWhite)) < 2)
    {
      data_.state_ |= DrawInsuf;
      return;
    }
  }
  
  if(irreversibleLast)
  {
    X_ASSERT(data_.repsCounter_ != 1, "repetitions count should be 0");
    return;
  }
  
  data_.repsCounter_ = countReps(2, fmgr_.hashCode());

  if(data_.repsCounter_ >= 3)
    data_.state_ |= DrawReps;
}

bool Board::verifyCastling(const Figure::Color c, int t) const
{
  if(c && getField(4).type() != Figure::TypeKing)
    return false;

  if(!c && getField(60).type() != Figure::TypeKing)
    return false;

  if(c && t == 0 && getField(7).type() != Figure::TypeRook)
    return false;

  if(c && t == 1 && getField(0).type() != Figure::TypeRook)
    return false;

  if(!c && t == 0 && getField(63).type() != Figure::TypeRook)
    return false;

  if(!c && t == 1 && getField(56).type() != Figure::TypeRook)
    return false;

  return true;
}

bool Board::ptAttackedBy(int8 pt, int p) const
{
  const Field & field = getField(p);
  int dir = figureDir().dir(field.type(), field.color(), p, pt);
  if(dir < 0)
    return false;

  if(field.type() == Figure::TypeKnight)
    return true;

  const uint64 & mask = betweenMasks().between(p, pt);
  const uint64 & black = fmgr_.mask(Figure::ColorBlack);
  if((~black & mask) != mask)
    return false;

  const uint64 & white = fmgr_.mask(Figure::ColorWhite);
  if((~white & mask) != mask)
    return false;

  return true;
}

/// returns field index of checking figure or -1 if not found
/// mask_all is completely prepared, all figures are on their places
bool Board::findDiscovered(int from, Figure::Color acolor, const BitMask & mask_all, const BitMask & brq_mask, int ki_pos) const
{
  const BitMask & from_msk = betweenMasks().from(ki_pos, from);
  BitMask mask_all_ex = mask_all & from_msk;
  if((mask_all_ex & brq_mask) == 0)
    return false;

  int apos = ki_pos < from ? _lsb64(mask_all_ex) : _msb64(mask_all_ex);
  if((set_mask_bit(apos) & brq_mask) == 0) // no BRQ on this field
    return false;

  const Field & afield = getField(apos);
  X_ASSERT(afield.color() != acolor || afield.type() < Figure::TypeBishop || afield.type() > Figure::TypeQueen,
           "findDiscovered() - attacking figure isn't BRQ");

  return figureDir().dir(afield.type(), afield.color(), apos, ki_pos) >= 0;
}

bool Board::ptAttackedFrom(Figure::Color acolor, int8 pt, int8 from) const
{
  BitMask mask_all = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite);
  BitMask brq_mask = fmgr_.bishop_mask(acolor) | fmgr_.rook_mask(acolor) | fmgr_.queen_mask(acolor);
  const BitMask & btw_mask = betweenMasks().between(pt, from);
  brq_mask &= ~btw_mask; // exclude all figures, that are between 'pt' and 'from'

  return findDiscovered(from, acolor, mask_all, brq_mask, pt);
}

} // NEngine
