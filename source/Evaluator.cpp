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

void Evaluator::initialize(Board const* board, EHashTable* ehash, GHashTable* ghash)
{
  board_ = board;
  ehash_ = ehash;
  ghash_ = ghash;
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

  if(ehash_)
    ehash_->prefetch(board_->fmgr().kpwnCode());

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

  if(auto spec = specialCases().eval(*board_))
  {
    ScoreType score = *spec;
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

  // evaluate figures weight
  score.common_ = fmgr.weight() + evaluateMaterialDiff();

  /// use lazy evaluation level 0
  {
    auto score0 = considerColor(score.common_);
    if(score0 < alpha0_ || score0 > betta0_)
      return score0;
  }

  prepare();

  // take pawns eval from hash if possible
  auto hashedScore = hashedEvaluation();
  score += hashedScore;

  // distance from king to figures
  score += evaluateKpressure();

  // penalty for lost or fake castle
  score.opening_ += evaluateCastle();

  // determine game phase (opening, middle or end game)
  auto phaseInfo = detectPhase();

  score.opening_ += fmgr.eval(0);
  score.endGame_ += fmgr.eval(1);

  score.opening_ += evaluateBlockedKnights();
  if(phaseInfo.phase_ != EndGame)
  {
    score.opening_ += evaluateBlockedBishops();
  }

  auto scoreForks = evaluateForks(Figure::ColorWhite);
  scoreForks -= evaluateForks(Figure::ColorBlack);
  score.common_ += scoreForks;

  score.common_ += evaluateOpenRook(Figure::ColorWhite);
  score.common_ -= evaluateOpenRook(Figure::ColorBlack);

  //// pawns attack to king
  //score.common_ += evalCoeffs().pawnAttackBonus_ * ((finfo_[0].kingAttacks_ & finfo_[1].pawnAttacks_) != 0ULL);
  //score.common_ -= evalCoeffs().pawnAttackBonus_ * ((finfo_[1].kingAttacks_ & finfo_[0].pawnAttacks_) != 0ULL);

  //auto scoreKnights = evaluateKnights(Figure::ColorWhite);
  //scoreKnights -= evaluateKnights(Figure::ColorBlack);
  //score += scoreKnights;

  ///// use lazy evaluation level 1
  //{
  //  auto score1 = considerColor(lipolScore(score, phaseInfo));
  //  if(score1 < alpha1_ || score1 > betta1_)
  //    return score1;
  //}

  //// PSQ - evaluation
  //// + attacked fields
  //auto scorePsq = evaluatePsq(Figure::ColorWhite);
  //scorePsq -= evaluatePsq(Figure::ColorBlack);
  //score += scorePsq;

  //auto scorePP = evaluatePawnsPressure(Figure::ColorWhite);
  //scorePP -= evaluatePawnsPressure(Figure::ColorBlack);
  //score += scorePP;

  //// detailed passer evaluation
  //auto passerScore = passerEvaluation();
  //score += passerScore;

  //auto mobilityScore = evaluateMobility(Figure::ColorWhite);
  //mobilityScore -= evaluateMobility(Figure::ColorBlack);
  //score += mobilityScore;

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
  auto pw_mask = fmgr.pawn_mask(ocolor);
  auto pw_protected = pw_mask & finfo_[ocolor].pawnAttacks_;
  auto pw_unprotected = pw_mask ^ pw_protected;
  auto attackers = finfo_[color].attack_mask_ & ~finfo_[color].pawnAttacks_;
  score.common_ = pop_count(pw_protected & attackers) * evalCoeffs().protectedPawnPressure_;
  score.common_ += pop_count(pw_unprotected & attackers) * evalCoeffs().unprotectedPawnPressure_;
  // bishop treat
  if(fmgr.bishops(color) == 1)
  {
    auto bi_mask = (fmgr.bishop_mask(color) & FiguresCounter::s_whiteMask_)
      ?  FiguresCounter::s_whiteMask_
      : ~FiguresCounter::s_whiteMask_;
    score.endGame_ += pop_count(pw_protected   & bi_mask) * evalCoeffs().protectedPawnBishopTreat_;
    score.endGame_ += pop_count(pw_unprotected & bi_mask) * evalCoeffs().unprotectedPawnBishopTreat_;
  }
  return score;
}

Evaluator::FullScore Evaluator::hashedEvaluation()
{
  HEval * heval = 0;
  const uint64 & code = board_->fmgr().kpwnCode();
  uint32 hkey = (uint32)(code >> 32);

  if(ehash_)
  {
    heval = ehash_->get(code);

    if(heval->hkey_ == hkey && heval->initizalized_)
    {
      FullScore score;
      score.common_ = heval->common_;
      score.opening_ = heval->opening_;
      score.endGame_ = heval->endGame_;
      X_ASSERT(!(score == evaluatePawns() + evaluateKingSafety()), "invalid pawns+king score in hash");
      return score;
    }
  }

  auto score = evaluatePawns();

  // basic king safety - pawn shield
  auto kingScore = evaluateKingSafety();
  score += kingScore;

  if(heval)
  {
    heval->hkey_ = hkey;
    heval->common_ = score.common_;
    heval->opening_ = score.opening_;
    heval->endGame_ = score.endGame_;
    heval->initizalized_ = 1;
  }

  return score;
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

Evaluator::FullScore Evaluator::evaluatePawns(Figure::Color color) const
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmask = fmgr.pawn_mask(color);
  if(!pmask)
    return{};

  FullScore score;
  score.endGame_ = fmgr.pawns(color) * evalCoeffs().pawnEndgameBonus_;

  static int promo_y[] = { 0, 7 };
  static int delta_y[] = { -1, +1 };

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

    score.endGame_ -= evalCoeffs().kingToPawnBonus_[dist_to_oking];
    score.endGame_ += evalCoeffs().kingToPawnBonus_[dist_to_myking];

    // doubled pawn
    if(!(x_visited & (1<<x)))
    {
      x_visited |= 1<<x;
      auto const& dblmsk = pawnMasks().mask_doubled(x);
      auto num_dbl = pop_count(pmask & dblmsk);
      X_ASSERT(num_dbl <= 0, "doubled pawns number is zero or negative");
      score.common_ += (num_dbl - 1) * evalCoeffs().doubledPawn_;
    }

    // isolated pawn
    bool unsupported{ false };
    bool isolated = (pmask & pawnMasks().mask_isolated(x)) == 0ULL;
    score.common_ += isolated * evalCoeffs().isolatedPawn_;

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
        score.common_ += blocked * evalCoeffs().backwardPawn_;
      }
      // forward but not supported
      else if(!couldBeSupported(idx, color, ocolor, pmask, opmsk))
      {
        unsupported = true;
        score.common_ += evalCoeffs().unsupportedPawn_;
      }
      // unprotected
      else
      {
        bool unprotected = (pawnMasks().mask_supported(color, n) & pmask) == 0ULL;
        score.common_ += unprotected * evalCoeffs().unprotectedPawn_;
      }
    }

    // passed pawn
    {
      if((opmsk & pawnMasks().mask_passed(color, n)) == 0ULL)
      {
        x_passers |= 1 << x;
        score.common_ += evalCoeffs().passerPawn_[cy];
        // supported
        if(pawnMasks().mask_supported(color, n) & pmask)
          score.common_ += evalCoeffs().protectedPasser_[cy];
        Index pp(x, py);
        int pawn_dist_promo = std::abs(py - idx.y());
        int o_dist_promo = distanceCounter().getDistance(board_->kingPos(ocolor), pp) - (board_->color() == ocolor);
        if(pawn_dist_promo < o_dist_promo)
          score.endGame_ += evalCoeffs().farKingPawn_[cy] >> 1;
      }
      // quadpasser
      else if((pawnMasks().mask_line_blocked(color, n) & opmsk) == 0ULL)
      {
        bool left = (x != 0) && ((pawnMasks().mask_line_blocked(color, Index(x-1, idx.y())) & opmsk)== 0ULL);
        bool right = (x != 7) && ((pawnMasks().mask_line_blocked(color, Index(x+1, idx.y())) & opmsk) == 0ULL);
        X_ASSERT(((left && right) || (x == 0 && right) || (x == 7 && left)), "passed pawn was not detected");
        auto const& semipasserCoeff = evalCoeffs().semipasserPawn_[cy];
        auto scoreSemipasser = ((semipasserCoeff >> 1) + (left || right) * (semipasserCoeff >> 1));
        if(pawnMasks().mask_supported(color, n) & pmask)
          scoreSemipasser += evalCoeffs().protectedPasser_[cy] >> 2;
        score.common_ += scoreSemipasser;
      }
    }
  }

  return score;
}

Evaluator::FullScore Evaluator::passerEvaluation() const
{
  auto infoW = passerEvaluation(Figure::ColorWhite);
  auto infoB = passerEvaluation(Figure::ColorBlack);
  if(infoW.most_y > infoB.most_y && infoB.most_y > 0)
    infoW.score.endGame_ += evalCoeffs().closeToPromotion_[infoW.most_y - infoB.most_y];
  else if(infoB.most_y > infoW.most_y && infoW.most_y > 0)
    infoB.score.endGame_ += evalCoeffs().closeToPromotion_[infoB.most_y - infoW.most_y];
  infoW.score -= infoB.score;
  return infoW.score;
}

Evaluator::PasserInfo Evaluator::passerEvaluation(Figure::Color color) const
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmask = fmgr.pawn_mask(color);
  if(!pmask)
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
  for(; pawn_mask;)
  {
    const int n = clear_lsb(pawn_mask);
    Index idx(n);

    int x = idx.x();
    int y = idx.y();
    int cy = colored_y_[color][idx.y()];

    //// opponent king should be far from my pawn
    //// small bonus for short distance to my king
    //// double if opponent has no pawns
    //int dist_to_oking = distanceCounter().getDistance(board_->kingPos(ocolor), n);
    //int dist_to_myking = distanceCounter().getDistance(board_->kingPos(color), n);

    //info.score.endGame_ += evalCoeffs().oKingToPawnBonus_[dist_to_oking];
    //info.score.endGame_ += evalCoeffs().myKingToPawnBonus_[dist_to_myking] * (1 + no_opawns);

    const uint64 & passmsk = pawnMasks().mask_passed(color, n);
    const uint64 & blckmsk = pawnMasks().mask_line_blocked(color, n);
    
    if((opmsk & passmsk) || (pmask & blckmsk))
      continue;

    //// bonus for distance from passer to kings
    //info.score.endGame_ += evalCoeffs().oKingToPasserBonus_[dist_to_oking];
    //info.score.endGame_ += evalCoeffs().myKingToPasserBonus_[dist_to_myking] * (1 + no_opawns);

    if(cy > info.most_y)
      info.most_y = cy;

    int promo_pos = x | (py<<3);
    
    bool king_far = false;
    int pawn_dist_promo = std::abs(py - y);
    int o_dist_promo = distanceCounter().getDistance(board_->kingPos(ocolor), promo_pos) - (board_->color() == ocolor);
    king_far = pawn_dist_promo < o_dist_promo;
    
    // my rook behind passed pawn - give bonus
    BitMask behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & fmgr.rook_mask(color) ;
    BitMask o_behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & fmgr.rook_mask(ocolor) ;
    if ( behind_msk )
    {
      int rpos = color ? _msb64(behind_msk) : _lsb64(behind_msk);
      if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
        info.score.common_ += evalCoeffs().rookBehindBonus_;
    }
    else if ( o_behind_msk ) // may be opponent's rook - give penalty
    {
      int rpos = color ? _msb64(o_behind_msk) : _lsb64(o_behind_msk);
      if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
        info.score.common_ -= evalCoeffs().rookBehindBonus_;
    }
    
    // pawn can go to the next line
    int next_pos = x | ((y+dy)<<3);
    X_ASSERT( ((y+dy)) > 7 || ((y+dy)) < 0, "pawn goes to invalid line" );
    
    bool can_go = false;
    
    // field is empty
    // check is it attacked by opponent
    auto const& next_field = board_->getField(next_pos);
    if(!next_field)
    {
      BitMask next_mask = set_mask_bit(next_pos);
      if((next_mask & finfo_[ocolor].attack_mask_) == 0)
        can_go = true;
      //else
      //{
      //  Figure::Type type = Figure::TypeNone;
      //  if(next_pos == promo_pos)
      //    type = Figure::TypeQueen;

      //  Move move(n, next_pos, type);
      //  ScoreType see_score = board_->see(move);
      //  can_go = see_score >= 0;
      //}
      // additional bonus if pawn can go forward
      info.score.common_ += can_go * evalCoeffs().cangoPawn_[cy];
    }
    // field is occupied by my figure
    else if(next_field.color() == color)
    {
      can_go = true;
      // small additional bonus if pawn potentially can go forward
      info.score.common_ += (can_go * evalCoeffs().cangoPawn_[cy]) >> 1;
    }

    // all ahead fields are not occupied and not attacked | protected
    auto fwd_mask = pawnMasks().mask_line_blocked(color, n);
    bool can_promote = (fwd_mask & blockers_mask) == 0ULL;
    info.score.common_ += can_promote * evalCoeffs().canpromotePawn_[cy];
    
    // opponent king could not go to my pawns promotion field
    if(king_far || !findRootToPawn(color, promo_pos, pawn_dist_promo+1))
    {
      info.score.endGame_ += evalCoeffs().farKingPawn_[cy];
    }
  }
  info.most_y += (color == board_->color());
  return info;
}

// idea from CCRL
bool Evaluator::findRootToPawn(Figure::Color color, int promo_pos, int stepsMax) const
{
  return NEngine::findRootToPawn(*board_, inv_mask_all_, finfo_[color].attack_mask_, color, promo_pos, stepsMax);
}

int Evaluator::getCastleType(Figure::Color color) const
{
  static const BitMask castle_mask[2][2] = {
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

  auto const& ki_mask = board_->fmgr().king_mask(color);

  // short
  bool cking = (castle_mask[color][0] & ki_mask) != 0;

  // long
  bool cqueen = (castle_mask[color][1] & ki_mask) != 0;

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

  static const BitMask opponent_pawn_mask[2][2][3] =
  {
    // black
    {
      // short
      {
        set_mask_bit(H6)|set_mask_bit(H7),
        set_mask_bit(G6)|set_mask_bit(G7),
        set_mask_bit(F6)|set_mask_bit(F7)
      },
      // long
      {
        set_mask_bit(A6)|set_mask_bit(A7),
        set_mask_bit(B6)|set_mask_bit(B7),
        set_mask_bit(C6)|set_mask_bit(C7),
      }
    },
    // white
    {
      // short
      {
        set_mask_bit(H2)|set_mask_bit(H3),
        set_mask_bit(G2)|set_mask_bit(G3),
        set_mask_bit(F2)|set_mask_bit(F3)
      },
      // long
      {
        set_mask_bit(A2)|set_mask_bit(A3),
        set_mask_bit(B2)|set_mask_bit(B3),
        set_mask_bit(C2)|set_mask_bit(C3),
      }
    },
  };

  auto ctype = getCastleType(color);
  if(ctype >= 0)
  {
    int score = 0;
    int shield_bonus = 0;
    if(pawns_shield_mask[color][ctype][0][0] & pmask)
      shield_bonus += evalCoeffs().pawnShieldA_[0];
    else if(pawns_shield_mask[color][ctype][0][1] & pmask)
      shield_bonus += evalCoeffs().pawnShieldA_[1];
    else
      shield_bonus += evalCoeffs().pawnPenaltyA_;

    if(pawns_shield_mask[color][ctype][1][0] & pmask)
      shield_bonus += evalCoeffs().pawnShieldB_[0];
    else if(pawns_shield_mask[color][ctype][1][1] & pmask)
      shield_bonus += evalCoeffs().pawnShieldB_[1];
    else
      shield_bonus += evalCoeffs().pawnPenaltyB_ >> 1;

    if(pawns_shield_mask[color][ctype][2][0] & pmask)
      shield_bonus += evalCoeffs().pawnShieldC_[0];
    else if(pawns_shield_mask[color][ctype][2][1] & pmask)
      shield_bonus += evalCoeffs().pawnShieldC_[1];
    else
      shield_bonus += evalCoeffs().pawnPenaltyC_ >> 1;

    int shield_penalty =
        ((pawns_penalty_mask[color][ctype][0] & pmask) == 0ULL) * evalCoeffs().pawnPenaltyA_
      + ((pawns_penalty_mask[color][ctype][1] & pmask) == 0ULL) * evalCoeffs().pawnPenaltyB_
      + ((pawns_penalty_mask[color][ctype][2] & pmask) == 0ULL) * evalCoeffs().pawnPenaltyC_;

    int opponent_penalty =
        ((opponent_pawn_mask[color][ctype][0] & opmask) != 0ULL) * evalCoeffs().opponentPawnA_
      + ((opponent_pawn_mask[color][ctype][1] & opmask) != 0ULL) * evalCoeffs().opponentPawnB_
      + ((opponent_pawn_mask[color][ctype][2] & opmask) != 0ULL) * evalCoeffs().opponentPawnC_;

    score += shield_bonus;
    score += shield_penalty;
    score += opponent_penalty;

    return score;
  }
  else
  {
    static const int delta_y[2] = { -8, 8 };
    int score = evalCoeffs().castleImpossible_;

    int above1 = (board_->kingPos(color) + delta_y[color]) & 63;
    int above2 = (above1 + delta_y[color]) & 63;
    BitMask mabove1{ set_mask_bit(board_->kingPos(color)) | set_mask_bit(above1) };
    BitMask mabove2{ set_mask_bit(above2) };
    auto mleft1  = (mabove1 >> 1) & Figure::pawnCutoffMasks_[1];
    auto mright1 = (mabove1 << 1) & Figure::pawnCutoffMasks_[0];
    auto mleft2  = (mabove2 >> 1) & Figure::pawnCutoffMasks_[1];
    auto mright2 = (mabove2 << 1) & Figure::pawnCutoffMasks_[0];

    if(mabove1 & pmask)
      score += evalCoeffs().pawnShieldB_[0] >> 1;
    else if(mabove2 & pmask)
      score += evalCoeffs().pawnShieldB_[1] >> 1;
    else
      score += evalCoeffs().pawnPenaltyB_;

    if((board_->kingPos(color) & 7) > 3) // right side
    {
      if(mleft1 & pmask)
        score += evalCoeffs().pawnShieldC_[0] >> 1;
      else if(mleft2 & pmask)
        score += evalCoeffs().pawnShieldC_[1] >> 1;
      else
        score += evalCoeffs().pawnPenaltyC_;

      if(mright1 & pmask)
        score += evalCoeffs().pawnShieldA_[0] >> 1;
      else if(mright2 & pmask)
        score += evalCoeffs().pawnShieldA_[1] >> 1;
      else
        score += evalCoeffs().pawnPenaltyA_;
    }
    else // left side
    {
      if(mleft1 & pmask)
        score += evalCoeffs().pawnShieldA_[0] >> 1;
      else if(mleft2 & pmask)
        score += evalCoeffs().pawnShieldA_[1] >> 1;

      if(mright1 & pmask)
        score += evalCoeffs().pawnShieldC_[0] >> 1;
      else if(mright2 & pmask)
        score += evalCoeffs().pawnShieldC_[1] >> 1;
    }
    return score;
  }
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
      return evalCoeffs().fakeCastle_;
  }
  return evalCoeffs().castleBonus_;
}

int Evaluator::evaluateBlockedKnights()
{
  int score_b = 0, score_w = 0;
  BitMask knight_w = board_->fmgr().knight_mask(Figure::ColorWhite);
  for(; knight_w;)
  {
    int n = clear_lsb(knight_w);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
    finfo_[Figure::ColorWhite].attack_mask_ |= knight_moves;
    finfo_[Figure::ColorWhite].knightAttacks_ |= knight_moves;

    switch(n)
    {
    case A8:
      if(board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn))
         score_w -= evalCoeffs().knightBlocked_;
      break;

    case A7:
      if(board_->isFigure(A6, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
         score_w -= evalCoeffs().knightBlocked_;
      break;

    case B8:
      if(board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
        score_w -= evalCoeffs().knightBlocked_;
      break;

    case H8:
      if(board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn))
         score_w -= evalCoeffs().knightBlocked_;
      break;

    case H7:
      if(board_->isFigure(H6, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
         score_w -= evalCoeffs().knightBlocked_;
      break;

    case G8:
      if(board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
        score_w -= evalCoeffs().knightBlocked_;
      break;
    }
  }

  BitMask knight_b = board_->fmgr().knight_mask(Figure::ColorBlack);
  for(; knight_b;)
  {
    int n = clear_lsb(knight_b);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
    finfo_[Figure::ColorBlack].attack_mask_ |= knight_moves;
    finfo_[Figure::ColorBlack].knightAttacks_ |= knight_moves;

    switch(n)
    {
    case A1:
      if(board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn))
         score_b -= evalCoeffs().knightBlocked_;
      break;

    case A2:
      if(board_->isFigure(A3, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
         score_b -= evalCoeffs().knightBlocked_;
      break;

    case B1:
      if(board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
        score_b -= evalCoeffs().knightBlocked_;
      break;

    case H1:
      if(board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn))
         score_b -= evalCoeffs().knightBlocked_;
      break;

    case H2:
      if(board_->isFigure(H3, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
         score_b -= evalCoeffs().knightBlocked_;
      break;

    case G1:
      if(board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
        score_b -= evalCoeffs().knightBlocked_;
      break;
    }
  }
  return score_w - score_b;
}

int Evaluator::evaluateBlockedBishops()
{
  int score_b = 0, score_w = 0;
  BitMask bimask_w = board_->fmgr().bishop_mask(Figure::ColorWhite);
  for(; bimask_w;)
  {
    int n = clear_lsb(bimask_w);
    switch(n)
    {
    case A7:
      if(board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn))
        score_w -= evalCoeffs().bishopBlocked_;
      break;

    case A6:
      if(board_->isFigure(B5, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(C6, Figure::ColorBlack, Figure::TypePawn))
         score_w -= evalCoeffs().bishopBlocked_;
      break;

    case A8:
      if(board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
        score_w -= evalCoeffs().bishopBlocked_;
      break;

    case B8:
      if(board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn) &&
         (board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn)))
         score_w -= evalCoeffs().bishopBlocked_;
      break;

    case H7:
      if(board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn))
        score_w -= evalCoeffs().bishopBlocked_;
      break;

    case H8:
      if(board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
        score_w -= evalCoeffs().bishopBlocked_;
      break;

    case G8:
      if(board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn) &&
         (board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn)))
         score_w -= evalCoeffs().bishopBlocked_;
      break;

    case H6:
      if(board_->isFigure(G5, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(F6, Figure::ColorBlack, Figure::TypePawn))
         score_w -= evalCoeffs().bishopBlocked_;
      break;
    }
  }

  BitMask bimask_b = board_->fmgr().bishop_mask(Figure::ColorBlack);
  for(; bimask_b;)
  {
    int n = clear_lsb(bimask_b);

    switch(n)
    {
    case A2:
      if(board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn))
        score_b -= evalCoeffs().bishopBlocked_;
      break;

    case A3:
      if(board_->isFigure(B4, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(C3, Figure::ColorWhite, Figure::TypePawn))
         score_b -= evalCoeffs().bishopBlocked_;
      break;

    case A1:
      if(board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
        score_b -= evalCoeffs().bishopBlocked_;
      break;

    case B1:
      if(board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn) &&
         (board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn)))
         score_b -= evalCoeffs().bishopBlocked_;
      break;

    case H2:
      if(board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn))
        score_b -= evalCoeffs().bishopBlocked_;
      break;

    case H1:
      if(board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
        score_b -= evalCoeffs().bishopBlocked_;
      break;

    case G1:
      if(board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn) &&
         (board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn)))
         score_b -= evalCoeffs().bishopBlocked_;
      break;

    case H3:
      if(board_->isFigure(G4, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(F3, Figure::ColorWhite, Figure::TypePawn))
         score_b -= evalCoeffs().bishopBlocked_;
      break;
    }
  }
  return score_w - score_b;
}

int Evaluator::evaluateBlockedRooks()
{
  auto evaluate_blocked = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    auto blockers = board_->fmgr().pawn_mask(ocolor) | board_->fmgr().bishop_mask(ocolor) | board_->fmgr().knight_mask(ocolor);
    BitMask romask = board_->fmgr().rook_mask(color);
    int dy = (color<<1) - 1;
    int score = 0;
    for(; romask;)
    {
      Index n(clear_lsb(romask));
      BitMask block_mask = movesTable().blocked_rook(n);
      bool is_blocked = ((blockers & block_mask) == block_mask);
      score -= is_blocked * evalCoeffs().rookBlocked_;
    }
    return score;
  };
  auto score_w = evaluate_blocked(Figure::ColorWhite);
  auto score_b = evaluate_blocked(Figure::ColorBlack);
  return score_w - score_b;
}

ScoreType Evaluator::evaluateMaterialDiff() const
{
  ScoreType score = 0;
  const FiguresManager & fmgr = board_->fmgr();

  // bonus for bishops
  score += fmgr.bishops(Figure::ColorWhite) * evalCoeffs().bishopBonus_;
  score -= fmgr.bishops(Figure::ColorBlack) * evalCoeffs().bishopBonus_;

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
    score += figuresDiff * evalCoeffs().figureAgainstPawnBonus_[pawnsN];
  }

  // Knight|Bishop+2Pawns vs. Rook
  else if(!queensDiff && (rooksDiff*figuresDiff == -1))
  {
    Figure::Color strongColor = (Figure::Color)(rooksDiff > 0);
    int pawnsN = fmgr.pawns(strongColor) != 0;
    score += rooksDiff * evalCoeffs().rookAgainstFigureBonus_[pawnsN];
  }

  // 2 figures vs. Rook
  else if(!queensDiff && rooksDiff*figuresDiff == -2 && std::abs(rooksDiff) == 1)
  {
    Figure::Color rookColor  = (Figure::Color)(rooksDiff > 0);
    Figure::Color ocolor = Figure::otherColor(rookColor);
    int pawnsN = fmgr.pawns(rookColor) != 0 && fmgr.pawns(ocolor) != 0;
    score -= rooksDiff * evalCoeffs().figuresAgainstRookBonus_[pawnsN];
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
    return evalCoeffs().doublePawnAttack_;
  BitMask kn_fork = o_rq_mask & finfo_[color].knightAttacks_;
  int knightsN = pop_count(kn_fork);
  if(knightsN > 1 || (pawnsN+knightsN > 0 && color == board_->color()))
    return evalCoeffs().forkBonus_;
  return 0;
}

} //NEngine
