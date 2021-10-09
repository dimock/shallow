/*************************************************************
  see.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include "Board.h"
#include "Helpers.h"
#include "fstream"

namespace NEngine
{

namespace
{

struct SeeCalc
{
  Board const& board_;
  Move  const move_;
  FiguresManager const& fmgr_;
  BitMask last_b_[2] = {};
  BitMask last_r_[2] = {};
  BitMask all_mask_;
  BitMask ncaps_moveto_[2];
  BitMask bcaps_moveto_[2];
  BitMask rcaps_moveto_[2];
  BitMask qcaps_moveto_[2];
  BitMask kicaps_moveto_;
  BitMask pwmask_moveto_[2];
  Figure::Color color_;
  Figure::Color ocolor_;
  bool promotion_;
  bool discovered_check_;
  mutable bool usual_check_detected_ = false;
  mutable bool usual_check_ = false;

  SeeCalc(Board const& board, Move const& move, Field const& ffield, bool en_passant) :
    board_(board),
    move_(move),
    fmgr_(board.fmgr())
  {
    all_mask_ = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite) | set_mask_bit(move_.to());
    color_ = ffield.color();
    ocolor_ = Figure::otherColor(color_);
    X_ASSERT((all_mask_ & set_mask_bit(move_.from())) == 0ULL, "no figure on field move.from");
    X_ASSERT(discovered_check(move_.from(), ocolor_, board_.kingPos(color_)), "SEE move is illegal");
    if (en_passant)
    {
      discovered_check_ = enpassant_check(move_.from(), color_, board_.kingPos(ocolor_));
      X_ASSERT(!discovered_check_ && discovered_check(move_.from(), color_, board_.kingPos(ocolor_)),
        "discovered check was not detected");
    }
    else
      discovered_check_ = discovered_check(move_.from(), color_, board_.kingPos(ocolor_));
    if (discovered_check_)
      return;
    all_mask_ ^= set_mask_bit(move_.from());
    promotion_ = (ffield.type() == Figure::TypePawn) && ((move_.to() >> 3) == 0 || (move_.to() >> 3) == 7);
  }

  void setupMovingFigures()
  {
    auto const ncaps_mask = movesTable().caps(Figure::TypeKnight, move_.to()) & all_mask_;
    ncaps_moveto_[0] = ncaps_mask & fmgr_.knight_mask(Figure::ColorBlack);
    ncaps_moveto_[1] = ncaps_mask & fmgr_.knight_mask(Figure::ColorWhite);
    auto const bcaps_mask = movesTable().caps(Figure::TypeBishop, move_.to()) & all_mask_;
    bcaps_moveto_[0] = fmgr_.bishop_mask(Figure::ColorBlack) & bcaps_mask;
    bcaps_moveto_[1] = fmgr_.bishop_mask(Figure::ColorWhite) & bcaps_mask;
    auto const rcaps_mask = movesTable().caps(Figure::TypeRook, move_.to()) & all_mask_;
    rcaps_moveto_[0] = fmgr_.rook_mask(Figure::ColorBlack) & rcaps_mask;
    rcaps_moveto_[1] = fmgr_.rook_mask(Figure::ColorWhite) & rcaps_mask;
    auto const qcaps_mask = movesTable().caps(Figure::TypeQueen, move_.to()) & all_mask_;
    qcaps_moveto_[0] = fmgr_.queen_mask(Figure::ColorBlack) & qcaps_mask;
    qcaps_moveto_[1] = fmgr_.queen_mask(Figure::ColorWhite) & qcaps_mask;
    kicaps_moveto_ = movesTable().caps(Figure::TypeKing, move_.to());
    pwmask_moveto_[0] = movesTable().pawnCaps(1, move_.to()) & fmgr_.pawn_mask(Figure::ColorBlack) & all_mask_;
    pwmask_moveto_[1] = movesTable().pawnCaps(0, move_.to()) & fmgr_.pawn_mask(Figure::ColorWhite) & all_mask_;
  }

  inline bool next(Figure::Color color, int& from, int& score)
  {
    return (!discovered_check_ && move_figure(color, from, score)) || move_king(color, from, score);
  }

  inline bool move_figure(Figure::Color color, int& from, int& score)
  {
    auto ocolor = Figure::otherColor(color);
    if(pwmask_moveto_[color])
    {
      if (do_pwmove(pwmask_moveto_[color], color, ocolor, from, score)) {
        pwmask_moveto_[color] &= all_mask_;
        return true;
      }
    }
    if(ncaps_moveto_[color])
    {
      if (do_fmove(ncaps_moveto_[color], color, ocolor, Figure::TypeKnight, from, score)) {
        ncaps_moveto_[color] &= all_mask_;
        return true;
      }
    }

    BitMask mask_b{};
    if(bcaps_moveto_[color])
    {
      last_b_[color] = mask_b = magic_ns::bishop_moves(move_.to(), all_mask_) & all_mask_;
      auto mask = mask_b & bcaps_moveto_[color];
      if (do_fmove(mask, color, ocolor, Figure::TypeBishop, from, score)) {
        bcaps_moveto_[color] &= all_mask_;
        return true;
      }
    }

    BitMask mask_r{};
    if(rcaps_moveto_[color])
    {
      last_r_[color] = mask_r = magic_ns::rook_moves(move_.to(), all_mask_) & all_mask_;
      auto mask = mask_r & rcaps_moveto_[color];
      if (do_fmove(mask, color, ocolor, Figure::TypeRook, from, score)) {
        rcaps_moveto_[color] &= all_mask_;
        return true;
      }
    }

    if(qcaps_moveto_[color])
    {
      if(!mask_b)
        mask_b = magic_ns::bishop_moves(move_.to(), all_mask_);
      if(!mask_r)
        mask_r = magic_ns::rook_moves(move_.to(), all_mask_);
      auto mask = (mask_b | mask_r) & qcaps_moveto_[color];
      if (do_fmove(mask, color, ocolor, Figure::TypeQueen, from, score)) {
        qcaps_moveto_[color] &= all_mask_;
        return true;
      }
    }

    return false;
  }

  inline bool move_king(Figure::Color color, int& from, int& score)
  {
    auto ocolor = Figure::otherColor(color);
    if((fmgr_.king_mask(color) & kicaps_moveto_) && !(fmgr_.king_mask(ocolor) & kicaps_moveto_) && !under_check(color, ocolor))
    {
      // score == 0 means king's attack. it should be the very last one
      score = 0;
      from = board_.kingPos(color);
      return true;
    }
    return false;
  }

  inline bool do_pwmove(BitMask mask, Figure::Color color, Figure::Color ocolor, int& from, int& score)
  {
    while (mask)
    {
      from = clear_lsb(mask);
      if (discovered_check(from, ocolor, board_.kingPos(color)))
        continue;
      X_ASSERT((all_mask_ & set_mask_bit(from)) == 0ULL, "no figure which is going to make move");
      all_mask_ ^= set_mask_bit(from);
      score = promotion_
        ? Figure::figureWeight_[Figure::TypeQueen] - Figure::figureWeight_[Figure::TypePawn]
        : Figure::figureWeight_[Figure::TypePawn];
      if (color == color_)
        score = -score;
      return true;
    }
    return false;
  }

  inline bool do_fmove(BitMask mask, Figure::Color color, Figure::Color ocolor, Figure::Type const type, int& from, int& score)
  {
    X_ASSERT(type == Figure::TypePawn,  "SEE incorrect pawn move");
    while(mask)
    {
      from = clear_lsb(mask);
      if(discovered_check(from, ocolor, board_.kingPos(color)))
        continue;
      X_ASSERT((all_mask_ & set_mask_bit(from)) == 0ULL, "no figure which is going to make move");
      all_mask_ ^= set_mask_bit(from);
      score = color == color_  ? -Figure::figureWeight_[type] : Figure::figureWeight_[type];
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
      auto const bi_mask = fmgr_.bishop_mask(ocolor);
      auto const q_mask = fmgr_.queen_mask(ocolor);
      auto bq_mask = (bi_mask | q_mask) & mask_all_t;
      if (bq_mask) {
        auto bi_moves = magic_ns::bishop_moves(ki_pos, mask_all_f);
        if ((bi_moves & bq_mask) != 0ULL)
          return true;
      }
    }
    auto const r_mask = fmgr_.rook_mask(ocolor);
    auto const q_mask = fmgr_.queen_mask(ocolor);
    auto rq_mask = (r_mask | q_mask) & mask_all_t;
    if (!rq_mask)
      return false;
    auto r_moves = magic_ns::rook_moves(ki_pos, mask_all_f);
    return (r_moves & rq_mask) != 0ULL;
  }

  inline bool discovered_check(int from, Figure::Color ocolor, int ki_pos) const
  {
    auto dir = figureDir().br_dir(ki_pos, from);
    if(!dir)
      return false;
    auto mask_all_f = all_mask_ & ~set_mask_bit(from);
    auto mask_all_t = mask_all_f & ~set_mask_bit(move_.to());
    auto tail_mask = betweenMasks().tail(ki_pos, from);
    if(dir == nst::bishop)
    {
      auto const bi_mask = fmgr_.bishop_mask(ocolor);
      auto const q_mask = fmgr_.queen_mask(ocolor);
      auto bq_mask = (bi_mask | q_mask) & mask_all_t & tail_mask;
      if (!bq_mask)
        return false;
      auto bi_moves = magic_ns::bishop_moves(ki_pos, mask_all_f);
      return (bi_moves & bq_mask) != 0ULL;
    }
    X_ASSERT(dir != nst::rook, "invalid direction from point to point");
    auto const r_mask = fmgr_.rook_mask(ocolor);
    auto const q_mask = fmgr_.queen_mask(ocolor);
    auto rq_mask = (r_mask | q_mask) & mask_all_t & tail_mask;
    if (!rq_mask)
      return false;
    auto r_moves = magic_ns::rook_moves(ki_pos, mask_all_f);
    return (r_moves & rq_mask) != 0ULL;
  }

  inline bool is_usual_check() const
  {
    if (usual_check_detected_)
      return usual_check_;
    usual_check_detected_ = true;
    return usual_check_ = detect_usual_check();
  }

  inline bool detect_usual_check() const
  {
#ifndef NDEBUG
    SBoard<Board, UndoInfo, Board::GameLength> sbrd{ board_ };
    sbrd.makeMove(move_);
    bool check_detected = sbrd.underCheck();
    auto fen = toFEN(board_);
#endif
    auto ftype = board_.getField(move_.from()).type();
    if(ftype == Figure::TypeKing)
      return false;
    auto to = move_.to();
    if(ftype == Figure::TypePawn)
    {
      if(!move_.new_type())
      {
        bool result = (movesTable().pawnCaps(color_, to) & board_.fmgr().king_mask(ocolor_)) != 0ULL;
        X_ASSERT(check_detected != result, "invalid check detected");
        return result;
      }
      ftype = (Figure::Type)move_.new_type();
    }
    if(ftype == Figure::TypeKnight)
    {
      bool result = (movesTable().caps(Figure::TypeKnight, to) & board_.fmgr().king_mask(ocolor_)) != 0ULL;
      X_ASSERT(check_detected != result, "invalid check detected");
      return result;
    }
    int ok_pos = board_.kingPos(ocolor_);
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

bool Board::see(const Move move, int threshold) const
{
  X_ASSERT(data_.state_ == Invalid, "try to SEE invalid board");

  const auto ffield = getField(move.from());
  const auto tfield = getField(move.to());
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

  SeeCalc see_calc(*this, move, ffield, en_passant);

  // assume discovered check is always good
  if(see_calc.discovered_check_)
    return true;

  // could be checkmate
  if(threshold > 0 && see_calc.is_usual_check())
    threshold = 0;

  if(en_passant)
    return Figure::figureWeight_[Figure::TypePawn] >= threshold;

  int score_gain = 0;
  if(tfield)
    score_gain = Figure::figureWeight_[tfield.type()];
  int fscore = -Figure::figureWeight_[ffield.type()];
  if (see_calc.promotion_)
  {
    score_gain += Figure::figureWeight_[Figure::TypeQueen] - Figure::figureWeight_[Figure::TypePawn];
    fscore = -Figure::figureWeight_[Figure::TypeQueen];
  }
  if(score_gain < threshold)
    return false;

  auto color = see_calc.ocolor_;
  see_calc.setupMovingFigures();
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
    if (color != see_calc.color_)
    {
      // already winner
      if (score_gain >= threshold)
        return true;
      // loose even if take last moved figure
      if (score_gain + score < threshold)
        return false;
    }
    else
    {
      // already winner even if loose last moved figure
      if (score_gain + score >= threshold)
        return true;
      // could not gain something
      if (score_gain < threshold)
        return false;
    }
    fscore = score;
    color = ocolor;
  }

  return score_gain >= threshold;
}


} // NEngine
