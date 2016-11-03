/*************************************************************
Helpers.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <Helpers.h>
#include <xindex.h>
#include <MovesGenerator.h>

// TODO: refactore with std::regex

namespace NEngine
{

bool iscolumn(char c)
{
  return c >= 'a' && c <= 'h';
}

eMoveNotation detectNotation(std::string const& str)
{
  if(str.size() < 2)
    return eMoveNotation::mnUnknown;

  /// Smith notation
  if(str.size() >= 4 && iscolumn(str[0]) && isdigit(str[1]) && iscolumn(str[2]) && isdigit(str[3]))
    return eMoveNotation::mnSmith;

  /// pawns capture
  if(str.size() == 2 && iscolumn(str[0]) && (iscolumn(str[1]) || isdigit(str[1])))
    return eMoveNotation::mnSAN;

  if(str.size() >= 3 && isalpha(str[0]) && (isalnum(str[1]) || str[1] == '-')) // may be SAN
  {
    if(strchr("PNBRQK", str[0]))
    {
      if(iscolumn(str[1]) && (isdigit(str[2]) || str[2] == 'x'))
        return eMoveNotation::mnSAN;

      if(isdigit(str[1]) && (iscolumn(str[2]) || str[2] == 'x'))
        return eMoveNotation::mnSAN;

      if(str[1] == 'x' && iscolumn(str[2]))
        return eMoveNotation::mnSAN;
    }

    /// castle
    if(str.find("O-O") != std::string::npos)
      return eMoveNotation::mnSAN;

    /// pawns movement
    if(iscolumn(str[0]) && (isdigit(str[1]) || str[1] == 'x'))
      return eMoveNotation::mnSAN;

    /// very special case - null move. used for debugging only!!!
    if(str == "null")
      return eMoveNotation::mnSAN;
  }

  return eMoveNotation::mnUnknown;
}

Move parseSAN(const Board & board, std::string const& str)
{
  if(str.empty())
    return Move{0};

  // internal feature -> null-move
  if(str == "null")
  {
    return Move{0};
  }

  Figure::Type type = Figure::Type::TypePawn;
  Figure::Type new_type = Figure::Type::TypeNone;

  int xfrom = -1, yfrom = -1;
  int from = -1, to = -1;
  bool capture = false;
  bool check = false;
  bool chessmat = false;

  const char * s = str.c_str();

  if(strchr("PNBRQK", *s))
  {
    type = Figure::toFtype(*s);
    s++;
  }
  else if(strstr(s, "O-O-O")) // long castle
  {
    from = board.getColor() ? 4 : 60;
    to = board.getColor() ? 2 : 58;
    s += 5;
    type = Figure::Type::TypeKing;
  }
  else if(strstr(s, "O-O")) // short castle
  {
    from = board.getColor() ? 4 : 60;
    to = board.getColor() ? 6 : 62;
    s += 3;
    type = Figure::Type::TypeKing;
  }

  if(to < 0) // not found yet
  {
    // should be at least 2 chars
    size_t n = strlen(s);
    if(n < 2)
      return false;

    if(isdigit(s[0]) && (iscolumn(s[1]) || 'x' == s[1])) // from row number
    {
      yfrom = s[0] - '1';
      s++;
    }
    else if(iscolumn(s[0]) && (iscolumn(s[1]) || 'x' == s[1])) // from column number
    {
      xfrom = s[0] - 'a';
      s++;
    }
    else if(n > 2 && iscolumn(s[0]) && isdigit(s[1]) && (iscolumn(s[2]) || 'x' == s[2])) // exact from point
    {
      xfrom = s[0] - 'a';
      yfrom = s[1] - '1';
      s += 2;
    }

    if('x' == s[0]) // capture
    {
      capture = true;
      s++;
    }

    n = strlen(s);
    if(!*s || !iscolumn(s[0]) || n < 2)
      return false;

    to = (s[0] - 'a') | ((s[1] - '1') << 3);
    s += 2;
    n = strlen(s);

    if('=' == s[0])
    {
      if(n < 2)
        return false;
      new_type = Figure::toFtype(s[1]);
      if(new_type < Figure::Type::TypeKnight || new_type > Figure::Type::TypeQueen)
        return false;
      s += 2;
      n = strlen(s);
    }

    if('+' == s[0])
      check = true;
    else if('#' == s[0])
      chessmat = true;
  }

  if(to < 0)
    return false;

  if(xfrom >= 0 && yfrom >= 0)
    from = xfrom | (yfrom << 3);

  MovesGenerator mg(board);
  for(;;)
  {
    const Move & m = mg.move();
    if(!m)
      break;

    if(!board.validateMove(m))
      continue;

    const Field & field = board.getField(m.from_);

    if(to == m.to_
      && static_cast<Figure::Type>(m.new_type_) == new_type
      && field.type() == type
      && ((m.capture_ != 0) == capture)
      && (from > 0 && from == m.from_ || xfrom >= 0
      && Index(m.from_).x() == xfrom || yfrom >= 0
      && Index(m.from_).y() == yfrom || xfrom < 0 && yfrom < 0))
    {
      return m;
    }
  }
  return Move{ 0 };
}

std::string printSAN(Board & board, const Move & move)
{
  if(!move) // null-move passed
  {
    return "null";
  }

  Field field = board.getField(move.from_);

  bool found = false;
  int disambiguations = 0;
  int same_x = 0, same_y = 0;
  int xfrom = Index(move.from_).x();
  int yfrom = Index(move.from_).y();
  int xto = Index(move.to_).x();
  int yto = Index(move.to_).y();
  uint8 state = Board::State::Invalid;

#ifndef NDEBUG
  Board board0(board);
#endif

  MovesGenerator mg(board);
  for(;;)
  {
    const Move & m = mg.move();
    if(!m)
      break;

    if(!board.validateMove(m))
    {
      X_ASSERT(move == m, "invalid move given to printSAN");
      continue;
    }

    if(m == move)
    {
      board.makeMove(move);
      board.verifyState();
      state = board.getState();
      board.unmakeMove();
      found = true;
    }

    X_ASSERT(board0 != board, "board is not restored by undo move method");

    const Field & f = board.getField(m.from_);

    if(m.to_ != move.to_ || f.type() != field.type() || m.new_type_ != move.new_type_)
      continue;

    // check for disambiguation in 'from' position
    if(Index(m.from_).x() == xfrom)
      same_x++;

    if(Index(m.from_).y() == yfrom)
      same_y++;

    disambiguations++;
  }

  if(!found)
    return false;

  std::string str;
  if(field.type() == Figure::Type::TypeKing && (2 == move.to_ - move.from_ || -2 == move.to_ - move.from_))// castle
  {
    if(move.to_ > move.from_) // short castle
    {
      str += "O-O";
    }
    else
    {
      str += "O-O-O";
    }
  }
  else
  {
    if(field.type() != Figure::Type::TypePawn)
    {
      str += fromFtype(field.type());
    }

    if(disambiguations > 1 || (field.type() == Figure::Type::TypePawn && move.capture_))
    {
      if(same_x <= 1)
      {
        // x different
        str += static_cast<char>('a' + xfrom);
      }
      else if(same_y <= 1)
      {
        // y different
        str += static_cast<char>('1' + yfrom);
      }
      else
      {
        // write both
        str += static_cast<char>('a' + xfrom);
        str += static_cast<char>('1' + yfrom);
      }
    }
    // capture
    if(move.capture_)
    {
      str += 'x';
    }

    str += 'a' + xto;
    str += '1' + yto;

    if(move.new_type_ > 0)
    {
      str += '=';
      str += fromFtype((Figure::Type)move.new_type_);
    }
  }

  if(Board::ChessMat & state)
  {
    str += '#';
  }
  else if(Board::UnderCheck & state)
  {
    str += '+';
  }
  return str;
}
//////////////////////////////////////////////////////////////////////////

std::string moveToStr(const Move & move, bool wbf)
{
  if(move.from_ < 0 || move.to_ < 0)
    return "";

  Index from(move.from_);
  Index to(move.to_);

  std::string str;
  if(wbf)
  {
    str = "move ";
  }

  str += static_cast<char>('a' + from.x());
  str += static_cast<char>('1' + from.y());
  str += static_cast<char>('a' + to.x());
  str += static_cast<char>('1' + to.y());

  if(move.new_type_ <= 0)
    return str;

  switch(static_cast<Figure::Type>(move.new_type_))
  {
  case Figure::Type::TypeBishop:
    str += 'b';
    break;

  case Figure::Type::TypeKnight:
    str += 'n';
    break;

  case Figure::Type::TypeRook:
    str += 'r';
    break;

  case Figure::Type::TypeQueen:
    str += 'q';
    break;
  }

  return str;
}

Move strToMove(std::string const& str, const Board & board)
{
  if(str.empty())
    return Move{ 0 };

  eMoveNotation mnot = detectNotation(str);
  if(mnot == eMoveNotation::mnUnknown)
    return Move{ 0 };
  else if(mnot == eMoveNotation::mnSAN)
    return parseSAN(board, str);

  if(str.size() < 4)
    return Move{ 0 };

  //char str[256];
  //strncpy(str, i_str, sizeof(str));
  //_strlwr(str);

  Move move{ 0 };

  Figure::Color color = board.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  if(!iscolumn(str[0]) || !isdigit(str[1]) && !iscolumn(str[2]) && !isdigit(str[3]))
    return false;

  int xfrom = str[0] - 'a';
  int yfrom = str[1] - '1';
  int xto = str[2] - 'a';
  int yto = str[3] - '1';

  if(str.size() > 4 && isalpha(str[4]))
  {
    if('b' == str[4])
      move.new_type_ = static_cast<int8>(Figure::Type::TypeBishop);
    else if('n' == str[4])
      move.new_type_ = static_cast<int8>(Figure::Type::TypeKnight);
    else if('r' == str[4])
      move.new_type_ = static_cast<int8>(Figure::Type::TypeRook);
    else if('q' == str[4])
      move.new_type_ = static_cast<int8>(Figure::Type::TypeQueen);
  }

  move.from_ = Index(xfrom, yfrom);
  move.to_ = Index(xto, yto);

  int to = move.to_;

  const Field & ffrom = board.getField(move.from_);
  if(!ffrom || ffrom.color() != color)
    return Move{ 0 };

  // maybe en-passant
  if(ffrom.type() == Figure::Type::TypePawn && board.enpassant() == move.to_)
  {
    int dx = Index(move.to_).x() - Index(move.from_).x();
    if(dx != 0)
      to = board.enpassantPos();
  }

  const Field & fto = board.getField(to);
  if(fto && fto.color() == ocolor)
    move.capture_ = 1;

  return board.possibleMove(move) ? move : Move{ 0 };
}

} // NEngine
