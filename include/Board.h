/*************************************************************
  Board.h - Copyright (C) 2016 by Dmitry Sultanov
  *************************************************************/
#pragma once

#include <Figure.h>
#include <Field.h>
#include <Move.h>
#include <MovesTable.h>
#include <FigureDirs.h>
#include <globals.h>
#include <magicbb.h>

namespace NEngine
{
  template <int>
  class MovesGeneratorBase;

  class MovesGenerator;
  class CapsGenerator;
  class EscapeGenerator;
  class ChecksGenerator;
  class UsualGenerator;
  class EscapeGeneratorLimited;
  class Player;
  class Evaluator;
  class EHashTable;

#if ( (defined VERIFY_CHECKS_GENERATOR) || (defined VERIFY_ESCAPE_GENERATOR) || (defined VERIFY_CAPS_GENERATOR) || (defined VERIFY_FAST_GENERATOR) )
  class Player;
#endif


/*! board representation
  */

class Board
{
  template <int>
  friend class MovesGeneratorBase;

  friend class MovesGenerator;
  friend class CapsGenerator;
  friend class EscapeGenerator;
  friend class ChecksGenerator;
  friend class UsualGenerator;
  friend class EscapeGeneratorLimited;
  friend class Player;
  friend class Evaluator;

  friend bool fromFEN(std::string const&, Board&);
  friend std::string toFEN(Board const&);

#if ( (defined VERIFY_CHECKS_GENERATOR) || (defined VERIFY_ESCAPE_GENERATOR) || (defined VERIFY_CAPS_GENERATOR) || (defined VERIFY_FAST_GENERATOR) )
  friend class Player;
#endif

public:
  enum State
  {
    Invalid = 0,
    Ok = 1,
    UnderCheck = 2,
    Stalemat = 4,
    DrawReps = 8,
    DrawInsuf = 16,
    Draw50Moves = 64,
    ChessMat = 128
  };

  /// constants
  enum { NumOfFields = 64, MovesMax = 256, FENsize = 8192, GameLength = 4096 };

  bool operator != (const Board &) const;

  /// c-tor
  Board();

  /// initialize empty board with given color to move
  bool initEmpty(Figure::Color);

  /*! movements
    */

  bool extractKiller(const Move & ki, const Move & hmove, Move & killer) const
  {
    if(!ki ||
      (hmove == ki) ||
      (getField(ki.to_) ||
      (en_passant_ == ki.to_ && getField(ki.from_).type() == Figure::TypePawn) ||
      ki.new_type_))
    {
      return false;
    }

    killer = ki;
    killer.clearFlags();
    killer.threat_ = 1; // to prevent LMR

    bool ok = possibleMove(killer);
    if(!ok)
      killer.clear();

    return ok;
  }

  /// unpack from hash, move have to be physically possible
  bool unpackMove(const PackedMove & pm, Move & move) const
  {
    move.clear();

    if(!pm)
      return false;

    move.from_ = pm.from_;
    move.to_ = pm.to_;
    move.new_type_ = pm.new_type_;

    if(getField(move.to_) || (en_passant_ == move.to_ && getField(move.from_).type() == Figure::TypePawn))
      move.capture_ = true;

    X_ASSERT(!possibleMove(move), "move in hash is impossible");

    return true;
  }

  /// put move to hash
  PackedMove packMove(const Move & move) const
  {
    PackedMove pm;
    if(!move)
      return pm;

    pm.from_ = move.from_;
    pm.to_ = move.to_;
    pm.new_type_ = move.new_type_;
    return pm;
  }

  /// used in LMR
  bool canBeReduced() const;

  /// don't allow LMR of strong pawn's moves
  bool isDangerPawn(Move & move) const;

  /// don't allow LMR of strong queen's moves - ie queen near opponent's king and king is immobile
  bool isDangerQueen(const Move & move) const;

  /// don't allow LMR of knight's fork
  bool isKnightFork(const Move & move) const;

  /// verify if it 's fork or double pawn attack after making move
  bool isKnightForkAfter(const Move & move) const;
  bool isDoublePawnAttack(const Move & move) const;
  bool isBishopAttack(const Move & move) const;

  /// don't allow LMR of strong moves
  bool isMoveThreat(Move & move) const
  {
    if(isDangerPawn(move))
      return true;

    //if ( (!move.seen_ || move.see_good_) && (isDangerQueen(move) || isKnightFork(move)) )
    //  {
    //	bool ok = move.see_good_ || (!move.seen_ && see(move) >= 0);
    //   move.seen_ = 1;
    //	move.see_good_ = ok;
    //	return ok;
    //  }

    return false;
  }

  //// becomes passed
  //bool pawnPassed(const UndoInfo & move) const
  //{
  //  const Field & fto = getField(move.to_);
  //  if(fto.type() != Figure::TypePawn)
  //    return false;

  //  X_ASSERT(color_ == fto.color(), "invalid color of passed pawn");

  //  Figure::Color ocolor = Figure::otherColor(color_);
  //  const uint64 & pmsk = fmgr_.pawn_mask_t(ocolor);
  //  const uint64 & opmsk = fmgr_.pawn_mask_t(color_);
  //  const uint64 & passmsk = pawnMasks().mask_passed(ocolor, move.to_);
  //  const uint64 & blckmsk = pawnMasks().mask_blocked(ocolor, move.to_);

  //  return !(opmsk & passmsk) && !(pmsk & blckmsk);
  //}

  /// is pt attacked by figure in position 'p'
  inline bool ptAttackedBy(int8 pt, int p) const
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

  /// get position of figure of color 'acolor', which attacks field 'pt'
  /// returns -1 if 'pt' was already attacked from this direction
  /// even if it is attacked by figure that occupies field 'from'
  inline int getAttackedFrom(Figure::Color acolor, int8 pt, int8 from) const
  {
    BitMask mask_all = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite);
    BitMask brq_mask = fmgr_.bishop_mask(acolor) | fmgr_.rook_mask(acolor) | fmgr_.queen_mask(acolor);
    const BitMask & btw_mask = betweenMasks().between(pt, from);
    brq_mask &= ~btw_mask; // exclude all figures, that are between 'pt' and 'from'

    return findDiscovered(from, acolor, mask_all, brq_mask, pt);
  }

  const FiguresManager & fmgr() const
  {
    return fmgr_;
  }

  inline bool allowNullMove() const
  {
    return can_win_[color_] &&
      (fmgr_.queens(color_) + fmgr_.rooks(color_) + fmgr_.knights(color_)+fmgr_.bishops(color_) > 0);
  }

  inline int nullMoveDepthMin() const
  {
    if(fmgr_.queens(color_) + fmgr_.rooks(color_) + fmgr_.knights(color_)+fmgr_.bishops(color_) > 1)
      return NullMove_DepthMin;
    else
      return NullMove_DepthMin + 2 * ONE_PLY;
  }

  inline int nullMoveReduce() const
  {
    if(fmgr_.queens(color_) + fmgr_.rooks(color_) + fmgr_.knights(color_)+fmgr_.bishops(color_) > 1)
      return NullMove_PlyReduce;
    else
      return NullMove_PlyReduce - ONE_PLY;
  }

  inline int nullMoveVerify() const
  {
    if(fmgr_.queens(color_) + fmgr_.rooks(color_) + fmgr_.knights(color_)+fmgr_.bishops(color_) > 1)
      return NullMove_PlyVerify;
    else
      return NullMove_PlyVerify - ONE_PLY;
  }

  inline int nullMoveDepth(int depth, ScoreType betta) const
  {
    Figure::Color ocolor = Figure::otherColor(color_);
    ScoreType score = fmgr().weight(color_) - fmgr().weight(ocolor);
    if(score < betta + (Figure::figureWeight_[Figure::TypePawn]<<1))
    {
      if(depth < 4*ONE_PLY)
        return 0;
      else if(depth < 5*ONE_PLY)
        return ONE_PLY;
      else if(depth < 6*ONE_PLY)
        return 2 * ONE_PLY;
    }

    int null_depth = depth - nullMoveReduce();
    if(null_depth < 0)
      null_depth = 0;

    return null_depth;
  }

  inline int nullMoveDepthVerify(int depth) const
  {
    int null_depth = depth - nullMoveVerify();
    if(null_depth >(depth>>1))
      null_depth = depth>>1;
    if(null_depth < 0)
      null_depth = 0;
    return null_depth;
  }

  inline bool shortNullMoveReduction() const
  {
    return fmgr_.weight(color_) <= Figure::figureWeight_[Figure::TypeRook]+Figure::figureWeight_[Figure::TypePawn];
  }

  inline bool limitedNullMoveReduction() const
  {
    return fmgr_.weight(color_)- fmgr_.pawns()*Figure::figureWeight_[Figure::TypePawn] <= Figure::figureWeight_[Figure::TypeQueen];
  }

  inline bool isWinnerLoser() const
  {
    return !can_win_[0] || !can_win_[1];
  }

  Figure::Color getWinnerColor() const
  {
    return can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  }

  bool isWinnerColor(Figure::Color color) const
  {
    return can_win_[color];
  }

  /// to detect draw by moves repetitons
  int countReps() const;

  /// with given zcode
  int countReps(int from, const BitMask & zcode) const
  {
    int reps = 1;
    int i = halfmovesCounter_ - from;
    for(; reps < 2 && i >= 0; i -= 2)
    {
      if(undoInfo(i).zcode_ == zcode)
        reps++;

      if(undoInfo(i).irreversible_)
        break;
    }

    // may be we forget to test initial position?
    if(reps < 2 && i == -1 && zcode == undoInfo(0).zcode_old_)
      reps++;

    return reps;
  }


  /// used for hashed moves
  int  calculateReps(const Move & move) const;

  /// is move physically possible
  bool possibleMove(const Move & mv) const;

  /// is move meet rules
  bool validateMove(const Move & mv) const;

#ifdef VALIDATE_VALIDATOR
  bool validateMove2(const Move & mv) const;
  bool validateValidator(const Move & mv);
#endif

  /// make move
  void makeMove(const Move &);
  void unmakeMove();

  /// null-move
  void makeNullMove();
  void unmakeNullMove();

  /// verify if there is draw or mat
  void verifyState();

  /// add new figure
  bool addFigure(const Figure::Color color, const Figure::Type type, int pos);

  /// verify position and calculate checking figures
  /// very slow. should be used only while initialization
  bool invalidate();

  /// position, where capturing pawn goes to
  inline int enpassant() const
  {
    return en_passant_;
  }

  /// position of pawn, to be captured by en-passant
  inline int enpassantPos() const
  {
    if(en_passant_ < 0)
      return -1;

    // if current color is black, color of en-passant pawn is white and vise versa
    static int pw_delta[2] = { 8, -8 };
    return en_passant_ + pw_delta[color_];
  }

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
    const Field & field = getField(n);
    return field.type() == type && field.color() == color;
  }

  /// returns number of moves. starts from 1
  int movesCount() const { return movesCounter_; }

  /// returns number of half-moves from beginning of game. starts from 0
  int halfmovesCount() const { return halfmovesCounter_; }

  /// returns max number of repetitions found. it will be set to 0 after pawns move or capture
  uint8 repsCount() const { return repsCounter_; }

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

  /// get i-th move from end
  /// '0' means the last one, '-1' means 1 before last
  UndoInfo & undoInfoRev(int i)
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(i > 0 || i <= -halfmovesCounter_, "attempt to get move before 1st or after last");
    return g_undoStack[halfmovesCounter_+i-1];
  }

  const UndoInfo & undoInfoRev(int i) const
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(i > 0 || i <= -halfmovesCounter_, "attempt to get move before 1st or after last");
    return g_undoStack[halfmovesCounter_+i-1];
  }

  const UndoInfo & lastUndo() const
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(halfmovesCounter_ <= 0, "invalid halfmovesCounter");
    return g_undoStack[halfmovesCounter_-1];
  }

  UndoInfo & lastUndo()
  {
    X_ASSERT(!g_undoStack, "board isn't initialized");
    X_ASSERT(halfmovesCounter_ <= 0, "invalid halfmovesCounter");
    return g_undoStack[halfmovesCounter_-1];
  }

  /// returns current move color
  Figure::Color getColor() const { return color_; }

  /// returns current state, ie check, mat etc
  uint8 getState() const { return state_; }

  /// returns true if we are under check
  bool underCheck() const { return (state_ & State::UnderCheck) != 0; }

  /// just a useful method to quickly check if there is a draw
  static bool isDraw(uint8 state)
  {
    return (state & (State::Stalemat | State::DrawReps | State::DrawInsuf | State::Draw50Moves)) != State::Invalid;
  }

  inline bool matState() const { return (state_ & State::ChessMat) != 0; }
  inline bool drawState() const { return isDraw(state_); }

  // draw or mat
  inline bool terminalState() const
  {
    return (state_ & (State::Stalemat | State::DrawReps | State::DrawInsuf | State::Draw50Moves | State::ChessMat)) != State::Invalid;
  }

  inline void setNoMoves()
  {
    if((State::Invalid == state_) || (State::ChessMat & state_) || drawState())
      return;
    if(underCheck())
      state_ |= State::ChessMat;
    else
      state_ |= State::Stalemat;
  }

  void verifyMasks() const;

  const uint64 & hashCode() const
  {
    return fmgr_.hashCode();
  }

  const uint64 & pawnCode() const
  {
    return fmgr_.pawnCode();
  }

  ScoreType material(Figure::Color color) const
  {
    return fmgr_.weight(color);
  }

  ScoreType material() const
  {
    ScoreType score = fmgr_.weight();
    if(!color_)
      score = -score;
    return score;
  }

  // will be 'pos' under check after removing figure from 'exclude' position
  bool isAttacked(const Figure::Color c, int pos, int exclude) const
  {
    BitMask mask_all_inv = ~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite));
    mask_all_inv |= set_mask_bit(exclude);
    return fieldAttacked(c, pos, mask_all_inv);
  }

  /// is field 'pos' attacked by given color?
  bool isAttacked(const Figure::Color c, int8 pos) const
  {
    BitMask mask_all_inv = ~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite));
    return fieldAttacked(c, pos, mask_all_inv);
  }

  /// static exchange evaluation, should be called before move
  int see_old(const Move & move) const;
  int see(const Move & move) const;
  //int see(const Move & move) const;

  //static int see_failed_;

  /// find king's position
  inline int kingPos(Figure::Color c) const
  {
    return _lsb64(fmgr_.king_mask(c));
  }

  /// methods

  /// clear board. reset all fields, number of moves etc...
  void clear();

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

  inline bool see_check_mbb(Figure::Color acolor, uint8 from, uint8 to, uint8 ki_pos, const BitMask & all_mask_inv) const
  {
    auto mask_from = set_mask_bit(from);
    auto mask_to = set_mask_bit(to);
    auto all_mask = ~(all_mask_inv | mask_from);
    all_mask |= mask_to;
    mask_to = ~mask_to;
    auto bq_mask = (fmgr_.bishop_mask(acolor) | fmgr_.queen_mask(acolor)) & all_mask & mask_to;
    auto bishop_attack = magic_ns::bishop_moves(ki_pos, all_mask);
    if(bishop_attack & bq_mask)
      return true;
    auto rq_mask = (fmgr_.rook_mask(acolor) | fmgr_.queen_mask(acolor)) & all_mask & mask_to;
    auto rook_attack = magic_ns::rook_moves(ki_pos, all_mask);
    return rook_attack & rq_mask;
  }
private:

  /// return short/long castle possibility
  bool castling() const { return castling_ != 0; }

  bool castling(Figure::Color c) const
  {
    int offset = c<<1;
    return ((castling_ >> offset) & 3) != 0;
  }

  bool castling(Figure::Color c, int t /* 0 - short (K), 1 - long (Q) */) const
  {
    int offset = ((c<<1) | t);
    return (castling_ >> offset) & 1;
  }

  // white
  bool castling_K() const { return (castling_ >> 2) & 1; }
  bool castling_Q() const { return (castling_ >> 3) & 1; }

  // black
  bool castling_k() const { return castling_ & 1; }
  bool castling_q() const { return (castling_ >> 1) & 1; }

  /// set short/long castle
  void set_castling(Figure::Color c, int t)
  {
    int offset = ((c<<1) | t);
    castling_ |= set_bit(offset);
  }

  void clear_castling(Figure::Color c, int t)
  {
    int offset = ((c<<1) | t);
    castling_ &= ~set_bit(offset);
  }

  bool verifyCastling(const Figure::Color, int t) const;

  bool verifyChessDraw();

  /// find all checking figures, save them into board
  void detectCheck(const UndoInfo & move);

  /// is king of given color attacked by given figure
  inline bool isAttackedBy(Figure::Color color, const Figure::Color acolor, const Figure::Type type, int from) const
  {
    int king_pos = kingPos(color);
    int dir = figureDir().dir(type, acolor, from, king_pos);
    if((dir < 0) || (Figure::TypeKing == type && dir > 7))
      return false;

    BitMask inv_mask_all = ~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite));
    return is_nothing_between(from, king_pos, inv_mask_all);
  }

  // is field 'pos' attacked by color 'c'. mask_all - all figures, mask is inverted
  bool fieldAttacked(const Figure::Color c, int8 pos, const BitMask & mask_all_inv) const;

  // returns number of checking figures.
  // very slow. used only for initial validation
  int findCheckingFigures(Figure::Color color, int ki_pos);

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
  
public:
  // check if there is some figure between 'from' and 'to'
  // inv_mask - inverted mask of all interesting figures
  inline bool is_something_between(int from, int to, const BitMask & inv_mask) const
  {
    const BitMask & btw_msk = betweenMasks().between(from, to);
    return (btw_msk & inv_mask) != btw_msk;
  }

  // check if there is nothing between 'from' and 'to'
  // inv_mask - inverted mask of all interesting figures
  inline bool is_nothing_between(int from, int to, const BitMask & inv_mask) const
  {
    const BitMask & btw_msk = betweenMasks().between(from, to);
    return (btw_msk & inv_mask) == btw_msk;
  }

  // acolor - color of attacking side, ki_pos - attacked king pos
  inline bool discoveredCheck(int pt, Figure::Color acolor, const BitMask & mask_all, const BitMask & brq_mask, int ki_pos) const
  {
    const BitMask & from_msk = betweenMasks().from(ki_pos, pt);
    BitMask mask_all_ex = mask_all & ~set_mask_bit(pt);
    mask_all_ex &= from_msk;
    if((mask_all_ex & brq_mask) == 0)
      return false;

    int apos = ki_pos < pt ? _lsb64(mask_all_ex) : _msb64(mask_all_ex);
    if((set_mask_bit(apos) & brq_mask) == 0) // no BRQ on this field
      return false;

    const Field & afield = getField(apos);
    X_ASSERT(afield.color() != acolor || afield.type() < Figure::TypeBishop || afield.type() > Figure::TypeQueen, "discoveredCheck() - attacking figure isn't BRQ");

    int dir = figureDir().dir(afield.type(), afield.color(), apos, ki_pos);
    return dir >= 0;
  }

  /// returns field index of checking figure or -1 if not found
  /// mask_all is completely prepared, all figures are on their places
  inline int findDiscovered(int from, Figure::Color acolor, const BitMask & mask_all, const BitMask & brq_mask, int ki_pos) const
  {
    const BitMask & from_msk = betweenMasks().from(ki_pos, from);
    BitMask mask_all_ex = mask_all & from_msk;
    if((mask_all_ex & brq_mask) == 0)
      return -1;

    int apos = ki_pos < from ? _lsb64(mask_all_ex) : _msb64(mask_all_ex);
    if((set_mask_bit(apos) & brq_mask) == 0) // no BRQ on this field
      return -1;

    const Field & afield = getField(apos);
    X_ASSERT(afield.color() != acolor || afield.type() < Figure::TypeBishop || afield.type() > Figure::TypeQueen, "findDiscovered() - attacking figure isn't BRQ")

      int dir = figureDir().dir(afield.type(), afield.color(), apos, ki_pos);
    if(dir < 0)
      return -1;

    return apos;
  }


  /// data
private:
  /// for chess draw detector
  bool can_win_[2];

  /// en-passant field index. must be cleared (set to -1) after move
  /// this is the field, where capturing pawn will go, it's written in FEN
  /// it has color different from "color_"
  int8  en_passant_{};

  /// current state, i.e check, draw, mat, invalid, etc.
  uint8 state_{ State::Invalid };

  /// color to make move from this position
  Figure::Color color_{};

  uint8 repsCounter_{};

  // castling possibility flag
  // (0, 1) bits block for black
  // (2, 3) bits block for white
  // lower bit in block is short (K)
  // higher bit in block is long (Q)
  uint8 castling_{};

  // check
  uint8 checkingNum_{};

  uint8 fiftyMovesCount_{};

  int32 halfmovesCounter_{};
  int32 movesCounter_{};

  union
  {
    uint8  checking_[2];
    uint16 checking_figs_;
  };

  /// fields array 8 x 8
  Field fields_[NumOfFields];

  /// holds number of figures of each color, hash key, masks etc.
  FiguresManager fmgr_;

protected:
  UndoInfo* g_undoStack{};
};

template <int STACK_SIZE>
class SBoard : public Board
{
public:
  SBoard() :
    Board(),
    undoStackIntr_(STACK_SIZE)
  {
    g_undoStack = undoStackIntr_.data();
  }

  template <int OTHER_SIZE>
  SBoard(SBoard<OTHER_SIZE> const& oboard) :
    Board(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    g_undoStack = undoStackIntr_.data();
  }

  template <int OTHER_SIZE>
  SBoard(SBoard<OTHER_SIZE> const& oboard, bool) :
    Board(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    g_undoStack = undoStackIntr_.data();
    copyStack(oboard);
  }

  SBoard(Board const& oboard) :
    Board(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    g_undoStack = undoStackIntr_.data();
  }

  SBoard& operator = (Board const& oboard)
  {
    this->Board::operator = (oboard);
    g_undoStack = undoStackIntr_.data();
    return *this;
  }

  SBoard(Board const& oboard, bool) :
    Board(oboard),
    undoStackIntr_(STACK_SIZE)
  {
    g_undoStack = undoStackIntr_.data();
    copyStack(oboard);
  }
private:
  void copyStack(Board const& oboard)
  {
    for(int i = 0; i < halfmovesCount() && i < undoStackIntr_.size(); ++i)
      g_undoStack[i] = oboard.undoInfo(i);
  }

  std::vector<UndoInfo> undoStackIntr_;
};

} // NEngine
