#pragma once

#include <Board.h>
#include <FigureDirs.h>

namespace NEngine
{

extern int64 x_movesCounter;
using StateType = uint16;

#pragma pack (push, 1)

enum State
{
  Invalid = 0,
  Ok = 1,
  UnderCheck = 2,
  Stalemat = 4,
  DrawReps = 8,
  DrawInsuf = 16,
  Draw50Moves = 64,
  ChessMat = 128,
  DoubleCheck = 256
};

struct BoardSaveData
{
  union
  {
  struct
  {
  int8  en_passant_;
  uint8 repsCounter_;
  uint8 checking_;
  uint8 fiftyMovesCount_;

  // castling possibility flag
  // (0, 1) bits block for black
  // (2, 3) bits block for white
  // lower bit in block is short (K)
  // higher bit in block is long (Q)
  uint8 castling_;

  StateType state_;

  /// color to make move from this position
  Figure::Color color_;
  };
  uint64  mask_;
  };
};

struct UndoInfo2
{
  enum MoveFlags
  {
    None          = 0,
    Castle        = 1,
    Capture       = 2,
    Irreversible  = 4,
    Check         = 8,
    EnPassant     = 16
  };

  uint64        zcode_;
  uint64        zcode_pw_;
  BoardSaveData data_;
  uint8         mflags_;
  int8          eaten_type_;
  int8          dummy_[6];

  bool irreversible() const { return mflags_ & Irreversible; }
  bool capture() const { return mflags_ & Capture; }
  bool castle() const { return mflags_ & Castle; }
  bool is_enpassant() const { return mflags_ & EnPassant; }
  int8 enpassant() const { return data_.en_passant_; }
};

struct Move2
{
  union
  {
  struct
  {
  uint16  from : 6,
          to   : 6,
          new_type : 4;
  uint16  sort_value;
  };
  uint32  mask;
  };

  Move2() {}

  Move2(int f, int t, int n) :
    mask(0)
  {
    from      = f;
    to        = t;
    new_type  = n;
  }

  inline bool operator < (Move2 const& m) const
  {
    return sort_value < m.sort_value;
  }

  inline bool operator == (Move2 const& m) const
  {
    return from == m.from && to == m.to && new_type == m.new_type;
  }
};

struct Board2
{
  enum { NumOfFields = 64, MovesMax = 256, FENsize = 8192, GameLength = 4096 };

  /// verification of move/unmove methods; use it for debug only
  bool operator != (const Board2 & other) const
  {
    const char * buf0 = reinterpret_cast<const char*>(this);
    const char * buf1 = reinterpret_cast<const char*>(&other);
    for(int i = 0; i < sizeof(*this); ++i)
    {
      if(buf0[i] != buf1[i])
        return true;
    }
    return false;
  }

  void clear();

  /// is move meet rules. if under check verifies only discovered checks to my king
  bool validateMove(const Move2 & mv) const;

  /// slow, but verify all the conditions
  bool validateMoveBruteforce(const Move2 & mv) const;

  /// make move
  void makeMove(const Move2&);
  void unmakeMove(const Move2&);

  FiguresManager const& fmgr() const { return fmgr_; }

  /// always use this method to get field
  inline const Field & getField(int index) const
  {
    X_ASSERT((unsigned)index > 63, "field index is invalid");
    return fields_[index];
  }

  /// always use this method to get field
  inline Field & getField(int index)
  {
    X_ASSERT((unsigned)index > 63, "field index is invalid");
    return fields_[index];
  }

  /// add new figure
  bool addFigure(const Figure::Color color, const Figure::Type type, int pos);

  /// position, where capturing pawn goes to
  inline int enpassant() const
  {
    return data_.en_passant_;
  }

  /// position of pawn, to be captured by en-passant
  inline int enpassantPos() const
  {
    X_ASSERT(data_.en_passant_ <= 0, "try to take en-passant pawn position but have no one");
    // if current color is black, color of en-passant pawn is white and vise versa
    static int pw_delta[2] = { 8, -8 };
    return data_.en_passant_ + pw_delta[color()];
  }

  /// get i-th move from begin
  const UndoInfo2 & undoInfo(int i) const
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(i < 0 || i >= GameLength, "there was no move");
    return g_undoStack[i];
  }

  UndoInfo2 & undoInfo(int i)
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(i < 0 || i >= GameLength, "there was no move");
    return g_undoStack[i];
  }

  const UndoInfo2 & lastUndo() const
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(halfmovesCounter_ <= 0, "invalid halfmovesCounter");
    return g_undoStack[halfmovesCounter_-1];
  }

  UndoInfo2 & lastUndo()
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(halfmovesCounter_ <= 0, "invalid halfmovesCounter");
    return g_undoStack[halfmovesCounter_-1];
  }

  /// just a useful method to quickly check if there is a draw
  static bool isDraw(uint16 state)
  {
    return (state & (State::Stalemat | State::DrawReps | State::DrawInsuf | State::Draw50Moves)) != State::Invalid;
  }

  inline bool matState() const { return (data_.state_ & State::ChessMat) != 0; }
  inline bool drawState() const { return isDraw(data_.state_); }

  bool underCheck() const { return (data_.state_ & State::UnderCheck) != 0; }
  bool doubleCheck() const
  {
    X_ASSERT(!underCheck(), "not under check but double check");
    return (data_.state_ & State::DoubleCheck) != 0;
  }
  // 1st checking figure position
  uint8 const& checking() const
  {
    X_ASSERT(!underCheck(), "get checking figure but not under check");
    return data_.checking_;
  }

  /// returns current move color
  Figure::Color color() const { return data_.color_; }
 
  // initialization methods
  void setColor(Figure::Color c) { data_.color_ = c;  }

  void hashColor() { fmgr_.hashColor(); }

  bool verifyCastling(const Figure::Color, int t) const;
  bool castling() const { return data_.castling_ != 0; }

  // white
  bool castling_K() const { return (data_.castling_ >> 2) & 1; }
  bool castling_Q() const { return (data_.castling_ >> 3) & 1; }

  // black
  bool castling_k() const { return data_.castling_ & 1; }
  bool castling_q() const { return (data_.castling_ >> 1) & 1; }

  bool castling(Figure::Color c, int t /* 0 - short (K), 1 - long (Q) */) const
  {
    int offset = ((c<<1) | t);
    return (data_.castling_ >> offset) & 1;
  }

  bool castling(Figure::Color c) const
  {
    int offset = c<<1;
    return ((data_.castling_ >> offset) & 3) != 0;
  }

  /// set short/long castle
  void set_castling(Figure::Color c, int t)
  {
    if(!isCastlePossible(c, t))
      return;
    int offset = ((c<<1) | t);
    data_.castling_ |= set_bit(offset);
    fmgr_.hashCastling(c, t);
  }

  void clear_castling(Figure::Color c, int t)
  {
    int offset = ((c<<1) | t);
    data_.castling_ &= ~set_bit(offset);
  }

  void setEnpassant(int ep, Figure::Color c)
  {
    data_.en_passant_ = ep;
    fmgr_.hashEnPassant(ep, c);
  }

  int fiftyMovesCount() const { return data_.fiftyMovesCount_; }
  void setFiftyMovesCount(int c) { data_.fiftyMovesCount_ = c; }

  int movesCounter() const { return movesCounter_; }
  void setMovesCounter(int c);

  /// find king's position
  inline int kingPos(Figure::Color c) const
  {
    return _lsb64(fmgr_.king_mask(c));
  }

  bool invalidate();
  void verifyState();
  void verifyChessDraw();
  void findCheckingFigures(Figure::Color ocolor, int ki_pos);

  inline bool hasReps() const
  {
    X_ASSERT(data_.repsCounter_ < countReps(2, fmgr_.hashCode()), "invalid number of repetitions");
    return data_.repsCounter_ > 1;
  }

  /// with given zcode
  int countReps(int from, const BitMask & zcode) const
  {
    int reps = 1;
    int i = halfmovesCounter_ - from;
    int stop = halfmovesCounter_ - data_.fiftyMovesCount_; // TODO: is it correct?
    for(; i >= stop && reps < 3; i -= 2)
    {
      if(undoInfo(i).zcode_ == zcode)
        reps++;
      if(undoInfo(i).irreversible())
        break;
    }
    // may be we forget to test initial position?
    if(reps < 3 && halfmovesCounter_ > 0 && i == -1 && zcode == undoInfo(0).zcode_)
      reps++;
    return reps;
  }

  // is there some figure between 'from' and 'to'
  // inv_mask - inverted mask of all interesting figures
  inline bool is_something_between(int from, int to, const BitMask & inv_mask) const
  {
    const BitMask & btw_msk = betweenMasks().between(from, to);
    return (btw_msk & inv_mask) != btw_msk;
  }

  // is there nothing between 'from' and 'to'
  // inv_mask - inverted mask of all interesting figures
  inline bool is_nothing_between(int from, int to, const BitMask & inv_mask) const
  {
    const BitMask & btw_msk = betweenMasks().between(from, to);
    return (btw_msk & inv_mask) == btw_msk;
  }

  inline bool discoveredCheck(int pos, BitMask const& mask_all, Figure::Color ocolor, int ki_pos) const
  {
    auto dir = figureDir().br_dir(ki_pos, pos);
    if(!dir)
      return false;
    auto through = set_mask_bit(pos);
    auto mask_all_t = mask_all & ~through;
    if(dir == nst::bishop)
    {
      auto const& bi_moves = magic_ns::bishop_moves(ki_pos, mask_all_t);
      auto const& bi_mask = fmgr_.bishop_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      return (bi_moves & through) && (bi_moves & (bi_mask | q_mask));
    }
    X_ASSERT(dir != nst::rook, "invalid direction from point to point");
    auto const& r_moves = magic_ns::rook_moves(ki_pos, mask_all_t);
    auto const& r_mask = fmgr_.rook_mask(ocolor);
    auto const& q_mask = fmgr_.queen_mask(ocolor);
    return ((r_moves & through) && (r_moves & (r_mask | q_mask)));
  }

  int see2(Move2 const& move) const;
  int see(Move2 const& move) const;
  
  // find 1st figure on the whole semi-line given by direction 'from' -> 'to'
  // mask gives all interesting figures
  inline int find_first_index(int from, int to, const BitMask & mask) const
  {
    BitMask mask_from = mask & betweenMasks().from(from, to);
    if(!mask_from)
      return -1;

    int index = from < to ? _lsb64(mask_from) : _msb64(mask_from);
    return index;
  }

  // detect discovered check to king of 'kc' color
  inline bool see_check(Figure::Color kc, uint8 from, uint8 ki_pos, const BitMask & all_mask_inv, const BitMask & a_brq_mask) const
  {
    // we need to verify if there is some attacker on line to king
    const BitMask & from_msk = betweenMasks().from(ki_pos, from);

    // no attachers at all
    if(!(a_brq_mask & from_msk))
      return false;

    // is there some figure between king and field that we move from
    BitMask all_mask_inv2 = (all_mask_inv | set_mask_bit(from));

    if(is_something_between(ki_pos, from, all_mask_inv2))
      return false;

    int index = find_first_index(ki_pos, from, ~all_mask_inv2);
    if(index < 0)
      return false;

    const Field & field = getField(index);

    // figure is the same color as king
    if(field.color() == kc)
      return false;

    // figure have to be in updated BRQ mask
    if(!(set_mask_bit(index) & a_brq_mask))
      return false;

    X_ASSERT(field.type() < Figure::TypeBishop || field.type() > Figure::TypeQueen, "see: not appropriate attacker type");

    // could figure attack king from it's position
    if(figureDir().dir(field.type(), field.color(), index, ki_pos) >= 0)
      return true;

    return false;
  }

private:
  // for initialization only
  bool isCastlePossible(Figure::Color c, int t) const;

  bool verifyCheckingFigure(int ch_pos, Figure::Color checking_color) const;

  inline bool discoveredCheckUsual(int from, int to, BitMask const& mask_all, Figure::Color ocolor, int ki_pos) const
  {
    auto dir = figureDir().br_dir(ki_pos, from);
    if(!dir)
      return false;
    auto through = set_mask_bit(from);
    auto exclude = set_mask_bit(to);
    auto mask_all_t = (mask_all & ~through) | exclude;
    exclude = ~exclude;
    if(dir == nst::bishop)
    {
      auto const& bi_moves = magic_ns::bishop_moves(ki_pos, mask_all_t);
      auto const& bi_mask = fmgr_.bishop_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      return (bi_moves & through) && (bi_moves & exclude & (bi_mask | q_mask));
    }
    X_ASSERT(dir != nst::rook, "invalid direction from point to point");
    auto const& r_moves = magic_ns::rook_moves(ki_pos, mask_all_t);
    auto const& r_mask = fmgr_.rook_mask(ocolor);
    auto const& q_mask = fmgr_.queen_mask(ocolor);
    return ((r_moves & through) && (r_moves & exclude & (r_mask | q_mask)));
  }

  inline bool discoveredCheckEp(BitMask const& through,
                                BitMask const& mask_all,
                                Figure::Color ocolor,
                                int ki_pos) const
  {
    auto const& bi_moves = magic_ns::bishop_moves(ki_pos, mask_all);
    auto const& bi_mask = fmgr_.bishop_mask(ocolor);
    auto const& q_mask = fmgr_.queen_mask(ocolor);
    if((bi_moves & through) && (bi_moves & (bi_mask | q_mask)))
      return true;
    auto const& r_moves = magic_ns::rook_moves(ki_pos, mask_all);
    auto const& r_mask = fmgr_.rook_mask(ocolor);
    if((r_moves & through) && (r_moves & (r_mask | q_mask)))
      return true;
    return false;
  }

  inline uint8 findDiscoveredPtEp(int pos, BitMask const& mask_all, Figure::Color ocolor, int ki_pos) const
  {
    auto dir = figureDir().br_dir(ki_pos, pos);
    if(!dir)
      return 64;
    if(dir == nst::bishop)
    {
      auto const& bi_moves = magic_ns::bishop_moves(ki_pos, mask_all);
      auto const& bi_mask = fmgr_.bishop_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      auto m = bi_moves & (bi_mask | q_mask);
      if(m)
      {
        X_ASSERT(m != set_mask_bit(_lsb64(m)), "more than 1 checking figure were discovered");
        return _lsb64(m);
      }
    }
    else
    {
      X_ASSERT(dir != nst::rook, "invalid direction from point to point");
      auto const& r_moves = magic_ns::rook_moves(ki_pos, mask_all);
      auto const& r_mask = fmgr_.rook_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      auto m = r_moves & (r_mask | q_mask);
      if(m)
      {
        X_ASSERT(m != set_mask_bit(_lsb64(m)), "more than 1 checking figure were discovered");
        return _lsb64(m);
      }
    }
    return 64;
  }

  inline uint8 findDiscoveredPt(int from, int to, BitMask const& mask_all, Figure::Color ocolor, int ki_pos) const
  {
    auto dir = figureDir().br_dir(ki_pos, from);
    if(!dir)
      return 64;
    auto exclude = ~set_mask_bit(to);
    if(dir == nst::bishop)
    {
      auto const& bi_moves = magic_ns::bishop_moves(ki_pos, mask_all);
      auto const& bi_mask = fmgr_.bishop_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      auto m = bi_moves & exclude & (bi_mask | q_mask);
      if(m)
      {
        X_ASSERT(m != set_mask_bit(_lsb64(m)), "more than 1 checking figure were discovered");
        return _lsb64(m);
      }
    }
    else
    {
      X_ASSERT(dir != nst::rook, "invalid direction from point to point");
      auto const& r_moves = magic_ns::rook_moves(ki_pos, mask_all);
      auto const& r_mask = fmgr_.rook_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      auto m = r_moves & exclude & (r_mask | q_mask);
      if(m)
      {
        X_ASSERT(m != set_mask_bit(_lsb64(m)), "more than 1 checking figure were discovered");
        return _lsb64(m);
      }
    }
    return 64;
  }

  inline bool isAttacked(int p, BitMask const& mask_all, Figure::Color ocolor) const
  {
    auto const& ki_caps = movesTable().caps(Figure::TypeKing, p);
    if(ki_caps & fmgr_.king_mask(ocolor))
      return true;
    auto const& pw_caps = movesTable().pawnCaps(color(), p);
    if(pw_caps & mask_all & fmgr_.pawn_mask(ocolor))
      return true;
    auto const& kn_caps = movesTable().caps(Figure::TypeKnight, p);
    if(kn_caps & mask_all & fmgr_.knight_mask(ocolor))
      return true;
    auto const& bi_moves = magic_ns::bishop_moves(p, mask_all);
    auto const& bi_mask = fmgr_.bishop_mask(ocolor);
    auto const& q_mask = fmgr_.queen_mask(ocolor);
    if(bi_moves & (bi_mask | q_mask))
      return true;
    auto const& r_moves = magic_ns::rook_moves(p, mask_all);
    auto const& r_mask = fmgr_.rook_mask(ocolor);
    if(r_moves & (r_mask | q_mask))
      return true;
    return false;
  }

  inline bool isAttacked(int p, BitMask const& mask_all, BitMask const& exclude, Figure::Color ocolor) const
  {
    auto const& ki_caps = movesTable().caps(Figure::TypeKing, p);
    if(ki_caps & fmgr_.king_mask(ocolor))
      return false;
    auto const& pw_caps = movesTable().pawnCaps(color(), p);
    if(pw_caps & mask_all & fmgr_.pawn_mask(ocolor) & exclude)
      return true;
    auto const& kn_caps = movesTable().caps(Figure::TypeKnight, p);
    if(kn_caps & mask_all & fmgr_.knight_mask(ocolor) & exclude)
      return true;
    auto const& bi_moves = magic_ns::bishop_moves(p, mask_all);
    auto const& bi_mask = fmgr_.bishop_mask(ocolor);
    auto const& q_mask = fmgr_.queen_mask(ocolor);
    if(bi_moves & (bi_mask | q_mask) & exclude)
      return true;
    auto const& r_moves = magic_ns::rook_moves(p, mask_all);
    auto const& r_mask = fmgr_.rook_mask(ocolor);
    if(r_moves & (r_mask | q_mask) & exclude)
      return true;
    return false;
  }

  void detectCheck(Move2 const& move);

  int findDiscoveredPt()
  {
    return 64;
  }

  inline bool isAttackedBy(Figure::Color color,
                           const Figure::Color acolor,
                           const Figure::Type type,
                           int from,
                           int king_pos,
                           BitMask const& mask_all) const
  {
    X_ASSERT(type != Figure::TypeBishop && type != Figure::TypeRook && type != Figure::TypeQueen, "");
    if(figureDir().dir(type, acolor, from, king_pos) < 0)
      return false;
    return is_nothing_between(from, king_pos, ~mask_all);
  }

  bool fieldAttacked(const Figure::Color c, int8 pos, const BitMask & mask_all_inv) const;
  
  /// is field 'pos' attacked by given color?
  bool isAttacked(const Figure::Color c, int8 pos) const
  {
    BitMask mask_all_inv = ~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite));
    return fieldAttacked(c, pos, mask_all_inv);
  }

  inline void setNoMoves()
  {
    if((State::Invalid == data_.state_) || (State::ChessMat & data_.state_) || drawState())
      return;
    if(underCheck())
      data_.state_ |= State::ChessMat;
    else
      data_.state_ |= State::Stalemat;
  }

// data members
private:
  BoardSaveData data_{};
  /// holds number of figures of each color, hash key, masks etc.
  FiguresManager fmgr_;

  int32 halfmovesCounter_{};
  int32 movesCounter_{};

  /// fields array 8 x 8
  Field fields_[NumOfFields];


protected:
  UndoInfo2* g_undoStack{};
};

#pragma pack (pop)

void xsearch(Board2& board, int depth);
bool xverifyMoves(Board2&);

std::string toFEN(Board2 const& board);
bool fromFEN(std::string const& i_fen, Board2& board);

} // NEngine