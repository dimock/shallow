/*************************************************************
  MovesGenerator.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Board.h>
#include <xlist.h>
#include <algorithm>

#include <iostream>

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

  using MovesList = xlist<Move, Board::MovesMax>;

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

  MovesGeneratorBase(const Board & board) : board_(board)
  {}

  operator bool () const
  {
    return movesCount_ > 0;
  }

  int count() const
  {
    return movesCount_;
  }

  MovesList& moves()
  {
    return moves_;
  }

  bool find(const Move & m) const;
  bool has_duplicates() const;

protected:

  inline unsigned adjust_vsort(unsigned value, unsigned delta) const
  {
    value += delta;
    value |= 1; // to prevent zero value
    return value;
  }

  inline void adjust_vsort(Move & move, unsigned delta) const
  {
    move.vsort_ = adjust_vsort(move.vsort_, delta);
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

  int movesCount_ = 0;
  const Board & board_;
  MovesList moves_;
};

/// generate all movies from this position. don't verify and sort them. only calculate sort weights
class MovesGenerator : public MovesGeneratorBase
{
public:

  MovesGenerator(const Board & board, const Move & killer);
  MovesGenerator(const Board & );

  Move * move()
  {
    for(;;)
    {
      auto iter = std::max_element(moves_.begin(), moves_.end(),
        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
      if(iter == moves_.end())
        return nullptr;
      auto* move = &*iter;    
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
      }
      moves_.erase(iter);
      return move;
    }
    return nullptr;
  }

private:

  void generate();

  inline void add(int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    moves_.emplace_back(from, to, new_type, capture);
    Move & move = moves_.back();
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

  void generate(const Move & hmove, const Move & killer);

  inline Move* move()
  {
    auto iter = count() == moves_.size()
      ? moves_.begin()
      : std::max_element(moves_.begin(), moves_.end(), [](Move const& m1, Move const& m2)
        {
          return m1.vsort_ < m2.vsort_;
        });
    if(iter == moves_.end())
      return nullptr;
    auto* move = &*iter;
    moves_.erase(iter);
    return move; 
  }

private:

  inline bool add(int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    if((hmove_.from_ == from && hmove_.to_ == to && hmove_.new_type_ == new_type) ||
       (killer_.from_ == from && killer_.to_ == to && killer_.new_type_ == new_type))
    {
      return false;
    }
    auto vsort = calculateSortValue(from, to);
    if(!moves_.empty() && moves_.front().vsort_ < vsort)
    {
      moves_.emplace_front(from, to, new_type, capture);
      moves_.front().vsort_ = vsort;
    }
    else
    {
      moves_.emplace_back(from, to, new_type, capture);
      moves_.back().vsort_ = vsort;
    }
    return true;
  }

  inline unsigned calculateSortValue(int8 from, int8 to)
  {
    X_ASSERT( !board_.getField(from), "no figure on field we move from" );
    const History & hist = history(from, to);
    return adjust_vsort(hist.score(), 10);
  }

  Move hmove_, killer_;
};

/// generate captures and promotions to queen only, don't detect checks
class CapsGenerator : public MovesGeneratorBase
{
public:

  CapsGenerator(Board & );
  CapsGenerator(const Move & hcap, Board & );

  void generate(const Move & hcap, Figure::Type thresholdType);

  inline Move* move()
  {
    for(;;)
    {
      auto iter = std::max_element(moves_.begin(), moves_.end(),
        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
      if(iter == moves_.end())
        return nullptr;
      auto* move = &*iter;
      moves_.erase(iter);
      if(!filter(*move))
        continue;
      return move;
    }
    return nullptr;
  }

  inline Move* next_move()
  {
    auto iter = std::max_element(moves_.begin(), moves_.end(),
      [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
    if(iter == moves_.end())
      return nullptr;
    auto* move = &*iter;
    moves_.erase(iter);
    if(!move->seen_)
    {
      move->see_good_ = board_.see(*move) >= 0;
      move->seen_ = 1;
    }
    return move;
  }


private:

  void generate();

  inline void add(int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    X_ASSERT( !board_.getField(from), "no figure on field we move from" );
    if(hcap_.from_ == from && hcap_.to_ == to && hcap_.new_type_ == new_type)
      return;
    moves_.emplace_back(from, to, new_type, capture);
    auto& move = moves_.back();
    sortValueOfCap(move);
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

  Figure::Type thresholdType_{Figure::TypeNone};
  Move hcap_;

  // for checks detector
  BitMask mask_all_, mask_brq_;
  int oking_pos_{0};
};

/// generate all moves, that escape from check
class EscapeGenerator : public MovesGeneratorBase
{
public:

  EscapeGenerator(Board &);
  EscapeGenerator(const Move & hmove, Board & );

  void generate(const Move & hmove);

  bool find(const Move & m) const
  {
    if ( m && m == hmove_ )
      return true;

    return MovesGeneratorBase::find(m); 
  }

  inline Move* move()
  {
    if(takeHash_)
    {
      takeHash_ = 0;
      if(hmove_)
      {
        hmove_.see_good_ = (board_.see(hmove_) >= 0);
        return &hmove_;
      }
    }

    for(; !do_weak_;)
    {
      Move* move = nullptr;
      auto iter = std::max_element(moves_.begin(), moves_.end(),
        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
      if(iter == moves_.end())
      {
        do_weak_ = true;
        break;
      }
      move = &*iter;
      moves_.erase(iter);
      if(move->capture_ && !move->seen_ && count() > 1)
      {
        move->seen_ = 1;
        move->see_good_ = board_.see(*move) >= 0;
        if(!move->see_good_)
        {
          weaks_.push_back(*move);
          continue;
        }
      }
      return move;
    }

    if(do_weak_)
    {
      Move* move = nullptr;
      auto iter = std::max_element(weaks_.begin(), weaks_.end(),
        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
      if(iter == weaks_.end())
        return nullptr;
      move = &*iter;
      weaks_.erase(iter);
      return move;
    }

    return nullptr;
  }

protected:

  void generate();
  void generateUsual();
  void generateKingonly();

  inline void add(int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    if(hmove_.from_ == from && hmove_.to_ == to && hmove_.new_type_ == new_type)
    {
      takeHash_ = 1;
      return;
    }
    moves_.emplace_back(from, to, new_type, capture);
    auto& move = moves_.back();
    move.checkVerified_ = 1;
    if(capture || move.new_type_)
    {
      sortValueOfCap(move);
      adjust_vsort(move, 10000000);
    }
    else
    {
      adjust_vsort(move, 10);
    }
  }

  Move hmove_;
  int takeHash_{0};

  xlist<Move, 32> weaks_;
  bool do_weak_{false};
};

/// first use move from hash, then generate all captures and promotions to queen, at the last generate other moves
/// generates all valid moves if under check
class FastGenerator
{
public:

  FastGenerator(Board & board);
  FastGenerator(Board & board, const Move & hmove, const Move & killer);

  Move* move();

  bool singleReply() const
  {
    return board_.underCheck() && eg_.count() == 1;
  }

private:

  enum GOrder
  {
    oHash, oEscapes, oGenCaps, oCaps, oKiller, oGenUsual, oUsual, oWeak
  } order_;

  CapsGenerator cg_;
  UsualGenerator ug_;
  EscapeGenerator eg_;

  xlist<Move, 32> weaks_;
  Move hmove_, killer_, fake_;
  Board & board_;
};

//////////////////////////////////////////////////////////////////////////
// generate checks without captures
class ChecksGenerator : public MovesGeneratorBase
{
public:

  ChecksGenerator(Board & board);

  Move* move()
  {
    for(;;)
    {
      auto iter = std::max_element(moves_.begin(), moves_.end(),
        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
      if(iter == moves_.end())
        return nullptr;
      auto* move = &*iter;
      moves_.erase(iter);
      if(!move->discoveredCheck_ && board_.see(*move) < 0)
        continue;
      return move;
    }
    return nullptr;
  }

  void generate();
  void generate(const Move & hmove);

private:
  inline void add(int8 from, int8 to, Figure::Type new_type, bool discovered)
  {
    if(hmove_.from_ == from && hmove_.to_ == to && hmove_.new_type_ == new_type)
      return;
    moves_.emplace_back(from, to, new_type, false); 
    auto& move = moves_.back();
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
  }

  Move hmove_;
};


//////////////////////////////////////////////////////////////////////////
/// Generate tactical moves after horizon
class TacticalGenerator
{
public:

  TacticalGenerator(Board & board, Figure::Type thresholdType, int depth);

  Move* move();

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
  Order order_;
  int depth_;
};

} // NEngine
