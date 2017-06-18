/*************************************************************
  MovesGenerator.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Board.h>
#include <xlist.h>
#include <algorithm>
#include <MovesTable.h>
#include <fstream>
#include <CapsGenerator.h>
#include <ChecksGenerator.h>
#include <EscapeGenerator.h>
#include <UsualGenerator.h>

namespace NEngine
{

template <class BOARD, class MOVE, int NUM = BOARD::MovesMax>
xlist<MOVE, NUM> generate(BOARD const& board)
{
  xlist<MOVE, NUM> moves;
  const auto& color = board.color();
  const auto ocolor = Figure::otherColor(color);
  auto const& fmgr = board.fmgr();

  if(!board.underCheck() || !board.doubleCheck())
  {
    // pawns movements
    if(auto pw_mask = fmgr.pawn_mask(color))
    {
      for(; pw_mask;)
      {
        auto pw_pos = clear_lsb(pw_mask);
        const auto* table = movesTable().pawn(color, pw_pos);
        for(int i = 0; i < 2; ++i, ++table)
        {
          if(*table < 0)
            continue;
          const auto& field = board.getField(*table);
          bool capture = false;
          if((field && field.color() == ocolor) ||
             (board.enpassant() > 0 && *table == board.enpassant()))
          {
            capture = true;
          }
          if(!capture)
            continue;
          bool promotion = *table > 55 || *table < 8;
          MOVE move{ pw_pos, *table, Figure::TypeNone };
          // promotion
          if(*table > 55 || *table < 8)
          {
            moves.emplace_back(pw_pos, *table, Figure::TypeQueen);
            moves.emplace_back(pw_pos, *table, Figure::TypeRook);
            moves.emplace_back(pw_pos, *table, Figure::TypeBishop);
            moves.emplace_back(pw_pos, *table, Figure::TypeKnight);
          }
          else
          {
            moves.emplace_back(pw_pos, *table);
          }
        }

        for(; *table >= 0 && !board.getField(*table); ++table)
        {
          // promotion
          if(*table > 55 || *table < 8)
          {
            moves.emplace_back(pw_pos, *table, Figure::TypeQueen);
            moves.emplace_back(pw_pos, *table, Figure::TypeRook);
            moves.emplace_back(pw_pos, *table, Figure::TypeBishop);
            moves.emplace_back(pw_pos, *table, Figure::TypeKnight);
          }
          else
          {
            moves.emplace_back(pw_pos, *table);
          }
        }
      }
    }

    // knights movements
    if(fmgr.knight_mask(color))
    {
      auto kn_mask = fmgr.knight_mask(color);
      for(; kn_mask;)
      {
        auto kn_pos = clear_lsb(kn_mask);
        const auto* table = movesTable().knight(kn_pos);
        for(; *table >= 0; ++table)
        {
          const auto& field = board.getField(*table);
          bool capture = false;
          if(field)
          {
            if(field.color() == color)
              continue;
            capture = true;
          }
          moves.emplace_back(kn_pos, *table);
        }
      }
    }

    // bishops, rooks and queens movements
    for(int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      auto fg_mask = fmgr.type_mask((Figure::Type)type, color);
      for(; fg_mask;)
      {
        auto fg_pos = clear_lsb(fg_mask);
        const auto* table = movesTable().move(type-Figure::TypeBishop, fg_pos);
        for(; *table; ++table)
        {
          const auto* packed = reinterpret_cast<const int8*>(table);
          auto count = packed[0];
          auto const& delta = packed[1];
          auto p = fg_pos;
          bool capture = false;
          for(; count && !capture; --count)
          {
            p += delta;
            const auto & field = board.getField(p);
            if(field)
            {
              if(field.color() == color)
                break;
              capture = true;
            }
            moves.emplace_back(fg_pos, p);
          }
        }
      }
    }
  }

  // kings movements
  {
    auto ki_mask = fmgr.king_mask(color);
    X_ASSERT(ki_mask == 0, "invalid position - no king");
    auto ki_pos = clear_lsb(ki_mask);
    const auto* table = movesTable().king(ki_pos);
    for(; *table >= 0; ++table)
    {
      const auto & field = board.getField(*table);
      bool capture = false;
      if(field)
      {
        if(field.color() == color)
          continue;
        capture = true;
      }
      moves.emplace_back(ki_pos, *table);
    }

    if(!board.underCheck())
    {
      // short castle
      if(board.castling(board.color(), 0) && !board.getField(ki_pos+2) && !board.getField(ki_pos+1))
        moves.emplace_back(ki_pos, ki_pos+2);

      // long castle
      if(board.castling(board.color(), 1) && !board.getField(ki_pos-2) && !board.getField(ki_pos-1) && !board.getField(ki_pos-3))
        moves.emplace_back(ki_pos, ki_pos-2);
    }
  }

  return moves;
}

template <class BOARD, class MOVE>
struct FastGenerator
{
  using MovesList = xlist<MOVE, BOARD::MovesMax>;

  enum Order { oEscape, oGenCaps, oCaps, oGenUsual, oUsual } order_{};

  FastGenerator(BOARD const& board) :
    board_(board),
    cg_(board), ug_(board), eg_(board)
  {
    if(!board.underCheck())
      order_ = oGenCaps;
  }

  bool singleReply() const
  {
    return board_.underCheck() && eg_.caps_.size() + eg_.usual_.size() == 1;
  }

  MOVE* next()
  {
    if(order_ == oEscape)
    {
      return eg_.next();
    }
    if(order_ == oGenCaps)
    {
      cg_.generateCaps();
      order_ = oCaps;
    }
    if(order_ == oCaps)
    {
      auto* move = cg_.next();
      if(move)
        return move;
      order_ = oGenUsual;
    }
    if(order_ == oGenUsual)
    {
      ug_.generate();
      order_ = oUsual;
    }
    if(order_ == oUsual)
    {
      auto* move = ug_.next();
      return move;
    }
    return nullptr;
  }

  CapsGenerator<BOARD, MOVE> cg_;
  UsualGenerator<BOARD, MOVE> ug_;
  EscapeGenerator<BOARD, MOVE> eg_;
  BOARD const& board_;
};

template <class BOARD, class MOVE>
struct TacticalGenerator
{
  using MovesList = xlist<MOVE, BOARD::MovesMax>;

  enum Order { oEscape, oGenCaps, oCaps, oGenChecks, oChecks } order_{};

  TacticalGenerator(BOARD const& board, int depth) :
    board_(board),
    cg_(board), ckg_(board), eg_(board), depth_(depth)
  {
    if(!board.underCheck())
      order_ = oGenCaps;
  }

  MOVE* next()
  {
    if(order_ == oEscape)
    {
      return eg_.next();
    }
    if(order_ == oGenCaps)
    {
      cg_.generateCaps();
      order_ = oCaps;
    }
    if(order_ == oCaps)
    {
      if(auto* move = cg_.next())
        return move;
      if(depth_ < 0)
        return nullptr;
      order_ = oGenChecks;
    }
    if(order_ == oGenChecks)
    {
      ckg_.generate();
      order_ = oChecks;
    }
    if(order_ == oChecks)
    {
      return ckg_.next();
    }
    return nullptr;
  }

  bool singleReply() const
  {
    return board_.underCheck() && eg_.caps_.size() + eg_.usual_.size() == 1;
  }

  CapsGenerator<BOARD, MOVE> cg_;
  ChecksGenerator<BOARD, MOVE> ckg_;
  EscapeGenerator<BOARD, MOVE> eg_;
  BOARD const& board_;
  int depth_{};
};





















//
//
//// base class for all moves generators
//template <int NumMovesMax>
//class MovesGeneratorBase
//{
//public:
//
//  using MovesList = xlist<Move, NumMovesMax>;
//
//  static const unsigned int vsort_add_ = 10000;
//
//  MovesGeneratorBase(const Board & board) : board_(board)
//  {
//  }
//
//  operator bool () const
//  {
//    return movesCount_ > 0;
//  }
//
//  int count() const
//  {
//    return movesCount_;
//  }
//
//  MovesList& moves()
//  {
//    return moves_;
//  }
//
//  bool find(const Move & move) const
//  {
//    auto it = std::find_if(moves_.begin(), moves_.end(),
//                           [&move](Move const& m) { return m == move; });
//    return it != moves_.end();
//  }
//
//  bool has_duplicates() const
//  {
//    for(auto iter = moves_.begin(); iter != moves_.end(); ++iter)
//    {
//      auto const& move = *iter;
//      auto iter1 = iter++;
//      auto it = std::find_if(iter, moves_.end(),
//                             [&move](Move const& m) { return m == move; });
//      if(it != moves_.end())
//        return true;
//    }
//    return false;
//  }
//
//
//protected:
//
//  inline unsigned adjust_vsort(unsigned value, unsigned delta) const
//  {
//    value += delta;
//    value |= 1; // to prevent zero value
//    return value;
//  }
//
//  inline void adjust_vsort(Move & move, unsigned delta) const
//  {
//    move.vsort_ = adjust_vsort(move.vsort_, delta);
//  }
//
//  inline void sortValueOfCap(Move & move)
//  {
//    const Field & fto = board_.getField(move.to_);
//    const Field & ffrom = board_.getField(move.from_);
//
//    Figure::Type vtype = fto.type();
//
//    X_ASSERT(vtype != Figure::TypeNone && fto.color() != Figure::otherColor(board_.color_), "invalid color of captured figure");
//
//    // en-passant case
//    if ( vtype == Figure::TypeNone && move.to_ == board_.en_passant_ && ffrom.type() == Figure::TypePawn )
//    {
//      X_ASSERT( board_.getField(board_.enpassantPos()).type() != Figure::TypePawn ||
//        board_.getField(board_.enpassantPos()).color() == board_.color_, "no en-passant pawn" );
//
//      vtype = Figure::TypePawn;
//      move.vsort_ = Figure::figureWeight_[vtype];
//      adjust_vsort(move, vsort_add_);
//      return;
//    }
//
//    int vsort = 0;
//
//    // capture, prefer stronger opponent's figure
//    if ( vtype != Figure::TypeNone )
//    {
//      Figure::Type atype = ffrom.type();
//      vsort = (int)(Figure::figureWeight_[vtype] << 1) - (int)Figure::figureWeight_[atype];
//    }
//
//    // pawn promotion
//    if ( move.new_type_ > Figure::TypeNone && move.new_type_ < Figure::TypeKing )
//    {
//      vsort += (int)Figure::figureWeight_[move.new_type_];
//    }
//
//    // at first we try to eat recently moved opponent's figure
//    if ( move.capture_ && board_.halfmovesCount() > 1 )
//    {
//      const UndoInfo & prev = board_.undoInfoRev(-1);
//      if ( prev.to_ == move.to_ )
//        vsort += (int)(Figure::figureWeight_[vtype] >> 1);
//    }
//
//    vsort += vsort_add_;
//    move.vsort_ = (unsigned)vsort;
//    adjust_vsort(move, 0);
//  }
//
//  int movesCount_ = 0;
//  const Board & board_;
//  MovesList moves_;
//};
//
///// generate all movies from this position. don't verify and sort them. only calculate sort weights
//class MovesGenerator : public MovesGeneratorBase<Board::MovesMax>
//{
//public:
//
//  MovesGenerator(const Board & board, const Move & killer);
//  MovesGenerator(const Board & );
//
//  Move * move()
//  {
//    for(;;)
//    {
//      auto iter = std::max_element(moves_.begin(), moves_.end(),
//        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
//      if(iter == moves_.end())
//        return nullptr;
//      auto* move = &*iter;    
//      if ( (move->capture_ || move->new_type_ > 0) && !move->seen_ )
//      {
//        move->seen_ = 1;
//        int see_gain = 0;
//        if ( (see_gain = board_.see(*move)) < 0 )
//        {
//          if ( move->capture_ )
//            move->vsort_ = see_gain + 3000;
//          else
//            move->vsort_ = see_gain + 2000;
//          continue;
//        }
//      }
//      moves_.erase(iter);
//      return move;
//    }
//    return nullptr;
//  }
//
//private:
//
//  void generate();
//
//  inline void add(int8 from, int8 to, Figure::Type new_type, bool capture)
//  {
//    moves_.emplace_back(from, to, new_type, capture);
//    Move & move = moves_.back();
//    calculateSortValue(move);
//  }
//
//  void calculateSortValue(Move & move)
//  {
//    X_ASSERT( !board_.getField(move.from_), "no figure on field we move from" );
//
//    if ( move.capture_ || move.new_type_ )
//    {
//      sortValueOfCap(move);
//      adjust_vsort(move, 10000000);
//      return;
//    }
//    else if ( move == killer_ )
//    {
//      adjust_vsort(move, 3000000);
//      return;
//    }
//
//    const History & hist = history(move.from_, move.to_);
//    move.vsort_ = hist.score();
//    adjust_vsort(move, 10000);
//  }
//
//  Move killer_;
//};
//
///// generate all moves but captures and promotion to queen. could generate only promotion to knight if it checks
///// couldn't be called under check !!!
//class UsualGenerator : public MovesGeneratorBase<Board::MovesMax>
//{
//public:
//  UsualGenerator(Board & );
//
//  void generate(const Move & hmove, const Move & killer);
//
//  inline Move* test_move()
//  {
//    auto iter = moves_.begin();
//    if(iter == moves_.end())
//      return nullptr;
//    auto* move = &*iter;
//    moves_.erase(iter);
//    return move;
//  }
//
//  inline Move* move()
//  {
//    auto iter = count() == moves_.size()
//      ? moves_.begin()
//      : std::max_element(moves_.begin(), moves_.end(), [](Move const& m1, Move const& m2)
//        {
//          return m1.vsort_ < m2.vsort_;
//        });
//    if(iter == moves_.end())
//      return nullptr;
//    auto* move = &*iter;
//    moves_.erase(iter);
//    return move; 
//  }
//
//private:
//
//  inline bool add(int8 from, int8 to, Figure::Type new_type, bool capture)
//  {
//    if((hmove_.from_ == from && hmove_.to_ == to && hmove_.new_type_ == new_type) ||
//       (killer_.from_ == from && killer_.to_ == to && killer_.new_type_ == new_type))
//    {
//      return false;
//    }
//    auto vsort = calculateSortValue(from, to);
//    if(!moves_.empty() && moves_.front().vsort_ < vsort)
//    {
//      moves_.emplace_front(from, to, new_type, capture);
//      moves_.front().vsort_ = vsort;
//    }
//    else
//    {
//      moves_.emplace_back(from, to, new_type, capture);
//      moves_.back().vsort_ = vsort;
//    }
//    return true;
//  }
//
//  inline unsigned calculateSortValue(int8 from, int8 to)
//  {
//    X_ASSERT( !board_.getField(from), "no figure on field we move from" );
//    const History & hist = history(from, to);
//    return adjust_vsort(hist.score(), 10);
//  }
//
//  Move hmove_, killer_;
//};
//
///// generate captures and promotions to queen only, don't detect checks
//class CapsGenerator : public MovesGeneratorBase<Board::MovesMax/2>
//{
//public:
//
//  CapsGenerator(Board & );
//  CapsGenerator(const Move & hcap, Board & );
//
//  void generate(const Move & hcap, Figure::Type thresholdType);
//
//  inline Move* test_move()
//  {
//    for(;;)
//    {
//      auto iter = moves_.begin();
//      if(iter == moves_.end())
//        return nullptr;
//      auto* move = &*iter;
//      moves_.erase(iter);
//      return move;
//    }
//    return nullptr;
//  }
//
//  inline Move* move()
//  {
//    for(;;)
//    {
//      auto iter = std::max_element(moves_.begin(), moves_.end(),
//        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
//      if(iter == moves_.end())
//        return nullptr;
//      auto* move = &*iter;
//      moves_.erase(iter);
//      if(!filter(*move))
//        continue;
//      return move;
//    }
//    return nullptr;
//  }
//
//  inline Move* next_move()
//  {
//    auto iter = std::max_element(moves_.begin(), moves_.end(),
//      [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
//    if(iter == moves_.end())
//      return nullptr;
//    auto* move = &*iter;
//    moves_.erase(iter);
//    if(!move->seen_)
//    {
//      move->see_good_ = board_.see(*move) >= 0;
//      move->seen_ = 1;
//    }
//    return move;
//  }
//
//
//private:
//
//  void generate();
//
//  inline void add(int8 from, int8 to, Figure::Type new_type, bool capture)
//  {
//    X_ASSERT( !board_.getField(from), "no figure on field we move from" );
//    if(hcap_.from_ == from && hcap_.to_ == to && hcap_.new_type_ == new_type)
//      return;
//    moves_.emplace_back(from, to, new_type, capture);
//    auto& move = moves_.back();
//    sortValueOfCap(move);
//  }
//
//  inline bool filter(Move & move) const
//  {
//    if ( move.capture_ && move.new_type_ )
//      return true;
//
//    bool check = expressCheck(move);
//
//    if ( move.discoveredCheck_ )
//      return true; // always good
//
//    Figure::Type type = Figure::TypeNone;
//    const Field & tfield = board_.getField(move.to_);
//    if ( tfield )
//      type = tfield.type();
//    else if ( move.new_type_ )
//      type = (Figure::Type)move.new_type_;
//    else if ( board_.en_passant_ == move.to_ && board_.getField(move.from_).type() == Figure::TypePawn ) // en-passant capture
//      type = Figure::TypePawn;
//
//    if ( !check && type < thresholdType_ )
//      return false;
//
//    int s = board_.see(move);
//		move.seen_ = 1;
//		move.see_good_ = s >= 0;
//    return move.see_good_;
//  }
//
//  bool expressCheck(Move & move) const;
//
//  Figure::Type thresholdType_{Figure::TypeNone};
//  Move hcap_;
//
//  // for checks detector
//  BitMask mask_all_, mask_brq_;
//  int oking_pos_{ 0 };
//};
//
///// generate all moves, that escape from check
//class EscapeGenerator : public MovesGeneratorBase<32>
//{
//public:
//
//  EscapeGenerator(Board &);
//  EscapeGenerator(const Move & hmove, Board &);
//
//  void generate(const Move & hmove);
//
//  bool find(const Move & m) const
//  {
//    if ( m && m == hmove_ )
//      return true;
//
//    return MovesGeneratorBase::find(m); 
//  }
//
//  inline Move* test_move()
//  {
//    Move* move = nullptr;
//    auto iter = moves_.begin();
//    if(iter == moves_.end())
//      return nullptr;
//    move = &*iter;
//    moves_.erase(iter);
//    return move;
//  }
//
//  inline Move* move()
//  {
//    if(takeHash_)
//    {
//      takeHash_ = 0;
//      if(hmove_)
//      {
//        hmove_.see_good_ = (board_.see(hmove_) >= 0);
//        return &hmove_;
//      }
//    }
//
//    for(; !do_weak_;)
//    {
//      Move* move = nullptr;
//      auto iter = std::max_element(moves_.begin(), moves_.end(),
//        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
//      if(iter == moves_.end())
//      {
//        do_weak_ = true;
//        break;
//      }
//      move = &*iter;
//      moves_.erase(iter);
//      if(!move->capture_ && move->new_type_ == Figure::TypeNone && !move->seen_ && count() > 1)
//      {
//        move->seen_ = 1;
//        move->see_good_ = board_.see(*move) >= 0;
//        if(!move->see_good_)
//        {
//          weaks_.push_back(*move);
//          continue;
//        }
//      }
//      return move;
//    }
//
//    if(do_weak_)
//    {
//      Move* move = nullptr;
//      auto iter = std::max_element(weaks_.begin(), weaks_.end(),
//        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
//      if(iter == weaks_.end())
//        return nullptr;
//      move = &*iter;
//      weaks_.erase(iter);
//      return move;
//    }
//
//    return nullptr;
//  }
//
//protected:
//
//  void generate();
//  void generateUsual();
//  void generateKingonly();
//
//  inline void add(int8 from, int8 to, Figure::Type new_type, bool capture)
//  {
//    if(hmove_.from_ == from && hmove_.to_ == to && hmove_.new_type_ == new_type)
//    {
//      takeHash_ = 1;
//      return;
//    }
//    moves_.emplace_back(from, to, new_type, capture);
//    auto& move = moves_.back();
//    move.checkVerified_ = 1;
//    if(capture || move.new_type_)
//    {
//      sortValueOfCap(move);
//      adjust_vsort(move, 10000000);
//    }
//    else
//    {
//      adjust_vsort(move, 10);
//    }
//  }
//
//  Move hmove_;
//  int takeHash_{0};
//
//  xlist<Move, 32> weaks_;
//  bool do_weak_{false};
//};
//
///// first use move from hash, then generate all captures and promotions to queen, at the last generate other moves
///// generates all valid moves if under check
//class FastGenerator
//{
//public:
//
//  FastGenerator(Board & board);
//  FastGenerator(Board & board, const Move & hmove, const Move & killer);
//
//  Move* move();
//  
//  Move* test_move()
//  {
//    if(order_ == oHash)
//    {
//      order_ = oGenCaps;
//    }
//    if(order_ == oEscapes)
//    {
//      return eg_.test_move();
//    }
//    if(order_ == oGenCaps)
//    {
//      cg_.generate(hmove_, Figure::TypePawn);
//      order_ = oCaps;
//    }
//    if(order_ == oCaps)
//    {
//      if(auto* move = cg_.test_move())
//        return move;
//      order_ = oGenUsual;
//    }
//    if(order_ == oGenUsual)
//    {
//      ug_.generate(hmove_, killer_);
//      order_ = oUsual;
//    }
//    if(order_ == oUsual)
//    {
//      return ug_.test_move();
//    }
//    return nullptr;
//  }
//
//  bool singleReply() const
//  {
//    return board_.underCheck() && eg_.count() == 1;
//  }
//
//private:
//
//  enum GOrder
//  {
//    oHash, oEscapes, oGenCaps, oCaps, oKiller, oGenUsual, oUsual, oWeak
//  } order_;
//
//  CapsGenerator cg_;
//  UsualGenerator ug_;
//  EscapeGenerator eg_;
//
//  xlist<Move, 64> weaks_;
//  Move hmove_, killer_, fake_;
//  Board & board_;
//};
//
////////////////////////////////////////////////////////////////////////////
//// generate checks without captures
//class ChecksGenerator : public MovesGeneratorBase<64>
//{
//public:
//
//  ChecksGenerator(Board & board);
//
//  Move* test_move()
//  {
//    for(;;)
//    {
//      auto iter = moves_.begin();
//      if(iter == moves_.end())
//        return nullptr;
//      auto* move = &*iter;
//      moves_.erase(iter);
//      return move;
//    }
//    return nullptr;
//  }
//
//  Move* move()
//  {
//    for(;;)
//    {
//      auto iter = std::max_element(moves_.begin(), moves_.end(),
//        [](Move const& m1, Move const& m2) { return m1.vsort_ < m2.vsort_; });
//      if(iter == moves_.end())
//        return nullptr;
//      auto* move = &*iter;
//      moves_.erase(iter);
//      if(!move->discoveredCheck_ && board_.see(*move) < 0)
//        continue;
//      return move;
//    }
//    return nullptr;
//  }
//
//  void generate();
//  void generate(const Move & hmove);
//
//private:
//  inline void add(int8 from, int8 to, Figure::Type new_type, bool discovered)
//  {
//    if(hmove_.from_ == from && hmove_.to_ == to && hmove_.new_type_ == new_type)
//      return;
//    moves_.emplace_back(from, to, new_type, false); 
//    auto& move = moves_.back();
//    move.discoveredCheck_ = discovered;
//    if ( move.capture_ || move.new_type_ )
//    {
//      sortValueOfCap(move);
//      adjust_vsort(move, 10000000);
//    }
//    else
//    {
//      const History & hist = history(move.from_, move.to_);
//      move.vsort_ = hist.score();
//      adjust_vsort(move, 10);
//    }
//  }
//
//  Move hmove_;
//};
//
//
////////////////////////////////////////////////////////////////////////////
///// Generate tactical moves after horizon
//class TacticalGenerator
//{
//public:
//
//  TacticalGenerator(Board & board, Figure::Type thresholdType, int depth);
//  TacticalGenerator(Board & board, const Move & hmove, Figure::Type thresholdType, int depth);
//
//  Move* move();
//
//  Move* test_move()
//  {
//    if(order_ == oEscape)
//    {
//      return eg_.test_move();
//    }
//    if(order_ == oHash)
//    {
//      order_ = oGenCaps;
//    }
//    if(order_ == oGenCaps)
//    {
//      cg_.generate(hmove_, thresholdType_);
//      order_ = oCaps;
//    }
//    if(order_ == oCaps)
//    {
//      if(auto* cap = cg_.test_move())
//      {
//        return cap;
//      }
//      if(depth_ < 0)
//        return nullptr;
//      order_ = oGenChecks;
//    }
//    if(order_ == oGenChecks)
//    {
//      chg_.generate(hmove_);
//      order_ = oChecks;
//    }
//    if(order_ == oChecks)
//    {
//      return chg_.test_move();
//    }
//    return nullptr;
//  }
//
//  Move* weak()
//  {
//    auto iter = weaks_.begin();
//    if(iter != weaks_.end())
//    {
//      auto* m = &*iter;
//      weaks_.erase(iter);
//      return m;
//    }
//    return nullptr;
//  }
//
//  // valid only under check
//  bool singleReply() const
//  {
//    return board_.underCheck() && eg_.count() == 1;
//  }
//
//private:
//
//  CapsGenerator cg_;
//  EscapeGenerator eg_;
//  ChecksGenerator chg_;
//
//  enum Order { oNone, oHash, oEscape, oGenCaps, oCaps, oGenChecks, oChecks };
//
//  Board & board_;
//  Figure::Type thresholdType_;
//  Order order_;
//  int depth_;
//  xlist<Move, 64> weaks_;
//  Move hmove_;
//};

} // NEngine
