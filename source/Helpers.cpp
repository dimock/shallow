/*************************************************************
Helpers.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <Helpers.h>
#include <xindex.h>
#include <MovesGenerator.h>
#include <boost/algorithm/string/trim.hpp>

#ifdef _USE_LOG
#include <fstream>
#endif


// TODO: refactore with std::regex

namespace NEngine
{

static std::string const s_nullmove{"0000"};
static std::string const stdFEN_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

inline
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
    static std::string const figletters{"PNBRQK"};
    if(figletters.find(str[0]) != std::string::npos)
    {
      if(iscolumn(str[1]) && (isdigit(str[2]) || str[2] == 'x' || iscolumn(str[2])))
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
    if(str == s_nullmove)
      return eMoveNotation::mnSAN;
  }

  return eMoveNotation::mnUnknown;
}

Move parseSAN(const Board & board, std::string const& str)
{
  if(str.empty())
    return Move{0};

  // internal feature -> null-move
  if(str == s_nullmove)
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

  static std::string const figletters{"PNBRQK"};
  if(figletters.find_first_of(*s) != std::string::npos)
  {
    type = Figure::toFtype(*s);
    s++;
  }
  else if(std::string(s).find("O-O-O") != std::string::npos) // long castle
  {
    from = board.getColor() ? 4 : 60;
    to = board.getColor() ? 2 : 58;
    s += 5;
    type = Figure::Type::TypeKing;
  }
  else if(std::string(s).find("O-O") != std::string::npos) // short castle
  {
    from = board.getColor() ? 4 : 60;
    to = board.getColor() ? 6 : 62;
    s += 3;
    type = Figure::Type::TypeKing;
  }

  if(to < 0) // not found yet
  {
    // should be at least 2 chars
    size_t n = std::string(s).length();
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

    n = std::string(s).length();
    if(!*s || !iscolumn(s[0]) || n < 2)
      return false;

    to = (s[0] - 'a') | ((s[1] - '1') << 3);
    s += 2;
    n = std::string(s).length();

    if('=' == s[0] || boost::is_any_of("NBRQ")(s[0]))
    {
      if('=' == s[0])
      {
        if(n < 2)
          return false;
        s++;
        n = std::string(s).length();
      }
      if(n < 1)
        return false;
      new_type = Figure::toFtype(s[0]);
      if(new_type < Figure::Type::TypeKnight || new_type > Figure::Type::TypeQueen)
        return false;
      s++;
      n = std::string(s).length();
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
    auto const* pm = mg.move();
    if(!pm)
      break;

    auto const& m = *pm;
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
    return s_nullmove;
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
    auto const* pm = mg.move();
    if(!pm)
      break;

    auto const& m = *pm;
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


// TODO: rewrite with std::string/std::regex/boost::trim/...
/* rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 */
bool fromFEN(std::string const& i_fen, Board& board)
{
  board.clear();

  std::string fen = i_fen.empty() ? stdFEN_ : i_fen;
  boost::algorithm::trim(fen);

  const char * s = fen.c_str();
  int x = 0, y = 7;
  for(; s && y >= 0; ++s)
  {
    char c = *s;
    if('/' == c)
      continue;
    else if(isdigit(c))
    {
      int dx = c - '0';
      x += dx;
      if(x > 7)
      {
        --y;
        x = 0;
      }
    }
    else
    {
      Figure::Type  ftype = Figure::TypeNone;
      Figure::Color color = Figure::ColorBlack;

      if(isupper(c))
        color = Figure::ColorWhite;

      ftype = Figure::toFtype(toupper(c));

      if(Figure::TypeNone == ftype)
        return false;

      Index p(x, y);
      board.addFigure(color, ftype, p);

      if(++x > 7)
      {
        --y;
        x = 0;
      }
    }
  }

  int i = 0;
  for(; *s;)
  {
    char c = *s;
    if(c <= ' ')
    {
      ++s;
      continue;
    }

    if(0 == i) // process move color
    {
      if('w' == c)
        board.color_ = Figure::ColorWhite;
      else if('b' == c)
        board.color_ = Figure::ColorBlack;
      else
        return false;
      ++i;
      ++s;
      continue;
    }

    if(1 == i) // process castling possibility
    {
      if('-' == c)
      {
        ++i;
        ++s;
        continue;
      }

      for(; *s && *s > 32; ++s)
      {
        Field fk, fr;

        switch(*s)
        {
        case 'k':
          board.set_castling(Figure::ColorBlack, 0);
          board.fmgr_.hashCastling(Figure::ColorBlack, 0);
          break;

        case 'K':
          board.set_castling(Figure::ColorWhite, 0);
          board.fmgr_.hashCastling(Figure::ColorWhite, 0);
          break;

        case 'q':
          board.set_castling(Figure::ColorBlack, 1);
          board.fmgr_.hashCastling(Figure::ColorBlack, 1);
          break;

        case 'Q':
          board.set_castling(Figure::ColorWhite, 1);
          board.fmgr_.hashCastling(Figure::ColorWhite, 1);
          break;

        default:
          return false;
        }
      }

      ++i;
      continue;
    }

    if(2 == i)
    {
      ++i;
      char cx = *s++;
      if('-' == cx) // no en-passant pawn
        continue;

      if(!*s)
        return false;

      char cy = *s++;

      Index enpassant(cx, cy);

      if(board.color_)
        cy--;
      else
        cy++;

      Index pawn_pos(cx, cy);

      Field & fp = board.getField(pawn_pos);
      if(fp.type() != Figure::TypePawn || fp.color() == board.color_)
        return false;

      board.en_passant_ = enpassant;
      board.fmgr_.hashEnPassant(board.en_passant_, fp.color());
      continue;
    }

    if(3 == i) // fifty moves rule
    {
      char str[4];
      str[0] = 0;
      for(int j = 0; *s && j < 4; ++j, ++s)
      {
        if(*s > 32)
          str[j] = *s;
        else
        {
          str[j] = 0;
          break;
        }
      }

      board.fiftyMovesCount_ = atoi(str);
      ++i;
      continue;
    }

    if(4 == i) // moves counter
    {
      board.movesCounter_ = atoi(s);
      break;
    }
  }

  if(!board.invalidate())
    return false;

  return true;
}

std::string toFEN(Board const& board)
{
  std::string fen;

  // 1 - write figures
  for(int y = 7; y >= 0; --y)
  {
    int n = 0;
    for(int x = 0; x < 8; ++x)
    {
      Index idx(x, y);
      const Field & field = board.getField(idx);
      if(!field)
      {
        ++n;
        continue;
      }

      if(n > 0)
      {
        fen += '0' + n;
        n = 0;
      }

      char c = fromFtype(field.type());
      if(field.color() == Figure::ColorBlack)
        c = tolower(c);

      fen += c;
    }

    if(n > 0)
      fen += '0' + n;

    if(y > 0)
      fen += '/';
  }

  // 2 - color to move
  {
    fen += ' ';
    if(Figure::ColorBlack == board.color_)
      fen += 'b';
    else
      fen += 'w';
  }

  // 3 - castling possibility
  {
    fen += ' ';
    if(!board.castling())
    {
      fen += '-';
    }
    else
    {
      if(board.castling_K())
      {
        if(!board.verifyCastling(Figure::ColorWhite, 0))
          return false;

        fen += 'K';
      }
      if(board.castling_Q())
      {
        if(!board.verifyCastling(Figure::ColorWhite, 1))
          return false;

        fen += 'Q';
      }
      if(board.castling_k())
      {
        if(!board.verifyCastling(Figure::ColorBlack, 0))
          return false;

        fen += 'k';
      }
      if(board.castling_q())
      {
        if(!board.verifyCastling(Figure::ColorBlack, 1))
          return false;

        fen += 'q';
      }
    }
  }

  {
    // 4 - en passant
    fen += ' ';
    if(board.en_passant_ >= 0)
    {
      Index ep_pos(board.en_passant_);

      int x = ep_pos.x();
      int y = ep_pos.y();

      if(board.color_)
        y--;
      else
        y++;

      Index pawn_pos(x, y);
      const Field & ep_field = board.getField(pawn_pos);
      if(ep_field.color() == board.color_ || ep_field.type() != Figure::TypePawn)
        return false;

      char cx = 'a' + ep_pos.x();
      char cy = '1' + ep_pos.y();
      fen += cx;
      fen += cy;
    }
    else
      fen += '-';

    // 5 - fifty move rule
    fen += " " + std::to_string(board.fiftyMovesCount_);

    // 6 - moves counter
    fen += " " + std::to_string(board.movesCounter_);
  }

  return fen;
}

bool load(Board & board, std::istream & is)
{
  const int N = 1024;
  std::string const sepr = " \t\n\r";
  std::string const strmv = "123456789abcdefghkrq+-OxPNBRQKnul=#";
  bool fen_expected = false, fen_init = false;

  std::vector<std::string> smoves;

  bool annot = false;
  bool stop = false;
  std::string str;
  while(!stop && std::getline(is, str))
  {
    bool skip = false;
    auto si = str.begin();
    for(; si != str.end() && !skip; ++si)
    {
      // skip separators
      if(sepr.find(*si) != std::string::npos)
        continue;

      // skip annotations {}
      if('{' == *si)
        annot = true;

      if(annot && '}' != *si)
        continue;

      annot = false;

      // skip comment string
      if('%' == *si)
      {
        skip = true;
        break;
      }

      if('.' == *si) // skip .
        continue;
      auto si1 = si;
      si1++;
      if(isdigit(*si) && si1 != str.end() && 
        ((isdigit(*si1) || sepr.find(*(si1)) != std::string::npos || '.' == *si1))) // skip move number
      {
        for(; si != str.end() && isdigit(*si); ++si);
        if(si == str.end())
          skip = true;
      }
      else if(isalpha(*si) || (isdigit(*si) && (si1 != str.end() && isalpha(*si1)))) // read move
      {
        std::string moveStr;
        for(; si != str.end() && strmv.find(*si) != std::string::npos; ++si)
          moveStr += *si;
        smoves.emplace_back(moveStr);
        if(si == str.end())
          skip = true;
      }
      else if(('*' == *si) || (isdigit(*si) && (si1 != str.end() && '-' == *si1))) // end game
      {
        skip = true;
        stop = true;
      }
      else if('[' == *si) // read some extra params, like FEN
      {
        std::string param;
        std::string value;
        for(; si != str.end() && ']' != *si; ++si)
        {
          // skip separators
          if(sepr.find(*si) != std::string::npos)
            continue;

          // read param name
          if(isalpha(*si) && param.empty())
          {
            for(; si != str.end() && sepr.find(*si) == std::string::npos; ++si)
              param += *si;
            continue;
          }

          // read value
          if('"' == *si && value.empty())
          {
            for(si++; si != str.end() && '"' != *si; ++si)
              value += *si;
            continue;
          }
        }

        if(param == "SetUp" && value == "1")
          fen_expected = true;
        else if(param == "FEN" && fen_expected)
        {
          if(!fromFEN(value, board))
            return false;
          fen_init = true;
        }
        skip = true;
      }
    }
  }

  if(!fen_init)
    fromFEN("", board);

  for(auto const& str : smoves)
  {
    Move move = strToMove(str, board);
    if(!move)
      return false;

    if(!board.validateMove(move))
      return false;

    board.makeMove(move);
  }

  board.verifyState();

  return true;
}

bool save(const Board & board, std::ostream & os, bool write_prefix)
{
  std::string sres = "*";
  if(board.matState())
  {
    if(board.getColor())
      sres = "0-1";
    else
      sres = "1-0";
  }
  else if(board.drawState())
    sres = "1/2-1/2";

  SBoard<Board::GameLength> sboard(board, true);

  int num = sboard.halfmovesCount();

  while(sboard.halfmovesCount() > 0)
    sboard.unmakeMove();

  if(write_prefix)
  {
    auto fen = toFEN(sboard);
    if(!fen.empty() && fen != stdFEN_)
    {
      os << "[SetUp \"1\"] " << std::endl;
      os << "[FEN \"" << fen << "\"]" << std::endl;
    }

    os << "[Result \"";
    os << sres;
    os << "\"]" << std::endl;
  }

  for(int i = 0; i < num; ++i)
  {
    Figure::Color color = sboard.getColor();
    int moveNum = sboard.movesCount();

    // stash move
    Move move = board.undoInfo(i);

    auto str = printSAN(sboard, move);
    if(str.empty())
      return false;

    // now apply move
    if(!sboard.validateMove(move))
      return false;

    sboard.makeMove(move);

    if(color || !i)
      os << moveNum << ". ";

    if(!i && !color)
      os << " ... ";

    os << str;

    if(!color)
      os << std::endl;
    else
      os << " ";
  }

  os << sres << std::endl;

  return true;
}


#ifdef _USE_LOG
void addLog(std::string const& str)
{
  std::ofstream ofs("my_log.txt", std::ios::app);
  ofs << str << std::endl;
}
#endif


} // NEngine
