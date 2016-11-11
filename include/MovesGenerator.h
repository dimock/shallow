/*************************************************************
  MovesGenerator.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Board.h>

namespace NEngine
{

class Player;
class ChecksGenerator;

struct History
{
  History() : score_(0), good_count_(0), bad_count_(0) {}

  void clear()
  {
    score_ = 0;
    good_count_ = 0;
    bad_count_ = 0;
  }

  unsigned score() const
  {
    //return score_;//((unsigned long long)score_ * good_count_) / (bad_count_ + good_count_ + 1);//
    return mul_div(score_, good_count_, bad_count_);
  }

  void normalize(int n)
  {
    score_ >>= n;
    good_count_ >>= n;
    bad_count_ >>= n;
  }

  unsigned good() const
  {
    return good_count_;
  }

  unsigned bad() const
  {
    return bad_count_;
  }

  void inc_gb(bool g)
  {
    good_count_ +=  g;
    bad_count_  += !g;
  }

  void inc_good()
  {
    good_count_++;
  }

  void inc_bad()
  {
    bad_count_++;
  }

  void inc_score(int ds)
  {
    if ( ds < 1 )
      ds = 1;
    score_ += ds;
  }

protected:

  unsigned score_;
  unsigned good_count_, bad_count_;
};


// base class for all moves generators
class MovesGeneratorBase
{
  static History history_[Board::NumOfFields][Board::NumOfFields];

public:

  static void clear_history();
  static void normalize_history(int n);

  static const unsigned int vsort_add_ = 10000;

  static void save_history(std::string const& fname);
  static void load_history(std::string const& fname);


  static inline History & history(int from, int to)
  {
    X_ASSERT( (unsigned)from > 63 || (unsigned)to > 63, "invalid history field index" );
    return history_[from][to];
  }

  MovesGeneratorBase(const Board & board) : board_(board),
    numOfMoves_(0)
  {}

  operator bool () const
  {
    return numOfMoves_ > 0;
  }

  int count() const
  {
    return numOfMoves_;
  }

  Move * moves()
  {
    return moves_;
  }

  const Move & operator [] (int i) const
  {
    X_ASSERT( (unsigned)i >= (unsigned)numOfMoves_ || numOfMoves_ >= Board::MovesMax, "index of move is out of range" );
    return moves_[i];
  }

  bool find(const Move & m) const;
  bool has_duplicates() const;

protected:

  inline void adjust_vsort(Move & move, unsigned delta) const
  {
    move.vsort_ += delta;
    move.vsort_ |= 1; // to prevent zero value
  }

  inline void sortValueOfCap(Move & move)
  {
    const Field & fto = board_.getField(move.to_);
    const Field & ffrom = board_.getField(move.from_);

    Figure::Type vtype = fto.type();

    X_ASSERT(vtype != Figure::TypeNone && fto.color() != Figure::otherColor(board_.color_), "invalid color of captured figure");

    // en-passant case
    if ( vtype == Figure::TypeNone && move.to_ == board_.en_passant_ && ffrom.type() == Figure::TypePawn )
    {
      X_ASSERT( board_.getField(board_.enpassantPos()).type() != Figure::TypePawn ||
        board_.getField(board_.enpassantPos()).color() == board_.color_, "no en-passant pawn" );

      vtype = Figure::TypePawn;
      move.vsort_ = Figure::figureWeight_[vtype];
      adjust_vsort(move, vsort_add_);
      return;
    }

    int vsort = 0;

    // capture, prefer stronger opponent's figure
    if ( vtype != Figure::TypeNone )
    {
      Figure::Type atype = ffrom.type();
      vsort = (int)(Figure::figureWeight_[vtype] << 1) - (int)Figure::figureWeight_[atype];
    }

    // pawn promotion
    if ( move.new_type_ > Figure::TypeNone && move.new_type_ < Figure::TypeKing )
    {
      vsort += (int)Figure::figureWeight_[move.new_type_];
    }

    // at first we try to eat recently moved opponent's figure
    if ( move.capture_ && board_.halfmovesCount() > 1 )
    {
      const UndoInfo & prev = board_.undoInfoRev(-1);
      if ( prev.to_ == move.to_ )
        vsort += (int)(Figure::figureWeight_[vtype] >> 1);
    }

    vsort += vsort_add_;
    move.vsort_ = (unsigned)vsort;
    adjust_vsort(move, 0);
  }

  int numOfMoves_;
  const Board & board_;
  Move moves_[Board::MovesMax];
};

/// generate all movies from this position. don't verify and sort them. only calculate sort weights
class MovesGenerator : public MovesGeneratorBase
{
public:

  MovesGenerator(const Board & board, const Move & killer);
  MovesGenerator(const Board & );

  Move & move()
  {
    for ( ;; )
    {
      Move * move = moves_ + numOfMoves_;
      Move * mv = moves_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ < move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        return *move;

      if ( (move->capture_ || move->new_type_ > 0) && !move->seen_ )
      {
        move->seen_ = 1;

        int see_gain = 0;
        if ( (see_gain = board_.see(*move)) < 0 )
        {
          if ( move->capture_ )
            move->vsort_ = see_gain + 3000;
          else
            move->vsort_ = see_gain + 2000;

          continue;
        }

        //if ( see_gain >= 0 )
        //  move->recapture_ = 1;
      }

      move->alreadyDone_ = 1;
      return *move;
    }
  }

private:

  /// returns number of moves found
  int generate();

  inline void add(int & index, int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    Move & move = moves_[index++];
    move.set(from, to, new_type, capture);
    
    calculateSortValue(move);
  }

  void calculateSortValue(Move & move)
  {
    X_ASSERT( !board_.getField(move.from_), "no figure on field we move from" );

    if ( move.capture_ || move.new_type_ )
    {
      sortValueOfCap(move);
      adjust_vsort(move, 10000000);
      return;
    }
    else if ( move == killer_ )
    {
      adjust_vsort(move, 3000000);
      return;
    }

    const History & hist = history(move.from_, move.to_);
    move.vsort_ = hist.score();
    adjust_vsort(move, 10000);
  }

  Move killer_;
};

/// generate all moves but captures and promotion to queen. could generate only promotion to knight if it checks
/// couldn't be called under check !!!
class UsualGenerator : public MovesGeneratorBase
{
public:

  UsualGenerator(Board & );

  /// returns number of moves found
  int generate(const Move & hmove, const Move & killer);

  void restart();

  Move & move()
  {
    for ( ;; )
    {
      Move * move = moves_ + numOfMoves_;
      Move * mv = moves_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ <= move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        return *move;

      move->alreadyDone_ = 1;
      return *move;
    }
  }

private:

  inline bool add(int & index, int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    Move & move = moves_[index];
    move.set(from, to, new_type, capture);

    if ( move == hmove_ || move == killer_ )
      return false;

    index++;
    calculateSortValue(move);
    return true;
  }

  inline void calculateSortValue(Move & move)
  {
    X_ASSERT( !board_.getField(move.from_), "no figure on field we move from" );
    const History & hist = history(move.from_, move.to_);
    move.vsort_ = hist.score();
    adjust_vsort(move, 10);
  }

  Move hmove_, killer_;
};

/// generate captures and promotions to queen only, don't detect checks
class CapsGenerator : public MovesGeneratorBase
{
public:

  CapsGenerator(Board & );
  CapsGenerator(const Move & hcap, Board & );

  int generate(const Move & hcap, Figure::Type thresholdType);

  void restart();

  inline Move & capture()
  {
    for ( ;; )
    {
      Move * move = moves_ + numOfMoves_;
      Move * mv = moves_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ <= move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        return *move;

      move->alreadyDone_ = 1;

      if ( !filter(*move) )
        continue;

      return *move;
    }
  }

private:

  /// returns number of moves found
  int generate();

  inline void add(int & m, int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    X_ASSERT( !board_.getField(from), "no figure on field we move from" );
    
    Move & move = moves_[m];    
    move.set(from, to, new_type, capture);
    if ( move == hcap_ )
      return;

    sortValueOfCap(move);
    m++;
  }

  inline bool filter(Move & move) const
  {
    if ( move.capture_ && move.new_type_ )
      return true;

    bool check = expressCheck(move);

    if ( move.discoveredCheck_ )
      return true; // always good

    Figure::Type type = Figure::TypeNone;
    const Field & tfield = board_.getField(move.to_);
    if ( tfield )
      type = tfield.type();
    else if ( move.new_type_ )
      type = (Figure::Type)move.new_type_;
    else if ( board_.en_passant_ == move.to_ && board_.getField(move.from_).type() == Figure::TypePawn ) // en-passant capture
      type = Figure::TypePawn;

    if ( !check && type < thresholdType_ )
      return false;

    int s = board_.see(move);
		move.seen_ = 1;
		move.see_good_ = s >= 0;
    return move.see_good_;
  }

  bool expressCheck(Move & move) const;

  Figure::Type thresholdType_;
  Move hcap_;

  // for checks detector
  BitMask mask_all_, mask_brq_;
  int oking_pos_;
};

/// generate all moves, that escape from check
class EscapeGenerator : public MovesGeneratorBase
{
public:

  EscapeGenerator(Board &);
  EscapeGenerator(const Move & hmove, Board & );

  void generate(const Move & hmove);

  void restart();

  int count() const
  {
    return movesCount_;
  }

  bool find(const Move & m) const
  {
    if ( m && m == hmove_ )
      return true;

    return MovesGeneratorBase::find(m); 
  }

  inline Move & escape()
  {
    if ( takeHash_ )
    {
      takeHash_ = 0;

      if ( hmove_ )
      {
        hmove_.see_good_ = (board_.see(hmove_) >= 0);
        return hmove_;
      }
    }

    for ( ; !do_weak_; )
    {
      Move * move = moves_ + numOfMoves_;
      Move * mv = moves_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ <= move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
      {
        do_weak_ = true;
        break;
      }

      if ( move->capture_ && !move->seen_ && numOfMoves_ > 1 )
      {
        move->seen_ = 1;
        move->see_good_ = board_.see(*move) >= 0;
        if ( !move->see_good_ )
        {
          weak_[weakN_++] = *move;
          move->alreadyDone_ = 1;
          continue;
        }
      }

      move->alreadyDone_ = 1;
      return *move;
    }

    if ( do_weak_ && weakN_ > 0 )
    {
      weak_[weakN_].clear();
      for ( ;; )
      {
        Move * move = weak_ + weakN_;
        Move * mv = weak_;
        for ( ; *mv; ++mv)
        {
          if ( mv->alreadyDone_ || mv->vsort_ <= move->vsort_ )
            continue;

          move = mv;
        }
        if ( !*move )
          return *move;

        move->alreadyDone_ = 1;
        return *move;
      }
    }

    return fake_;
  }

protected:

  /// returns number of moves found
  int generate();
  int generateUsual();
  int generateKingonly(int m);

  inline void add(int & m, int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    Move & move = moves_[m];
    move.set(from, to, new_type, capture);
    move.checkVerified_ = 1;

    if ( move == hmove_ )
    {
      takeHash_ = 1;
      return;
    }

    if ( capture || move.new_type_ )
    {
      sortValueOfCap(move);
      adjust_vsort(move, 10000000);
    }
    else
    {
      adjust_vsort(move, 10);
    }

    ++m;
  }

  Move hmove_;
  int takeHash_;
  int movesCount_;

  Move weak_[Board::MovesMax];
  int weakN_;
  bool do_weak_;
  Move fake_;
};

/// first use move from hash, then generate all captures and promotions to queen, at the last generate other moves
/// generates all valid moves if under check
class FastGenerator
{
public:

  FastGenerator(Board & board);
  FastGenerator(Board & board, const Move & hmove, const Move & killer);

  Move & move();

  bool singleReply() const
  {
    return board_.underCheck() && eg_.count() == 1;
  }

  void restart();

private:

  enum GOrder
  {
    oHash, oEscapes, oGenCaps, oCaps, oKiller, oGenUsual, oUsual, oWeak
  } order_;

  CapsGenerator cg_;
  UsualGenerator ug_;
  EscapeGenerator eg_;

  Move weak_[Board::MovesMax];
  int  weakN_;

  Move hmove_, killer_, fake_;
  Board & board_;
};

//////////////////////////////////////////////////////////////////////////
// generate checks without captures
class ChecksGenerator : public MovesGeneratorBase
{
public:

  ChecksGenerator(Board & board);

  Move & check()
  {
    for ( ;; )
    {
      Move * move = moves_ + numOfMoves_;
      Move * mv = moves_;
      for ( ; *mv; ++mv)
      {
        if ( mv->alreadyDone_ || mv->vsort_ <= move->vsort_ )
          continue;

        move = mv;
      }
      if ( !*move )
        return *move;

      move->alreadyDone_ = 1;

      if ( !move->discoveredCheck_ && board_.see(*move) < 0 )
        continue;

      return *move;
    }
  }

  int generate();
  int generate(const Move & hmove);

private:

  int genChecks();

  inline void add(int & m, int8 from, int8 to, Figure::Type new_type, bool discovered)
  {
    Move & move = moves_[m];
    move.set(from, to, new_type, false);
    if ( move == hmove_ )
      return;

    move.discoveredCheck_ = discovered;

    if ( move.capture_ || move.new_type_ )
    {
      sortValueOfCap(move);
      adjust_vsort(move, 10000000);
    }
    else
    {
      const History & hist = history(move.from_, move.to_);
      move.vsort_ = hist.score();
      adjust_vsort(move, 10);
    }

    m++;
  }

  Move hmove_;
};


//////////////////////////////////////////////////////////////////////////
/// Generate tactical moves after horizon
class TacticalGenerator
{
public:

  TacticalGenerator(Board & board, Figure::Type thresholdType, int depth);

  Move & next();

  // valid only under check
  bool singleReply() const
  {
    return board_.underCheck() && eg_.count() == 1;
  }

private:

  CapsGenerator cg_;
  EscapeGenerator eg_;
  ChecksGenerator chg_;

  enum Order { oNone, oEscape, oGenCaps, oCaps, oGenChecks, oChecks };

  Board & board_;
  Figure::Type thresholdType_;
  Move fake_;
  Order order_;
  int depth_;
};

} // NEngine
