/*************************************************************
  Board.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Board.h>
#include <FigureDirs.h>
#include <MovesGenerator.h>
#include <Evaluator.h>
#include <xindex.h>
#include <Helpers.h>
#include <fstream>
#include <boost/algorithm/string/trim.hpp>

namespace NEngine
{

// static data
char Board::fen_[FENsize];
std::string const Board::stdFEN_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int64 Board::ticks_;
int   Board::tcounter_;

Board::Board()
{
  clear();
}

void Board::clear()
{
  fmgr_.clear();

  // clear global FEN
  fen_[0] = 0;

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
  const uint64 & p_caps = movesTable().pawnCaps_o(ffrom.color(), move.to_);
  const uint64 & o_mask = fmgr_.mask(ocolor);
  if ( p_caps & o_mask )
    return true;

  //// becomes passed
  const uint64 & pmsk = fmgr_.pawn_mask_t(color);
  const uint64 & opmsk = fmgr_.pawn_mask_t(ocolor);
  const uint64 & passmsk = pawnMasks().mask_passed(color, move.to_);
  const uint64 & blckmsk = pawnMasks().mask_blocked(color, move.to_);

  if ( !(opmsk & passmsk) && !(pmsk & blckmsk) )
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

	const BitMask & pw_caps = movesTable().pawnCaps_o(ocolor, move.to_);
	BitMask op_mask = fmgr().knight_mask(color_) | fmgr().bishop_mask(color_) | fmgr().rook_mask(color_) | fmgr().queen_mask(color_);
	op_mask &= pw_caps;

	return op_mask != 0 && !one_bit_set(op_mask);
}

bool Board::isBishopAttack(const Move & move) const
{
	const Field & fto = getField(move.to_);
	if ( fto.type() != Figure::TypeBishop )
		return false;

	Figure::Color  color = color_;
	Figure::Color ocolor = Figure::otherColor(color);

	const BitMask & bi_caps = movesTable().caps(Figure::TypeBishop, move.to_);
	BitMask op_mask = fmgr().rook_mask(color) | fmgr().queen_mask(color);
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

// TODO: rewrite with std::string/std::regex/boost::trim/...
/* rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 */
bool Board::fromFEN(std::string const& i_fen)
{
  clear();

  std::string fen = i_fen.empty() ? stdFEN_ : i_fen;
  boost::algorithm::trim(fen);

  const char * s = fen.c_str();
  int x = 0, y = 7;
  for ( ; s && y >= 0; ++s)
  {
    char c = *s;
    if ( '/' == c )
      continue;
    else if ( isdigit(c) )
    {
      int dx = c - '0';
      x += dx;
      if ( x > 7 )
      {
        --y;
        x = 0;
      }
    }
    else
    {
      Figure::Type  ftype = Figure::TypeNone;
      Figure::Color color = Figure::ColorBlack;

      if ( isupper(c) )
        color = Figure::ColorWhite;

      ftype = Figure::toFtype(toupper(c));

      if ( Figure::TypeNone == ftype )
        return false;

      Index p(x, y);
      addFigure(color, ftype, p);

      if ( ++x > 7 )
      {
        --y;
        x = 0;
      }
    }
  }

  int i = 0;
  for ( ; *s; )
  {
    char c = *s;
    if ( c <= ' ' )
    {
      ++s;
      continue;
    }

    if ( 0 == i ) // process move color
    {
      if ( 'w' == c )
        color_ = Figure::ColorWhite;
      else if ( 'b' == c )
        color_ = Figure::ColorBlack;
      else
        return false;
      ++i;
      ++s;
      continue;
    }

    if ( 1 == i ) // process castling possibility
    {
      if ( '-' == c )
      {
        ++i;
        ++s;
        continue;
      }

      for ( ; *s && *s > 32; ++s)
      {
        Field fk, fr;

        switch ( *s )
        {
        case 'k':
          set_castling(Figure::ColorBlack, 0);
          fmgr_.hashCastling(Figure::ColorBlack, 0);
          break;

        case 'K':
          set_castling(Figure::ColorWhite, 0);
          fmgr_.hashCastling(Figure::ColorWhite, 0);
          break;

        case 'q':
          set_castling(Figure::ColorBlack, 1);
          fmgr_.hashCastling(Figure::ColorBlack, 1);
          break;

        case 'Q':
          set_castling(Figure::ColorWhite, 1);
          fmgr_.hashCastling(Figure::ColorWhite, 1);
          break;

        default:
          return false;
        }
      }

      ++i;
      continue;
    }

    if ( 2 == i )
    {
      ++i;
      char cx = *s++;
      if ( '-' == cx ) // no en-passant pawn
        continue;

      if ( !*s )
        return false;

      char cy = *s++;

      Index enpassant(cx, cy);

      if ( color_ )
        cy--;
      else
        cy++;

      Index pawn_pos(cx, cy);

      Field & fp = getField(pawn_pos);
      if ( fp.type() != Figure::TypePawn || fp.color() == color_ )
        return false;

      en_passant_ = enpassant;
      fmgr_.hashEnPassant(en_passant_, fp.color());
      continue;
    }

    if ( 3 == i ) // fifty moves rule
    {
      char str[4];
      str[0] = 0;
      for (int j = 0; *s && j < 4; ++j, ++s)
      {
        if ( *s > 32 )
          str[j] = *s;
        else
        {
          str[j] = 0;
          break;
        }
      }

      fiftyMovesCount_ = atoi(str);
      ++i;
      continue;
    }

    if ( 4 == i ) // moves counter
    {
      movesCounter_ = atoi(s);
      break;
    }
  }

  if ( !invalidate() )
    return false;

  return true;
}

//////////////////////////////////////////////////////////////////////////
std::string Board::toFEN() const
{
  std::string fen;

  // 1 - write figures
  for (int y = 7; y >= 0; --y)
  {
    int n = 0;
    for (int x = 0; x < 8; ++x)
    {
      Index idx(x, y);
      const Field & field = getField(idx);
      if ( !field )
      {
        ++n;
        continue;
      }

      if ( n > 0 )
      {
        fen += '0' + n;
        n = 0;
      }

      char c = fromFtype(field.type());
      if ( field.color() == Figure::ColorBlack )
        c = tolower(c);

      fen += c;
    }
    
    if ( n > 0 )
      fen += '0' + n;

    if ( y > 0 )
      fen += '/';
  }

  // 2 - color to move
  {
    fen += ' ';
    if ( Figure::ColorBlack == color_ )
      fen += 'b';
    else
      fen += 'w';
  }

  // 3 - castling possibility
  {
    fen += ' ';
	  if ( !castling() )
    {
      fen += '-';
    }
    else
    {
      if ( castling_K() )
      {
        if ( !verifyCastling(Figure::ColorWhite, 0) )
          return false;

        fen += 'K';
      }
      if ( castling_Q() )
      {
        if ( !verifyCastling(Figure::ColorWhite, 1) )
          return false;

        fen += 'Q';
      }
      if ( castling_k() )
      {
        if ( !verifyCastling(Figure::ColorBlack, 0) )
          return false;
        
        fen += 'k';
      }
      if ( castling_q() )
      {
        if ( !verifyCastling(Figure::ColorBlack, 1) )
          return false;

        fen += 'q';
      }
    }
  }

  {
    // 4 - en passant
    fen += ' ';
    if ( en_passant_ >= 0 )
    {
      Index ep_pos(en_passant_);

      int x = ep_pos.x();
      int y = ep_pos.y();

      if ( color_ )
        y--;
      else
        y++;

      Index pawn_pos(x, y);
      const Field & ep_field = getField(pawn_pos);
      if ( ep_field.color() == color_ || ep_field.type() != Figure::TypePawn )
        return false;

      char cx = 'a' + ep_pos.x();
      char cy = '1' + ep_pos.y();
      fen += cx;
      fen += cy;
    }
    else
      fen += '-';

    // 5 - fifty move rule
    fen += " " + std::to_string(fiftyMovesCount_);

    // 6 - moves counter
    fen += " " + std::to_string(movesCounter_);
  }

  return fen;
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
    const Move & m = mg.move();
    if ( !m )
      break;

    if ( validateMove(m) )
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
bool Board::load(Board & board, std::istream & is)
{
  const int N = 1024;
  char str[N];
  const char * sepr  = " \t\n\r";
  const char * strmv = "123456789abcdefghkrq+-OxPNBRQKnul=#";
  bool fen_expected = false, fen_init = false;

  const int ML = 16;
  char smoves[GameLength][ML];
  int  hmovesN = 0;

  bool annot = false;
  bool stop = false;
  while ( !stop && is.getline(str, N-1, '\n') )
  {
    bool skip = false;
    char * s = str;
    for ( ; *s && !skip; ++s)
    {
      // skip separators
      if ( strchr(sepr, *s) )
        continue;

      // skip annotations {}
      if ( '{' == *s )
        annot = true;

      if ( annot && '}' != *s )
        continue;

      annot = false;

      // skip comment string
      if ( '%' == *s )
      {
        skip = true;
        break;
      }

      if ( '.' == *s ) // skip .
        continue;
      else if ( isdigit(*s) && (*(s+1) && (isdigit(*(s+1)) || strchr(sepr, *(s+1)) || '.' == *(s+1))) ) // skip move number
      {
        for ( ;*s && isdigit(*s); ++s);
        if ( !*s )
          skip = true;
      }
      else if ( isalpha(*s) || (isdigit(*s) && (*(s+1) && isalpha(*(s+1)))) ) // read move
      {
        int i = 0;
        for ( ;*s && i < ML && strchr(strmv, *s); ++s)
          smoves[hmovesN][i++] = *s;
        if ( i < ML )
          smoves[hmovesN][i] = 0;
        hmovesN++;
        if ( !*s )
          skip = true;
      }
      else if ( ('*' == *s) || (isdigit(*s) && (*(s+1) && '-' == *(s+1))) ) // end game
      {
        skip = true;
        stop = true;
      }
      else if ( '[' == *s ) // read some extra params, like FEN
      {
        char param[N]= {'\0'};
        char value[N]= {'\0'};
        for ( ; *s && ']' != *s; ++s)
        {
          // skip separators
          if ( strchr(sepr, *s) )
            continue;

          // read param name
          if ( isalpha(*s) && !param[0] )
          {
            for (int i = 0; *s && i < N && !strchr(sepr, *s); ++s, ++i)
              param[i] = *s;
            continue;
          }

          // read value
          if ( '"' == *s && !value[0] )
          {
            s++;
            for (int i = 0; *s && i < N && '"' != *s; ++s, ++i)
              value[i] = *s;
            continue;
          }
        }

        if ( strcmp(param, "SetUp") == 0 && '1' == value[0] )
          fen_expected = true;
        else if ( strcmp(param, "FEN") == 0 && fen_expected )
        {
          if ( !board.fromFEN(value) )
            return false;
          fen_init = true;
        }
        skip = true;
      }
    }
  }

  if ( !fen_init )
    board.fromFEN(0);

  for (int i = 0; i < hmovesN; ++i)
  {
    Move move = strToMove(smoves[i], board);
    if(!move)
      return false;

    //char str[64];
    //moveToStr(move, str, false);

    //Move mv;
    //strToMove(str, board, mv);

    if ( !board.validateMove(move) )
      return false;

    board.makeMove(move);
  }

  board.verifyState();

  return true;
}

bool Board::save(const Board & board, std::ostream & os, bool write_prefix)
{
  const char * sres = "*";
  if ( board.matState() )
  {
    if ( board.getColor() )
      sres = "0-1";
    else
      sres = "1-0";
  }
  else if ( board.drawState() )
    sres = "1/2-1/2";

  Board sboard = board;
  UndoInfo tempUndo[Board::GameLength];

  int num = sboard.halfmovesCount();
  for (int i = 0; i < num; ++i)
    tempUndo[i] = board.undoInfo(i);
  sboard.set_undoStack(tempUndo);

  while ( sboard.halfmovesCount() > 0 )
    sboard.unmakeMove();

  if ( write_prefix )
  {
    auto fen = sboard.toFEN();
    if(!fen.empty() && fen != stdFEN_)
    {
      os << "[SetUp \"1\"] " << std::endl;
      os << "[FEN \"" << fen << "\"]" << std::endl;
    }

    os << "[Result \"";
    os << sres;
    os << "\"]" << std::endl;
  }

  for (int i = 0; i < num; ++i)
  {
    Figure::Color color = sboard.getColor();
    int moveNum = sboard.movesCount();

    // stash move
    Move move = board.undoInfo(i);

    auto str = printSAN(sboard, move);
    if(str.empty() )
      return false;

    // now apply move
    if ( !sboard.validateMove(move) )
      return false;

    sboard.makeMove(move);

    if ( color || !i )
      os << moveNum << ". ";
    
    if ( !i && !color )
      os << " ... ";

    os << str;

    if ( !color )
      os << std::endl;
    else
      os << " ";
  }

  os << sres << std::endl;

  return true;
}

//////////////////////////////////////////////////////////////////////////
void Board::verifyMasks() const
{
  for (int c = 0; c < 2; ++c)
  {
    BitMask pawn_mask_o = 0ULL;
    BitMask pawn_mask_t = 0ULL;
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
          pawn_mask_t |= set_mask_bit(Index(p).transp());
          pawn_mask_o |= set_mask_bit(p);
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

    X_ASSERT( pawn_mask_o != fmgr_.pawn_mask_o(color), "pawn mask invalid" );
    X_ASSERT( pawn_mask_t != fmgr_.pawn_mask_t(color), "pawn mask invalid" );
    X_ASSERT( knight_mask != fmgr_.knight_mask(color), "knight mask invalid" );
    X_ASSERT( bishop_mask != fmgr_.bishop_mask(color), "bishop mask invalid" );
    X_ASSERT( rook_mask != fmgr_.rook_mask(color), "rook mask invalid" );
    X_ASSERT( queen_mask != fmgr_.queen_mask(color), "queen mask invalid" );
    X_ASSERT( king_mask != fmgr_.king_mask(color), "king mask invalid" );
    X_ASSERT( all_mask != fmgr_.mask(color), "invalid all figures mask" );

  }
}

//////////////////////////////////////////////////////////////////////////
// only for debugging purposes, saves complete board memory dump
void Board::save(const char * fname) const
{
  std::ofstream ofs(fname, std::ofstream::binary);
  if(!ofs)
    return;
  ofs.write(reinterpret_cast<char*>(const_cast<Board*>(this)), sizeof(*this));
  ofs.write(reinterpret_cast<char*>(g_undoStack), sizeof(UndoInfo)*GameLength);
}

void Board::load(const char * fname)
{
  std::ifstream ifs(fname, std::ifstream::binary);
  if(!ifs)
    return;
  UndoInfo * undoStack = g_undoStack;
  ifs.read(reinterpret_cast<char*>(const_cast<Board*>(this)), sizeof(*this));
  g_undoStack = undoStack;
  ifs.read(reinterpret_cast<char*>(g_undoStack), sizeof(UndoInfo)*GameLength);
}

} // NEngine
