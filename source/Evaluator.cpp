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
const nst::dirs Evaluator::dir_behind_[] = { nst::no, nst::so };

const BitMask Evaluator::castle_mask_[2][2] = {
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

const BitMask Evaluator::blocked_rook_mask_[2][2] = {
  {
    set_mask_bit(G1) | set_mask_bit(H1) | set_mask_bit(F1) |
    set_mask_bit(G2) | set_mask_bit(H2) | set_mask_bit(H3),

    set_mask_bit(A1) | set_mask_bit(B1) | set_mask_bit(C1) |
    set_mask_bit(A2) | set_mask_bit(B2) | set_mask_bit(A3)
  },

  {
    set_mask_bit(G8) | set_mask_bit(H8) | set_mask_bit(F8) |
    set_mask_bit(G7) | set_mask_bit(H7) | set_mask_bit(H6),

    set_mask_bit(A8) | set_mask_bit(B8) | set_mask_bit(C8) |
    set_mask_bit(A7) | set_mask_bit(B7) | set_mask_bit(A6)
  }
};

void Evaluator::initialize(Board const* board)
{
  board_ = board;
}

void Evaluator::prepare()
{
  auto const& fmgr = board_->fmgr();

  mask_all_ = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
  inv_mask_all_ = ~mask_all_;

  finfo_[0] = FieldsInfo{};
  finfo_[1] = FieldsInfo{};

  // pawns attacks
  {
    const BitMask & pawn_msk_w = fmgr.pawn_mask(Figure::ColorWhite);
    const BitMask & pawn_msk_b = fmgr.pawn_mask(Figure::ColorBlack);

    finfo_[Figure::ColorWhite].pawnAttacks_ = ((pawn_msk_w << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_w << 7) & Figure::pawnCutoffMasks_[1]);
    finfo_[Figure::ColorBlack].pawnAttacks_ = ((pawn_msk_b >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_b >> 9) & Figure::pawnCutoffMasks_[1]);

    finfo_[0].attack_mask_ = finfo_[0].pawnAttacks_;
    finfo_[1].attack_mask_ = finfo_[1].pawnAttacks_;
  }

  // attacked fields near king
  {
    finfo_[0].ki_fields_ = finfo_[0].kingAttacks_ = movesTable().caps(Figure::TypeKing, board_->kingPos(Figure::ColorBlack));
    finfo_[1].ki_fields_ = finfo_[1].kingAttacks_ = movesTable().caps(Figure::TypeKing, board_->kingPos(Figure::ColorWhite));

    finfo_[0].multiattack_mask_ = finfo_[0].attack_mask_ & finfo_[0].kingAttacks_;
    finfo_[1].multiattack_mask_ = finfo_[1].attack_mask_ & finfo_[1].kingAttacks_;

    finfo_[0].attack_mask_ |= finfo_[0].kingAttacks_;
    finfo_[1].attack_mask_ |= finfo_[1].kingAttacks_;

    finfo_[0].ki_fields_ |= (finfo_[0].ki_fields_ >> 8);
    finfo_[1].ki_fields_ |= (finfo_[1].ki_fields_ << 8);
  }
  
  // other mask
  {
    auto kings_mask = fmgr.king_mask(Figure::ColorBlack) | fmgr.king_mask(Figure::ColorWhite);
    finfo_[0].cango_mask_ = ~(finfo_[1].pawnAttacks_ | fmgr.king_mask(Figure::ColorBlack) | fmgr.pawn_mask(Figure::ColorBlack));
    finfo_[1].cango_mask_ = ~(finfo_[0].pawnAttacks_ | fmgr.king_mask(Figure::ColorWhite) | fmgr.pawn_mask(Figure::ColorWhite));

    finfo_[0].mask_xray_b_ = mask_all_ & ~(fmgr.bishop_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack));
    finfo_[0].mask_xray_r_ = mask_all_ & ~(fmgr.rook_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack));

    finfo_[1].mask_xray_b_ = mask_all_ & ~(fmgr.bishop_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite));
    finfo_[1].mask_xray_r_ = mask_all_ & ~(fmgr.rook_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite));

    finfo_[0].brq_mask_ = fmgr.bishop_mask(Figure::ColorBlack) | fmgr.rook_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack);
    finfo_[0].nbrq_mask_ = finfo_[0].brq_mask_ | fmgr.knight_mask(Figure::ColorBlack);
    finfo_[0].rq_mask_ = fmgr.rook_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack);

    finfo_[1].brq_mask_ = fmgr.bishop_mask(Figure::ColorWhite) | fmgr.rook_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite);
    finfo_[1].nbrq_mask_ = finfo_[1].brq_mask_ | fmgr.knight_mask(Figure::ColorWhite);
    finfo_[1].rq_mask_ = fmgr.rook_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite);

    finfo_[0].pawns_fwd_ = (fmgr.pawn_mask(Figure::ColorBlack) >> 8) & inv_mask_all_;
    finfo_[1].pawns_fwd_ = (fmgr.pawn_mask(Figure::ColorWhite) << 8) & inv_mask_all_;

    finfo_[0].pawns_fwd_ |= ((finfo_[0].pawns_fwd_ & Figure::pawns2ndLineMask_[0]) >> 8) & inv_mask_all_;
    finfo_[1].pawns_fwd_ |= ((finfo_[1].pawns_fwd_ & Figure::pawns2ndLineMask_[1]) << 8) & inv_mask_all_;
  }
}

//////////////////////////////////////////////////////////////////////////
ScoreType Evaluator::operator () (ScoreType alpha, ScoreType betta)
{
  X_ASSERT(!board_, "Evaluator wasn't properly initialized");

  if(!ehash_.empty())
    ehash_.prefetch(board_->fmgr().kpwnCode());

  if(board_->matState())
    return -Figure::MatScore;
  else if(board_->drawState())
    return Figure::DrawScore;

  ScoreType score = evaluate(alpha, betta);
  X_ASSERT(score <= -ScoreMax || score >= ScoreMax, "invalid score");
  return score;
}

ScoreType Evaluator::materialScore() const
{
  const FiguresManager& fmgr = board_->fmgr();
  auto result = considerColor(fmgr.weight());
  return result;
}

ScoreType Evaluator::evaluate(ScoreType alpha, ScoreType betta)
{
#ifndef NDEBUG
  std::string sfen = toFEN(*board_);
#endif

  auto spec = specialCases().eval(*board_);
  if (spec.first)
  {
    ScoreType score = spec.second;
    score = considerColor(score);
    return score;
  }

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
    auto score0 = considerColor(lipolScore(score, phaseInfo));
    if(score0 < alpha0_ || score0 > betta0_)
      return score0;
  }

  prepare();

  // take pawns eval from hash if possible
  auto hashedScore = hashedEvaluation();
  score += hashedScore.score;

  score += evaluateKnights();
  score += evaluateBishops();
  score += evaluateRook();
  score += evaluateQueens();

  int score_mob = finfo_[Figure::ColorWhite].score_mob_ - finfo_[Figure::ColorBlack].score_mob_;
  score.common_ += score_mob;

  auto scoreKing = evaluateKingPressure(Figure::ColorWhite);
  scoreKing -= evaluateKingPressure(Figure::ColorBlack);
  score += scoreKing;

  auto scoreForks = evaluateForks(Figure::ColorWhite);
  scoreForks -= evaluateForks(Figure::ColorBlack);
  score.common_ += scoreForks;

  auto scorePP = evaluatePawnsPressure(Figure::ColorWhite);
  scorePP -= evaluatePawnsPressure(Figure::ColorBlack);
  score += scorePP;

  auto scorePassers = passerEvaluation(hashedScore);
  score += scorePassers;

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
  score.common_  = pop_count(pw_protected   & attackers) * EvalCoefficients::protectedPawnPressure_;
  score.common_ += pop_count(pw_unprotected & attackers & ~finfo_[ocolor].attack_mask_) * EvalCoefficients::unprotectedPawnPressure_;
  score.common_ += pop_count(pw_unprotected & attackers &  finfo_[ocolor].attack_mask_) * EvalCoefficients::semiprotectedPawnPressure_;
  // bishop treat
  if(fmgr.bishops(color))
  {
    auto bi_mask_w = fmgr.bishop_mask(color) &  FiguresCounter::s_whiteMask_;
    auto bi_mask_b = fmgr.bishop_mask(color) & ~FiguresCounter::s_whiteMask_;
    if(bi_mask_w)
      score.endGame_ += pop_count((pw_unprotected &  FiguresCounter::s_whiteMask_) & ~attackers) * EvalCoefficients::unprotectedPawnBishopTreat_;
    if(bi_mask_b)
      score.endGame_ += pop_count((pw_unprotected & ~FiguresCounter::s_whiteMask_) & ~attackers) * EvalCoefficients::unprotectedPawnBishopTreat_;
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
#ifndef NDEBUG
      auto hscore = evaluatePawns().score;
      hscore.opening_ += evaluateKingSafety(Figure::ColorWhite) - evaluateKingSafety(Figure::ColorBlack);
      X_ASSERT(!(info.score == hscore), "invalid pawns+king score in hash");
#endif
      return info;
    }
  }

  PasserInfo info = evaluatePawns();
  int kingSafety = evaluateKingSafety(Figure::ColorWhite) - evaluateKingSafety(Figure::ColorBlack);
  info.score.opening_ += kingSafety;

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
  int py = promo_y_[color];

  BitMask pawn_mask = pmask;
  for(; pawn_mask;)
  {
    int const n = clear_lsb(pawn_mask);
    Index idx(n);

    const int x = idx.x();
    const int y = idx.y();
    auto n1 = n + (dy << 3);
    auto pw_field = set_mask_bit(n);
    auto fwd_field = set_mask_bit(n1);

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

    auto const& protectMask = movesTable().pawnCaps(ocolor, n);
    bool pprotected = (protectMask & pmask) != 0ULL;

    bool isolated = (pmask & pawnMasks().mask_isolated(x)) == 0ULL;
    bool backward = !isolated && ((pawnMasks().mask_backward(color, n) & pmask) == 0ULL) &&
      isPawnBackward(idx, color, pmask, opmsk, fwd_field);
    bool unguarded = !isolated && !backward && !couldBeGuarded(idx, color, ocolor, pmask, opmsk, fwd_field, n1);
    bool unprotected = !unguarded && !isolated && !backward && !pprotected;
    bool neighbors = (pawnMasks().mask_neighbor(color, n) & pmask) != 0ULL;

    info.score.opening_ += isolated * EvalCoefficients::isolatedPawn_[0];
    info.score.opening_ += backward * EvalCoefficients::backwardPawn_[0];
    info.score.opening_ += unguarded * EvalCoefficients::unguardedPawn_[0];
    info.score.opening_ += unprotected * EvalCoefficients::unprotectedPawn_[0];
    info.score.opening_ += neighbors * EvalCoefficients::hasneighborPawn_[0];

    info.score.endGame_ += isolated * EvalCoefficients::isolatedPawn_[1];
    info.score.endGame_ += backward * EvalCoefficients::backwardPawn_[1];
    info.score.endGame_ += unguarded * EvalCoefficients::unguardedPawn_[1];
    info.score.endGame_ += unprotected * EvalCoefficients::unprotectedPawn_[1];
    info.score.endGame_ += neighbors * EvalCoefficients::hasneighborPawn_[1];

    // passer pawn
    const auto & halfpassmsk = pawnMasks().mask_forward(color, n);
    if(!(halfpassmsk & (opmsk|pmask)))
    {
      // save position for further usage
      info.passers_[color] |= set_mask_bit(n);

      const auto & passmsk = pawnMasks().mask_passed(color, n);
      bool halfpasser = (opmsk & passmsk);
      int cy = colored_y_[color][y];
      Index pp(x, py);

      FullScore pwscore;
      if (halfpasser)
      {
        pwscore.common_ = EvalCoefficients::passerPawn_[cy] >> 1;
        BitMask attackers = passmsk & opmsk;
        int nguards = 0;
        BitMask guards = pawnMasks().mask_guards(color, n) & pmask;
        nguards = 0;
        while (guards) {
          Index g{ clear_lsb(guards) };
          if (g.y() == y)
            nguards++;
          else {
            Index g1{ g.x(), g.y() + dy };
            if (!(set_mask_bit(g1) & (opmsk | finfo_[ocolor].pawnAttacks_))) {
              nguards++;
            }
          }
        }
        if (nguards > 0 && nguards >= pop_count(attackers)) {
          pwscore.common_ += EvalCoefficients::passerPawn_[cy] >> 2;
        }
        pwscore >>= 1;
      }
      else
      {
        pwscore.common_ = EvalCoefficients::passerPawn_[cy];
        int oking_dist_promo = distanceCounter().getDistance(board_->kingPos(ocolor), pp);
        int king_dist_promo = distanceCounter().getDistance(board_->kingPos(color), pp);
        pwscore.endGame_ += (oking_dist_promo - king_dist_promo) * EvalCoefficients::kingToPasserDistanceBonus_;
        if (BitMask guards = (pawnMasks().mask_guards(color, n) & pmask)) {
          while (guards) {
            int g = clear_lsb(guards);
            if (!(pawnMasks().mask_forward(color, g) & (opmsk | finfo_[ocolor].pawnAttacks_))) {
              pwscore.common_ += EvalCoefficients::passerPawn_[cy] >> 2;
              break;
            }
          }
        }
      }
      info.score += pwscore;
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
  infoW.score -= infoB.score;
  return infoW.score;
}

Evaluator::PasserInfo Evaluator::passerEvaluation(Figure::Color color, PasserInfo const& pi)
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmask = fmgr.pawn_mask(color);
  if(!pmask || (pi.passers_observed_[color] && !pi.passers_[color]))
    return{};
    
  const int py = promo_y_[color];
  const int dy = delta_y_[color];

  PasserInfo pinfo;

  Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & opmsk = fmgr.pawn_mask(ocolor);
  bool no_opawns = opmsk == 0ULL;

  BitMask pawn_mask = pmask;
  if(pi.passers_observed_[color])
    pawn_mask = pi.passers_[color];
  
  while(pawn_mask)
  {
    const int n = clear_lsb(pawn_mask);
    const auto & passmsk = pawnMasks().mask_passed(color, n);
    const auto& halfpassmsk = pawnMasks().mask_forward(color, n);
    if((opmsk|pmask) & halfpassmsk)
      continue;

    const Index idx(n);
    const int x = idx.x();
    const int y = idx.y();
    const int cy = colored_y_[color][y];

    const bool halfpasser = (opmsk & passmsk);
    const Index pp(x, py);
    FullScore pwscore;

    auto attack_mask = finfo_[color].attack_mask_;
    auto multiattack_mask = finfo_[color].multiattack_mask_;
    auto o_attack_mask = finfo_[ocolor].attack_mask_;
    auto o_multiattack_mask = finfo_[ocolor].multiattack_mask_;
    if (BitMask behind_msk = betweenMasks().from_dir(n, dir_behind_[color]) & (fmgr.rook_mask(color) | fmgr.queen_mask(color)))
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
    else if (BitMask o_behind_msk = betweenMasks().from_dir(n, dir_behind_[color]) & (fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor)))
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
    auto blockers_mask = ((o_attack_mask & ~attack_mask) | (o_multiattack_mask & ~multiattack_mask) | fmgr.mask(ocolor)) & ~finfo_[color].pawnAttacks_;

    // ahead fields are not blocked by opponent
    auto fwd_mask = pawnMasks().mask_forward(color, n) & blockers_mask;
    if (!fwd_mask) {
      pwscore.common_  += EvalCoefficients::passerPawn_[cy];
      pwscore.endGame_ += EvalCoefficients::passerPawn_[cy];
      // could promote on the next move and not attacked or it's turn
      if (cy == 6 && (!(set_mask_bit(n) & o_attack_mask) || color == board_->color()) &&
        !((o_attack_mask | fmgr.mask(ocolor)) & pawnMasks().mask_forward(color, n))) {
        pwscore.common_ += EvalCoefficients::canpromotePawn_[cy];
      }
    }
    else {
      int closest_blocker = (color == Figure::ColorWhite) ? _lsb64(fwd_mask) : _msb64(fwd_mask);
      int last_cango = colored_y_[color][Index(closest_blocker).y()] - 1;
      int steps = last_cango - cy;
      if (steps > 0) {
        int steps_to_promotion = 7 - cy;
        X_ASSERT(steps < 0 || steps_to_promotion < 1, "invalid number of pawn steps");
        int prmBonus = (steps*EvalCoefficients::passerPawn_[cy]) / steps_to_promotion;
        pwscore.common_  += prmBonus;
        pwscore.endGame_ += prmBonus;
      }
    }

    //// opponent king could not go to my pawns promotion path
    //if(couldIntercept(color, n, pp, std::abs(py - y) +1)) {
    //  pwscore.endGame_ -= EvalCoefficients::farKingPawn_[cy];
    //}

    if (halfpasser)
      pwscore >>= 1;
    pinfo.score += pwscore;
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
  int ky = kingPos.y();
  if((ky < 2 && color) || (ky > 5 && !color))
    ky = promo_y_[Figure::otherColor(color)];
  int ctype = getCastleType(color);
  int score = 0;
  if (ctype == 0) // king side
  {
    score = evaluateKingSafety(color, Index{6, ky}) + evaluateKingsPawn(color, kingPos);
  }
  else if (ctype == 1) // queen side
  {
    score = evaluateKingSafety(color, Index{1, ky}) + evaluateKingsPawn(color, kingPos);
  }
  else
  {
    score = evaluateKingSafety(color, kingPos) + (EvalCoefficients::kingIsUnsafe_>>1);
    if (board_->castling(color, 0)) {
      int scoreK = evaluateKingSafety(color, Index{6, promo_y_[Figure::otherColor(color)]});
      score = std::max(score, scoreK);
    }
    if (board_->castling(color, 1)) {
      int scoreQ = evaluateKingSafety(color, Index{1, promo_y_[Figure::otherColor(color)]});
      score = std::max(score, scoreQ);
    }
  }
  return score;
}

int Evaluator::evaluateKingSafety(Figure::Color color, Index const& kingPos) const
{
  static const int delta_y[2] = { -8, 8 };
  const FiguresManager & fmgr = board_->fmgr();
  auto const& pmask  = fmgr.pawn_mask(color);
  Figure::Color ocolor = Figure::otherColor(color);
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
  else if (mabove3 & pmask)
    score += EvalCoefficients::pawnPenaltyB_ >> 1;
  else
    score += EvalCoefficients::pawnPenaltyB_;

  if (kingPos.x() > 3) // right side
  {
    if (mleft1 & pmask)
      score += EvalCoefficients::pawnShieldC_[0];
    else if (mleft2 & pmask)
      score += EvalCoefficients::pawnShieldC_[1];
    else if (mleft3 & pmask)
      score += EvalCoefficients::pawnPenaltyC_ >> 1;
    else
      score += EvalCoefficients::pawnPenaltyC_;

    if (mright1 & pmask)
      score += EvalCoefficients::pawnShieldA_[0];
    else if (mright2 & pmask)
      score += EvalCoefficients::pawnShieldA_[1];
    else if (mright3 & pmask)
      score += EvalCoefficients::pawnPenaltyA_ >> 1;
    else
      score += EvalCoefficients::pawnPenaltyA_;
  }
  else // left side
  {
    if (mleft1 & pmask)
      score += EvalCoefficients::pawnShieldA_[0];
    else if (mleft2 & pmask)
      score += EvalCoefficients::pawnShieldA_[1];
    else if (mleft3 & pmask)
      score += EvalCoefficients::pawnPenaltyA_ >> 1;
    else
      score += EvalCoefficients::pawnPenaltyA_;

    if (mright1 & pmask)
      score += EvalCoefficients::pawnShieldC_[0];
    else if (mright2 & pmask)
      score += EvalCoefficients::pawnShieldC_[1];
    else if (mright3 & pmask)
      score += EvalCoefficients::pawnPenaltyC_ >> 1;
    else
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

int Evaluator::evaluateKingsPawn(Figure::Color color, Index const& kingPos) const
{
  static const int delta_y[2] = { -8, 8 };
  const FiguresManager & fmgr = board_->fmgr();
  auto const& pmask = fmgr.pawn_mask(color);
  Figure::Color ocolor = Figure::otherColor(color);
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
  
  int score = 0;
  if (mabove1 & pmask)
    score += EvalCoefficients::pawnShieldB_[0];
  else if (mabove2 & pmask)
    score += EvalCoefficients::pawnShieldB_[1];
  else if (mabove3 & pmask)
    score += EvalCoefficients::pawnPenaltyB_ >> 1;
  else
    score += EvalCoefficients::pawnPenaltyB_;
  return score;
}

bool Evaluator::fakeCastle(Figure::Color color, int rpos, BitMask rmask) const
{
  const FiguresManager & fmgr = board_->fmgr();
  Index ki_pos(board_->kingPos(color));
  Index r_pos(rpos);
  int ctype = r_pos.x() < ki_pos.x();
  if (board_->castling(color, ctype))
    return false;
  if ((ctype == 1 && ki_pos.x() > 3) || (ctype == 0 && ki_pos.x() < 4))
    return false;
  if ((color == Figure::ColorWhite && ki_pos.y() != 0) || (color == Figure::ColorBlack && ki_pos.y() != 7))
    return false;
  auto ocolor = Figure::otherColor(color);
  bool rblocked = ((blocked_rook_mask_[ocolor][ctype] & set_mask_bit(rpos)) != 0ULL) && ((rmask & ~blocked_rook_mask_[ocolor][ctype]) == 0ULL);
  if (!rblocked)
    return false;
  int y = r_pos.y() < 3 ? 0 : 7;
  int x = r_pos.x() < 2 ? 0 : 7;
  int dx = x == 0 ? 1 : -1;
  Index ppos01{ x, y + delta_y_[color] };
  Index ppos02{ x, y + 2 * delta_y_[color] };
  Index ppos03{ x, y + 3 * delta_y_[color] };
  Index ppos11{ x + dx, y + delta_y_[color] };
  Index ppos12{ x + dx, y + 2 * delta_y_[color] };
  auto pwblockers0 = set_mask_bit(ppos01) | set_mask_bit(ppos02) | set_mask_bit(ppos03);
  auto pwblockers1 = set_mask_bit(ppos11) | set_mask_bit(ppos12);
  auto const& pmask = fmgr.pawn_mask(color);
  return (pmask & pwblockers0) != 0ULL && (pmask & pwblockers1) != 0ULL;
}

bool Evaluator::blockedRook(Figure::Color color, Index rpos, BitMask rmask) const
{
  int ctype = rpos.x() < 4;
  bool rblocked = ((blocked_rook_mask_[color][ctype] & set_mask_bit(rpos)) != 0ULL) && ((rmask & ~blocked_rook_mask_[color][ctype]) == 0ULL);
  if (!rblocked)
    return false;
  auto ocolor = Figure::otherColor(color);
  const FiguresManager & fmgr = board_->fmgr();
  int y = rpos.y() < 3 ? 0 : 7;
  int x = rpos.x() < 2 ? 0 : 7;
  int dx = x == 0 ? 1 : -1;
  Index ppos01{ x, y + delta_y_[ocolor] };
  Index ppos02{ x, y + 2 * delta_y_[ocolor] };
  Index ppos03{ x, y + 3 * delta_y_[ocolor] };
  Index ppos11{ x + dx, y + delta_y_[ocolor] };
  Index ppos12{ x + dx, y + 2 * delta_y_[ocolor] };
  auto pwblockers0 = set_mask_bit(ppos01) | set_mask_bit(ppos02) | set_mask_bit(ppos03);
  auto pwblockers1 = set_mask_bit(ppos11) | set_mask_bit(ppos12);
  auto const& pmask = fmgr.pawn_mask(ocolor);
  bool opawns_blocking = (pmask & pwblockers0) != 0ULL || (pmask & pwblockers1) != 0ULL;
  if (!opawns_blocking)
    return false;
  //int ki_pos = board_->kingPos(color);
  //int ki_dist = distanceCounter().getDistance(rpos, ki_pos);
  //if (ki_dist < 3)
  //  return false;
  return true;
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

  // bonus for double bishop
  if (fmgr.bishops(Figure::ColorWhite) >= 2)
  {
    const int pawnsN = fmgr.pawns(Figure::ColorWhite);
    score.opening_ += EvalCoefficients::doubleBishopBonus_[0][pawnsN];
    score.endGame_ += EvalCoefficients::doubleBishopBonus_[1][pawnsN];
  }
  if (fmgr.bishops(Figure::ColorBlack) >= 2)
  {
    const int pawnsN = fmgr.pawns(Figure::ColorBlack);
    score.opening_ -= EvalCoefficients::doubleBishopBonus_[0][pawnsN];
    score.endGame_ -= EvalCoefficients::doubleBishopBonus_[1][pawnsN];
  }

  // bonus for 2 knights
  if (knightsDiff >= 2 || knightsDiff <= -2)
  {
    int kndiff = sign(knightsDiff);
    Figure::Color ncolor = static_cast<Figure::Color>(knightsDiff > 0);
    const int pawnsN = fmgr.pawns(ncolor);
    score.opening_ += kndiff * EvalCoefficients::twoKnightsBonus_[0][pawnsN];
    score.endGame_ += kndiff * EvalCoefficients::twoKnightsBonus_[1][pawnsN];
  }
  // bonus for one bishops
  if (bishopsDiff == 1 || bishopsDiff == -1)
  {
    Figure::Color ncolor = static_cast<Figure::Color>(bishopsDiff > 0);
    const int pawnsN = fmgr.pawns(ncolor);
    score.opening_ += bishopsDiff * EvalCoefficients::oneBishopBonus_[0][pawnsN];
    score.endGame_ += bishopsDiff * EvalCoefficients::oneBishopBonus_[1][pawnsN];
  }
  // bonus for 2 bishops
  if (bishopsDiff >= 2 || bishopsDiff <= -2)
  {
    int bdiff = sign(bishopsDiff);
    Figure::Color bcolor = static_cast<Figure::Color>(bishopsDiff > 0);
    const int pawnsN = fmgr.pawns(bcolor);
    score.opening_ += bdiff * EvalCoefficients::twoBishopsBonus_[0][pawnsN];
    score.endGame_ += bdiff * EvalCoefficients::twoBishopsBonus_[1][pawnsN];
  }
  // bonus for 2 rooks
  if (rooksDiff >= 2 || rooksDiff <= -2)
  {
    int rdiff = sign(rooksDiff);
    Figure::Color rcolor = static_cast<Figure::Color>(rooksDiff > 0);
    const int pawnsN = fmgr.pawns(rcolor);
    score.opening_ += rdiff * EvalCoefficients::twoRooksBonus_[0][pawnsN];
    score.endGame_ += rdiff * EvalCoefficients::twoRooksBonus_[1][pawnsN];
  }
  // Figure vs. Pawns
  if(!rooksDiff && figuresDiff*pawnsDiff < 0)
  {
    Figure::Color fcolor = static_cast<Figure::Color>(figuresDiff > 0);
    const int pawnsN = fmgr.pawns(fcolor);
    int k = sign(figuresDiff);
    score.opening_ += k * EvalCoefficients::figureAgainstPawnBonus_[0][pawnsN];
    score.endGame_ += k * EvalCoefficients::figureAgainstPawnBonus_[1][pawnsN];
  }
  // Rook vs. Pawns
  if(!figuresDiff && rooksDiff*pawnsDiff < 0)
  {
    Figure::Color rcolor = static_cast<Figure::Color>(rooksDiff > 0);
    const int pawnsN = fmgr.pawns(rcolor);
    int k = sign(rooksDiff);
    score.opening_ += k * EvalCoefficients::rookAgainstPawnBonus_[0][pawnsN];
    score.endGame_ += k * EvalCoefficients::rookAgainstPawnBonus_[1][pawnsN];
  }
  // Knight|Bishop+Pawns vs. Rook
  if(rooksDiff*figuresDiff == -1)
  {
    Figure::Color rcolor = static_cast<Figure::Color>(rooksDiff > 0);
    const int pawnsN = fmgr.pawns(rcolor);
    score.opening_ += rooksDiff * EvalCoefficients::rookAgainstFigureBonus_[0][pawnsN];
    score.endGame_ += rooksDiff * EvalCoefficients::rookAgainstFigureBonus_[1][pawnsN];
  }
  // 2 figures vs. Rook
  if((rooksDiff*figuresDiff <= -2) && (std::abs(rooksDiff) == 1))
  {
    Figure::Color fcolor = static_cast<Figure::Color>(figuresDiff > 0);
    const int pawnsN = fmgr.pawns(fcolor);
    int k = sign(rooksDiff);
    score.opening_ -= k * EvalCoefficients::figuresAgainstRookBonus_[0][pawnsN];
    score.endGame_ -= k * EvalCoefficients::figuresAgainstRookBonus_[1][pawnsN];
  }

  return score;
}

ScoreType Evaluator::evaluateForks(Figure::Color color)
{
  Figure::Color ocolor = Figure::otherColor(color);
  BitMask o_rq_mask = board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
  BitMask o_mask = board_->fmgr().knight_mask(ocolor) | board_->fmgr().bishop_mask(ocolor) | o_rq_mask;
  BitMask pawn_fork = o_mask & (finfo_[color].pawnAttacks_);
  int pawnsN = pop_count(pawn_fork);
  ScoreType forkScore = 0;
  BitMask counted_mask = pawn_fork;
  if (pawnsN > 1) {
    forkScore += EvalCoefficients::doublePawnAttack_;
  }
  else if(pawnsN == 1) {
    forkScore += EvalCoefficients::doublePawnAttack_ >> 2;
    if(color == board_->color())
      forkScore += EvalCoefficients::doublePawnAttack_ >> 2;
  }
  
  BitMask pfwd_attacks = (finfo_[color].pawns_fwd_ & (finfo_[color].attack_mask_ | ~finfo_[ocolor].attack_mask_)) & ~finfo_[ocolor].pawnAttacks_;
  if(color)
    pfwd_attacks = (((pfwd_attacks << 9) & Figure::pawnCutoffMasks_[0]) | ((pfwd_attacks << 7) & Figure::pawnCutoffMasks_[1])) & 0xffffffffffffff00;
  else
    pfwd_attacks = (((pfwd_attacks >> 7) & Figure::pawnCutoffMasks_[0]) | ((pfwd_attacks >> 9) & Figure::pawnCutoffMasks_[1])) & 0x00ffffffffffffff;
  pawn_fork = o_mask & pfwd_attacks;
  counted_mask |= pawn_fork;
  pawnsN = pop_count(pawn_fork);
  if (pawnsN > 1) {
    forkScore += EvalCoefficients::doublePawnAttack_ >> 1;
  }
  else if (pawnsN == 1) {
    forkScore += EvalCoefficients::doublePawnAttack_ >> 4;
    if (color == board_->color())
      forkScore += EvalCoefficients::doublePawnAttack_ >> 4;
  }

  BitMask kn_fork = o_rq_mask & finfo_[color].knightMoves_;
  counted_mask |= kn_fork;
  int knightsN = pop_count(kn_fork);
  if (knightsN > 1) {
    forkScore += EvalCoefficients::knightForkBonus_;
  }
  else if(knightsN == 1) {
    forkScore += EvalCoefficients::knightForkBonus_ >> 2;
    if (color == board_->color())
      forkScore += EvalCoefficients::knightForkBonus_ >> 2;
  }
  BitMask bi_treat = o_rq_mask & finfo_[color].bishopTreatAttacks_;
  counted_mask |= bi_treat;
  int bishopsN = pop_count(bi_treat);
  if (bishopsN > 1) {
    forkScore += EvalCoefficients::bishopsAttackBonus_;
  }
  else if (bishopsN == 1) {
    forkScore += EvalCoefficients::bishopsAttackBonus_ >> 2;
    if (color == board_->color())
      forkScore += EvalCoefficients::bishopsAttackBonus_ >> 2;
  }
  auto r2q_treat = board_->fmgr().queen_mask(ocolor) & finfo_[color].rookMoves_;
  counted_mask |= r2q_treat;
  if (r2q_treat) {
    forkScore += EvalCoefficients::queenUnderRookAttackBonus_ >> 1;
    if(color == board_->color())
      forkScore += EvalCoefficients::queenUnderRookAttackBonus_ >> 1;
  }
  auto treat_mask = (finfo_[color].attack_mask_ & ~finfo_[ocolor].attack_mask_) | (finfo_[color].multiattack_mask_ & ~finfo_[ocolor].multiattack_mask_);
  treat_mask &= ~counted_mask;
  auto min_treat_mask = (finfo_[color].attack_mask_ & ~treat_mask) & ~counted_mask;
  auto generalScore = pop_count(treat_mask & finfo_[ocolor].nbrq_mask_) * EvalCoefficients::generalAttackBonus_;
  generalScore += (pop_count(min_treat_mask & finfo_[ocolor].nbrq_mask_) * EvalCoefficients::generalAttackBonus_) >> 2;
  if (generalScore > 0 && color == board_->color()) {
    generalScore += (generalScore >> 1);
  }
  forkScore += generalScore;
  return forkScore;
}

} //NEngine
