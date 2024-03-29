/*************************************************************
Helpers.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include "Helpers.h"
#include "xindex.h"
#include "MovesGenerator.h"
#include "locale"
#include "regex"

#ifdef _USE_LOG
#include "fstream"
#endif


// TODO: refactore with std::regex
static std::regex rfen_{"([a-h1-8pnbrqkPNBRQK/]*)([ \\t]+)([bwKQkqa-h0-9 \\t\\-]*)"};
static std::regex rafen_{"([bw]+)([ \\t]+)?([\\-KQkq]+)?([ \\t]+)?([\\-a-h\\d]+)?([ \\t]*)?([\\d]*)?([ \\t]*)?([\\d]*)?"};

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
    return Move{true};

  // internal feature -> null-move
  if(str == s_nullmove)
  {
    return Move{true};
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
    from = board.color() ? 4 : 60;
    to = board.color() ? 2 : 58;
    s += 5;
    type = Figure::Type::TypeKing;
  }
  else if(std::string(s).find("O-O") != std::string::npos) // short castle
  {
    from = board.color() ? 4 : 60;
    to = board.color() ? 6 : 62;
    s += 3;
    type = Figure::Type::TypeKing;
  }

  if(to < 0) // not found yet
  {
    // should be at least 2 chars
    size_t n = std::string(s).length();
    if(n < 2)
      return Move{true};

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
      return Move{true};

    to = (s[0] - 'a') | ((s[1] - '1') << 3);
    s += 2;
    n = std::string(s).length();

    if('=' == s[0] || is_any_of("NBRQ", s[0]))
    {
      if('=' == s[0])
      {
        if(n < 2)
          return Move{true};
        s++;
        n = std::string(s).length();
      }
      if(n < 1)
        return Move{true};
      new_type = Figure::toFtype(s[0]);
      if(new_type < Figure::Type::TypeKnight || new_type > Figure::Type::TypeQueen)
        return Move{true};
      s++;
      n = std::string(s).length();
    }

    if('+' == s[0])
      check = true;
    else if('#' == s[0])
      chessmat = true;
  }

  if(to < 0)
    return Move{true};

  if(xfrom >= 0 && yfrom >= 0)
    from = xfrom | (yfrom << 3);

  auto moves = generate<Board, Move>(board);
  for(auto m : moves)
  {
    if(!board.validateMoveBruteforce(m))
      continue;

    bool mcapture = board.getField(m.to()) || (board.enpassant() == m.to() && m.to() > 0);
    const auto field = board.getField(m.from());
    if(to == m.to()
      && static_cast<Figure::Type>(m.new_type()) == new_type
      && field.type() == type
      && (mcapture == capture)
      && (from > 0 && from == m.from() || xfrom >= 0
      && Index(m.from()).x() == xfrom || yfrom >= 0
      && Index(m.from()).y() == yfrom || xfrom < 0 && yfrom < 0))
    {
      return m;
    }
  }
  return Move{true};
}

std::string printSAN(Board & board, const Move move)
{
  if(!move) // null-move passed
  {
    return s_nullmove;
  }

  auto const field = board.getField(move.from());
  auto capture = board.getField(move.to()) || (board.enpassant() == move.to() && move.to() > 0);

  bool found = false;
  int disambiguations = 0;
  int same_x = 0, same_y = 0;
  int xfrom = Index(move.from()).x();
  int yfrom = Index(move.from()).y();
  int xto = Index(move.to()).x();
  int yto = Index(move.to()).y();
  StateType state = State::Invalid;

  auto moves = generate<Board, Move>(board);
  for(auto m : moves)
  {
    if(!board.validateMoveBruteforce(m))
    {
      X_ASSERT(move == m, "invalid move given to printSAN");
      continue;
    }

    if(m == move)
    {
      board.makeMove(move);
      board.verifyState();
      state = board.state();
      board.unmakeMove(move);
      found = true;
    }

    const auto f = board.getField(m.from());
    if(m.to() != move.to() || f.type() != field.type() || m.new_type() != move.new_type())
      continue;

    // check for disambiguation in 'from' position
    if(Index(m.from()).x() == xfrom)
      same_x++;

    if(Index(m.from()).y() == yfrom)
      same_y++;

    disambiguations++;
  }

  if(!found)
    return "";

  std::string str;
  if(field.type() == Figure::Type::TypeKing && (2 == move.to() - move.from() || -2 == move.to() - move.from()))// castle
  {
    if(move.to() > move.from()) // short castle
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

    if(disambiguations > 1 || (field.type() == Figure::Type::TypePawn && capture))
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
    if(capture)
    {
      str += 'x';
    }

    str += static_cast<char>('a' + xto);
    str += static_cast<char>('1' + yto);

    if(move.new_type() > 0)
    {
      str += '=';
      str += fromFtype((Figure::Type)move.new_type());
    }
  }

  if(ChessMat & state)
  {
    str += '#';
  }
  else if(UnderCheck & state)
  {
    str += '+';
  }
  return str;
}
//////////////////////////////////////////////////////////////////////////

std::string moveToStr(const Move move, bool wbf)
{
  if(!move || move.from() < 0 || move.to() < 0 || move.from() == move.to())
    return s_nullmove;

  Index from(move.from());
  Index to(move.to());

  std::string str;
  if(wbf)
  {
    str = "move ";
  }

  str += static_cast<char>('a' + from.x());
  str += static_cast<char>('1' + from.y());
  str += static_cast<char>('a' + to.x());
  str += static_cast<char>('1' + to.y());

  if(move.new_type() <= 0)
    return str;

  switch(static_cast<Figure::Type>(move.new_type()))
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

Move strToMove(std::string const& str)
{
  if (str.size() < 4)
    return Move{ true };

  if (!iscolumn(str[0]) || !isdigit(str[1]) && !iscolumn(str[2]) && !isdigit(str[3]))
    return Move{ true };

  int xfrom = str[0] - 'a';
  int yfrom = str[1] - '1';
  int xto = str[2] - 'a';
  int yto = str[3] - '1';

  Figure::Type new_type{};
  int from{}, to{};
  if (str.size() > 4 && isalpha(str[4]))
  {
    if ('b' == str[4])
      new_type = Figure::Type::TypeBishop;
    else if ('n' == str[4])
      new_type = Figure::Type::TypeKnight;
    else if ('r' == str[4])
      new_type = Figure::Type::TypeRook;
    else if ('q' == str[4])
      new_type = Figure::Type::TypeQueen;
  }

  from = Index(xfrom, yfrom);
  to = Index(xto, yto);
  return Move{ from, to, new_type };
}

Move strToMove(std::string const& str, const Board & board)
{
  if(str.empty())
    return Move{true};

  eMoveNotation mnot = detectNotation(str);
  if(mnot == eMoveNotation::mnUnknown)
    return Move{true};
  else if(mnot == eMoveNotation::mnSAN)
    return parseSAN(board, str);

  if(str.size() < 4)
    return Move{true};

  Figure::Color color = board.color();

  auto move = strToMove(str);
  if (!move)
    return move;

  const Field ffrom = board.getField(move.from());
  if(!ffrom || ffrom.color() != color)
    return Move{true};

  return board.possibleMove(move) ? move : Move{true};
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: rewrite with std::string/std::regex
/* rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 */
bool fromFEN(std::string const& i_fen, Board& board)
{
  static std::string const stdFEN_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  board.clear();

  std::string fen = i_fen.empty() ? stdFEN_ : i_fen;
  trim(fen);

  std::smatch m;
  if (!std::regex_search(fen, m, rfen_) || m.size() < 3) {
    return false;
  }
  std::string fstr = m[1];
  std::string astr = m[3];
  auto lines = split(fstr, [](char c) { return c == '/'; });
  for (int y = 7; y >= 0; --y) {
    auto const& line = lines[7 - y];
    int x = 0;
    for (char c : line) {
      if (x > 7) {
        break;
      }
      if (isdigit(c)) {
        int dx = c - '0';
        x += dx;
        continue;
      }
      else if (isalpha(c)) {
        Figure::Type  ftype = Figure::TypeNone;
        Figure::Color color = Figure::ColorBlack;
        if (isupper(c))
          color = Figure::ColorWhite;
        ftype = Figure::toFtype(toupper(c));
        if (Figure::TypeNone == ftype)
          return false;
        board.addFigure(color, ftype, Index(x, y));
        ++x;
      }
    }
  }
  std::smatch sr;
  if (!std::regex_search(astr, sr, rafen_) || sr.size() < 10) {
    return false;
  }
  std::string scolor = sr[1];
  std::string scastle = sr[3];
  std::string senpass = sr[5];
  std::string sfifty = sr[7];
  std::string shalf = sr[9];
  if (!scolor.empty()) {
    char c = scolor[0];
    if ('w' == c)
      board.setColor(Figure::ColorWhite);
    else if ('b' == c)
    {
      board.setColor(Figure::ColorBlack);
      board.hashColor();
    }
  }
  for (char c : scastle) {
    switch (c) {
    case 'k':
      board.set_castling(Figure::ColorBlack, 0);
      break;
    case 'K':
      board.set_castling(Figure::ColorWhite, 0);
      break;
    case 'q':
      board.set_castling(Figure::ColorBlack, 1);
      break;
    case 'Q':
      board.set_castling(Figure::ColorWhite, 1);
      break;
    default:
      break;
    }
  }
  if (senpass.size() >= 2 && senpass[0] != '-') {
    char cx = senpass[0];
    char cy = senpass[1];
    Index enpassant(cx, cy);
    if (board.color())
      cy--;
    else
      cy++;
    Index pwpos(cx, cy);
    const Field fp = board.getField(pwpos);
    if (fp.type() != Figure::TypePawn || fp.color() == board.color()) {
      return false;
    }
    board.setEnpassant(enpassant, fp.color());
  }
  if (!sfifty.empty() && isdigit(sfifty[0])) {
    board.setFiftyMovesCount(std::stoi(sfifty));
  }
  if (!shalf.empty() && isdigit(shalf[0])) {
    board.setMovesCounter(std::stoi(shalf));
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
      const Field field = board.getField(idx);
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
  if(Figure::ColorBlack == board.color())
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
        return "";

      fen += 'K';
    }
    if(board.castling_Q())
    {
      if(!board.verifyCastling(Figure::ColorWhite, 1))
        return "";

      fen += 'Q';
    }
    if(board.castling_k())
    {
      if(!board.verifyCastling(Figure::ColorBlack, 0))
        return "";

      fen += 'k';
    }
    if(board.castling_q())
    {
      if(!board.verifyCastling(Figure::ColorBlack, 1))
        return "";

      fen += 'q';
    }
  }
}

{
  // 4 - en passant
  fen += ' ';
  if(board.enpassant() > 0)
  {
    Index ep_pos(board.enpassant());

    int x = ep_pos.x();
    int y = ep_pos.y();

    if(board.color())
      y--;
    else
      y++;

    Index pawn_pos(x, y);
    const Field ep_field = board.getField(pawn_pos);
    if(ep_field.color() == board.color() || ep_field.type() != Figure::TypePawn)
      return "";

    char cx = 'a' + ep_pos.x();
    char cy = '1' + ep_pos.y();
    fen += cx;
    fen += cy;
  }
  else
    fen += '-';

  // 5 - fifty move rule
  fen += " " + std::to_string(static_cast<int>(board.fiftyMovesCount()));

  // 6 - moves counter
  fen += " " + std::to_string(board.movesCounter());
}

return fen;
}

bool load(Board & board, std::istream & is)
{
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
    for(; !skip && si != str.end(); ++si)
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
            if(si == str.end())
            {
              skip = true;
              break;
            }
            continue;
          }

          // read value
          if('"' == *si && value.empty())
          {
            for(si++; si != str.end() && '"' != *si; ++si)
              value += *si;
            if(si == str.end())
            {
              skip = true;
              break;
            }
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
      if(skip)
        break;
    }
  }

  if(smoves.empty())
    return false;

  if(!fen_init)
    fromFEN("", board);

  for(auto const& str : smoves)
  {
    Move move = strToMove(str, board);
    if(!move)
      return false;

    if(!board.validateMoveBruteforce(move))
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
    if(board.color())
      sres = "0-1";
    else
      sres = "1-0";
  }
  else if(board.drawState())
    sres = "1/2-1/2";

  SBoard<Board, UndoInfo, Board::GameLength> sboard(board, true);

  int num = sboard.halfmovesCount();

  while(sboard.halfmovesCount() > 0)
    sboard.unmakeMove(sboard.lastUndo().move_);

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
    Figure::Color color = sboard.color();
    int moveNum = sboard.movesCount();

    // stash move
    Move move = board.undoInfo(i).move_;

    auto str = printSAN(sboard, move);
    if(str.empty())
      return false;

    // now apply move
    if(!sboard.validateMoveBruteforce(move))
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

  os << sres << std::endl << std::endl;

  return true;
}

std::string join(std::vector<std::string> const& vec, std::string const& delim)
{
  return join(vec.begin(), vec.end(), delim);
}

std::string join(std::vector<std::string>::const_iterator from, std::vector<std::string>::const_iterator to, std::string const& delim)
{
  std::string result;
  for (auto i = from; i != to; ++i)
  {
    result += *i;
    if (i != to - 1)
    {
      result += delim;
    }
  }
  return result;
}

void trim(std::string& str)
{
  auto i = std::find_if(str.begin(), str.end(), [](char c) { return !std::isspace(c, std::locale{}); });
  str.erase(str.begin(), i);
  auto j = std::find_if(str.rbegin(), str.rend(), [](char c) { return !std::isspace(c, std::locale{}); });
  str.erase(j.base(), str.end());
}

void to_lower(std::string& str)
{
  std::transform(str.begin(), str.end(), str.begin(), [](char c) { return std::tolower(c, std::locale{}); });
}

bool is_any_of(std::string const& of, char c)
{
  return std::find_if(of.begin(), of.end(), [c](char x) { return c == x; }) != of.end();
}

std::vector<std::string> split(std::string const& str, std::function<bool(char)> const& pred)
{
  std::vector<std::string> result;
  auto to = str.begin();
  while (to != str.end())
  {
    auto from = std::find_if_not(to, str.end(), pred);
    if (from == str.end())
      break;
    to = std::find_if(from, str.end(), pred);
    std::string s{ from, to };
    result.push_back(std::move(s));
  }
  return result;
}

int XCounter::count_ = 0;

#ifdef _USE_LOG
void addLog(std::string const& str)
{
  std::ofstream ofs("my_log.txt", std::ios::app);
  ofs << str << std::endl;
}
#endif


} // NEngine
