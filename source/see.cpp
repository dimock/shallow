/*************************************************************
  see.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Board.h>
#include <Helpers.h>
#include <fstream>

namespace NEngine
{

namespace
{

struct SeeCalc
{
  Board const& board_;
  Move  const& move_;
  FiguresManager const& fmgr_;
  BitMask last_b_[2] = {};
  BitMask last_r_[2] = {};
  BitMask all_mask_;
  Figure::Color color_;
  bool promotion_;
  bool discovered_check_;

  SeeCalc(Board const& board, Move const& move, bool en_passant) :
    board_(board),
    move_(move),
    fmgr_(board.fmgr())
  {
    all_mask_ = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite) | set_mask_bit(move_.to());
    color_ = board_.getField(move_.from()).color();
    X_ASSERT((all_mask_ & set_mask_bit(move_.from())) == 0ULL, "no figure on field move.from");
    X_ASSERT(discovered_check(move_.from(), Figure::otherColor(color_), board_.kingPos(color_)), "SEE move is illegal");
    if(en_passant)
    {
      discovered_check_ = enpassant_check(move_.from(), color_, board_.kingPos(Figure::otherColor(color_)));
      X_ASSERT(!discovered_check_ && discovered_check(move_.from(), color_, board_.kingPos(Figure::otherColor(color_))),
               "discovered check was not detected");
    }
    else
      discovered_check_ = discovered_check(move_.from(), color_, board_.kingPos(Figure::otherColor(color_)));
    if(discovered_check_)
      return;
    all_mask_ ^= set_mask_bit(move_.from());
    promotion_ = (move_.to() >> 3) == 0 || (move_.to() >> 3) == 7;
  }

  inline bool next(Figure::Color color, int& from, int& score)
  {
    return (!discovered_check_ && move_figure(color, from, score)) || move_king(color, from, score);
  }

  inline bool move_figure(Figure::Color color, int& from, int& score)
  {
    auto ocolor = Figure::otherColor(color);
    {
      auto mask = fmgr_.pawn_mask(color) & all_mask_ & movesTable().pawnCaps(ocolor, move_.to());
      if(do_move(mask, color, ocolor, Figure::TypePawn, from, score))
        return true;
    }
  {
    auto mask = fmgr_.knight_mask(color) & all_mask_ & movesTable().caps(Figure::TypeKnight, move_.to());
    if(do_move(mask, color, ocolor, Figure::TypeKnight, from, score))
      return true;
  }

  BitMask mask_b{};
  if(fmgr_.bishop_mask(color) & all_mask_)
  {
    last_b_[color] = mask_b = magic_ns::bishop_moves(move_.to(), all_mask_) & all_mask_;
    auto mask = mask_b & fmgr_.bishop_mask(color);
    if(do_move(mask, color, ocolor, Figure::TypeBishop, from, score))
      return true;
  }

  BitMask mask_r{};
  if(fmgr_.rook_mask(color) & all_mask_)
  {
    last_r_[color] = mask_r = magic_ns::rook_moves(move_.to(), all_mask_) & all_mask_;
    auto mask = mask_r & fmgr_.rook_mask(color);
    if(do_move(mask, color, ocolor, Figure::TypeRook, from, score))
      return true;
  }

  if(fmgr_.queen_mask(color) & all_mask_)
  {
    if(!mask_b)
      mask_b = magic_ns::bishop_moves(move_.to(), all_mask_);
    if(!mask_r)
      mask_r = magic_ns::rook_moves(move_.to(), all_mask_);
    auto mask = (mask_b | mask_r) & all_mask_ & board_.fmgr().queen_mask(color);
    if(do_move(mask, color, ocolor, Figure::TypeQueen, from, score))
      return true;
  }

  return false;
  }

  inline bool move_king(Figure::Color color, int& from, int& score)
  {
    auto ocolor = Figure::otherColor(color);
    if((fmgr_.king_mask(color) & movesTable().caps(Figure::TypeKing, move_.to()))
        && !(fmgr_.king_mask(ocolor) & movesTable().caps(Figure::TypeKing, move_.to()))
        && !under_check(color, ocolor))
    {
      // score == 0 means king's attack. it should be the very last one
      score = 0;
      from = board_.kingPos(color);
      return true;
    }
    return false;
  }

  inline bool do_move(BitMask mask, Figure::Color color, Figure::Color ocolor, Figure::Type const type, int& from, int& score)
  {
    while(mask)
    {
      from = clear_lsb(mask);
      if(discovered_check(from, ocolor, board_.kingPos(color)))
        continue;
      X_ASSERT((all_mask_ & set_mask_bit(from)) == 0ULL, "no figure which is going to make move");
      all_mask_ ^= set_mask_bit(from);
      score = promotion_ && type == Figure::TypePawn
        ? Figure::figureWeight_[Figure::TypeQueen] - Figure::figureWeight_[Figure::TypePawn]
        : Figure::figureWeight_[type];
      if(color == color_)
        score = -score;
      return true;
    }
    return false;
  }

  inline bool enpassant_check(int from, Figure::Color ocolor, int ki_pos) const
  {
    auto epp = board_.enpassantPos();
    auto mask_all_f = all_mask_ & ~set_mask_bit(from) & ~set_mask_bit(epp);
    auto mask_all_t = mask_all_f & ~set_mask_bit(move_.to());
    {
      auto bi_moves = magic_ns::bishop_moves(ki_pos, mask_all_f) & mask_all_t;
      auto const& bi_mask = fmgr_.bishop_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      if((bi_moves & (bi_mask | q_mask)) != 0ULL)
        return true;
    }
    auto r_moves = magic_ns::rook_moves(ki_pos, mask_all_f) & mask_all_t;
    auto const& r_mask = fmgr_.rook_mask(ocolor);
    auto const& q_mask = fmgr_.queen_mask(ocolor);
    return (r_moves & (r_mask | q_mask)) != 0ULL;
  }

  inline bool discovered_check(int from, Figure::Color ocolor, int ki_pos) const
  {
    auto dir = figureDir().br_dir(ki_pos, from);
    if(!dir)
      return false;
    auto mask_all_f = all_mask_ & ~set_mask_bit(from);
    auto mask_all_t = mask_all_f & ~set_mask_bit(move_.to());
    if(dir == nst::bishop)
    {
      auto bi_moves = magic_ns::bishop_moves(ki_pos, mask_all_f) & mask_all_t;
      auto const& bi_mask = fmgr_.bishop_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      return (bi_moves & (bi_mask | q_mask)) != 0ULL;
    }
    X_ASSERT(dir != nst::rook, "invalid direction from point to point");
    auto r_moves = magic_ns::rook_moves(ki_pos, mask_all_f) & mask_all_t;
    auto const& r_mask = fmgr_.rook_mask(ocolor);
    auto const& q_mask = fmgr_.queen_mask(ocolor);
    return (r_moves & (r_mask | q_mask)) != 0ULL;
  }

  inline bool is_usual_check() const
  {
#ifndef NDEBUG
    SBoard<Board, UndoInfo, Board::GameLength> sbrd{ board_ };
    sbrd.makeMove(move_);
    bool check_detected = sbrd.underCheck();
    auto fen = toFEN(board_);
#endif
    auto ocolor = Figure::otherColor(color_);
    auto ftype = board_.getField(move_.from()).type();
    if(ftype == Figure::TypeKing)
      return false;
    auto to = move_.to();
    if(ftype == Figure::TypePawn)
    {
      if(!move_.new_type())
      {
        bool result = (movesTable().pawnCaps(color_, to) & board_.fmgr().king_mask(ocolor)) != 0ULL;
        X_ASSERT(check_detected != result, "invalid check detected");
        return result;
      }
      ftype = (Figure::Type)move_.new_type();
    }
    if(ftype == Figure::TypeKnight)
    {
      bool result = (movesTable().caps(Figure::TypeKnight, to) & board_.fmgr().king_mask(ocolor)) != 0ULL;
      X_ASSERT(check_detected != result, "invalid check detected");
      return result;
    }
    int ok_pos = board_.kingPos(ocolor);
    int dir = figureDir().dir(ftype, color_, to, ok_pos);
    if(dir < 0)
    {
      X_ASSERT(check_detected, "check is not detected");
      return false;
    }
    auto result = board_.is_nothing_between(to, ok_pos, ~all_mask_ | set_mask_bit(move_.from()));
    X_ASSERT(check_detected != result, "invalid check detection");
    return result;
  }

  inline bool under_check(Figure::Color color, Figure::Color ocolor) const
  {
    if(board_.fmgr().pawn_mask(ocolor) & all_mask_ & movesTable().pawnCaps(color, move_.to()))
      return true;

    if(board_.fmgr().knight_mask(ocolor) & all_mask_ & movesTable().caps(Figure::TypeKnight, move_.to()))
      return true;

    auto bq_mask = (fmgr_.bishop_mask(ocolor) | fmgr_.queen_mask(ocolor)) & all_mask_;
    if((last_b_[ocolor] & bq_mask) || (magic_ns::bishop_moves(move_.to(), all_mask_) & bq_mask))
    {
      return true;
    }

    auto rq_mask = (fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor)) & all_mask_;
    return (last_r_[ocolor] & rq_mask) || (magic_ns::rook_moves(move_.to(), all_mask_) & rq_mask);
  }
};

} // end of namespace {} for SeeCalc

bool Board::see(const Move & move, int threshold) const
{
  X_ASSERT(data_.state_ == Invalid, "try to SEE invalid board");

  const auto& ffield = getField(move.from());
  const auto& tfield = getField(move.to());
  bool en_passant{ false };

  if(tfield)
  {
    // victim >= attacker
    int tgain = Figure::figureWeight_[tfield.type()] - Figure::figureWeight_[ffield.type()];
    if(tgain >= threshold)
      return true;
  }
  // en-passant
  else if(!tfield && ffield.type() == Figure::TypePawn && move.to() > 0 && move.to() == enpassant())
  {
    X_ASSERT(getField(enpassantPos()).type() != Figure::TypePawn
              || getField(enpassantPos()).color() == color(),
              "no en-passant pawn");
    if(Figure::figureWeight_[Figure::TypePawn] >= threshold)
      return true;
    en_passant = true;
  }

  SeeCalc see_calc(*this, move, en_passant);

  // assume discovered check is always good
  if(see_calc.discovered_check_)
    return true;

  // could be checkmate
  if(threshold > 0 && see_calc.is_usual_check())
  {
    threshold = 0;
  }

  if(en_passant)
    return Figure::figureWeight_[Figure::TypePawn] >= threshold;

  int score_gain = (tfield) ? Figure::figureWeight_[tfield.type()] : 0;
  int fscore = -Figure::figureWeight_[ffield.type()];
  if (see_calc.promotion_)
  {
    score_gain += Figure::figureWeight_[Figure::TypeQueen] - Figure::figureWeight_[Figure::TypePawn];
    fscore = -Figure::figureWeight_[Figure::TypeQueen];
  }
  if(score_gain < threshold)
    return false;

  auto color = Figure::otherColor(ffield.color());

  for(;;)
  {
    int from{};
    int score{};
    if(!see_calc.next(color, from, score))
      break;
    auto ocolor = Figure::otherColor(color);
    see_calc.discovered_check_ = see_calc.discovered_check(from, color, kingPos(ocolor));
    // assume this move is good if it was mine and I discover check
    if(see_calc.discovered_check_ && color == see_calc.color_)
      return true;
    score_gain += fscore;
    // king's move always last
    if(score == 0)
      break;
    // already winner even if loose last moved figure
    if(score_gain + score >= threshold && color == see_calc.color_)
      return true;
    // loose even if take last moved figure
    if(score_gain + score < threshold && color != see_calc.color_)
      return false;
    // could not gain something
    if(score_gain < threshold && color == see_calc.color_)
      return false;
    // already winner
    if(score_gain >= threshold && color != see_calc.color_)
      return true;
    fscore = score;
    color = ocolor;
  }

  return score_gain >= threshold;
}


} // NEngine
