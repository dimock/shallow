#include <xoptimize.h>
#include <MovesGenerator.h>
#include <xindex.h>
#include <Helpers.h>
#include <boost/algorithm/string/trim.hpp>

namespace NEngine
{

int64 x_movesCounter = 0;

namespace
{

void generate(Board2 const& board, xlist<Move2, 256>& moves)
{
  const Figure::Color & color = board.color();
  const Figure::Color ocolor = Figure::otherColor(color);
  auto const& fmgr = board.fmgr();

  if(board.checkingNum() < 2)
  {
    // pawns movements
    if(BitMask pw_mask = fmgr.pawn_mask(color))
    {
      for(; pw_mask;)
      {
        int pw_pos = clear_lsb(pw_mask);

        const int8 * table = movesTable().pawn(color, pw_pos);

        for(int i = 0; i < 2; ++i, ++table)
        {
          if(*table < 0)
            continue;

          const Field & field = board.getField(*table);
          bool capture = false;
          if((field && field.color() == ocolor) ||
             (board.enpassant() >= 0 && *table == board.enpassant()))
          {
            capture = true;
          }

          if(!capture)
            continue;

          bool promotion = *table > 55 || *table < 8;

          Move2 move{ pw_pos, *table, Figure::TypeNone };

          if(promotion)
          {
            move.new_type = Figure::TypeQueen;
            moves.push_back(move);

            move.new_type = Figure::TypeRook;
            moves.push_back(move);

            move.new_type = Figure::TypeBishop;
            moves.push_back(move);

            move.new_type = Figure::TypeKnight;
            moves.push_back(move);
          }
          else
          {
            moves.push_back(move);
          }
        }

        for(; *table >= 0 && !board.getField(*table); ++table)
        {
          bool promotion = *table > 55 || *table < 8;

          Move2 move{ pw_pos, *table, Figure::TypeNone };

          if(promotion)
          {
            move.new_type = Figure::TypeQueen;
            moves.push_back(move);

            move.new_type = Figure::TypeRook;
            moves.push_back(move);

            move.new_type = Figure::TypeBishop;
            moves.push_back(move);

            move.new_type = Figure::TypeKnight;
            moves.push_back(move);
          }
          else
          {
            moves.push_back(move);
          }
        }
      }
    }

    // knights movements
    if(fmgr.knight_mask(color))
    {
      BitMask kn_mask = fmgr.knight_mask(color);
      for(; kn_mask;)
      {
        int kn_pos = clear_lsb(kn_mask);

        const int8 * table = movesTable().knight(kn_pos);

        for(; *table >= 0; ++table)
        {
          const Field & field = board.getField(*table);
          bool capture = false;
          if(field)
          {
            if(field.color() == color)
              continue;

            capture = true;
          }

          moves.emplace_back(kn_pos, *table, Figure::TypeNone);
        }
      }
    }

    // bishops, rooks and queens movements
    for(int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      BitMask fg_mask = fmgr.type_mask((Figure::Type)type, color);

      for(; fg_mask;)
      {
        int fg_pos = clear_lsb(fg_mask);

        const uint16 * table = movesTable().move(type-Figure::TypeBishop, fg_pos);

        for(; *table; ++table)
        {
          const int8 * packed = reinterpret_cast<const int8*>(table);
          int8 count = packed[0];
          int8 const& delta = packed[1];

          int8 p = fg_pos;
          bool capture = false;
          for(; count && !capture; --count)
          {
            p += delta;

            const Field & field = board.getField(p);
            if(field)
            {
              if(field.color() == color)
                break;

              capture = true;
            }

            moves.emplace_back(fg_pos, p, Figure::TypeNone);
          }
        }
      }
    }
  }

  // kings movements
  {
    BitMask ki_mask = fmgr.king_mask(color);

    X_ASSERT(ki_mask == 0, "invalid position - no king");

    int ki_pos = clear_lsb(ki_mask);

    const int8 * table = movesTable().king(ki_pos);

    for(; *table >= 0; ++table)
    {
      const Field & field = board.getField(*table);
      bool capture = false;
      if(field)
      {
        if(field.color() == color)
          continue;

        capture = true;
      }

      moves.emplace_back(ki_pos, *table, Figure::TypeNone);
    }

    if(!board.underCheck())
    {
      // short castle
      if(board.castling(board.color(), 0) && !board.getField(ki_pos+2) && !board.getField(ki_pos+1))
        moves.emplace_back(ki_pos, ki_pos+2, Figure::TypeNone);

      // long castle
      if(board.castling(board.color(), 1) && !board.getField(ki_pos-2) && !board.getField(ki_pos-1) && !board.getField(ki_pos-3))
        moves.emplace_back(ki_pos, ki_pos-2, Figure::TypeNone);
    }
  }
}

template <class BOARD, class MOVE>
struct CapsGenerator2
{
  using MovesList = xlist<MOVE, BOARD::MovesMax>;


  CapsGenerator2(BOARD const& board) :
    board_(board)
  {}

  inline void add(int8 from, int8 to, int8 new_type)
  {
    moves_.emplace_back(from, to, new_type);
  }

  void generateCaps()
  {
    const Figure::Color color = board_.getColor();
    const Figure::Color ocolor = Figure::otherColor(color);

    auto const& fmgr = board_.fmgr();
    BitMask opponent_mask = fmgr.mask(ocolor) ^ fmgr.king_mask(ocolor);
    const BitMask & black = board_.fmgr().mask(Figure::ColorBlack);
    const BitMask & white = board_.fmgr().mask(Figure::ColorWhite);
    BitMask mask_all = white | black;
    int ki_pos  = board_.kingPos(color);
    int oki_pos = board_.kingPos(ocolor);

    // generate pawn promotions
    const BitMask & pawn_msk = fmgr.pawn_mask(color);
    {
      static int pw_delta[] = { -8, +8 };
      BitMask promo_msk = movesTable().promote(color);
      promo_msk &= pawn_msk;

      for(; promo_msk;)
      {
        int from = clear_lsb(promo_msk);

        X_ASSERT((unsigned)from > 63, "invalid promoted pawn's position");

        const Field & field = board_.getField(from);

        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypePawn, "there is no pawn to promote");

        int to = from + pw_delta[color];

        X_ASSERT((unsigned)to > 63, "pawn tries to go to invalid field");

        if(board_.getField(to))
          continue;

        add(from, to, Figure::TypeQueen);

        // add promotion to checking knight
        if(figureDir().dir(Figure::TypeKnight, color, to, oki_pos) >= 0)
          add(from, to, Figure::TypeKnight);
      }
    }

    // firstly check if we have at least 1 attacking pawn
    bool pawns_eat = pawn_msk != 0ULL;
    if(pawns_eat)
    {
      BitMask pawn_eat_msk = 0;
      if(color)
        pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
      else
        pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
      pawns_eat = (pawn_eat_msk & opponent_mask) != 0ULL;
      if(!pawns_eat && board_.enpassant() >= 0)
        pawns_eat = (pawn_eat_msk & set_mask_bit(board_.enpassant())) != 0ULL;
    }
    // generate captures

    // 1. Pawns
    if(pawns_eat)
    {
      BitMask pw_mask = board_.fmgr().pawn_mask(color);

      for(; pw_mask;)
      {
        int pw_pos = clear_lsb(pw_mask);

        BitMask p_caps = movesTable().pawnCaps(color, pw_pos) & opponent_mask;

        for(; p_caps;)
        {
          X_ASSERT(!pawns_eat, "have pawns capture, but not detected by mask");

          int to = clear_lsb(p_caps);

          bool promotion = to > 55 || to < 8; // 1st || last line

          X_ASSERT((unsigned)to > 63, "invalid pawn's capture position");

          const Field & field = board_.getField(to);
          if(!field || field.color() != ocolor)
            continue;

          add(pw_pos, to, promotion ? Figure::TypeQueen : Figure::TypeNone);

          // add promotion to checking knight
          if(promotion && figureDir().dir(Figure::TypeKnight, color, to, oki_pos) >= 0)
            add(pw_pos, to, Figure::TypeKnight);
        }
      }

      if(board_.enpassant() >= 0)
      {
        X_ASSERT(board_.getField(board_.enpassantPos()).type() != Figure::TypePawn || board_.getField(board_.enpassantPos()).color() != ocolor, "there is no en passant pawn");
        BitMask ep_mask = movesTable().pawnCaps(ocolor, board_.enpassant()) & board_.fmgr().pawn_mask(color);

        for(; ep_mask;)
        {
          int from = clear_lsb(ep_mask);
          add(from, board_.enpassant(), Figure::TypeNone);
        }
      }
    }

    // 2. Knights
    BitMask kn_mask = board_.fmgr().knight_mask(color);
    for(; kn_mask;)
    {
      int kn_pos = clear_lsb(kn_mask);

      // don't need to verify capture possibility by mask
      BitMask f_caps = movesTable().caps(Figure::TypeKnight, kn_pos) & opponent_mask;
      for(; f_caps;)
      {
        int to = clear_lsb(f_caps);

        X_ASSERT((unsigned)to > 63, "invalid field index while capture");

        const Field & field = board_.getField(to);

        X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

        add(kn_pos, to, Figure::TypeNone);
      }
    }

    {
      auto bi_mask = board_.fmgr().type_mask(Figure::TypeBishop, color);
      for(; bi_mask;)
      {
        int from = clear_lsb(bi_mask);
        auto f_caps = magic_ns::bishop_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          int8 to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const Field & field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    {
      auto r_mask = board_.fmgr().type_mask(Figure::TypeRook, color);
      for(; r_mask;)
      {
        int from = clear_lsb(r_mask);
        auto f_caps = magic_ns::rook_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          int8 to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const Field & field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    {
      auto q_mask = board_.fmgr().type_mask(Figure::TypeQueen, color);
      for(; q_mask;)
      {
        int from = clear_lsb(q_mask);
        auto f_caps = magic_ns::queen_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          int8 to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const Field & field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    // 4. King
    {
      // don't need to verify capture possibility by mask
      BitMask f_caps = movesTable().caps(Figure::TypeKing, ki_pos) & opponent_mask;
      for(; f_caps;)
      {
        int to = clear_lsb(f_caps);

        X_ASSERT((unsigned)to > 63, "invalid field index while capture");

        const Field & field = board_.getField(to);

        X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

        add(ki_pos, to, Figure::TypeNone);
      }
    }
  }

  BOARD const& board_;
  MovesList moves_;
};

}

void xcaptures(Board2& board, int depth)
{
  if(board.drawState() || board.countReps() > 1 || depth < -10)
    return;
  TacticalGenerator tg(board, Figure::TypeNone, depth);
  for(;;)
  {
    auto* pmove = tg.test_move();
    if(!pmove)
      break;
    auto& move = *pmove;
    if(!board.validateMove(move))
      continue;
    board.makeMove(move);
    x_movesCounter++;
    xcaptures(board, depth-1);
    board.unmakeMove();
  }
}

void xsearch(Board2& board, int depth)
{
  if(board.drawState() || board.countReps() > 1)
    return;
  if(depth <= 0)
  {
    xcaptures(board, depth);
    return;
  }
  FastGenerator fg(board);
  for(;;)
  {
    auto* pmove = fg.test_move();
    if(!pmove)
      break;
    auto& move = *pmove;
    if(!board.validateMove(move))
      continue;
    board.makeMove(move);
    x_movesCounter++;
    xsearch(board, depth-1);
    board.unmakeMove();
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Board2::addFigure(const Figure::Color color, const Figure::Type type, int pos)
{
  if(!type || (unsigned)pos > 63)
    return false;

  Field & field = getField(pos);
  if(field)
    return false;

  fields_[pos].set(color, type);
  fmgr_.incr(color, type, pos);

  return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: rewrite with std::string/std::regex/boost::trim/...
/* rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 */
bool fromFEN(std::string const& i_fen, Board2& board)
{
  static std::string const stdFEN_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  board = Board2{};

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

      board.addFigure(color, ftype, Index(x, y));

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
        board.setColor(Figure::ColorWhite);
      else if('b' == c)
      {
        board.setColor(Figure::ColorBlack);
        board.hashColor();
      }
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

      if(board.color())
        cy--;
      else
        cy++;

      Index pawn_pos(cx, cy);

      Field & fp = board.getField(pawn_pos);
      if(fp.type() != Figure::TypePawn || fp.color() == board.color())
        return false;

      board.setEnpassant(enpassant, fp.color());
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

      board.setFiftyMovesCount(atoi(str));
      ++i;
      continue;
    }

    if(4 == i) // moves counter
    {
      board.setMovesCounter(atoi(s));
      break;
    }
  }

  if(!board.invalidate())
    return false;

  return true;
}

bool Board2::invalidate()
{
  Figure::Color ocolor = Figure::otherColor(color_);

  int ki_pos  = _lsb64(fmgr_.king_mask(color_));
  int oki_pos = _lsb64(fmgr_.king_mask(ocolor));

  state_ = Ok;

  if(isAttacked(color_, oki_pos))
  {
    state_ = Invalid;
    return false;
  }

  if(isAttacked(ocolor, ki_pos))
    state_ |= UnderCheck;

  verifyChessDraw();

  if(drawState())
    return true;

  int cnum = findCheckingFigures(ocolor, ki_pos);
  if(cnum > 2)
  {
    state_ = Invalid;
    return false;
  }
  else if(cnum > 0)
  {
    state_ |= UnderCheck;
  }

  verifyState();

  return true;
}

bool Board2::fieldAttacked(const Figure::Color c, int8 pos, const BitMask & mask_all_inv) const
{
  Figure::Color ocolor = Figure::otherColor(c);

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

int Board2::findCheckingFigures(Figure::Color ocolor, int ki_pos)
{
  checkingNum_ = 0;
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
        if(checkingNum_ > 1)
          return ++checkingNum_;

        checking_[checkingNum_++] = n;
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
        if(checkingNum_ > 1)
          return ++checkingNum_;

        checking_[checkingNum_++] = n;
      }
    }
  }

  return checkingNum_;
}

// verify is there draw or mat
void Board2::verifyState()
{
  if(matState() || drawState())
    return;

  xlist<Move2, 256> moves;
  generate(*this, moves);
  bool found = false;
  for(auto it = moves.begin(); it != moves.end() && !found; ++it)
  {
    auto const& move = *it;
    if(validateMove(move))
      found = true;
  }

  if(!found)
  {
    setNoMoves();

    // update move's state because it is last one
    if(halfmovesCounter_ > 0)
    {
      UndoInfo & undo = undoInfo(halfmovesCounter_-1);
      undo.state_ = state_;
    }
  }
}


bool Board2::validateMove(const Move2 & move) const
{
  if(drawState() || matState())
    return false;

  if(!move) // null-move
    return true;

  const Field & ffrom = getField(move.from_);

  Figure::Color ocolor = Figure::otherColor(color_);

  // validate under check
  if(checkingNum_)
  {
#ifdef NDEBUG
    if(move.checkVerified_)
      return true;
#endif

    {
      if(Figure::TypeKing == ffrom.type())
      {
        X_ASSERT((move.from_&7)-(move.to_&7) > 1 || (move.from_&7)-(move.to_&7) < -1, "try to castle under check");

        return !isAttacked(ocolor, move.to_, move.from_);
      }
      else if(checkingNum_ > 1)
        return false;

      X_ASSERT((unsigned)checking_[0] >= NumOfFields, "invalid checking figure");

      // regular capture
      if(move.to_ == checking_[0])
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
      if(move.to_ == en_passant_ && Figure::TypePawn == ffrom.type())
      {
        int ep_pos = enpassantPos();
        if(ep_pos == checking_[0])
        {
          BitMask mask_all = fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack);
          mask_all |= set_mask_bit(move.to_);
          mask_all ^= set_mask_bit(move.from_);
          mask_all &= ~set_mask_bit(ep_pos);

          BitMask brq_mask = fmgr_.bishop_mask(ocolor) | fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor);

          int ki_pos = kingPos(color_);

          // through removed en-passant pawn's field
          if(discoveredCheck(ep_pos, ocolor, mask_all, brq_mask, ki_pos))
            return false;

          // through moved pawn's field
          return !discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos);
        }
      }

      // moving figure covers king
      {
        const Field & cfield = getField(checking_[0]);
        X_ASSERT(Figure::TypeKing == cfield.type(), "king is attacking king");
        X_ASSERT(!cfield, "king is attacked by non existing figure");

        // Pawn and Knight could be only removed to escape from check
        if(Figure::TypeKnight == cfield.type() || Figure::TypePawn == cfield.type())
          return false;

        int ki_pos = kingPos(color_);
        const BitMask & protect_king_msk = betweenMasks().between(ki_pos, checking_[0]);
        if((protect_king_msk & set_mask_bit(move.to_)) == 0)
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
  if(ffrom.type() == Figure::TypeKing)
  {
    // castling
    int d = move.to_ - move.from_;
    if((2 == d || -2 == d))
    {
      X_ASSERT(!castling(color_, d > 0 ? 0 : 1), "castling impossible");
      X_ASSERT(!verifyCastling(color_, d > 0 ? 0 : 1), "castling flag is invalid");

      // don't do castling under check
      X_ASSERT(checkingNum_ > 0, "can not castle under check");
      X_ASSERT(move.capture_, "can't capture while castling");
      X_ASSERT(!(move.from_ == 4 && color_ || move.from_ == 60 && !color_), "kings position is wrong");
      X_ASSERT(getField(move.to_), "king position after castle is occupied");

      d >>= 1;

      // verify if there is suitable rook for castling
      int rook_from = move.from_ + ((d>>1) ^ 3); //d < 0 ? move.from_ - 4 : move.from_ + 3

      X_ASSERT((rook_from != 0 && color_ && d < 0) || (rook_from != 7 && color_ && d > 0), "invalid castle rook position");
      X_ASSERT((rook_from != 56 && !color_ && d < 0) || (rook_from != 63 && !color_ && d > 0), "invalid castle rook position");

      int rook_to = move.from_ + d;

      X_ASSERT(getField(rook_to), "field, that rook is going to move to, is occupied");

      X_ASSERT(!(rook_from & 3) && getField(rook_from+1), "long castling impossible");

      if(isAttacked(ocolor, move.to_) || isAttacked(ocolor, rook_to))
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

    if(discoveredCheck(move.from_, ocolor, mask_all, brq_mask, ki_pos))
      return false;

    // en-passant check
    if(move.to_ == en_passant_ && Figure::TypePawn == ffrom.type())
    {
      int ep_pos = enpassantPos();
      BitMask mask_all_ep = (mask_all ^ set_mask_bit(move.from_)) | set_mask_bit(en_passant_);
      return !discoveredCheck(ep_pos, ocolor, mask_all_ep, brq_mask, ki_pos);
    }

    return true;
  }
}

void Board2::makeMove(const Move2 & mv)
{
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

  // store masks - don't undo it to save time
  undo.mask_[0] = fmgr_.mask(Figure::ColorBlack);
  undo.mask_[1] = fmgr_.mask(Figure::ColorWhite);

  Field & ffrom = getField(undo.from_);
  Field & fto = getField(undo.to_);

  // castle
  bool castle_k = castling(color, 0);
  bool castle_q = castling(color, 1);
  bool castle_kk = castle_k, castle_qq = castle_q;
  static int castle_rook_pos[2][2] = { { 63, 7 }, { 56, 0 } }; // 0 - short, 1 - long

  if(ffrom.type() == Figure::TypeKing)
  {
    int d = undo.to_ - undo.from_;
    if((2 == d || -2 == d))
    {
      undo.castle_ = true;

      // don't do castling under check
      X_ASSERT(checkingNum_ > 0, "can not castle under check");

      d >>= 1;
      int rook_to = undo.from_ + d;
      int rook_from = undo.from_ + ((d>>1) ^ 3);//d < 0 ? undo.from_ - 4 : undo.from_ + 3

      Field & rf_field = getField(rook_from);
      X_ASSERT(rf_field.type() != Figure::TypeRook || rf_field.color() != color_, "no rook for castle");
      X_ASSERT(!(rook_from & 3) && getField(rook_from+1), "long castle is impossible");

      rf_field.clear();
      fmgr_.move(color_, Figure::TypeRook, rook_from, rook_to);
      Field & field_rook_to = getField(rook_to);
      X_ASSERT(field_rook_to, "field that rook is going to undo to while castling is occupied");
      field_rook_to.set(color_, Figure::TypeRook);
    }

    castle_kk = false;
    castle_qq = false;
  }
  // castling of current color is still possible
  else if((castle_k || castle_q) && (ffrom.type() == Figure::TypeRook))
  {
    // short castle
    castle_kk = undo.from_ != castle_rook_pos[0][color];

    // long castle
    castle_qq = undo.from_ != castle_rook_pos[1][color];
  }

  // eat rook of another side
  if(castling() && fto.type() == Figure::TypeRook)
  {
    bool ocastle_k = castling(ocolor, 0);
    bool ocastle_q = castling(ocolor, 1);

    if(ocastle_k && undo.to_ == castle_rook_pos[0][ocolor])
    {
      clear_castling(ocolor, 0);
      fmgr_.hashCastling(ocolor, 0);
    }
    else if(ocastle_q && undo.to_ == castle_rook_pos[1][ocolor])
    {
      clear_castling(ocolor, 1);
      fmgr_.hashCastling(ocolor, 1);
    }
  }

  // hash castle and change flags
  if(castle_k && !castle_kk)
  {
    clear_castling(color_, 0);
    fmgr_.hashCastling(color_, 0);
  }

  if(castle_q && !castle_qq)
  {
    clear_castling(color_, 1);
    fmgr_.hashCastling(color_, 1);
  }

  // remove captured figure
  if(fto)
  {
    X_ASSERT(fto.color() == color_, "invalid color of captured figure");

    undo.eaten_type_ = fto.type();
    fmgr_.decr(fto.color(), fto.type(), undo.to_);
    fto.clear();
    X_ASSERT(!undo.capture_, "capture flag isn't set");
  }
  else if(undo.to_ == en_passant_ && Figure::TypePawn == ffrom.type())
  {
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    X_ASSERT(epfield.color() != ocolor || epfield.type() != Figure::TypePawn, "en-passant pawn is invalid");
    fmgr_.decr(epfield.color(), epfield.type(), ep_pos);
    epfield.clear();
    X_ASSERT(!undo.capture_, "en-passant isn't detected as capture");
  }
  else
  {
    X_ASSERT(undo.capture_, "capture flag set but no figure to eat");
  }

  // clear en-passant hash code
  if(en_passant_ >= 0)
    fmgr_.hashEnPassant(en_passant_, ocolor);

  // save en-passant field
  undo.en_passant_ = en_passant_;
  en_passant_ = -1;


  // en-passant
  if(Figure::TypePawn == ffrom.type())
  {
    int dir = figureDir().dir(ffrom.type(), ffrom.color(), undo.from_, undo.to_);
    if(3 == dir)
    {
      en_passant_ = (undo.to_ + undo.from_) >> 1;
      fmgr_.hashEnPassant(en_passant_, color_);
    }
  }

  // undo figure 'from' -> 'to'
  if(undo.new_type_ > 0)
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
  undo.fifty_moves_ = fiftyMovesCount_;
  undo.reps_counter_ = repsCounter_;

  if(Figure::TypePawn == fto.type() || undo.capture_ || undo.new_type_ > 0)
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

  X_ASSERT(isAttacked(Figure::otherColor(color_), kingPos(color_)) && !underCheck(), "check isn't detected");
  X_ASSERT(!isAttacked(Figure::otherColor(color_), kingPos(color_)) && underCheck(), "detected check, that doesn't exist");
  X_ASSERT(isAttacked(color_, kingPos(Figure::otherColor(color_))), "our king is under check after undo");
}

void Board2::unmakeMove()
{
  X_ASSERT(halfmovesCounter_ <= 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  UndoInfo & undo = lastUndo();

  if(!undo)
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
  if(undo.new_type_ > 0)
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
  if(en_passant_ == undo.to_ && Figure::TypePawn == ffrom.type())
  {
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    epfield.set(ocolor, Figure::TypePawn);
    fmgr_.u_incr(epfield.color(), epfield.type(), ep_pos);
  }

  // restore eaten figure
  if(undo.eaten_type_ > 0)
  {
    fto.set(ocolor, (Figure::Type)undo.eaten_type_);
    fmgr_.u_incr(fto.color(), fto.type(), undo.to_);
  }

  // restore rook after castling
  if(ffrom.type() == Figure::TypeKing)
  {
    int d = undo.to_ - undo.from_;
    if((2 == d || -2 == d))
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

  // restore figures masks
  fmgr_.restoreMasks(undo.mask_);

  fiftyMovesCount_ = undo.fifty_moves_;
  repsCounter_ = undo.reps_counter_;

  // restore hash code and state
  state_ = (State)undo.old_state_;
  fmgr_.restoreHash(undo.zcode_old_);
  fmgr_.restorePawnCode(undo.zcode_pawn_);
}


} // NEngine