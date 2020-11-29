/*************************************************************
  Board.h - Copyright (C) 2016 by Dmitry Sultanov
  *************************************************************/
#pragma once

#include <Move.h>
#include <Field.h>
#include <MovesTable.h>
#include <FigureDirs.h>
#include <globals.h>
#include <magicbb.h>
#include <History.h>

namespace NEngine
{

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

  void clear()
  {
    mask_ = 0;
  }
};

struct UndoInfo
{
  enum MoveFlags
  {
    None = 0,
    Castle = 1,
    Capture = 2,
    Irreversible = 4,
    Check = 8,
    EnPassant = 16,
    Reduced = 32,
    Threat = 64,
    Nullmove = 128
  };

  uint64        zcode_;
  uint64        zcode_kpw_;
  uint64        zcode_fgrs_;
  BoardSaveData data_;
  Move          move_;
  uint8         mflags_;
  int8          eaten_type_;
  int32         psq32_;

  int8 enpassant() const { return data_.en_passant_; }

  bool castle() const { return (mflags_ & Castle) != 0; }
  bool capture() const { return (mflags_ & Capture) != 0; }
  bool irreversible() const { return (mflags_ & Irreversible) != 0; }
  bool check() const { return (mflags_ & Check) != 0; }
  bool is_enpassant() const { return (mflags_ & EnPassant) != 0; }
  bool is_reduced() const { return (mflags_ & Reduced) != 0; }
  bool threat() const { return (mflags_ & Threat) != 0; }
  bool is_nullmove() const { return (mflags_ & Nullmove) != 0; }

  void clear()
  {
    zcode_ = 0;
    zcode_kpw_ = 0;
    zcode_fgrs_ = 0;
    data_.clear();
    move_ = Move{ true };
    mflags_ = 0;
    eaten_type_ = 0;
    psq32_ = 0;
  }
};

struct Board
{
  enum { MovesMax = 256, FENsize = 8192, GameLength = 4096 };

  /// verification of move/unmove methods; use it for debug only
  bool operator != (const Board & other) const
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
  
  bool initEmpty(Figure::Color color)
  {
    clear();
    setColor(color);
    return true;
  }

  /// quick verification of move's possibility
  inline bool validateMoveExpress(const Move& mv) const
  {
    const auto from = mv.from();
    const auto& ffrom = getField(from);
    const auto to = mv.to();
    const auto& fto = getField(to);
    return (ffrom.color() == color()) && (movesTable().figure_moves(color(), ffrom.type(), from) & set_mask_bit(to)) && (!fto.type() || (fto.color() != color()));
  }

  /// is move meet rules. if under check verifies only discovered checks to my king
  bool validateMove(const Move & mv) const;

  /// slow, but verify all the conditions
  bool validateMoveBruteforce(const Move & mv) const;

  /// make move
  void makeMove(const Move&);
  void unmakeMove(const Move&);

  /// hash code for prefetch
  inline uint64 hashAfterMove(const Move & move) const
  {
    uint64 zcode = fmgr_.hashCode();

    const Field & ffrom = getField(move.from());
    const Field & fto = getField(move.to());

    // clear en-passant hash code
    if (enpassant() > 0)
      zcode ^= FiguresManager::enpassantCode(enpassant(), Figure::otherColor(color()));

    // remove captured figure
    if (fto)
    {
      const BitMask & uc = FiguresManager::code(fto.color(), fto.type(), move.to());
      zcode ^= uc;
    }

    // 'from' -> 'to'
    if (move.new_type() == 0) {
      // usual movement
      const BitMask & uc0 = FiguresManager::code(ffrom.color(), ffrom.type(), move.from());
      const BitMask & uc1 = FiguresManager::code(ffrom.color(), ffrom.type(), move.to());
      zcode ^= uc0;
      zcode ^= uc1;
    }
    else {
      // pawn promotion
      const BitMask & uc0 = FiguresManager::code(ffrom.color(), ffrom.type(), move.from());
      const BitMask & uc1 = FiguresManager::code(color(), (Figure::Type)move.new_type(), move.to());
      zcode ^= uc0;
      zcode ^= uc1;
    }

    // add hash color key
    zcode ^= FiguresManager::colorCode();
    return zcode;
  }


  /// null-move
  void makeNullMove();
  void unmakeNullMove();

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

  inline bool isFigure(int n, Figure::Color color, Figure::Type type) const
  {
    const auto& field = getField(n);
    return field.type() == type && field.color() == color;
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
  const UndoInfo & undoInfo(int i) const
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(i < 0 || i >= GameLength, "there was no move");
    return g_undoStack[i];
  }

  UndoInfo & undoInfo(int i)
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(i < 0 || i >= GameLength, "there was no move");
    return g_undoStack[i];
  }

  const UndoInfo & lastUndo() const
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(halfmovesCounter_ <= 0, "invalid halfmovesCounter");
    return g_undoStack[halfmovesCounter_-1];
  }

  const UndoInfo & reverseUndo(int i) const
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(halfmovesCounter_ <= 0, "invalid halfmovesCounter");
    X_ASSERT(i < 0 || i >= halfmovesCounter_, "invalid reverse undo index");
    return g_undoStack[halfmovesCounter_-1-i];
  }

  UndoInfo & lastUndo()
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

  inline StateType state() const { return data_.state_; }

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
  void setColor(Figure::Color c) { data_.color_ = c; }

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
  inline int8 const& kingPos(Figure::Color c) const
  {
    X_ASSERT(_lsb64(fmgr_.king_mask(c)) != king_pos_[c], "invalid king position");
    X_ASSERT(getField(king_pos_[c]).type() != Figure::TypeKing || getField(king_pos_[c]).color() != c, "no king on proper field");
    return king_pos_[c];
  }

  bool invalidate();
  void verifyState();
  bool isDrawMaterial() const;
  void verifyChessDraw(bool irreversibleLast);
  void findCheckingFigures(Figure::Color ocolor, int ki_pos);

  int halfmovesCount() const { return halfmovesCounter_; }
  int movesCount() const { return movesCounter_; }

  inline bool hasReps() const
  {
    X_ASSERT(data_.repsCounter_ < countReps(2, fmgr_.hashCode()), "invalid number of repetitions");
    return data_.repsCounter_ > 1;
  }

  inline bool is_capture(Move const& move) const
  {
    X_ASSERT(!move || getField(move.to()) && getField(move.to()).color() == color(), "invalid move given");
    return getField(move.to())
      || (getField(move.from()).type() == Figure::TypePawn && enpassant() > 0 && move.to() == enpassant());
  }

  // after given move
  bool hasReps(const Move & move) const;

  /// with given zcode
  int countReps(int from, const BitMask & zcode) const
  {
    int reps = 1;
    int i = halfmovesCounter_ - from;
    int stop = halfmovesCounter_ - data_.fiftyMovesCount_; // TODO: is it correct?
    X_ASSERT(stop < 0, "stop in countReps is invalid");
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
    auto tail_mask = betweenMasks().tail(ki_pos, pos);
    if(dir == nst::bishop)
    {
      auto const bi_moves = magic_ns::bishop_moves(ki_pos, mask_all_t) & tail_mask;
      auto const& bi_mask = fmgr_.bishop_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      return (bi_moves & (bi_mask | q_mask)) != 0ULL;
    }
    X_ASSERT(dir != nst::rook, "invalid direction from point to point");
    auto const r_moves = magic_ns::rook_moves(ki_pos, mask_all_t) & tail_mask;
    auto const& r_mask = fmgr_.rook_mask(ocolor);
    auto const& q_mask = fmgr_.queen_mask(ocolor);
    return (r_moves & (r_mask | q_mask)) != 0ULL;
  }

  inline BitMask isPinned(int pos, BitMask const& mask_all, Figure::Color ocolor, int t_pos, nst::bishop_rook_dirs interestDir) const
  {
    auto dir = figureDir().br_dir(t_pos, pos);
    if (!dir)
      return 0ULL;
    auto mask_all_t = mask_all & ~set_mask_bit(pos);
    auto tail_mask = betweenMasks().tail(t_pos, pos);
    if (dir == nst::bishop && interestDir != nst::rook)
    {
      auto const bi_moves = magic_ns::bishop_moves(t_pos, mask_all_t) & tail_mask;
      auto const& bi_mask = fmgr_.bishop_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      return (bi_moves & (bi_mask | q_mask));
    }
    if (dir == nst::rook && interestDir != nst::bishop)
    {
      auto const r_moves = magic_ns::rook_moves(t_pos, mask_all_t) & tail_mask;
      auto const& r_mask = fmgr_.rook_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      return (r_moves & (r_mask | q_mask));
    }
    return 0ULL;
  }

  bool see(Move const& move, int threshold) const;

  bool possibleMove(const Move & move) const;
  bool escapeMove(const Move& move) const;

  // for validation only
  bool moveExists(const Move& move) const;
  bool hasMove() const;

  bool canBeReduced(Move const& move) const
  {
    auto const& hist = history(Figure::otherColor(color()), move.from(), move.to());
    auto const& undo = lastUndo();
    return  //((hist.good()<<2) <= hist.bad()) &&
      !(lastUndo().capture()
       || move.new_type() > 0
       || undo.castle()
       || underCheck());
  }

  inline bool allowNullMove() const
  {
    return fmgr_.pawns(color()) > 0
      || fmgr_.queens(color()) + fmgr_.rooks(color()) > 0
      || fmgr_.knights(color())+fmgr_.bishops(color()) > 1;
  }

  inline int nullMoveDepthMin() const
  {
    if(fmgr_.queens(color()) + fmgr_.rooks(color()) + fmgr_.knights(color())+fmgr_.bishops(color()) > 1)
      return NullMove_DepthMin;
    else
      return NullMove_DepthMin + ONE_PLY;
  }

  inline int nullMoveDepth(int depth, ScoreType betta) const
  {
    Figure::Color ocolor = Figure::otherColor(color());
    ScoreType score = (fmgr().weight(color()) - fmgr().weight(ocolor)).eval0();
    if(score > betta + (Figure::figureWeight_[Figure::TypePawn] << 2) && depth > 7*ONE_PLY)
    {
      return std::max(1, depth - NullMove_PlyReduce - ONE_PLY);
    }
    return std::max(1, depth - NullMove_PlyReduce);
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
  
  inline SortValueType sortValueOfCap(int from, int to, Figure::Type new_type) const
  {
    const auto& fto   = getField(to);
    const auto& ffrom = getField(from);
    X_ASSERT(fto.type() != Figure::TypeNone && fto.color() != Figure::otherColor(color()), "invalid color of captured figure");
    X_ASSERT(!ffrom, "no moving figure");
    //// en-passant case
    //if(ffrom.type() == Figure::TypePawn && to > 0 && to == enpassant())
    //{
    //  X_ASSERT(fto.type() != Figure::TypeNone, "en-passant field is occupied");
    //  X_ASSERT(getField(enpassantPos()).type() != Figure::TypePawn ||
    //           getField(enpassantPos()).color() == color(), "no en-passant pawn");
    //  return Figure::figureWeight_[Figure::TypePawn] - Figure::TypePawn;
    //}
    // MVV LVA
    SortValueType sort_value = Figure::figureWeight_[fto.type()] -ffrom.type();
    //// + promotion
    //if (new_type)
    //  sort_value += PromotionBonus;
    // at first we try to eat recently moved opponent's figure
    //if(halfmovesCount() > 0 && lastUndo().move_.to() == to)
    //  sort_value += CaptureRecentlyBonus;
    return sort_value;
  }

  /// is pt attacked by figure in position 'p'
  bool ptAttackedBy(int8 pt, int p) const;

  /// mask_all is completely prepared, all figures are on their places
  bool findDiscovered(int from, Figure::Color acolor, const BitMask & mask_all, const BitMask & brq_mask, int ki_pos) const;

  /// is 'pt' attacked by figure of color 'acolor' if we remove figure from position 'from'
  /// returns 'false' if 'pt' was already attacked from this direction
  /// even if it is attacked by figure that occupies field 'from'
  bool ptAttackedFrom(Figure::Color acolor, int8 pt, int8 from) const;

  bool isDangerPawn(Move & move) const;


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

  inline int findDiscoveredPt(int from, int to, BitMask const& mask_all, Figure::Color ocolor, int ki_pos) const
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

  void detectCheck(Move const& move);

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

  // data members
private:
  BoardSaveData data_{};
  /// holds number of figures of each color, hash key, masks etc.
  FiguresManager fmgr_;

  int32 halfmovesCounter_{};
  int32 movesCounter_{};

  /// fields array 8 x 8
  Field fields_[NumOfFields];

  int8  king_pos_[2] = {};

protected:
  UndoInfo* g_undoStack{};

#ifdef RELEASEDEBUGINFO
public:
  std::vector<std::string> stestMoves_;
  int stestDepthMin_{};
  int stestDepthMax_{};
  uint64_t stestHashKey_{};
#endif
};

#pragma pack (pop)

template <class B, class U, int STACK_SIZE>
class SBoard : public B
{
public:
  SBoard() :
    B(),
    undoStackIntr_(STACK_SIZE)
  {
    B::g_undoStack = undoStackIntr_.data();
  }

  SBoard(SBoard&& oboard) :
    B(std::move(oboard)),
    undoStackIntr_(std::move(oboard.undoStackIntr_))
  {
    B::g_undoStack = undoStackIntr_.data();
  }

  template <int OTHER_SIZE>
  SBoard(SBoard<B, U, OTHER_SIZE>&& oboard) :
    B(std::move(oboard)),
    undoStackIntr_(STACK_SIZE)
  {
    B::g_undoStack = undoStackIntr_.data();
  }

  SBoard(SBoard const& oboard) :
    B(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    B::g_undoStack = undoStackIntr_.data();
  }


  template <int OTHER_SIZE>
  SBoard(SBoard<B, U, OTHER_SIZE> const& oboard) :
    B(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    B::g_undoStack = undoStackIntr_.data();
  }

  template <int OTHER_SIZE>
  SBoard(SBoard<B, U, OTHER_SIZE> const& oboard, bool) :
    B(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    B::g_undoStack = undoStackIntr_.data();
    copyStack(oboard);
  }

  SBoard(Board const& oboard) :
    B(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    B::g_undoStack = undoStackIntr_.data();
  }

  template <int OTHER_SIZE>
  SBoard& operator = (SBoard<B, U, OTHER_SIZE>&& oboard)
  {
    this->B::operator = (oboard);
    B::g_undoStack = undoStackIntr_.data();
    return *this;
  }

  SBoard& operator = (B const& oboard)
  {
    this->B::operator = (oboard);
    B::g_undoStack = undoStackIntr_.data();
    return *this;
  }

  SBoard(B const& oboard, bool) :
    B(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    B::g_undoStack = undoStackIntr_.data();
    copyStack(oboard);
  }

  void clearStack()
  {
    for (auto& u : undoStackIntr_) {
      u.clear();
    }
  }
private:
  void copyStack(B const& oboard)
  {
    for(int i = 0; i < B::halfmovesCount() && i < undoStackIntr_.size(); ++i)
      B::g_undoStack[i] = oboard.undoInfo(i);
  }

  std::vector<U> undoStackIntr_;
};

bool couldIntercept(Board const& board,
  BitMask const& inv_mask_all,
  BitMask const& attack_mask_c,
  int8 color,
  int pawn_pos,
  int promo_pos,
  int stepsLimit);

} // NEngine
