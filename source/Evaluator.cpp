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
#include <xalgorithm.h>

namespace NEngine
{

const int Evaluator::colored_y_[2][8] = {
  { 7, 6, 5, 4, 3, 2, 1, 0 },
  { 0, 1, 2, 3, 4, 5, 6, 7 } };

const int Evaluator::promo_y_[] = { 0, 7 };
const int Evaluator::delta_y_[] = { -1, +1 };


const BitMask Evaluator::castle_mask_[2][2] = {
  {
    set_mask_bit(G8) | set_mask_bit(H8) |
    set_mask_bit(G7) | set_mask_bit(H7) |
    set_mask_bit(G6) | set_mask_bit(H6),

    set_mask_bit(A8) | set_mask_bit(B8) |
    set_mask_bit(A7) | set_mask_bit(B7) |
    set_mask_bit(A6) | set_mask_bit(B6)
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

const BitMask Evaluator::blocked_rook_mask_[2] = {
  set_mask_bit(G1) | set_mask_bit(H1) |
  set_mask_bit(G2) | set_mask_bit(H2) | set_mask_bit(H3)|
  set_mask_bit(A1) | set_mask_bit(B1) |
  set_mask_bit(A2) | set_mask_bit(B2) | set_mask_bit(A3),

  set_mask_bit(G8) | set_mask_bit(H8) |
  set_mask_bit(G7) | set_mask_bit(H7) | set_mask_bit(H6) |
  set_mask_bit(A8) | set_mask_bit(B8) |
  set_mask_bit(A7) | set_mask_bit(B7) | set_mask_bit(A6)
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
#ifdef PROCESS_DANGEROUS_EVAL
    dangerous_ = false;
#endif // PROCESS_DANGEROUS_EVAL

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

  // determine game phase (opening, middle or end game)
  auto phaseInfo = detectPhase();

  // evaluate figures weight
  score.common_ = fmgr.weight();
  score += evaluateMaterialDiff();

  score.opening_ += fmgr.eval(0);
  score.endGame_ += fmgr.eval(1);

  /// use lazy evaluation level 0
  {
    auto score0 = considerColor(score.common_);
    if(score0 < alpha0_ || score0 > betta0_)
      return score0;
  }

  prepare();

  // take pawns eval from hash if possible
  auto hashedScore = hashedEvaluation();
  score += hashedScore.score;

  int kingSafety = evaluateKingSafety(Figure::ColorWhite) - evaluateKingSafety(Figure::ColorBlack);
  score.opening_ += kingSafety;

#ifdef EVAL_FORKS
  auto scoreForks = evaluateForks(Figure::ColorWhite);
  scoreForks -= evaluateForks(Figure::ColorBlack);
  score.common_ += scoreForks;
#endif

#if 1
  score += evaluateKnights();
  score += evaluateBishops();

  score += evaluateRook(Figure::ColorWhite);
  score -= evaluateRook(Figure::ColorBlack);

  score += evaluateQueens(Figure::ColorWhite);
  score -= evaluateQueens(Figure::ColorBlack);
#else
  evaluateKnights();
  evaluateBishops();
  evaluateRook(Figure::ColorWhite);
  evaluateRook(Figure::ColorBlack);
  evaluateQueens(Figure::ColorWhite);
  evaluateQueens(Figure::ColorBlack);
#endif // 0


#if (defined EVAL_MOB || defined EVAL_KING_PR)
  auto scoreKing = evaluateMobilityAndKingPressure(Figure::ColorWhite);
  scoreKing -= evaluateMobilityAndKingPressure(Figure::ColorBlack);
  score.common_ += scoreKing.common_;
#endif

#if 0
  auto scorePP = evaluatePawnsPressure(Figure::ColorWhite);
  scorePP -= evaluatePawnsPressure(Figure::ColorBlack);
  score += scorePP;
#endif // 0

  auto scorePassers = passerEvaluation(hashedScore);
  score += scorePassers;


#ifdef PROCESS_DANGEROUS_EVAL
  detectDangerous();

#ifdef EVALUATE_DANGEROUS_ATTACKS
  auto dangerousScore = evaluateDangerous();
  score += dangerousScore;
#endif // EVALUATE_DANGEROUS_ATTACKS

#endif // PROCESS_DANGEROUS_EVAL


  auto result = considerColor(lipolScore(score, phaseInfo));
  return result;
}

Evaluator::PhaseInfo Evaluator::detectPhase() const
{
  const FiguresManager & fmgr = board_->fmgr();

  auto wei = fmgr.figuresWeight(Figure::ColorBlack) + fmgr.figuresWeight(Figure::ColorWhite);

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
      X_ASSERT(!(info.score == evaluatePawns().score), "invalid pawns+king score in hash");
      return info;
    }
  }

  PasserInfo info = evaluatePawns();

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
  BitMask left  = (x != 0) ? (pawnMasks().mask_forward(color, Index(x-1, y)) & pmask) : 0ULL;
  BitMask right = (x != 7) ? (pawnMasks().mask_forward(color, Index(x+1, y)) & pmask) : 0ULL;
  int y_closest = y;
  if(color)
  {
    y_closest = 7;
    if(left)
      y_closest = (_lsb64(left) >> 3) + 1;
    if(right)
      y_closest = std::min(y_closest, ((_lsb64(right) >> 3) + 1));
  }
  else
  {
    y_closest = 0;
    if(left)
      y_closest = (_msb64(left) >> 3)- 1;
    if(right)
      y_closest = std::max(y_closest, ((_msb64(right) >> 3) - 1));
  }
  return y_closest;
}

bool Evaluator::couldBeGuarded(Index const& idx, Figure::Color color, Figure::Color ocolor, BitMask const& pmask, BitMask const& opmsk, BitMask const& fwd_field, int n1) const
{
  // already guarded
  if ((finfo_[color].pawnAttacks_ & set_mask_bit(idx)) != 0ULL)
    return true;
  // could move fwd to guarded field
  if ((opmsk & fwd_field) == 0ULL) {
    // protected
    if ((finfo_[color].pawnAttacks_ & fwd_field) != 0ULL) {
      // not attacked
      if ((finfo_[ocolor].pawnAttacks_ & fwd_field) == 0ULL)
        return true;
      // protected by 2 and attacked by 1
      auto attackers = movesTable().pawnCaps(color, n1) & opmsk;
      auto guardians = movesTable().pawnCaps(ocolor, n1) & pmask;
      if (pop_count(attackers) <= pop_count(guardians))
        return true;
    }
    // on 1st line. could it move 2 lines forward?
    if (colored_y_[color][idx.y()] == 1) {
      int n2 = static_cast<int>(Index(idx.x(), idx.y() + (delta_y_[color] << 1)));
      BitMask fwd_field2 = set_mask_bit(n2);
      // not occupied and protected
      if (((opmsk & fwd_field2) == 0ULL) && ((finfo_[color].pawnAttacks_ & fwd_field2) != 0ULL)) {
        // not attacked
        if ((finfo_[ocolor].pawnAttacks_ & fwd_field2) == 0ULL)
          return true;
        // protected by 2 and attacked by 1
        auto attackers = movesTable().pawnCaps(color, n2) & opmsk;
        auto guardians = movesTable().pawnCaps(ocolor, n2) & pmask;
        if (pop_count(attackers) <= pop_count(guardians))
          return true;
      }
    }
  }
  BitMask blockers_mask = (finfo_[ocolor].pawnAttacks_ & (~finfo_[color].pawnAttacks_)) | opmsk;
  BitMask left  = (idx.x() != 0) ? (pawnMasks().mask_forward(ocolor, idx - 1) & pmask) : 0ULL;
  if(left)
  {
    int y = color ? (_msb64(left) >> 3) : (_lsb64(left) >> 3);
    BitMask pmask_before = betweenMasks().between(idx-1, Index(idx.x()-1, y));
    if((pmask_before & blockers_mask) == 0ULL)
      return true;
  }
  BitMask right = (idx.x() != 7) ? (pawnMasks().mask_forward(ocolor, idx + 1) & pmask) : 0ULL;
  if(right)
  {
    int y = color ? (_msb64(right) >> 3) : (_lsb64(right) >> 3);
    BitMask pmask_before = betweenMasks().between(idx+1, Index(idx.x()+1, y));
    if((pmask_before & blockers_mask) == 0ULL)
      return true;
  }
  return false;
}

bool Evaluator::isPawnBackward(Index const& idx, Figure::Color color, BitMask const& pmask, BitMask const& opmsk, BitMask const& fwd_field) const
{
  // TODO: optimization required
  Figure::Color ocolor = Figure::otherColor(color);
  int closest_y = closestToBackward(idx.x(), idx.y(), pmask, color);
  X_ASSERT(closest_y < 0 || closest_y > 7, "backward mask calculation error - invalid next y position");
  BitMask pmask_after = betweenMasks().between(idx, Index(idx.x(), closest_y));
  auto blocked_mask = (finfo_[ocolor].pawnAttacks_ & ~finfo_[color].pawnAttacks_) | opmsk;
  bool blocked = (blocked_mask & pmask_after) != 0ULL;
  if (!blocked && std::abs(closest_y - idx.y()) < 3 && (idx.y() < 6 && color || idx.y() > 3 && !color)) {
    auto fwd_field2 = color == Figure::ColorWhite ? fwd_field << 8 : fwd_field >> 8;
    blocked = (fwd_field & pmask) != 0ULL && (fwd_field2 & blocked_mask) != 0ULL;
  }
  return blocked;
}

bool Evaluator::isPawnBlocked(Index const& idx, Figure::Color color, BitMask const& pmask, BitMask const& opmsk, BitMask const& fwd_field) const
{
  Figure::Color ocolor = Figure::otherColor(color);
  auto blocked_mask = (finfo_[ocolor].pawnAttacks_ & ~finfo_[color].pawnAttacks_) | opmsk;
  bool blocked = (blocked_mask & fwd_field) != 0ULL;
  if (!blocked && (idx.y() < 6 && color || idx.y() > 3 && !color)) {
    auto fwd_field2 = color == Figure::ColorWhite ? fwd_field << 8 : fwd_field >> 8;
    blocked = (fwd_field & pmask) != 0ULL && (fwd_field2 & blocked_mask) != 0ULL;
  }
  return blocked;
}

Evaluator::PasserInfo Evaluator::evaluatePawns(Figure::Color color) const
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmask = fmgr.pawn_mask(color);
  if(!pmask)
    return{};

  PasserInfo info;

  Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & opmsk = fmgr.pawn_mask(ocolor);
  bool no_opawns = opmsk == 0ULL;
  auto pawns_all = opmsk | pmask;

  uint8 x_visited{ 0 };
  uint8 x_passers{ 0 };
  int dy = delta_y_[color];

  BitMask pawn_mask = pmask;
  for(; pawn_mask;)
  {
    int const n = clear_lsb(pawn_mask);
    Index idx(n);

    int x  = idx.x();
    int cy = colored_y_[color][idx.y()];
    int py = promo_y_[color];
    auto n1 = n + (dy << 3);
    auto pw_field = set_mask_bit(n);
    auto fwd_field = set_mask_bit(n1);

    //int dist_to_oking = distanceCounter().getDistance(board_->kingPos(ocolor), n);
    //int dist_to_myking = distanceCounter().getDistance(board_->kingPos(color), n);

    //info.score.endGame_ -= EvalCoefficients::kingToPawnBonus_[dist_to_oking];
    //info.score.endGame_ += EvalCoefficients::kingToPawnBonus_[dist_to_myking];

    // doubled pawn
    if(!(x_visited & (1<<x)))
    {
      x_visited |= 1<<x;
      auto const& dblmsk = pawnMasks().mask_doubled(x);
      auto num_dbl = pop_count(pmask & dblmsk);
      X_ASSERT(num_dbl <= 0, "doubled pawns number is zero or negative");
      info.score.opening_ += (num_dbl - 1) * EvalCoefficients::doubledPawn_[0];
      info.score.endGame_ += (num_dbl - 1) * EvalCoefficients::doubledPawn_[1];
    }

    bool isolated = (pmask & pawnMasks().mask_isolated(x)) == 0ULL;
    bool backward = !isolated && ((pawnMasks().mask_backward(color, n) & pmask) == 0ULL) &&
      isPawnBackward(idx, color, pmask, opmsk, fwd_field);
    //bool unsupported = !isolated && !backward && !couldBeGuarded(idx, color, ocolor, pmask, opmsk, fwd_field, n1);

    info.score.opening_ += isolated * EvalCoefficients::isolatedPawn_[0];
    info.score.opening_ += backward * EvalCoefficients::backwardPawn_[0];
    //info.score.opening_ += unsupported * EvalCoefficients::unsupportedPawn_[0];

    info.score.endGame_ += isolated * EvalCoefficients::isolatedPawn_[1];
    info.score.endGame_ += backward * EvalCoefficients::backwardPawn_[1];
    //info.score.endGame_ += unsupported * EvalCoefficients::unsupportedPawn_[1];

    //// could be attacked by RQ
    //if (((pawnMasks().mask_forward(color, n) & pawns_all) == 0ULL) &&
    //     (isolated || backward || unsupported))
    //{
    //  info.score.opening_ += EvalCoefficients::weakHalfopenPawn_[0];
    //  info.score.endGame_ += EvalCoefficients::weakHalfopenPawn_[1];
    //}

    // stash passer pawn
    {
      if((pawnMasks().mask_forward(color, n) & opmsk) == 0ULL)
      {
        // just save position for further usage
        info.passers_[color] |= set_mask_bit(n);
      }
    }
  }

  return info;
}

Evaluator::PasserInfo Evaluator::evaluatePawns() const
{
  auto info_w = evaluatePawns(Figure::ColorWhite);
  auto info_b = evaluatePawns(Figure::ColorBlack);
  info_w.score -= info_b.score;
  info_w.passers_observed_[0] = true;
  info_w.passers_observed_[1] = true;
  info_w.passers_[0] = info_b.passers_[0];
  return info_w;
}


Evaluator::FullScore Evaluator::passerEvaluation(PasserInfo const& pi)
{
  auto infoW = passerEvaluation(Figure::ColorWhite, pi);
  auto infoB = passerEvaluation(Figure::ColorBlack, pi);

  //if ((infoW.most_unstoppable_y > infoB.most_unstoppable_y) && (board_->fmgr().allFigures(Figure::ColorBlack) == 0))
  //  infoW.score.endGame_ += 2 * EvalCoefficients::closeToPromotion_[infoW.most_unstoppable_y - infoB.most_unstoppable_y];
  //else if ((infoB.most_unstoppable_y > infoW.most_unstoppable_y) && (board_->fmgr().allFigures(Figure::ColorWhite) == 0))
  //  infoB.score.endGame_ += 2 * EvalCoefficients::closeToPromotion_[infoB.most_unstoppable_y - infoW.most_unstoppable_y];

  //if (infoW.most_passer_y > infoB.most_passer_y)
  //  infoW.score.endGame_ += EvalCoefficients::closeToPromotion_[infoW.most_passer_y - infoB.most_passer_y];
  //else if (infoB.most_passer_y > infoW.most_passer_y)
  //  infoB.score.endGame_ += EvalCoefficients::closeToPromotion_[infoB.most_passer_y - infoW.most_passer_y];

  infoW.score -= infoB.score;
  return infoW.score;
}

Evaluator::PasserInfo Evaluator::passerEvaluation(Figure::Color color, PasserInfo const& pi)
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmask = fmgr.pawn_mask(color);
  if(!pmask || (pi.passers_observed_[color] && !pi.passers_[color]))
    return{};
    
  int py = promo_y_[color];
  int dy = delta_y_[color];
  
  nst::dirs dir_behind[] = {nst::no, nst::so};

  PasserInfo pinfo;

  Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & opmsk = fmgr.pawn_mask(ocolor);
  bool no_opawns = opmsk == 0ULL;

  BitMask pawn_mask = pmask;
  if(pi.passers_observed_[color])
    pawn_mask = pi.passers_[color];
  
  for(; pawn_mask;)
  {
    const int n = clear_lsb(pawn_mask);
    Index idx(n);

    int x = idx.x();
    int y = idx.y();
    int cy = colored_y_[color][idx.y()];
    Index idx1{ x, y + dy };

    const auto & passmsk = pawnMasks().mask_passed(color, n);
    const auto& halfpassmsk = pawnMasks().mask_forward(color, n);
    if((opmsk & halfpassmsk) != 0ULL)
      continue;

    bool halfpasser = (opmsk & passmsk) != 0ULL;
    FullScore pwscore;
    if(halfpasser)
    {
      pwscore.common_ = EvalCoefficients::passerPawn_[cy] >> 1;
      BitMask guards = pawnMasks().mask_guards(color, n) & pmask;
      BitMask attackers = pawnMasks().mask_passed(color, n) & opmsk;
      auto nguards = pop_count(guards);
      for (; guards;) {
        int g = clear_lsb(guards);
        auto fwd = pawnMasks().mask_forward(color, g);
        if (fwd & opmsk) {
          int o = color ? _lsb64(fwd & opmsk) : _msb64(fwd & opmsk);
          fwd = betweenMasks().between(g, o);
        }
        if ((fwd & finfo_[ocolor].pawnAttacks_) == 0)
          nguards++;
      }
      auto nattackers = pop_count(attackers);
      if (nguards >= nattackers)
        pwscore.common_ += EvalCoefficients::passerPawn_[cy];
    }
    else
    {
      pwscore.common_ = EvalCoefficients::passerPawn_[cy];
      Index pp(x, py);
      int pawn_dist_promo = std::abs(py - idx.y());
      int o_dist_promo = distanceCounter().getDistance(board_->kingPos(ocolor), pp) - (board_->color() == ocolor);
      int dist_promo = distanceCounter().getDistance(board_->kingPos(color), pp);
      pwscore.endGame_ += (o_dist_promo - dist_promo) * EvalCoefficients::kingToPasserDistanceBonus_;

      BitMask guards = pawnMasks().mask_guards(color, n) & pmask;
      if (guards) {
        pwscore.common_ += EvalCoefficients::passerPawn_[cy] >> 2;
      }
      while (guards != 0ULL) {
        int g = clear_lsb(guards);
        auto fwd_mask = pawnMasks().mask_forward(color, g) & (opmsk | finfo_[ocolor].pawnAttacks_);
        if (!fwd_mask) {
          pwscore.common_ += EvalCoefficients::passerPawn_[cy] >> 2;
          break;
        }
      }
    }

    if (cy > pinfo.most_passer_y && !halfpasser)
      pinfo.most_passer_y = cy;

    BitMask behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & (fmgr.rook_mask(color) | fmgr.queen_mask(color));
    BitMask o_behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & (fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor));
    auto attack_mask = finfo_[color].attack_mask_;
    auto multiattack_mask = finfo_[color].multiattack_mask_;
    auto o_attack_mask = finfo_[ocolor].attack_mask_;
    auto o_multiattack_mask = finfo_[ocolor].multiattack_mask_;
    if ( behind_msk )
    {
      // my rook behind passed pawn - give bonus
      int rpos = color ? _msb64(behind_msk) : _lsb64(behind_msk);
      if (board_->is_nothing_between(n, rpos, inv_mask_all_))
      {
        pwscore.common_ += EvalCoefficients::rookBehindBonus_;
        auto r_attacks = magic_ns::rook_moves(rpos, mask_all_ & ~pmask);
        multiattack_mask |= attack_mask & r_attacks;
        attack_mask |= r_attacks;
      }
    }
    else if ( o_behind_msk )
    {
      // may be opponent's rook - give penalty
      int rpos = color ? _msb64(o_behind_msk) : _lsb64(o_behind_msk);
      if (board_->is_nothing_between(n, rpos, inv_mask_all_))
      {
        pwscore.common_ -= EvalCoefficients::rookBehindBonus_;
        auto or_attacks = magic_ns::rook_moves(rpos, mask_all_ & ~pmask);
        o_multiattack_mask |= o_attack_mask & or_attacks;
        o_attack_mask |= or_attacks;
      }
    }
    auto blockers_mask = (o_attack_mask & ~attack_mask) | (o_multiattack_mask & ~multiattack_mask) | fmgr.mask(ocolor);
    blockers_mask &= ~finfo_[color].pawnAttacks_;

    // ahead fields are not blocked by opponent
    auto fwd_mask = pawnMasks().mask_forward(color, n) & blockers_mask;
    if(!fwd_mask)
      pwscore.common_ += EvalCoefficients::canpromotePawn_[cy];
    else {
      int closest_blocker = (color == Figure::ColorWhite) ? _lsb64(fwd_mask) : _msb64(fwd_mask);
      int last_cango = colored_y_[color][Index(closest_blocker).y()] - 1;
      int steps = last_cango - cy;
      int steps_to_promotion = 7 - cy;
      X_ASSERT(steps < 0 || steps_to_promotion < 1, "invalid number of pawn steps");
      pwscore.common_ += (steps*EvalCoefficients::canpromotePawn_[cy]) / steps_to_promotion;
    }    

    // opponent king could not go to my pawns promotion path
    int promo_pos = x | (py<<3);
    int pawn_dist_promo = std::abs(py - y);
    if(!couldIntercept(color, n, promo_pos, pawn_dist_promo+1))
    {
      pwscore.endGame_ += EvalCoefficients::farKingPawn_[cy];
      if (cy > pinfo.most_unstoppable_y && !halfpasser)
        pinfo.most_unstoppable_y = cy;
    }

    if (halfpasser)
      pwscore >>= 1;
    pinfo.score += pwscore;

#ifdef PROCESS_DANGEROUS_EVAL
    X_ASSERT(((y + dy)) > 7 || ((y + dy)) < 0, "pawn goes to invalid line");
    if (!board_->getField(idx1) && !halfpasser)
    {
      BitMask next_mask = set_mask_bit(idx1);
      if ((cy == 6) && ((next_mask & blockers_mask) == 0ULL))
      {
        finfo_[color].pawnPromotion_ = true;
      }
    }
#endif // PROCESS_DANGEROUS_EVAL
  }
  
  if (color == board_->color())
  {
    if (pinfo.most_passer_y > 0)
      pinfo.most_passer_y++;
    if (pinfo.most_unstoppable_y > 0)
      pinfo.most_unstoppable_y++;
  }
  
  return pinfo;
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
  Index kingPos(board_->kingPos(color));
  int ctype = getCastleType(color);
  int score = 0;
  if (ctype == 0) // king side
  {
    Index kingPosK{ 6, kingPos.y() };
    score = evaluateKingSafety(color, kingPosK);
  }
  else if (ctype == 1) // queen side
  {
    Index kingPosQ{ 1, kingPos.y() };
    score = evaluateKingSafety(color, kingPosQ);
  }
  else
    score = evaluateKingSafety(color, kingPos);
  return score;
}

int Evaluator::evaluateKingSafety(Figure::Color color, Index const& kingPos) const
{
  static const int delta_y[2] = { -8, 8 };
  const FiguresManager & fmgr = board_->fmgr();
  auto const& pmask  = fmgr.pawn_mask(color);

  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
  auto const& opmask = fmgr.pawn_mask(ocolor);

  auto king_cy = colored_y_[color][kingPos.y()];
  if (king_cy > 5)
    return EvalCoefficients::kingIsUnsafe_;

  int above1 = (kingPos + delta_y[color]) & 63;
  int above2 = (above1 + delta_y[color]) & 63;
  BitMask mabove1{ set_mask_bit(above1) };
  BitMask mabove2{ set_mask_bit(above2) };
  BitMask mabove3{};
  if (king_cy < 5)
    mabove3 = set_mask_bit((above2 + delta_y[color]) & 63);

  auto mleft1 = (mabove1 >> 1) & Figure::pawnCutoffMasks_[1];
  auto mleft2 = (mabove2 >> 1) & Figure::pawnCutoffMasks_[1];
  auto mleft3 = (mabove3 >> 1) & Figure::pawnCutoffMasks_[1];

  auto mright1 = (mabove1 << 1) & Figure::pawnCutoffMasks_[0];
  auto mright2 = (mabove2 << 1) & Figure::pawnCutoffMasks_[0];
  auto mright3 = (mabove3 << 1) & Figure::pawnCutoffMasks_[0];

  int score = 0;

  if (mabove1 & pmask)
    score += EvalCoefficients::pawnShieldB_[0];
  else if (mabove2 & pmask)
    score += EvalCoefficients::pawnShieldB_[1];
  else if ((mabove3 & pmask) == 0ULL)
    score += EvalCoefficients::pawnPenaltyB_;

  if (kingPos.x() > 3) // right side
  {
    if (mleft1 & pmask)
      score += EvalCoefficients::pawnShieldC_[0];
    else if (mleft2 & pmask)
      score += EvalCoefficients::pawnShieldC_[1];
    else if ((mleft3 & pmask) == 0ULL)
      score += EvalCoefficients::pawnPenaltyC_;

    if (mright1 & pmask)
      score += EvalCoefficients::pawnShieldA_[0];
    else if (mright2 & pmask)
      score += EvalCoefficients::pawnShieldA_[1];
    else if ((mright3 & pmask) == 0ULL)
      score += EvalCoefficients::pawnPenaltyA_;
  }
  else // left side
  {
    if (mleft1 & pmask)
      score += EvalCoefficients::pawnShieldA_[0];
    else if (mleft2 & pmask)
      score += EvalCoefficients::pawnShieldA_[1];
    else if ((mleft3 & pmask) == 0ULL)
      score += EvalCoefficients::pawnPenaltyA_;

    if (mright1 & pmask)
      score += EvalCoefficients::pawnShieldC_[0];
    else if (mright2 & pmask)
      score += EvalCoefficients::pawnShieldC_[1];
    else if ((mright3 & pmask) == 0ULL)
      score += EvalCoefficients::pawnPenaltyC_;
  }

  // opponents pawns pressure
  int xk = kingPos.x();
  int opponent_penalty = 0;
  int x_arr[3] = { -1,-1,-1 };
  {
    int j = 0;
    if (xk > 0)
      x_arr[j++] = xk - 1;
    x_arr[j++] = xk;
    if (xk < 7)
      x_arr[j++] = xk + 1;
  }
  if (color)
  {
    for (auto x : x_arr)
    {
      if (x < 0)
        break;
      if (auto m = (opmask & pawnMasks().mask_column(x)))
      {
        int y = std::max(0, Index(_lsb64(m)).y() - king_cy);
        auto penalty = EvalCoefficients::opponentPawnPressure_[y];
        opponent_penalty += penalty;
      }
    }
  }
  else
  {
    for (auto x : x_arr)
    {
      if (x < 0)
        break;
      if (auto m = (opmask & pawnMasks().mask_column(x)))
      {
        int y = std::max(0, 7 - Index(_msb64(m)).y() - king_cy);
        auto penalty = EvalCoefficients::opponentPawnPressure_[y];
        if (x == xk)
          penalty >>= 1;
        opponent_penalty += penalty;
      }
    }
  }
  score -= opponent_penalty;

  return score;
}

bool Evaluator::fakeCastle(Figure::Color color, int rpos) const
{
  const FiguresManager & fmgr = board_->fmgr();
  Index ki_pos(board_->kingPos(color));
  Index r_pos(rpos);
  int ctype = r_pos.x() < ki_pos.x();
  if (board_->castling(color, ctype))
    return false;
  return blockedRook(Figure::otherColor(color), rpos);
}

bool Evaluator::blockedRook(Figure::Color color, int n) const
{
  return (blocked_rook_mask_[color] & set_mask_bit(n)) != 0ULL;
}

Evaluator::FullScore Evaluator::evaluateMaterialDiff() const
{
  const FiguresManager & fmgr = board_->fmgr();

  FullScore score;
  score.endGame_ = (fmgr.pawns(Figure::ColorWhite) - fmgr.pawns(Figure::ColorBlack)) * EvalCoefficients::pawnEndgameBonus_;

  int pawnsDiff = fmgr.pawns(Figure::ColorWhite) - fmgr.pawns(Figure::ColorBlack);
  int knightsDiff = fmgr.knights(Figure::ColorWhite) - fmgr.knights(Figure::ColorBlack);
  int bishopsDiff = fmgr.bishops(Figure::ColorWhite) - fmgr.bishops(Figure::ColorBlack);
  int figuresDiff = knightsDiff + bishopsDiff;
  int rooksDiff  = fmgr.rooks(Figure::ColorWhite)  - fmgr.rooks(Figure::ColorBlack);
  int queensDiff = fmgr.queens(Figure::ColorWhite) - fmgr.queens(Figure::ColorBlack);

  if (std::abs(queensDiff) == 1) {
    rooksDiff += 2 * queensDiff;
    queensDiff = 0;
  }

  // bonus for knights
  if (fmgr.knights(Figure::ColorWhite) > 0)
  {
    int knightsN = std::min(fmgr.knights(Figure::ColorWhite) - 1, 1);
    score.opening_ += EvalCoefficients::knightBonus_[0][knightsN];
    score.endGame_ += EvalCoefficients::knightBonus_[1][knightsN];
  }
  if (fmgr.knights(Figure::ColorBlack) > 0)
  {
    int knightsN = std::min(fmgr.knights(Figure::ColorBlack) - 1, 1);
    score.opening_ -= EvalCoefficients::knightBonus_[0][knightsN];
    score.endGame_ -= EvalCoefficients::knightBonus_[1][knightsN];
  }

  // bonus for bishops
  if (fmgr.bishops(Figure::ColorWhite) > 0)
  {
    int bishopsN = std::min(fmgr.bishops(Figure::ColorWhite) - 1, 1);
    score.opening_ += EvalCoefficients::bishopBonus_[0][bishopsN];
    score.endGame_ += EvalCoefficients::bishopBonus_[1][bishopsN];
  }
  if (fmgr.bishops(Figure::ColorBlack) > 0)
  {
    int bishopsN = std::min(fmgr.bishops(Figure::ColorBlack) - 1, 1);
    score.opening_ -= EvalCoefficients::bishopBonus_[0][bishopsN];
    score.endGame_ -= EvalCoefficients::bishopBonus_[1][bishopsN];
  }

  // bonus for rooks
  if (fmgr.rooks(Figure::ColorWhite) > 0)
  {
    int rooksN = std::min(fmgr.rooks(Figure::ColorWhite) - 1, 1);
    score.opening_ += EvalCoefficients::rookBonus_[0][rooksN];
    score.endGame_ += EvalCoefficients::rookBonus_[1][rooksN];
  }
  if (fmgr.rooks(Figure::ColorBlack) > 0)
  {
    int rooksN = std::min(fmgr.rooks(Figure::ColorBlack) - 1, 1);
    score.opening_ -= EvalCoefficients::rookBonus_[0][rooksN];
    score.endGame_ -= EvalCoefficients::rookBonus_[1][rooksN];
  }

  // Figure vs. Pawns
  if(!rooksDiff && figuresDiff*pawnsDiff < 0)
  {
    Figure::Color fcolor = static_cast<Figure::Color>(figuresDiff > 0);
    int pawnsN = fmgr.pawns(fcolor) != 0;
    score.opening_ += figuresDiff * EvalCoefficients::figureAgainstPawnBonus_[0][pawnsN];
    score.endGame_ += figuresDiff * EvalCoefficients::figureAgainstPawnBonus_[1][pawnsN];
  }
  // Rook vs. Pawns
  if(!figuresDiff && rooksDiff*pawnsDiff < 0)
  {
    Figure::Color rcolor = static_cast<Figure::Color>(rooksDiff > 0);
    int pawnsN = fmgr.pawns(rcolor) != 0;
    score.opening_ += rooksDiff * EvalCoefficients::rookAgainstPawnBonus_[0][pawnsN];
    score.endGame_ += rooksDiff * EvalCoefficients::rookAgainstPawnBonus_[1][pawnsN];
  }
  // Knight|Bishop+Pawns vs. Rook
  if(rooksDiff*figuresDiff == -1)
  {
    Figure::Color rcolor = static_cast<Figure::Color>(rooksDiff > 0);
    int pawnsN = fmgr.pawns(rcolor) != 0;
    score.opening_ += rooksDiff * EvalCoefficients::rookAgainstFigureBonus_[0][pawnsN];
    score.endGame_ += rooksDiff * EvalCoefficients::rookAgainstFigureBonus_[1][pawnsN];
  }
  // 2 figures vs. Rook
  if((rooksDiff*figuresDiff <= -2) && (std::abs(rooksDiff) == 1))
  {
    Figure::Color fcolor = static_cast<Figure::Color>(figuresDiff > 0);
    int pawnsN = fmgr.pawns(fcolor) != 0;
    score.opening_ -= rooksDiff * EvalCoefficients::figuresAgainstRookBonus_[0][pawnsN];
    score.endGame_ -= rooksDiff * EvalCoefficients::figuresAgainstRookBonus_[1][pawnsN];
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

#ifdef PROCESS_DANGEROUS_EVAL
void Evaluator::detectDangerous()
{
  auto color = board_->color();
  auto ocolor = Figure::otherColor(board_->color());
  dangerous_ = (finfo_[color].pawnsUnderAttack_ + finfo_[color].knightsUnderAttack_ + finfo_[color].bishopsUnderAttack_
    + finfo_[color].rooksUnderAttack_ + finfo_[color].queensUnderAttack_ > 1) ||
    finfo_[ocolor].pawnPromotion_ ||
    finfo_[color].matThreat_;
}

#ifdef EVALUATE_DANGEROUS_ATTACKS
Evaluator::FullScore Evaluator::evaluateDangerous() const
{
  FullScore score;
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    bool hasAttacks = finfo_[color].pawnsUnderAttack_ + finfo_[color].knightsUnderAttack_ + finfo_[color].bishopsUnderAttack_
      + finfo_[color].rooksUnderAttack_ + finfo_[color].queensUnderAttack_ > 1;
    int coeff = hasAttacks;
    if (color == Figure::ColorWhite)
      coeff = -coeff;
    if (coeff != 0)
    {
      coeff = coeff;
    }
    auto const& finfo = finfo_[color];
    score.common_ += (finfo.pawnsUnderAttack_*EvalCoefficients::dangerousAttacksOnPawn_ +
      finfo.knightsUnderAttack_*EvalCoefficients::dangerousAttacksOnKnight_ +
      finfo.bishopsUnderAttack_*EvalCoefficients::dangerousAttacksOnBishop_ +
      finfo.rooksUnderAttack_*EvalCoefficients::dangerousAttacksOnRook_ +
      finfo.queensUnderAttack_*EvalCoefficients::dangerousAttacksOnQueen_) * coeff;
  }
  return score;
}
#endif // EVALUATE_DANGEROUS_ATTACKS

#endif // PROCESS_DANGEROUS_EVAL

} //NEngine
