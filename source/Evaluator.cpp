/*************************************************************
  Evaluator.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Evaluator.h>
#include <Board.h>
#include <HashTable.h>
#include <xindex.h>
#include <magicbb.h>
#include <xbitmath.h>
#include <Helpers.h>

namespace NEngine
{

const int Evaluator::colored_y_[2][8] = {
  { 7, 6, 5, 4, 3, 2, 1, 0 },
  { 0, 1, 2, 3, 4, 5, 6, 7 } };

const BitMask Evaluator::castle_mask_[2][2] = {
  {
    set_mask_bit(F8) | set_mask_bit(G8) | set_mask_bit(H8) |
    set_mask_bit(F7) | set_mask_bit(G7) | set_mask_bit(H7),

    set_mask_bit(A8) | set_mask_bit(B8) | set_mask_bit(C8) |
    set_mask_bit(A7) | set_mask_bit(B7) | set_mask_bit(C7)
  },

  {
    set_mask_bit(F1) | set_mask_bit(G1) | set_mask_bit(H1) |
    set_mask_bit(F2) | set_mask_bit(G2) | set_mask_bit(H2),

    set_mask_bit(A1) | set_mask_bit(B1) | set_mask_bit(C1) |
    set_mask_bit(A2) | set_mask_bit(B2) | set_mask_bit(C2)
  }
};

const BitMask Evaluator::king_attack_mask_[2][2] = {
  {
    set_mask_bit(F8) | set_mask_bit(G8) | set_mask_bit(H8) |
    set_mask_bit(F7) | set_mask_bit(G7) | set_mask_bit(H7) |
    set_mask_bit(F6) | set_mask_bit(G6) | set_mask_bit(H6),

    set_mask_bit(A8) | set_mask_bit(B8) | set_mask_bit(C8) |
    set_mask_bit(A7) | set_mask_bit(B7) | set_mask_bit(C7) |
    set_mask_bit(A6) | set_mask_bit(B6) | set_mask_bit(C6)
  },

  {
    set_mask_bit(F1) | set_mask_bit(G1) | set_mask_bit(H1) |
    set_mask_bit(F2) | set_mask_bit(G2) | set_mask_bit(H2) |
    set_mask_bit(F3) | set_mask_bit(G3) | set_mask_bit(H3),

    set_mask_bit(A1) | set_mask_bit(B1) | set_mask_bit(C1) |
    set_mask_bit(A2) | set_mask_bit(B2) | set_mask_bit(C2) |
    set_mask_bit(A3) | set_mask_bit(B3) | set_mask_bit(C3)
  }
};

void Evaluator::initialize(Board const* board)
{
  board_ = board;
}

void Evaluator::prepare()
{
  mask_all_ = board_->fmgr().mask(Figure::ColorWhite) | board_->fmgr().mask(Figure::ColorBlack);
  inv_mask_all_ = ~mask_all_;

  finfo_[0] = FieldsInfo{};
  finfo_[1] = FieldsInfo{};

  // pawns attacks
  {
    const BitMask & pawn_msk_w = board_->fmgr().pawn_mask(Figure::ColorWhite);
    const BitMask & pawn_msk_b = board_->fmgr().pawn_mask(Figure::ColorBlack);

    finfo_[Figure::ColorWhite].pawnAttacks_ = ((pawn_msk_w << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_w << 7) & Figure::pawnCutoffMasks_[1]);
    finfo_[Figure::ColorBlack].pawnAttacks_ = ((pawn_msk_b >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_b >> 9) & Figure::pawnCutoffMasks_[1]);

    finfo_[0].attack_mask_ = finfo_[0].pawnAttacks_;
    finfo_[1].attack_mask_ = finfo_[1].pawnAttacks_;
  }

  // attacked fields near king
  {
    finfo_[0].kingAttacks_ = movesTable().caps(Figure::TypeKing, board_->kingPos(Figure::ColorBlack));
    finfo_[1].kingAttacks_ = movesTable().caps(Figure::TypeKing, board_->kingPos(Figure::ColorWhite));

    finfo_[0].multiattack_mask_ = finfo_[0].attack_mask_ & finfo_[0].kingAttacks_;
    finfo_[1].multiattack_mask_ = finfo_[1].attack_mask_ & finfo_[1].kingAttacks_;

    finfo_[0].attack_mask_ |= finfo_[0].kingAttacks_;
    finfo_[1].attack_mask_ |= finfo_[1].kingAttacks_;
  }
}

//////////////////////////////////////////////////////////////////////////
ScoreType Evaluator::operator () (ScoreType alpha, ScoreType betta)
{
  X_ASSERT(!board_, "Evaluator wasn't properly initialized");

#if 0

  if(board_->matState())
    return -Figure::MatScore;
  else if(board_->drawState())
    return Figure::DrawScore;

  return considerColor(board_->fmgr().weight());

#else

  if(!ehash_.empty())
    ehash_.prefetch(board_->fmgr().kpwnCode());

  if(board_->matState())
    return -Figure::MatScore;
  else if(board_->drawState())
    return Figure::DrawScore;

  ScoreType score = evaluate(alpha, betta);
  X_ASSERT(score <= -ScoreMax || score >= ScoreMax, "invalid score");
  return score;

#endif
}

ScoreType Evaluator::evaluate(ScoreType alpha, ScoreType betta)
{
#ifndef NDEBUG
  std::string sfen = toFEN(*board_);
#endif

#ifdef EVAL_SPECC
  {
    auto spec = specialCases().eval(*board_);
    if (spec.first)
    {
      ScoreType score = spec.second;
      score = considerColor(score);
      return score;
    }
  }
#endif

  // prepare lazy evaluation
  if(alpha > -Figure::MatScore)
  {
    alpha0_ = (int)alpha - lazyThreshold0_;
    alpha1_ = (int)alpha - lazyThreshold1_;
  }
  else
  {
    alpha0_ = -ScoreMax;
    alpha1_ = -ScoreMax;
  }

  if(betta < +Figure::MatScore)
  {
    betta0_ = (int)betta + lazyThreshold0_;
    betta1_ = (int)betta + lazyThreshold1_;
  }
  else
  {
    betta0_ = +ScoreMax;
    betta1_ = +ScoreMax;
  }

  const FiguresManager& fmgr = board_->fmgr();
  FullScore score;

  // evaluate figures weight
  score.common_ = fmgr.weight()
#ifdef EVAL_MATD
    + evaluateMaterialDiff()
#endif
    ;

  /// use lazy evaluation level 0
  {
    auto score0 = considerColor(score.common_);
    if(score0 < alpha0_ || score0 > betta0_)
      return score0;
  }

  prepare();

  // determine game phase (opening, middle or end game)
  auto phaseInfo = detectPhase();

  // take pawns eval from hash if possible
  auto hashedScore = hashedEvaluation();
  score += hashedScore.score;

  /// use lazy evaluation level 1
  {
    auto score1 = considerColor(lipolScore(score, phaseInfo));
    if(score1 < alpha1_ || score1 > betta1_)
      return score1;
  }

  // penalty for lost or fake castle
#ifdef EVAL_CASTLE
  score.opening_ += evaluateCastle();
#endif

  score.opening_ += fmgr.eval(0);
  score.endGame_ += fmgr.eval(1);

  score += evaluateKnights();

#ifdef EVAL_FORKS
  auto scoreForks = evaluateForks(Figure::ColorWhite);
  scoreForks -= evaluateForks(Figure::ColorBlack);
  score.common_ += scoreForks;
#endif
  
  score += evaluateBishops();

  score += evaluateRook(Figure::ColorWhite);
  score -= evaluateRook(Figure::ColorBlack);

  score += evaluateQueens(Figure::ColorWhite);
  score -= evaluateQueens(Figure::ColorBlack);

#if (defined EVAL_MOB || defined EVAL_KING_PR)
  auto mobKpScore = evaluateMobilityAndKingPressure(Figure::ColorWhite);
  mobKpScore -= evaluateMobilityAndKingPressure(Figure::ColorBlack);
  score += mobKpScore;
#endif

#ifdef EVAL_PAWN_PR
  auto scorePP = evaluatePawnsPressure(Figure::ColorWhite);
  scorePP -= evaluatePawnsPressure(Figure::ColorBlack);
  score += scorePP;
#endif

#ifdef EVAL_GP
  auto scoreGP = evaluateGeneralPressure(Figure::ColorWhite);
  scoreGP -= evaluateGeneralPressure(Figure::ColorBlack);
  score += scoreGP;
#endif

#ifdef EVAL_PASSERS
  auto scorePassers = passerEvaluation(hashedScore);
  score += scorePassers;
#endif

  auto result = considerColor(lipolScore(score, phaseInfo));
  return result;
}

Evaluator::PhaseInfo Evaluator::detectPhase() const
{
  const FiguresManager & fmgr = board_->fmgr();

  auto wei = fmgr.weight(Figure::ColorBlack) + fmgr.weight(Figure::ColorWhite);

  PhaseInfo phaseInfo;
  phaseInfo.phase_ = MiddleGame;

  if(wei >= openingWeight_)
  {
    phaseInfo.phase_ = Opening;
  }
  else if(wei <= endgameWeight_)
  {
    phaseInfo.phase_ = EndGame;
  }

  phaseInfo.opening_ = wei - endgameWeight_;
  if(phaseInfo.opening_ > weightOEDiff_)
    phaseInfo.opening_ = weightOEDiff_;
  else if(phaseInfo.opening_ < 0)
    phaseInfo.opening_ = 0;
  phaseInfo.endGame_ = weightOEDiff_ - phaseInfo.opening_;

  return phaseInfo;
}

Evaluator::FullScore Evaluator::evaluatePawnsPressure(Figure::Color color)
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  auto const ocolor = Figure::otherColor(color);
  auto const& pw_mask = fmgr.pawn_mask(ocolor);
  auto pw_protected = pw_mask & finfo_[ocolor].pawnAttacks_;
  auto pw_unprotected = pw_mask ^ pw_protected;
  auto attackers = finfo_[color].attack_mask_ & ~finfo_[color].pawnAttacks_;
  score.common_ = pop_count(pw_protected & attackers) * EvalCoefficients::protectedPawnPressure_;
  score.common_ += pop_count(pw_unprotected & attackers) * EvalCoefficients::unprotectedPawnPressure_;
  // bishop treat
  if(fmgr.bishops(color) == 1)
  {
    auto bi_mask = (fmgr.bishop_mask(color) & FiguresCounter::s_whiteMask_)
      ?  FiguresCounter::s_whiteMask_
      : ~FiguresCounter::s_whiteMask_;
    score.endGame_ += pop_count(pw_protected   & bi_mask) * EvalCoefficients::protectedPawnBishopTreat_;
    score.endGame_ += pop_count(pw_unprotected & bi_mask) * EvalCoefficients::unprotectedPawnBishopTreat_;
  }
  return score;
}

Evaluator::FullScore Evaluator::evaluateGeneralPressure(Figure::Color color)
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  auto const ocolor = Figure::otherColor(color);
  auto king_left = (board_->kingPos(ocolor) & 7) < 4;
  auto attacks_king  = finfo_[color].attack_mask_ & Figure::quaterBoard_[ocolor][king_left];
  auto attacks_other = finfo_[color].attack_mask_ & Figure::quaterBoard_[ocolor][!king_left];
  score.opening_ += pop_count(attacks_other) * EvalCoefficients::generalPressure_
    + pop_count(attacks_king) * EvalCoefficients::kingPressure_;
  return score;
}

Evaluator::PasserInfo Evaluator::hashedEvaluation()
{
  HEval * heval = 0;
  const uint64 & code = board_->fmgr().kpwnCode();
  uint32 hkey = (uint32)(code >> 32);

  if(!ehash_.empty())
  {
    heval = ehash_.get(code);

    if(heval->hkey_ == hkey && heval->initizalized_)
    {
      PasserInfo info;
      info.score.common_ = heval->common_;
      info.score.opening_ = heval->opening_;
      info.score.endGame_ = heval->endGame_;
      X_ASSERT(!(info.score == evaluatePawns().score + evaluateKingSafety()), "invalid pawns+king score in hash");
      return info;
    }
  }

  PasserInfo info;

#ifdef EVAL_PAWNS
  info = evaluatePawns();
#endif

  // basic king safety - pawn shield
#ifdef EVAL_KINGSF
  auto kingScore = evaluateKingSafety();
  info.score += kingScore;
#endif

  if(heval)
  {
    heval->hkey_ = hkey;
    heval->common_ = info.score.common_;
    heval->opening_ = info.score.opening_;
    heval->endGame_ = info.score.endGame_;
    heval->initizalized_ = 1;
  }

  return info;
}

// TODO: optimization required
int Evaluator::closestToBackward(int x, int y, const BitMask & pmask, Figure::Color color) const
{
  BitMask left  = (x != 0) ? (pawnMasks().mask_line_blocked(color, Index(x-1, y)) & pmask) : 0ULL;
  BitMask right = (x != 7) ? (pawnMasks().mask_line_blocked(color, Index(x+1, y)) & pmask) : 0ULL;
  int y_closest = y;
  if(color)
  {
    y_closest = 7;
    if(left)
      y_closest = (_lsb64(left) >> 3);
    if(right)
      y_closest = std::min(y_closest, ((_lsb64(right) >> 3)));
  }
  else
  {
    y_closest = 0;
    if(left)
      y_closest = (_msb64(left) >> 3);
    if(right)
      y_closest = std::max(y_closest, ((_msb64(right) >> 3)));
  }
  return y_closest;
}

bool Evaluator::couldBeSupported(Index const& idx, Figure::Color color, Figure::Color ocolor, BitMask const& pmask, BitMask const& opmsk) const
{
  if(pawnMasks().mask_supported(color, idx) & pmask)
    return true;  
  BitMask blockers_mask = (finfo_[ocolor].pawnAttacks_ & (~finfo_[color].pawnAttacks_)) | opmsk;
  BitMask left  = (idx.x() != 0) ? (pawnMasks().mask_line_blocked(ocolor, idx - 1) & pmask) : 0ULL;
  if(left)
  {
    int y = color ? (_msb64(left) >> 3) : (_lsb64(left) >> 3);
    BitMask pmask_before = betweenMasks().between(idx-1, Index(idx.x()-1, y));
    if((pmask_before & blockers_mask) == 0ULL)
      return true;
  }
  BitMask right = (idx.x() != 7) ? (pawnMasks().mask_line_blocked(ocolor, idx + 1) & pmask) : 0ULL;
  if(right)
  {
    int y = color ? (_msb64(right) >> 3) : (_lsb64(right) >> 3);
    BitMask pmask_before = betweenMasks().between(idx+1, Index(idx.x()+1, y));
    if((pmask_before & blockers_mask) == 0ULL)
      return true;
  }
  return false;
}

bool Evaluator::isPasserCandidate(Index const& idx, int cy, int n, Figure::Color color, BitMask const& pmask, BitMask const& opmsk) const
{
  static int delta_y[] = { -1, +1 };

  if ((pawnMasks().mask_line_blocked(color, n) & opmsk) != 0ULL)
    return false;

  int dy = delta_y[color];
  auto n1 = n + (dy << 3);
  auto fwd_field = set_mask_bit(n1);

  auto ocolor = Figure::otherColor(color);
  auto x = idx.x();
  bool is_candidate = false;
  bool be_captured = false;
  int guards_n = 0;
  int attackets_n = 0;
  if (board_->color() != color)
  {
    auto guards = movesTable().pawnCaps(ocolor, n) & pmask;
    auto attackers = movesTable().pawnCaps(color, n) & opmsk;
    guards_n = pop_count(guards);
    attackets_n = pop_count(attackers);
  }
  if (attackets_n <= guards_n)
  {
    ///
    ///   **
    ///   .
    ///    @
    ///
    auto guards = movesTable().pawnCaps(ocolor, n1) & pmask;
    auto attackers = movesTable().pawnCaps(color, n1) & opmsk;
    auto guards_n1 = pop_count(guards);
    auto attackers_n1 = pop_count(attackers);
    if (attackers_n1 <= guards_n1)
    {
      is_candidate = (pawnMasks().mask_passed(color, n1) & opmsk) == 0ULL;
    }
    ///
    ///  *
    ///  . *
    ///    @
    else if((attackers_n1 == guards_n1+1) && (cy < 5))
    {
      int n2 = n1 + (dy << 3);
      auto pmask_x = pmask & ~set_mask_bit(n);
      is_candidate = (x < 6 && ((set_mask_bit(n1 + 1) & pmask) != 0ULL) && ((pawnMasks().mask_passed(color, n2 + 1) & opmsk) == 0ULL) && 
          ((set_mask_bit(n2 + 1) & opmsk) != 0ULL) && ((pawnMasks().mask_passed(ocolor, n1 + 1) & pmask_x) != 0ULL || cy > 2)) ||
        (x > 1 && ((set_mask_bit(n1 - 1) & pmask) != 0ULL) && ((pawnMasks().mask_passed(color, n2 - 1) & opmsk) == 0ULL) &&
          ((set_mask_bit(n2 - 1) & opmsk) != 0ULL) && ((pawnMasks().mask_passed(ocolor, n1 - 1) & pmask_x) != 0ULL || cy > 2));
    }
  }
  else if ((attackets_n == guards_n+1) && (cy < 6))
  {
    ///
    ///   **
    ///   .@
    ///
    auto pmask_x = pmask & ~set_mask_bit(n);
    is_candidate = (x < 6 && ((set_mask_bit(n+1) & pmask) != 0ULL) && ((pawnMasks().mask_passed(color, n1+1) & opmsk) == 0ULL) &&
        ((set_mask_bit(n1 + 1) & opmsk) != 0ULL) && ((pawnMasks().mask_passed(ocolor, n + 1) & pmask_x) != 0ULL || cy > 3)) ||
      (x > 1 && ((set_mask_bit(n-1) & pmask) != 0ULL) && ((pawnMasks().mask_passed(color, n1-1) & opmsk) == 0ULL) &&
        ((set_mask_bit(n1 - 1) & opmsk) != 0ULL) && ((pawnMasks().mask_passed(ocolor, n - 1) & pmask_x) != 0ULL || cy > 3));
  }
  if (is_candidate)
    return true;

  ///
  ///   *
  /// * . *
  /// @   @
  if ((cy < 5 && cy > 3) && (x > 0 && x < 6))
  {
    auto lr_candidates = movesTable().pawnCaps(color, n) & pmask;
    if (pop_count(lr_candidates) == 2)
    {
      if (color == Figure::ColorWhite)
        lr_candidates <<= 8;
      else
        lr_candidates >>= 8;
      int n2 = n1 + (dy << 3);
      auto left_passer = pawnMasks().mask_passed(color, n2 - 1) & opmsk;
      auto right_passer = pawnMasks().mask_passed(color, n2 + 1) & opmsk;
      is_candidate = left_passer == 0ULL && right_passer == 0ULL;
    }
  }
  if (is_candidate)
    return true;
  return false;
}

Evaluator::PasserInfo Evaluator::evaluatePawns(Figure::Color color) const
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmask = fmgr.pawn_mask(color);
  if(!pmask)
    return{};

  PasserInfo info;
  info.score.endGame_ = fmgr.pawns(color) * EvalCoefficients::pawnEndgameBonus_;

  static int promo_y[] = { 0, 7 };

  Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & opmsk = fmgr.pawn_mask(ocolor);
  bool no_opawns = opmsk == 0ULL;

  uint8 x_visited{ 0 };
  uint8 x_passers{ 0 };

  BitMask pawn_mask = pmask;
  for(; pawn_mask;)
  {
    int const n = clear_lsb(pawn_mask);
    Index idx(n);

    int x  = idx.x();
    int cy = colored_y_[color][idx.y()];
    int py = promo_y[color];

    int dist_to_oking = distanceCounter().getDistance(board_->kingPos(ocolor), n);
    int dist_to_myking = distanceCounter().getDistance(board_->kingPos(color), n);

    info.score.endGame_ -= EvalCoefficients::kingToPawnBonus_[dist_to_oking];
    info.score.endGame_ += EvalCoefficients::kingToPawnBonus_[dist_to_myking];

    // doubled pawn
    if(!(x_visited & (1<<x)))
    {
      x_visited |= 1<<x;
      auto const& dblmsk = pawnMasks().mask_doubled(x);
      auto num_dbl = pop_count(pmask & dblmsk);
      X_ASSERT(num_dbl <= 0, "doubled pawns number is zero or negative");
      info.score.common_ += (num_dbl - 1) * EvalCoefficients::doubledPawn_;
    }

    // isolated pawn
    bool unsupported{ false };
    bool isolated = (pmask & pawnMasks().mask_isolated(x)) == 0ULL;
    info.score.common_ += isolated * EvalCoefficients::isolatedPawn_;

    if(!isolated)
    {
      // backward
      if(((pawnMasks().mask_backward(color, n) & pmask) == 0ULL))
      {
        // TODO: optimization required
        int closest_y = closestToBackward(idx.x(), idx.y(), pmask, color);
        X_ASSERT(closest_y < 0 || closest_y > 7, "backward mask calculation error - invalid next y position");
        BitMask pmask_after = betweenMasks().between(n, Index(idx.x(), closest_y));
        bool blocked  = ((finfo_[ocolor].pawnAttacks_ | opmsk) & pmask_after) != 0ULL;
        info.score.common_ += blocked * EvalCoefficients::backwardPawn_;
      }
      // forward but not supported
      else if(!couldBeSupported(idx, color, ocolor, pmask, opmsk))
      {
        unsupported = true;
        info.score.common_ += EvalCoefficients::unsupportedPawn_;
      }
      // unprotected
      else
      {
        bool unprotected = (pawnMasks().mask_supported(color, n) & pmask) == 0ULL;
        info.score.common_ += unprotected * EvalCoefficients::unprotectedPawn_;
      }
    }

    // passed pawn
    {
      if((opmsk & pawnMasks().mask_passed(color, n)) == 0ULL)
      {
        info.passers_[color] |= 1ULL << n;
        x_passers |= 1 << x;
        info.score.common_ += EvalCoefficients::passerPawn_[cy];
        // supported
        if(pawnMasks().mask_supported(color, n) & pmask)
          info.score.common_ += EvalCoefficients::protectedPasser_[cy];
        Index pp(x, py);
        int pawn_dist_promo = std::abs(py - idx.y());
        int o_dist_promo = distanceCounter().getDistance(board_->kingPos(ocolor), pp) - (board_->color() == ocolor);
        if(pawn_dist_promo < o_dist_promo)
          info.score.endGame_ += EvalCoefficients::farKingPawn_[cy] >> 1;
      }
      //else if(isPasserCandidate(idx, cy, n, color, pmask, opmsk))
      //{
      //  info.score.common_ += EvalCoefficients::passerCandidatePawn_[cy];
      //}
      // quadpasser
      else if((pawnMasks().mask_line_blocked(color, n) & opmsk) == 0ULL)
      {
        bool left = (x != 0) && ((pawnMasks().mask_line_blocked(color, Index(x-1, idx.y())) & opmsk)== 0ULL);
        bool right = (x != 7) && ((pawnMasks().mask_line_blocked(color, Index(x+1, idx.y())) & opmsk) == 0ULL);
        X_ASSERT(((left && right) || (x == 0 && right) || (x == 7 && left)), "passed pawn was not detected");
        auto const& semipasserCoeff = EvalCoefficients::semipasserPawn_[cy];
        auto scoreSemipasser = ((semipasserCoeff >> 1) + (left || right) * (semipasserCoeff >> 1));
        if(pawnMasks().mask_supported(color, n) & pmask)
          scoreSemipasser += EvalCoefficients::protectedPasser_[cy] >> 2;
        info.score.common_ += scoreSemipasser;
      }
    }
  }

  //// 2 more passers on neighbour lines
  //int num_passers = pawnMasks().count_multi_passer(x_passers);
  //if(num_passers > 1)
  //{
  //  auto mpassers = pmask & pawnMasks().mask_multi_passer(x_passers);
  //  int ymax = 0;
  //  while(mpassers)
  //  {
  //    int n = clear_lsb(mpassers);
  //    int cy = colored_y_[color][Index(n).y()];
  //    if(ymax < cy)
  //      ymax = cy;
  //  }
  //  X_ASSERT(ymax > 6, "invalid multi-pass y");
  //  auto s = EvalCoefficients::multipasserPawn_[ymax] * (num_passers -1);
  //  info.score.common_ += s;
  //}

  return info;
}

Evaluator::FullScore Evaluator::passerEvaluation(PasserInfo const& pi) const
{
  auto infoW = passerEvaluation(Figure::ColorWhite, pi);
  auto infoB = passerEvaluation(Figure::ColorBlack, pi);

  if(infoW.most_y > infoB.most_y)
    infoW.score.endGame_ += EvalCoefficients::closeToPromotion_[infoW.most_y - infoB.most_y];
  else if(infoB.most_y > infoW.most_y)
    infoB.score.endGame_ += EvalCoefficients::closeToPromotion_[infoB.most_y - infoW.most_y];

  infoW.score -= infoB.score;
  return infoW.score;
}

Evaluator::PasserInfo Evaluator::passerEvaluation(Figure::Color color, PasserInfo const& pi) const
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmask = fmgr.pawn_mask(color);
  if(!pmask || (pi.searched_passers_[color] && !pi.passers_[color]))
    return{};

  static int delta_y[] = { -1, 1 };
  static int promo_y[] = {  0, 7 };
    
  int py = promo_y[color];
  int dy = delta_y[color];
  
  nst::dirs dir_behind[] = {nst::no, nst::so};

  PasserInfo info;

  Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & opmsk = fmgr.pawn_mask(ocolor);
  bool no_opawns = opmsk == 0ULL;
  BitMask blockers_mask = (~finfo_[color].attack_mask_ & finfo_[ocolor].attack_mask_) | fmgr.mask(color) | fmgr.mask(ocolor);

  BitMask pawn_mask = pmask;
  if(pi.searched_passers_[color])
    pawn_mask = pi.passers_[color];
  for(; pawn_mask;)
  {
    const int n = clear_lsb(pawn_mask);
    Index idx(n);

    int x = idx.x();
    int y = idx.y();
    int cy = colored_y_[color][idx.y()];

    const uint64 & passmsk = pawnMasks().mask_passed(color, n);
    const uint64 & blckmsk = pawnMasks().mask_line_blocked(color, n);
    
    if((opmsk & passmsk) || (pmask & blckmsk))
      continue;

    if(cy > info.most_y)
      info.most_y = cy;

    // my rook behind passed pawn - give bonus
    BitMask behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & fmgr.rook_mask(color) ;
    BitMask o_behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & fmgr.rook_mask(ocolor) ;
    if ( behind_msk )
    {
      int rpos = color ? _msb64(behind_msk) : _lsb64(behind_msk);
      if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
        info.score.common_ += EvalCoefficients::rookBehindBonus_;
    }
    else if ( o_behind_msk ) // may be opponent's rook - give penalty
    {
      int rpos = color ? _msb64(o_behind_msk) : _lsb64(o_behind_msk);
      if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
        info.score.common_ -= EvalCoefficients::rookBehindBonus_;
    }
    
    // pawn can go to the next line
    int next_pos = x | ((y+dy)<<3);
    X_ASSERT( ((y+dy)) > 7 || ((y+dy)) < 0, "pawn goes to invalid line" );
    
    // field is empty
    // check is it attacked by opponent
    auto const& next_field = board_->getField(next_pos);
    if(!next_field)
    {
      BitMask next_mask = set_mask_bit(next_pos);
      // additional bonus if pawn can go forward
      if((next_mask & finfo_[ocolor].attack_mask_) == 0)
        info.score.common_ += EvalCoefficients::cangoPawn_[cy];
    }
    // field is occupied by my figure
    else if(next_field.color() == color)
    {
      // small additional bonus if pawn potentially can go forward
      info.score.common_ += EvalCoefficients::cangoPawn_[cy] >> 1;
    }

    // all ahead fields are not occupied and not attacked | protected
    auto fwd_mask = pawnMasks().mask_line_blocked(color, n);
    bool can_promote = (fwd_mask & blockers_mask) == 0ULL;
    info.score.common_ += can_promote * EvalCoefficients::canpromotePawn_[cy];

    // opponent king could not go to my pawns promotion field
    int promo_pos = x | (py<<3);
    int pawn_dist_promo = std::abs(py - y);
    if(!findRootToPawn(color, promo_pos, pawn_dist_promo+1))
    {
      info.score.endGame_ += EvalCoefficients::farKingPawn_[cy];
    }
  }
  info.most_y += (color == board_->color());
  return info;
}

int Evaluator::getCastleType(Figure::Color color) const
{
  auto const& ki_mask = board_->fmgr().king_mask(color);

  // short
  bool cking = (castle_mask_[color][0] & ki_mask) != 0;

  // long
  bool cqueen = (castle_mask_[color][1] & ki_mask) != 0;

  // -1 == no castle
  return (!cking && !cqueen) * (-1) + cqueen;
}

int Evaluator::evaluateKingSafety(Figure::Color color) const
{
  if(board_->castling(color))
    return 0;

  const FiguresManager & fmgr = board_->fmgr();
  auto const& pmask  = fmgr.pawn_mask(color);

  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
  auto const& opmask = fmgr.pawn_mask(ocolor);

  // pawns shield
  static const BitMask pawns_shield_mask[2][2][3][2] =
  {
    // black
    {
      // short
      {
        { set_mask_bit(H7), set_mask_bit(H6) },
        { set_mask_bit(G7), set_mask_bit(G6) },
        { set_mask_bit(F7), set_mask_bit(F6) }
      },

      // long
      {
        { set_mask_bit(A7), set_mask_bit(A6) },
        { set_mask_bit(B7), set_mask_bit(B6) },
        { set_mask_bit(C7), set_mask_bit(C6) }
      }
    },

    // white
    {
      // short
      {
        { set_mask_bit(H2), set_mask_bit(H3) },
        { set_mask_bit(G2), set_mask_bit(G3) },
        { set_mask_bit(F2), set_mask_bit(F3) }
      },

      // long
      {
        { set_mask_bit(A2), set_mask_bit(A3) },
        { set_mask_bit(B2), set_mask_bit(B3) },
        { set_mask_bit(C2), set_mask_bit(C3) }
      }
    }
  };

  static const BitMask pawns_penalty_mask[2][2][3] =
  {
    // black
    {
      // short
      {
        set_mask_bit(H7)|set_mask_bit(H6)|set_mask_bit(H5),
        set_mask_bit(G7)|set_mask_bit(G6)|set_mask_bit(G5),
        set_mask_bit(F7)|set_mask_bit(F6)|set_mask_bit(F5)
      },

      // long
      {
        set_mask_bit(A7)|set_mask_bit(A6)|set_mask_bit(A5),
        set_mask_bit(B7)|set_mask_bit(B6)|set_mask_bit(B5),
        set_mask_bit(C7)|set_mask_bit(C6)|set_mask_bit(C5)
      }
    },
    // white
    {
      // short
      {
        set_mask_bit(H4)|set_mask_bit(H3)|set_mask_bit(H2),
        set_mask_bit(G4)|set_mask_bit(G3)|set_mask_bit(G2),
        set_mask_bit(F4)|set_mask_bit(F3)|set_mask_bit(F2)
      },

      // long
      {
        set_mask_bit(A4)|set_mask_bit(A3)|set_mask_bit(A2),
        set_mask_bit(B4)|set_mask_bit(B3)|set_mask_bit(B2),
        set_mask_bit(C4)|set_mask_bit(C3)|set_mask_bit(C2)
      }
    }
  };

  static const BitMask no_pawns_penalty_mask[2][3] =
  {
    // short
    {
      set_mask_bit(H7)|set_mask_bit(H6)|set_mask_bit(H5)|set_mask_bit(H4)|set_mask_bit(H3)|set_mask_bit(H2),
      set_mask_bit(G7)|set_mask_bit(G6)|set_mask_bit(G5)|set_mask_bit(G4)|set_mask_bit(G3)|set_mask_bit(G2),
      set_mask_bit(F7)|set_mask_bit(F6)|set_mask_bit(F5)|set_mask_bit(F4)|set_mask_bit(F3)|set_mask_bit(F2)
    },

    // long
    {
      set_mask_bit(A7)|set_mask_bit(A6)|set_mask_bit(A5)|set_mask_bit(A4)|set_mask_bit(A3)|set_mask_bit(A2),
      set_mask_bit(B7)|set_mask_bit(B6)|set_mask_bit(B5)|set_mask_bit(B4)|set_mask_bit(B3)|set_mask_bit(B2),
      set_mask_bit(C7)|set_mask_bit(C6)|set_mask_bit(C5)|set_mask_bit(C4)|set_mask_bit(C3)|set_mask_bit(C2)
    }
  };

  int score = 0;
  auto ctype = getCastleType(color);
  if(ctype >= 0)
  {
    int shield_bonus = 0;
    if(pawns_shield_mask[color][ctype][0][0] & pmask)
      shield_bonus += EvalCoefficients::pawnShieldA_[0];
    else if(pawns_shield_mask[color][ctype][0][1] & pmask)
      shield_bonus += EvalCoefficients::pawnShieldA_[1];
    else
      shield_bonus += EvalCoefficients::pawnPenaltyA_;

    if(pawns_shield_mask[color][ctype][1][0] & pmask)
      shield_bonus += EvalCoefficients::pawnShieldB_[0];
    else if(pawns_shield_mask[color][ctype][1][1] & pmask)
      shield_bonus += EvalCoefficients::pawnShieldB_[1];
    else
      shield_bonus += EvalCoefficients::pawnPenaltyB_ >> 1;

    if(pawns_shield_mask[color][ctype][2][0] & pmask)
      shield_bonus += EvalCoefficients::pawnShieldC_[0];
    else if(pawns_shield_mask[color][ctype][2][1] & pmask)
      shield_bonus += EvalCoefficients::pawnShieldC_[1];
    else
      shield_bonus += EvalCoefficients::pawnPenaltyC_ >> 1;

    int shield_penalty =
        ((pawns_penalty_mask[color][ctype][0] & pmask) == 0ULL) * EvalCoefficients::pawnPenaltyA_
      + ((pawns_penalty_mask[color][ctype][1] & pmask) == 0ULL) * EvalCoefficients::pawnPenaltyB_
      + ((pawns_penalty_mask[color][ctype][2] & pmask) == 0ULL) * EvalCoefficients::pawnPenaltyC_;

    shield_penalty +=
        ((no_pawns_penalty_mask[ctype][0] & pmask) == 0ULL) * EvalCoefficients::noPawnPenaltyA_
      + ((no_pawns_penalty_mask[ctype][1] & pmask) == 0ULL) * EvalCoefficients::noPawnPenaltyB_
      + ((no_pawns_penalty_mask[ctype][2] & pmask) == 0ULL) * EvalCoefficients::noPawnPenaltyC_;

    score += shield_bonus;
    score += shield_penalty;
  }
  else
  {
    static const int delta_y[2] = { -8, 8 };
    score = EvalCoefficients::castleImpossible_;

    int above1 = (board_->kingPos(color) + delta_y[color]) & 63;
    int above2 = (above1 + delta_y[color]) & 63;
    BitMask mabove1{ set_mask_bit(board_->kingPos(color)) | set_mask_bit(above1) };
    BitMask mabove2{ set_mask_bit(above2) };
    auto mleft1  = (mabove1 >> 1) & Figure::pawnCutoffMasks_[1];
    auto mright1 = (mabove1 << 1) & Figure::pawnCutoffMasks_[0];
    auto mleft2  = (mabove2 >> 1) & Figure::pawnCutoffMasks_[1];
    auto mright2 = (mabove2 << 1) & Figure::pawnCutoffMasks_[0];

    if(mabove1 & pmask)
      score += EvalCoefficients::pawnShieldB_[0] >> 1;
    else if(mabove2 & pmask)
      score += EvalCoefficients::pawnShieldB_[1] >> 1;
    else
      score += EvalCoefficients::pawnPenaltyB_;

    if((board_->kingPos(color) & 7) > 3) // right side
    {
      if(mleft1 & pmask)
        score += EvalCoefficients::pawnShieldC_[0] >> 1;
      else if(mleft2 & pmask)
        score += EvalCoefficients::pawnShieldC_[1] >> 1;
      else
        score += EvalCoefficients::pawnPenaltyC_;

      if(mright1 & pmask)
        score += EvalCoefficients::pawnShieldA_[0] >> 1;
      else if(mright2 & pmask)
        score += EvalCoefficients::pawnShieldA_[1] >> 1;
      else
        score += EvalCoefficients::pawnPenaltyA_;
    }
    else // left side
    {
      if(mleft1 & pmask)
        score += EvalCoefficients::pawnShieldA_[0] >> 1;
      else if(mleft2 & pmask)
        score += EvalCoefficients::pawnShieldA_[1] >> 1;

      if(mright1 & pmask)
        score += EvalCoefficients::pawnShieldC_[0] >> 1;
      else if(mright2 & pmask)
        score += EvalCoefficients::pawnShieldC_[1] >> 1;
    }
  }

  // opponents pawns pressure
  {
    Index index(board_->kingPos(color));
    int xk = index.x();
    int opponent_penalty = 0;
    int x_arr[3] = {-1,-1,-1};
    if(ctype >= 0)
    {
      // short
      if(ctype == 0)
      {
        x_arr[0] = 5;
        x_arr[1] = 6;
        x_arr[2] = 7;
      }
      else // long
      {
        x_arr[0] = 0;
        x_arr[1] = 1;
        x_arr[2] = 2;
      }
    }
    else
    {
      int j = 0;
      if(xk > 0)
        x_arr[j++] = xk-1;
      x_arr[j++] = xk;
      if(xk < 7)
        x_arr[j++] = xk+1;
    }
    if(color)
    {
      for(auto x : x_arr)
      {
        if(x < 0)
          break;
        if(auto m = (opmask & pawnMasks().mask_column(x)))
        {
          int y = Index(_lsb64(m)).y();
          opponent_penalty += EvalCoefficients::opponentPawnPressure_[y];
        }
      }
    }
    else
    {
      for(auto x : x_arr)
      {
        if(x < 0)
          break;
        if(auto m = (opmask & pawnMasks().mask_column(x)))
        {
          int y = Index(_msb64(m)).y();
          opponent_penalty += EvalCoefficients::opponentPawnPressure_[7-y];
        }
      }
    }
    score -= opponent_penalty;
  }

  return score;
}

int Evaluator::evaluateCastle(Figure::Color color) const
{
  if(board_->castling(color))
    return 0;

  const FiguresManager & fmgr = board_->fmgr();
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
  Index ki_pos(board_->kingPos(color));
  auto castleType = getCastleType(color);
  X_ASSERT(castleType < -1 || castleType > 1, "invalid castle type detected");

  static const BitMask fake_castle_rook[2][2] = {
    { set_mask_bit(G8)|set_mask_bit(H8)|set_mask_bit(G7)|set_mask_bit(H7),
      set_mask_bit(A8)|set_mask_bit(B8)|set_mask_bit(C8)|set_mask_bit(A7)|set_mask_bit(B7)|set_mask_bit(C7)},

    { set_mask_bit(G1)|set_mask_bit(H1)|set_mask_bit(G2)|set_mask_bit(H2),
      set_mask_bit(A1)|set_mask_bit(B1)|set_mask_bit(C1)|set_mask_bit(A2)|set_mask_bit(B2)|set_mask_bit(C2)} };

  if(castleType < 0)
    return 0;

  // fake castle
  BitMask r_mask = fmgr.rook_mask(color) & fake_castle_rook[color][castleType];
  if ( r_mask )
  {
    Index r_pos( _lsb64(r_mask) );
    if(castleType == 0 && r_pos.x() > ki_pos.x() || castleType == 1 && r_pos.x() < ki_pos.x())
      return EvalCoefficients::fakeCastle_;
  }
  return EvalCoefficients::castleBonus_;
}

ScoreType Evaluator::evaluateMaterialDiff() const
{
  ScoreType score = 0;
  const FiguresManager & fmgr = board_->fmgr();

  // bonus for bishops
  score += EvalCoefficients::bishopBonus_[fmgr.bishops(Figure::ColorWhite) & 3];
  score -= EvalCoefficients::bishopBonus_[fmgr.bishops(Figure::ColorBlack) & 3];

  // Knight or Bishop against 3 pawns
  int figuresDiff = (fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite)) -
                    (fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack));
  int pawnsDiff  = fmgr.pawns(Figure::ColorWhite)  - fmgr.pawns(Figure::ColorBlack);
  int rooksDiff  = fmgr.rooks(Figure::ColorWhite)  - fmgr.rooks(Figure::ColorBlack);
  int queensDiff = fmgr.queens(Figure::ColorWhite) - fmgr.queens(Figure::ColorBlack);

  if(figuresDiff*pawnsDiff < 0 && !rooksDiff && !queensDiff)
  {
    Figure::Color strongColor = (Figure::Color)(figuresDiff > 0);
    int pawnsN = fmgr.pawns(strongColor) != 0;
    score += figuresDiff * EvalCoefficients::figureAgainstPawnBonus_[pawnsN];
  }
  // Rook vs. Pawns
  else if(!queensDiff && !figuresDiff && rooksDiff*pawnsDiff < 0)
  {
    Figure::Color strongColor = (Figure::Color)(rooksDiff > 0);
    int pawnsN = fmgr.pawns(strongColor) != 0;
    score += rooksDiff * EvalCoefficients::rookAgainstPawnBonus_[pawnsN];
  }
  // Knight|Bishop+2Pawns vs. Rook
  else if(!queensDiff && (rooksDiff*figuresDiff == -1))
  {
    Figure::Color strongColor = (Figure::Color)(rooksDiff > 0);
    int pawnsN = fmgr.pawns(strongColor) != 0;
    score += rooksDiff * EvalCoefficients::rookAgainstFigureBonus_[pawnsN];
  }
  // 2 figures vs. Rook
  else if(!queensDiff && rooksDiff*figuresDiff == -2 && std::abs(rooksDiff) == 1)
  {
    Figure::Color rookColor  = (Figure::Color)(rooksDiff > 0);
    Figure::Color ocolor = Figure::otherColor(rookColor);
    int pawnsN = fmgr.pawns(rookColor) != 0 && fmgr.pawns(ocolor) != 0;
    score -= rooksDiff * EvalCoefficients::figuresAgainstRookBonus_[pawnsN];
  }

  return score;
}

ScoreType Evaluator::evaluateForks(Figure::Color color) const
{
  Figure::Color ocolor = Figure::otherColor(color);
  BitMask o_rq_mask = board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
  BitMask o_mask = board_->fmgr().knight_mask(ocolor) | board_->fmgr().bishop_mask(ocolor) | o_rq_mask;
  auto pawn_msk = board_->fmgr().pawn_mask(color) & finfo_[color].pawnAttacks_;
  auto pawn_att = color
    ? ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1])
    : ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
  BitMask pawn_fork = o_mask & pawn_att;
  int pawnsN = pop_count(pawn_fork);
  if(pawnsN > 1)
    return EvalCoefficients::doublePawnAttack_;
  BitMask kn_fork = o_rq_mask & finfo_[color].knightAttacks_;
  int knightsN = pop_count(kn_fork);
  if(knightsN > 1 || (pawnsN+knightsN > 0 && color == board_->color()))
    return EvalCoefficients::forkBonus_;
  return 0;
}

} //NEngine
