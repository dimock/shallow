/*************************************************************
  Evaluator.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Evaluator.h>
#include <Board.h>
#include <HashTable.h>
#include <xindex.h>
#include <magicbb.h>
#include <xbitmath.h>

namespace NEngine
{
  
  //const ScoreType Evaluator::nullMoveMargin_ = 1000;
  //const ScoreType Evaluator::nullMoveVerifyMargin_ = 800;
  //
  //int Evaluator::score_ex_max_ = 0;
  //
  //
  //const ScoreType Evaluator::positionEvaluations_[2][8][64] = {
  //  // begin
  //  {
  //    // empty
  //    {},
  //
  //      // pawn
  //    {
  //      0,   0,   0,   0,   0,   0,   0,   0,
  //      5,   5,   5,   5,   5,   5,   5,   5,
  //      0,   0,   3,   8,   8,   3,   0,   0,
  //      0,   0,   2,   7,   7,   2,   0,   0,
  //      0,   0,   1,   8,   8,   1,   0,   0,
  //      2,   0,   0,   0,   0,   0,   0,   2,
  //      2,   4,   4, -10, -10,   4,   4,   2,
  //      0,   0,   0,   0,   0,   0,   0,   0
  //    },
  //
  //    // knight
  //    {
  //      -8,  -8,  -8,  -8,  -8,  -8,  -8,  -8,
  //      -8,  -8,   0,   0,   0,   0,  -8,  -8,
  //      -5,   0,   3,   4,   4,   3,   0,  -5,
  //       0,   5,   5,   5,   5,   5,   5,   0,
  //      -7,   0,   4,   5,   5,   4,   0,  -7,
  //      -8,   2,   4,   4,   4,   4,   2,  -8,
  //      -8,  -8,   0,   2,   2,   0,  -8,  -8,
  //      -8, -12,  -5,  -5,  -5,  -5, -12,  -8
  //    },
  //
  //    // bishop
  //    {
  //      -8,  -4,  -4,  -4,  -4,  -4,  -4,  -8,
  //      -2,   0,   0,   0,   0,   0,   0,  -2,
  //       2,   0,   2,   6,   6,   2,   0,   2,
  //      -2,   2,   2,   6,   6,   2,   2,  -2,
  //      -2,   0,   6,   6,   6,   6,   0,  -2,
  //       0,   6,   6,   6,   6,   6,   6,   0,
  //      -2,   2,   0,   0,   0,   0,   2,  -2,
  //      -5,  -4, -12,  -4,  -4, -12,  -4,  -5
  //    },
  //
  //    // rook
  //    {
  //     10,  10,  10,  10,  10,  10,  10,  10,
  //     15,  15,  15,  15,  15,  15,  15,  15,
  //     -2,   0,   0,   0,   0,   0,   0,  -2,
  //     -2,   0,   0,   0,   0,   0,   0,  -2,
  //     -2,   0,   0,   0,   0,   0,   0,  -2,
  //     -2,   0,   0,   0,   0,   0,   0,  -2,
  //     -2,   0,   0,   0,   0,   0,   0,  -2,
  //     -5,  -5,   0,   3,   3,   0,  -5,  -5
  //    },
  //
  //    // queen
  //    {
  //       0,   0,   0,   0,   0,   0,   0,   0,
  //       0,   0,   0,   0,   0,   0,   0,   0,
  //      -2,   0,   2,   2,   2,   2,   0,  -2,
  //      -2,   0,   2,   3,   3,   2,   0,  -2,
  //       0,   0,   2,   3,   3,   2,   0,  -2,
  //      -4,   0,   2,   2,   2,   2,   0,  -4,
  //      -4,   0,   0,   1,   1,   0,   0,  -4,
  //      -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5
  //    },
  //
  //
  //    // king
  //    {
  //      -20, -20, -20, -20, -20, -20, -20, -20,
  //      -20, -20, -20, -20, -20, -20, -20, -20,
  //      -20, -20, -20, -20, -20, -20, -20, -20,
  //      -20, -20, -20, -20, -20, -20, -20, -20,
  //      -10, -16, -16, -20, -20, -16, -16, -10,
  //      -4,  -8,   -8, -10, -10,  -8,  -8,  -4,
  //       0,   0,   -2,  -8,  -8,  -2,   0,   0,
  //       5,  12,    5,   0,   0,   5,  15,   5
  //    },
  //
  //    {}
  //  },
  //
  //    // end
  //  {
  //    // empty
  //    {},
  //
  //      // pawn
  //    {
  //      0,   0,   0,   0,   0,   0,   0,   0,
  //      16,  16,  16,  16, 16,  16,  16,  16,
  //      12,  12,  12,  12, 12,  12,  12,  12,
  //      8,   8,   8,   8,   8,   8,   8,   8,
  //      5,   5,   5,   5,   5,   5,   5,   5,
  //      3,   3,   3,   3,   3,   3,   3,   3,
  //      0,   0,   0,   0,   0,   0,   0,   0,
  //      0,   0,   0,   0,   0,   0,   0,   0
  //    },
  //
  //    // knight
  //    {},
  //
  //    // bishop
  //    {},
  //
  //    // rook
  //    {},
  //
  //    // queen
  //    {},
  //
  //    // king
  //    {
  //      -14, -12, -10, -10, -10, -10, -12, -14,
  //      -12,  -8,   0,   0,   0,   0,  -8, -12,
  //      -10,  -4,   6,   8,   8,   6,  -4, -10,
  //      -10,  -4,   8,  10,  10,   8,  -4, -10,
  //      -10,  -4,   8,  10,  10,   8,  -4, -10,
  //      -10,  -4,   6,   8,   8,   6,  -4, -10,
  //      -12,  -8,   0,   0,   0,   0,  -8, -12,
  //      -14, -12, -10, -10, -10, -10, -12, -14
  //    },
  //
  //    {}
  //  }
  //};
  //
  //
  //const ScoreType Evaluator::pawnDoubled_  = -12;
  //const ScoreType Evaluator::pawnIsolated_ = -15;
  //const ScoreType Evaluator::pawnBackward_ = -4;
  //const ScoreType Evaluator::pawnDisconnected_ = -2;
  //const ScoreType Evaluator::pawnBlocked_ = 0;
  //const ScoreType Evaluator::defendedBonus_ = 3;
  //const ScoreType Evaluator::groupsPenalty_ = 2;
  //const ScoreType Evaluator::assistantBishop_ = 8;
  //const ScoreType Evaluator::rookBehindBonus_ = 7;
  //const ScoreType Evaluator::semiopenRook_ =  12;
  //const ScoreType Evaluator::openRook_ =  15;
  //const ScoreType Evaluator::winloseBonus_ =  25;
  //const ScoreType Evaluator::bishopBonus_ = 10;
  //const ScoreType Evaluator::unstoppablePawn_ = 60;
  //const ScoreType Evaluator::kingFarBonus_ = 20;
  //const ScoreType Evaluator::fakecastlePenalty_ = 20;
  //const ScoreType Evaluator::castleImpossiblePenalty_ = 20;
  //const ScoreType Evaluator::attackedByWeakBonus_ = 10;
  //const ScoreType Evaluator::forkBonus_ = 60;
  //const ScoreType Evaluator::fianchettoBonus_ = 6;
  //const ScoreType Evaluator::rookToKingBonus_ = 6;
  //
  ///// some material difference patterns
  //const ScoreType Evaluator::figureAgainstPawnBonus_[2] = { 25, 90 };
  //const ScoreType Evaluator::rookAgainstFigureBonus_[2] = { 35, 80 };
  //const ScoreType Evaluator::rookAgainstPawnsBonus_[2] = { 45, 90 };
  //
  ///// blocked figures
  //const ScoreType Evaluator::bishopBlocked_ = 80;
  //const ScoreType Evaluator::knightBlocked_ = 80;
  //
  //const ScoreType Evaluator::pinnedKnight_ = 0;//-5;
  //const ScoreType Evaluator::pinnedBishop_ = 0;//-5;
  //const ScoreType Evaluator::pinnedRook_ = 0;//-5;
  //
  //// pawns shield
  //const ScoreType Evaluator::cf_columnOpened_ = 8;
  //const ScoreType Evaluator::bg_columnOpened_ = 25;
  //const ScoreType Evaluator::ah_columnOpened_ = 20;
  //
  //const ScoreType Evaluator::cf_columnSemiopened_ = 4;
  //const ScoreType Evaluator::bg_columnSemiopened_ = 8;
  //const ScoreType Evaluator::ah_columnSemiopened_ = 8;
  //
  //const ScoreType Evaluator::cf_columnCracked_ = 2;
  //const ScoreType Evaluator::bg_columnCracked_ = 4;
  //const ScoreType Evaluator::ah_columnCracked_ = 2;
  //
  //const ScoreType Evaluator::pawnBeforeKing_ = 5;
  //
  //// pressure to king by opponents figures
  //const ScoreType Evaluator::kingPawnPressure_   = 10;
  ////const ScoreType Evaluator::kingKnightPressure_ = 8;
  ////const ScoreType Evaluator::kingBishopPressure_ = 8;
  ////const ScoreType Evaluator::kingRookPressure_   = 8;
  ////const ScoreType Evaluator::kingQueenPressure_  = 10;
  //
  ///// pawns evaluation
  //#define MAX_PASSED_SCORE 80
  //
  //const ScoreType Evaluator::pawnPassed_[8] = { 0, 5, 10, 20, 40, 60, MAX_PASSED_SCORE, 0 };
  //const ScoreType Evaluator::passersGroup_[8] = { 0, 3, 5, 7, 9, 11, 13, 0 };
  //const ScoreType Evaluator::passerCandidate_[8] =  { 0, 2, 3, 5, 7, 10, 12, 0 };
  ////const ScoreType Evaluator::pawnCanGo_[8] = { 0, 5, 7, 10, 15, 20, 30, 0 };
  //
  //const ScoreType Evaluator::mobilityBonus_[8][32] = {
  //  {},
  //  {},
  //  {-30, -15, 0, 3, 5, 7, 9, 11},
  //  {-20, -10, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4},
  //  {-25, -15, -5, 1, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4},
  //  {-45, -35, -15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12},
  //};
  //
  //const ScoreType Evaluator::kingDistanceBonus_[8][8] = {
  //  {},
  //  {},
  //  //{12, 10, 8,  6,  3,  2, 1, 0},
  //  //{12, 10, 8,  6,  3,  2, 1, 0},
  //  //{15, 12, 10, 8,  4,  2, 1, 0},
  //  //{40, 45, 35, 20, 12, 3, 1, 0},
  //	{15, 12, 10, 7, 6, 1, 0, 0},
  //	{15, 12, 10, 7, 5, 3, 1, 0},
  //	{20, 18, 13, 9, 7, 3, 1, 0},
  //	{40, 55, 45, 25, 12, 3, 1, 0},
  //};

  //const ScoreType Evaluator::attackerNumberBonus_[8] = { 0, 2, 5, 10, 20, 30, 50, 100 };


  ////////////////////////////////////////////////////////////////////////////
  //Evaluator::Evaluator() :
  //  board_(0), ehash_(0)
  //{
  //  weightMax_ = 2*(Figure::figureWeight_[Figure::TypeQueen] +
  //    2*Figure::figureWeight_[Figure::TypeRook] + 2*Figure::figureWeight_[Figure::TypeBishop] + 2*Figure::figureWeight_[Figure::TypeKnight]);
  //
  //  alpha_ = -ScoreMax;
  //  betta_ = +ScoreMax;
  //}

  const int Evaluator::colored_y_[2][8] = {
    { 7, 6, 5, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7 } };

  void Evaluator::initialize(Board const* board, EHashTable* ehash, EvalCoefficients const* coeffs)
  {
    board_ = board;
    ehash_ = ehash;
    coeffs_ = coeffs;
  }

  void Evaluator::prepare()
  {
    mask_all_ = board_->fmgr().mask(Figure::ColorWhite) | board_->fmgr().mask(Figure::ColorBlack);
    inv_mask_all_ = ~mask_all_;

    finfo_[0] = FieldsInfo{};
    finfo_[1] = FieldsInfo{};

    finfo_[0].king_pos_ = board_->kingPos(Figure::ColorBlack);
    finfo_[1].king_pos_ = board_->kingPos(Figure::ColorWhite);


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
      finfo_[0].kingAttacks_ = movesTable().caps(Figure::TypeKing, finfo_[0].king_pos_);
      finfo_[1].kingAttacks_ = movesTable().caps(Figure::TypeKing, finfo_[1].king_pos_);

      finfo_[0].attack_mask_ |= finfo_[0].kingAttacks_;
      finfo_[1].attack_mask_ |= finfo_[1].kingAttacks_;
    }

    alpha0_ = -ScoreMax;
    betta0_ = +ScoreMax;

    alpha1_ = -ScoreMax;
    betta1_ = +ScoreMax;
  }

  //bool Evaluator::discoveredCheck(int pt, Figure::Color acolor, const BitMask & brq_mask, int ki_pos, enum PinType pinType) const
  //{
  //  const BitMask & from_msk = betweenMasks().from(ki_pos, pt);
  //  BitMask mask_all_ex = mask_all_ & ~set_mask_bit(pt);
  //  mask_all_ex &= from_msk;
  //  if ( (mask_all_ex & brq_mask) == 0 )
  //    return false;
  //
  //  int apos = ki_pos < pt ? _lsb64(mask_all_ex) : _msb64(mask_all_ex);
  //  if ( (set_mask_bit(apos) & brq_mask) == 0 ) // no BRQ on this field
  //    return false;
  //
  //  const Field & afield = board_->getField(apos);
  //  X_ASSERT( afield.color() != acolor || afield.type() < Figure::TypeBishop || afield.type() > Figure::TypeQueen, "discoveredCheck() - attacking figure isn't BRQ" );
  //
  //  nst::dirs d = figureDir().dir(ki_pos, pt);
  //  if ( d == nst::no_dir )
  //    return false;
  //
  //  if ( pinType == ptAll )
  //    return true;
  //
  //  static PinType pin_types[10] = { ptAll, ptDiag, ptOrtho, ptDiag, ptOrtho, ptDiag, ptOrtho, ptDiag, ptOrtho };
  //  
  //  return pin_types[d] == pinType;
  //}
  //

  //////////////////////////////////////////////////////////////////////////
  ScoreType Evaluator::operator () (ScoreType alpha, ScoreType betta)
  {
    X_ASSERT(!board_, "Evaluator wasn't properly initialized");
    X_ASSERT(!coeffs_, "Evaluation coefficients was not set");

    if(ehash_)
      ehash_->prefetch(board_->pawnCode());

    if(board_->matState())
    {
      return -Figure::MatScore;
    }
    else if(board_->drawState())
    {
      return Figure::DrawScore;
    }
    else if(auto spec = specialCases().eval(*board_))
    {
      ScoreType score = *spec;
      // consider current move side
      if(Figure::ColorBlack  == board_->getColor())
        score = -score;
      return score;
    }
    //else if(board_->isWinnerLoser())
    //{
    //  ScoreType score = evaluateWinnerLoser();
    //  // consider current move side
    //  if(Figure::ColorBlack  == board_->getColor())
    //    score = -score;
    //  return score;
    //}

    prepare();

    // prepare lazy evaluation
    if(alpha > -Figure::MatScore)
    {
      alpha0_ = (int)alpha - lazyThreshold0_;
      alpha1_ = (int)alpha - lazyThreshold1_;
    }

    if(betta < +Figure::MatScore)
    {
      betta0_ = (int)betta + lazyThreshold0_;
      betta1_ = (int)betta + lazyThreshold1_;
    }

    ScoreType score = evaluate();
    X_ASSERT(score <= -ScoreMax || score >= ScoreMax, "invalid score");
    return score;
  }

  ScoreType Evaluator::evaluate()
  {
    //std::string sfen = toFEN(*board_);

    const FiguresManager & fmgr = board_->fmgr();
    FullScore score;

    // evaluate figures weight
    score.common_ = fmgr.weight() + evaluateMaterialDiff();

    /// use lazy evaluation level 0
    {
      auto score0 = considerColor(score.common_);
      if(score0 < alpha0_ || score0 > betta0_)
        return score0;
    }

    // take pawns eval from hash if possible
    auto pawnScore = hashedEvaluation();

    // basic king safety - pawn shield, castle
    auto kingScore = evaluateKingSafety();

    score += pawnScore;
    score += kingScore;

    // determine game phase (opening, middle or end game)
    auto phaseInfo = detectPhase();

    if(phaseInfo.phase_ != EndGame)
    {
      score.opening_ += evaluateBlockedKnights();
      score.opening_ += evaluateBlockedBishops();
    }

    if(phaseInfo.phase_ != Opening)
    {
      // king psq endgame
      score.endGame_ += evaluateKingPsqEg(Figure::ColorWhite);
      score.endGame_ -= evaluateKingPsqEg(Figure::ColorBlack);
    }

    /// use lazy evaluation level 1
    {
      auto score1 = considerColor(lipolScore(score, phaseInfo));
      if(score1 < alpha1_ || score1 > betta1_)
        return score1;
    }

    // PSQ - evaluation
    // + attacked fields
    auto scorePsq = evaluatePsq(Figure::ColorWhite);
    scorePsq -= evaluatePsq(Figure::ColorBlack);
    score += scorePsq;

    auto scorePP = evaluatePawnsPressure(Figure::ColorWhite);
    scorePP -= evaluatePawnsPressure(Figure::ColorBlack);
    score += scorePP;

    // detailed passer evaluation
    auto passerScore = passerEvaluation(Figure::ColorWhite);
    passerScore -= passerEvaluation(Figure::ColorBlack);
    score += passerScore;

    auto mobilityScore = evaluateMobility(Figure::ColorWhite);
    mobilityScore -= evaluateMobility(Figure::ColorBlack);
    score += mobilityScore;

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
    score.common_ = pop_count(pw_protected & attackers) * coeffs_->protectedPawnPressure_;
    score.common_ += pop_count(pw_unprotected & attackers) * coeffs_->unprotectedPawnPressure_;
    return score;
  }

  ScoreType Evaluator::evaluateKingPsqEg(Figure::Color color) const
  {
    int p = finfo_[color].king_pos_;
    return coeffs_->knightPsq_[p];
  }

  Evaluator::FullScore Evaluator::hashedEvaluation()
  {
    HEval * heval = 0;
    const uint64 & code = board_->pawnCode();
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
        return score;
      }
    }

    auto score = evaluatePawns(Figure::ColorWhite);
    score -= evaluatePawns(Figure::ColorBlack);

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
  score.endGame_ = fmgr.pawns(color) * coeffs_->pawnEndgameBonus_;

  static int promo_y[] = { 0, 7 };
  static int delta_y[] = { -1, +1 };

  Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & opmsk = fmgr.pawn_mask(ocolor);


  uint8 x_visited{ 0 };
  uint8 x_passers{ 0 };

  BitMask pawn_mask = pmask;
  for(; pawn_mask;)
  {
    int n = clear_lsb(pawn_mask);
    Index idx(n);

    int x  = idx.x();
    int cy = colored_y_[color][idx.y()];
    
    // lets move forward
    score.common_ += coeffs_->forwardPawn_[cy];

    // 1. center pawns
    if(x == 3 || x == 4) // D, E columns
    {
      score.opening_ += coeffs_->centerPawn_[cy];
    }

    // 2. passed pawn
    {
      auto coeffPassed = coeffs_->passerPawn_[cy];
      bool passer = (opmsk & pawnMasks().mask_passed(color, n)) == 0ULL;
      x_passers |= passer << x;
      bool quadpasser = (pawnMasks().mask_line_blocked(color, n) & opmsk) == 0ULL;
      score.common_ += passer*coeffPassed;
      {
        bool left  = (x != 0) && ((pawnMasks().mask_line_blocked(color, Index(x-1, idx.y())) & opmsk)== 0ULL);
        bool right = (x != 7) && ((pawnMasks().mask_line_blocked(color, Index(x+1, idx.y())) & opmsk) == 0ULL);
        X_ASSERT(((left && right) || (x == 0 && right) || (x == 7 && left)) && (!passer) && quadpasser, "passed pawn was not detected");
        auto scoreSemipasser = ((coeffPassed >> 2) + (left || right) * (coeffPassed >> 2));
        score.common_ += quadpasser * scoreSemipasser * (!passer);
      }
    }

    // 3. doubled pawn
    if(!(x_visited & (1<<x)))
    {
      x_visited |= 1<<x;
      auto const& dblmsk = pawnMasks().mask_doubled(x);
      auto num_dbl = pop_count(pmask & dblmsk);
      X_ASSERT(num_dbl <= 0, "doubled pawns number is zero or negative");
      score.common_ += (num_dbl - 1) * coeffs_->doubledPawn_;
    }

    // 4. isolated pawn
    bool isolated = (pmask & pawnMasks().mask_isolated(x)) == 0ULL;
    score.common_ += isolated * coeffs_->isolatedPawn_;

    if(!isolated)
    {
      // 5. backward
      if(((pawnMasks().mask_backward(color, n) & pmask) == 0ULL))
      {
        // TODO: optimization required
        int closest_y = closestToBackward(idx.x(), idx.y(), pmask, color);
        X_ASSERT(closest_y < 0 || closest_y > 7, "backward mask calculation error - invalid next y position");
        BitMask pmask_after = betweenMasks().between(n, Index(idx.x(), closest_y));
        bool is_blocked  = ((finfo_[ocolor].pawnAttacks_ | opmsk) & pmask_after) != 0ULL;
        score.common_ += is_blocked * coeffs_->backwardPawn_;
      }
      // 6. forward but not supported
      else if(!couldBeSupported(idx, color, ocolor, pmask, opmsk))
      {
        score.common_ += coeffs_->unsupportedPawn_;
      }
      // 7. unprotected
      else
      {
        bool is_unprotected = (pawnMasks().mask_supported(color, idx) & pmask) == 0ULL;
        score.common_ += is_unprotected * coeffs_->unprotectedPawn_;
      }
    }
  }

  // 8. additional bonus for passers on neighbour columns
  {
    auto multi = pawnMasks().mask_multi_passer(x_passers) & pmask;
    if(multi)
    {
      int ymax = color ? _msb64(multi) : _lsb64(multi);
      ymax >>= 3;
      int cy = colored_y_[color][ymax];
      int num = pop_count(multi);
      score.common_ += coeffs_->passerGroup_[cy] * (num-1);
    }
  }

  return score;
}

Evaluator::FullScore Evaluator::passerEvaluation(Figure::Color color) const
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

  FullScore score;

  Figure::Color ocolor = Figure::otherColor(color);
  const BitMask & opmsk = fmgr.pawn_mask(ocolor);
  bool no_opawns = opmsk == 0ULL;

  BitMask pawn_mask = pmask;
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

    int promo_pos = x | (py<<3);
    
    bool king_far = false;
    int pawn_dist_promo = std::abs(py - y);
    int o_dist_promo = distanceCounter().getDistance(finfo_[ocolor].king_pos_, promo_pos) - (board_->color_ == ocolor);
    king_far = pawn_dist_promo < o_dist_promo;
    
    //// have bishop with color the same as promotion field's color
    //Figure::Color pcolor = ((Figure::Color)FiguresCounter::s_whiteColors_[promo_pos]);
    //if ( pcolor && fmgr.bishops_w(color) || !pcolor && fmgr.bishops_b(color) )
    //  score += assistantBishop_;
    
    // my rook behind passed pawn - give bonus
    BitMask behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & fmgr.rook_mask(color) ;
    BitMask o_behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & fmgr.rook_mask(ocolor) ;
    if ( behind_msk )
    {
      int rpos = color ? _msb64(behind_msk) : _lsb64(behind_msk);
      if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
        score.common_ += coeffs_->rookBehindBonus_;
    }
    else if ( o_behind_msk ) // may be opponent's rook - give penalty
    {
      int rpos = color ? _msb64(o_behind_msk) : _lsb64(o_behind_msk);
      if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
        score.common_ -= coeffs_->rookBehindBonus_;
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
      else
      {
        Move move;
        Figure::Type type = Figure::TypeNone;
        if(next_pos == promo_pos)
          type = Figure::TypeQueen;

        move.set(n, next_pos, type, false);
        ScoreType see_score = board_->see(move);
        can_go = see_score >= 0;
      }
    }
    // field is occupied by my figure
    else if(next_field.color() == color)
    {
      can_go = true;
    }

    // additional bonus if opponent's king can't go to my pawn's promotion field
    score.common_ += can_go * coeffs_->passerPawn_[cy] >> 1;
    
    // opponent king could not go to my pawns promotion field
    if(king_far || !findRootToPawn(color, promo_pos, pawn_dist_promo+1))
    {
      score.endGame_ += coeffs_->unstoppablePawn_[cy];
    }

    // opponent king should be far from my pawn
    int dist_to_oking = distanceCounter().getDistance(finfo_[ocolor].king_pos_, n);
    score.endGame_ += coeffs_->oKingToPasserBonus_[dist_to_oking];

    // small bonus for short distance to my king
    // double if opponent has no pawns
    int dist_to_myking = distanceCounter().getDistance(finfo_[color].king_pos_, n);
    score.endGame_ += coeffs_->myKingToPasserBonus_[dist_to_myking] * (1 + no_opawns);
  }
  return score;
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

Evaluator::FullScore Evaluator::evaluateKingSafety() const
{
  auto score = evaluateKingSafety(Figure::ColorWhite);
  score -= evaluateKingSafety(Figure::ColorBlack);
  return score;
}

Evaluator::FullScore Evaluator::evaluateKingSafety(Figure::Color color) const
{
  const FiguresManager & fmgr = board_->fmgr();
  auto pmask  = fmgr.pawn_mask(color);

  FullScore score;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
  auto opmask = fmgr.pawn_mask(ocolor);
  Index ki_pos(finfo_[color].king_pos_);

  int kx = ki_pos.x();
  int ky = ki_pos.y();

  int ctype = getCastleType(color);
  X_ASSERT(ctype < -1 || ctype > 1, "invalid castle type detected");

  int delta_y[] = {-1, +1};

  score.opening_ += evaluateCastle(color, ocolor, ctype, ki_pos);

  int cy = colored_y_[color][ky] - 1;
  score.opening_ += coeffs_->roamingKing_ * cy * (cy > 0);

  // pawns shield
  static const BitMask pawns_shield_mask[2][2][3] =
  {
    // black
    {
      // short
      {
        set_mask_bit(H7)|set_mask_bit(H6),
        set_mask_bit(G7)|set_mask_bit(G6),
        set_mask_bit(F7)|set_mask_bit(F6)
      },

      // long
      {
        set_mask_bit(A7)|set_mask_bit(A6),
        set_mask_bit(B7)|set_mask_bit(B6),
        set_mask_bit(C7)|set_mask_bit(C6)
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
        set_mask_bit(C2)|set_mask_bit(C3)
      }
    }
  };

  static const BitMask pawns_penalty_mask[2][3] =
  {
    // short
    {
      set_mask_bit(H7)|set_mask_bit(H6)|set_mask_bit(H5)|set_mask_bit(H4)|set_mask_bit(H3)|set_mask_bit(H2),
      set_mask_bit(G7)|set_mask_bit(G6)|set_mask_bit(G5)|set_mask_bit(H4)|set_mask_bit(H3)|set_mask_bit(H2),
      set_mask_bit(F7)|set_mask_bit(F6)|set_mask_bit(F5)|set_mask_bit(F4)|set_mask_bit(F3)|set_mask_bit(F2)
    },

    // long
    {
      set_mask_bit(A7)|set_mask_bit(A6)|set_mask_bit(A5)|set_mask_bit(A4)|set_mask_bit(A3)|set_mask_bit(A2),
      set_mask_bit(B7)|set_mask_bit(B6)|set_mask_bit(B5)|set_mask_bit(B4)|set_mask_bit(B3)|set_mask_bit(B2),
      set_mask_bit(C7)|set_mask_bit(C6)|set_mask_bit(C5)|set_mask_bit(C4)|set_mask_bit(C3)|set_mask_bit(C2)
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

  if(ctype >= 0)
  {
    int shield_bonus = ((pawns_shield_mask[color][ctype][0] & pmask) != 0) * coeffs_->pawnShieldA_
      + ((pawns_shield_mask[color][ctype][1] & pmask) != 0) * coeffs_->pawnShieldB_
      + ((pawns_shield_mask[color][ctype][2] & pmask) != 0) * coeffs_->pawnShieldC_;
    
    int shield_penalty = ((pawns_penalty_mask[ctype][0] & pmask) == 0) * coeffs_->pawnPenaltyA_
      + ((pawns_penalty_mask[ctype][1] & pmask) == 0) * coeffs_->pawnPenaltyB_
      + ((pawns_penalty_mask[ctype][2] & pmask) == 0) * coeffs_->pawnPenaltyC_;

    int opponent_penalty = ((opponent_pawn_mask[color][ctype][0] & opmask) != 0) * coeffs_->opponentPawnA_
      + ((opponent_pawn_mask[color][ctype][1] & opmask) != 0) * coeffs_->opponentPawnB_
      + ((opponent_pawn_mask[color][ctype][2] & opmask) != 0) * coeffs_->opponentPawnC_;

    score.opening_ += shield_bonus;
    score.opening_ += shield_penalty;
    score.opening_ += opponent_penalty;
  }
  return score;
}

int Evaluator::evaluateCastle(Figure::Color color, Figure::Color ocolor, int castleType, Index const& ki_pos) const
{
  const FiguresManager & fmgr = board_->fmgr();

  static const BitMask fake_castle_rook[2][2] = {
    { set_mask_bit(G8)|set_mask_bit(H8)|set_mask_bit(G7)|set_mask_bit(H7),
      set_mask_bit(A8)|set_mask_bit(B8)|set_mask_bit(C8)|set_mask_bit(A7)|set_mask_bit(B7)|set_mask_bit(C7)},

    { set_mask_bit(G1)|set_mask_bit(H1)|set_mask_bit(G2)|set_mask_bit(H2),
      set_mask_bit(A1)|set_mask_bit(B1)|set_mask_bit(C1)|set_mask_bit(A2)|set_mask_bit(B2)|set_mask_bit(C2)} };

  if(castleType < 0 && !board_->castling(color))
    return coeffs_->castleImpossible_;

  // fake castle
  if ( !board_->castling(color) && castleType >= 0 )
  {
    BitMask r_mask = fmgr.rook_mask(color) & fake_castle_rook[color][castleType];
    if ( r_mask )
    {
      Index r_pos( _lsb64(r_mask) );
      if(castleType == 0 && r_pos.x() > ki_pos.x() || castleType == 1 && r_pos.x() < ki_pos.x())
        return coeffs_->fakeCastle_;
    }
    return coeffs_->castleBonus_;
  }
  return 0;
}

int Evaluator::evaluateBlockedKnights()
{
  int score_b = 0, score_w = 0;
  BitMask knight_w = board_->fmgr().knight_mask(Figure::ColorWhite);
  for(; knight_w;)
  {
    int n = clear_lsb(knight_w);

    switch(n)
    {
    case A8:
      if(board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn))
         score_w -= coeffs_->knightBlocked_;
      break;

    case A7:
      if(board_->isFigure(A6, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
         score_w -= coeffs_->knightBlocked_;
      break;

    case B8:
      if(board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
        score_w -= coeffs_->knightBlocked_;
      break;

    case H8:
      if(board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn))
         score_w -= coeffs_->knightBlocked_;
      break;

    case H7:
      if(board_->isFigure(H6, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
         score_w -= coeffs_->knightBlocked_;
      break;

    case G8:
      if(board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
        score_w -= coeffs_->knightBlocked_;
      break;
    }
  }

  BitMask knight_b = board_->fmgr().knight_mask(Figure::ColorBlack);
  for(; knight_b;)
  {
    int n = clear_lsb(knight_b);

    switch(n)
    {
    case A1:
      if(board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn))
         score_b -= coeffs_->knightBlocked_;
      break;

    case A2:
      if(board_->isFigure(A3, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
         score_b -= coeffs_->knightBlocked_;
      break;

    case B1:
      if(board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
        score_b -= coeffs_->knightBlocked_;
      break;

    case H1:
      if(board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn))
         score_b -= coeffs_->knightBlocked_;
      break;

    case H2:
      if(board_->isFigure(H3, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
         score_b -= coeffs_->knightBlocked_;
      break;

    case G1:
      if(board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
        score_b -= coeffs_->knightBlocked_;
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
        score_w -= coeffs_->bishopBlocked_;
      break;

    case A6:
      if(board_->isFigure(B5, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(C6, Figure::ColorBlack, Figure::TypePawn))
         score_w -= coeffs_->bishopBlocked_;
      break;

    case A8:
      if(board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
        score_w -= coeffs_->bishopBlocked_;
      break;

    case B8:
      if(board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn) &&
         (board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn)))
         score_w -= coeffs_->bishopBlocked_;
      break;

    case H7:
      if(board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn))
        score_w -= coeffs_->bishopBlocked_;
      break;

    case H8:
      if(board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
        score_w -= coeffs_->bishopBlocked_;
      break;

    case G8:
      if(board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn) &&
         (board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn)))
         score_w -= coeffs_->bishopBlocked_;
      break;

    case H6:
      if(board_->isFigure(G5, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(F6, Figure::ColorBlack, Figure::TypePawn))
         score_w -= coeffs_->bishopBlocked_;
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
        score_b -= coeffs_->bishopBlocked_;
      break;

    case A3:
      if(board_->isFigure(B4, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(C3, Figure::ColorWhite, Figure::TypePawn))
         score_b -= coeffs_->bishopBlocked_;
      break;

    case A1:
      if(board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
        score_b -= coeffs_->bishopBlocked_;
      break;

    case B1:
      if(board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn) &&
         (board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn)))
         score_b -= coeffs_->bishopBlocked_;
      break;

    case H2:
      if(board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn))
        score_b -= coeffs_->bishopBlocked_;
      break;

    case H1:
      if(board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
        score_b -= coeffs_->bishopBlocked_;
      break;

    case G1:
      if(board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn) &&
         (board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn)))
         score_b -= coeffs_->bishopBlocked_;
      break;

    case H3:
      if(board_->isFigure(G4, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(F3, Figure::ColorWhite, Figure::TypePawn))
         score_b -= coeffs_->bishopBlocked_;
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
      score -= is_blocked * coeffs_->rookBlocked_;
    }
    return score;
  };
  auto score_w = evaluate_blocked(Figure::ColorWhite);
  auto score_b = evaluate_blocked(Figure::ColorBlack);
  return score_w - score_b;
}

ScoreType Evaluator::evaluateMaterialDiff()
{
  ScoreType score = 0;
  const FiguresManager & fmgr = board_->fmgr();

  // bonus for bishops
  score += fmgr.bishops(Figure::ColorWhite) * coeffs_->bishopBonus_;
  score -= fmgr.bishops(Figure::ColorBlack) * coeffs_->bishopBonus_;

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
    score += figuresDiff * coeffs_->figureAgainstPawnBonus_[pawnsN];
  }

  // Knight|Bishop+2Pawns vs. Rook
  else if(!queensDiff && (rooksDiff*figuresDiff == -1))
  {
    Figure::Color strongColor = (Figure::Color)(rooksDiff > 0);
    int pawnsN = fmgr.pawns(strongColor) != 0;
    score += rooksDiff * coeffs_->rookAgainstFigureBonus_[pawnsN];
  }

  return score;
}


ScoreType Evaluator::evaluateWinnerLoser()
{
  const FiguresManager & fmgr = board_->fmgr();
  Figure::Color win_color = board_->can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
  Figure::Color lose_color = Figure::otherColor(win_color);

  ScoreType score = fmgr.weight(win_color);

  // bonus for pawns
  score += fmgr.pawns(win_color)*coeffs_->pawnEndgameBonus_;

  Index king_pos_w = board_->kingPos(win_color);
  Index king_pos_l = board_->kingPos(lose_color);

  bool eval_pawns = true;

  if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) > 0 )
  {
    int num_lose_figs = fmgr.knights(lose_color) + fmgr.bishops(lose_color);
    ScoreType weight_lose_fig = 10;

    // if winner doesn't have light figure and loser has more than 1 figure we don't want to evaluate them less than a pawn
    if(fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 1 && fmgr.knights(win_color) +fmgr.bishops(win_color) == 0)
    {
      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn]
        + (coeffs_->passerPawn_[7])
        + coeffs_->pawnEndgameBonus_;
    }
    // if winner has more pawns than loser figures and also has some figure he must exchange all loser figures to pawns
    else if(fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0
            && fmgr.knights(win_color) +fmgr.bishops(win_color) > 0
            && fmgr.knights(lose_color)+fmgr.bishops(lose_color) < fmgr.pawns(win_color))
    {
      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn]
        + (coeffs_->passerPawn_[7])
        + coeffs_->pawnEndgameBonus_;
    }

    score -= num_lose_figs * weight_lose_fig;
  }
  else
    score -= fmgr.weight(lose_color);

  //// add small bonus for winner-loser state to force it
  //score += winloseBonus_;



  //else
  //{
  //  // some special almost-draw cases
  //  if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 0 &&
  //       fmgr.weight(win_color)-fmgr.weight(lose_color) < Figure::figureWeight_[Figure::TypeBishop]+Figure::figureWeight_[Figure::TypeKnight] )
  //  {
  //    score = 10;
  //  }
  //  else if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 1 &&
  //            fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0 )
  //  {
  //    if ( fmgr.knights(win_color)+fmgr.bishops(win_color) <= fmgr.knights(lose_color)+fmgr.bishops(lose_color) )
  //    {
  //      score = (MAX_PASSED_SCORE);
  //      uint64 pwmask = fmgr.pawn_mask_o(win_color);
  //      int pp = clear_lsb(pwmask);
  //      int x = pp & 7;
  //      int y = pp >> 3;
  //      if ( !win_color )
  //        y = 7-y;
  //      if ( y < 6 )
  //      {
  //        int ep = x | (win_color ? A8 : A1);
  //        int8 pwhite = FiguresCounter::s_whiteColors_[ep];
  //        if ( pwhite && fmgr.bishops_w(lose_color) || !pwhite && fmgr.bishops_b(lose_color) > 0 || y < 5 )
  //          score = 10;
  //      }
  //      else if ( board_->color_ == win_color )
  //        score = Figure::figureWeight_[Figure::TypePawn];
  //      score += y << 1;
  //      eval_pawns = false;
  //    }
  //  }

  //  if ( fmgr.pawns(win_color) == 1 )
  //  {
  //    uint64 pwmsk = fmgr.pawn_mask_o(win_color);
  //    int pp = clear_lsb(pwmsk);
  //    X_ASSERT( (unsigned)pp > 63, "no pawn found" );

  //    int ykl = king_pos_l.y();
  //    int ykw = king_pos_w.y();

  //    int xkl = king_pos_l.x();
  //    int xkw = king_pos_w.x();

  //    int x = pp & 7;
  //    int y = pp >> 3;
  //    int y_under = y;
  //    if ( win_color )
  //      y_under++;
  //    else
  //      y_under--;
  //    int pp_under = (x | (y_under << 3)) & 63;

  //    if ( !win_color )
  //    {
  //      y = 7-y;
  //      ykl = 7-ykl;
  //      ykw = 7-ykw;
  //    }

  //    int pr_pos = x | (win_color ? A8 : A1);
  //    Figure::Color pr_color = (Figure::Color)FiguresCounter::s_whiteColors_[pr_pos];

  //    int pr_moves = 7-y;

  //    int wk_pr_dist = distanceCounter().getDistance(king_pos_w, pr_pos);
  //    int lk_pr_dist = distanceCounter().getDistance(king_pos_l, pr_pos);

  //    int wdist = distanceCounter().getDistance(king_pos_w, pp);
  //    int ldist = distanceCounter().getDistance(king_pos_l, pp);

  //    int wudist = distanceCounter().getDistance(king_pos_w, pp_under);
  //    int ludist = distanceCounter().getDistance(king_pos_l, pp_under);

  //    // special case KPK
  //    if ( (fmgr.weight(win_color) == Figure::figureWeight_[Figure::TypePawn] && fmgr.weight(lose_color) == 0) )
  //    {
  //      bool almost_draw = false;
  //      if ( x == 0 || x == 7 )
  //      {
  //        if ( (lk_pr_dist + ludist <= wk_pr_dist + wudist) ||
  //             ((pr_moves >= lk_pr_dist && y > 1 || pr_moves > lk_pr_dist && y == 1) && wudist >= ludist) )
  //          almost_draw = true;
  //      }
  //      else
  //      {
  //        if ( (pr_moves >= lk_pr_dist && y > 1 || pr_moves > lk_pr_dist && y == 1) &&
  //             ( (wudist > ludist || wudist == ludist && lose_color == board_->color_) && y >= ykw ||
  //               (wudist > ludist+1 && y < ykw) ) )
  //          almost_draw = true;
  //      }

  //      if ( almost_draw )
  //      {
  //        score = 30 + (y<<1);
  //        eval_pawns = false;
  //      }
  //    }
  //    // KPBK. bishop color differs from promotion field color
  //    else if ( ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.knights(win_color) == 0 &&
  //                fmgr.bishops(win_color) && (x == 0 || x == 7) &&
  //               (!fmgr.bishops_w(win_color) && pr_color || !fmgr.bishops_b(win_color) && !pr_color) ) )
  //    {
  //      if ( (pr_moves > lk_pr_dist && lk_pr_dist <= wk_pr_dist) || (lk_pr_dist < 2 && pr_moves > 0) )
  //      {
  //        score = 30 + (y<<1);
  //        eval_pawns = false;
  //      }
  //    }

  //    // opponent's king should be as far as possible from my pawn
  //    score -= (7-ldist);

  //    // my king should be as near as possible to my pawn
  //    score -= wdist;
  //  }
  //  else
  //  {
  //    int dist  = distanceCounter().getDistance(king_pos_w, king_pos_l);
  //    score -= dist << 1;
  //    score -= positionEvaluation(1, lose_color, Figure::TypeKing, king_pos_l);
  //    score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, king_pos_w) >> 1;
  //  }
  //}

  if ( win_color == Figure::ColorBlack )
    score = -score;

  if(eval_pawns)
  {
    auto pawnScore = hashedEvaluation();
    score += pawnScore.common_ + pawnScore.endGame_;
  }

  return score;
}









//
//// most expensive part: mobility of figures, rooks on open columns, passed pawns additional bonus
//ScoreType Evaluator::evaluateExpensive(GamePhase phase, int coef_o, int coef_e)
//{
//  ScoreType score = 0;
//
//  // knights-bishops mobility and attacked fields
//  score += evaluateKnights();
//  score += evaluateBishops();
//
//  // forks
//  score -= evaluateForks(Figure::ColorBlack);
//  score += evaluateForks(Figure::ColorWhite);
//
//  // rooks and queens mobility and attacked fields
//  evaluateRooks(phase != EndGame);
//  evaluateQueens();
//
//  ScoreType score_r = finfo_[1].rookOpenScore_ - finfo_[0].rookOpenScore_;
//
//  if ( phase == MiddleGame )
//    score_r = (score_r * coef_o) / weightMax_;
//
//  score += score_r;
//
//  score -= finfo_[0].rookMobility_;
//  score += finfo_[1].rookMobility_;
//
//  score -= finfo_[0].queenMobility_;
//  score += finfo_[1].queenMobility_;
//
//  score -= finfo_[0].rookPressure_;
//  score += finfo_[1].rookPressure_;
//
//  score -= finfo_[0].queenPressure_;
//  score += finfo_[1].queenPressure_;
//
//	// fields near king are attacked by opponent
//	score += evaluateKingPressure();
//
//  // unstoppable passed pawns and pawns, that can go to the next line
//  score += evaluatePassersAdditional(phase, coef_e);
//
//  if ( Figure::ColorBlack  == board_->getColor() )
//    score = -score;
//
//  return score;
//}
//
//ScoreType Evaluator::evaluatePassersAdditional(GamePhase phase, int coef_e)
//{
//  ScoreType score = 0, pw_score_eg = 0;
//  int most_adv_yb = -1, most_adv_yw = -1;
//
//  score -= evaluatePasserAdditional(phase, Figure::ColorBlack, pw_score_eg, most_adv_yb);
//  score += evaluatePasserAdditional(phase, Figure::ColorWhite, pw_score_eg, most_adv_yw);
//
//  // give bonus for most advanced pawn if it is closer to promotion field
//  if ( phase != Opening && most_adv_yb >= 0 && most_adv_yw >= 0 )
//  {
//    if ( most_adv_yw > most_adv_yb )
//      pw_score_eg += unstoppablePawn_;
//    else if ( most_adv_yb > most_adv_yw )
//      pw_score_eg -= unstoppablePawn_;
//  }
//
//  if ( phase == MiddleGame )
//    pw_score_eg = (pw_score_eg * coef_e) / weightMax_;
//
//  score += pw_score_eg;
//
//  return score;
//}
//
//ScoreType Evaluator::evaluateKnights()
//{
//  ScoreType score = 0;
//
//  for (int c = 0; c < 2; ++c)
//  {
//    Figure::Color color = (Figure::Color)c;
//    Figure::Color ocolor = Figure::otherColor(color);
//    BitMask not_occupied = ~finfo_[ocolor].pawnAttacks_ & inv_mask_all_;
//    const int & oki_pos = finfo_[ocolor].king_pos_;
//		const BitMask & oki_mask = movesTable().caps(Figure::TypeKing, oki_pos);
//
//    BitMask kn_mask = board_->fmgr().knight_mask(color);
//    for ( ; kn_mask; )
//    {
//      int from = clear_lsb(kn_mask);
//      const BitMask & kn_cap = movesTable().caps(Figure::TypeKnight, from);
//      finfo_[c].kn_attack_mask_ |= kn_cap;
//
//      int ki_dist = distanceCounter().getDistance(from, oki_pos);
//      finfo_[c].knightPressure_ += kingDistanceBonus_[Figure::TypeKnight][ki_dist];
//
//      finfo_[c].attack_mask_ |= kn_cap;
//
//      BitMask kmob_mask = kn_cap & not_occupied;
//      int movesN = pop_count(kmob_mask);
//      finfo_[c].knightMobility_ += mobilityBonus_[Figure::TypeKnight][movesN];
//
//			// is attacking fields near king
//			finfo_[c].attackersN_[Figure::TypeKnight] += isAttackingFields(from, oki_mask);
//    }
//  }
//
//  score -= finfo_[0].knightPressure_;
//  score += finfo_[1].knightPressure_;
//
//  score -= finfo_[0].knightMobility_;
//  score += finfo_[1].knightMobility_;
//
//  return score;
//}
//
//ScoreType Evaluator::evaluateBishops()
//{
//  ScoreType score = 0;
//
//  for (int c = 0; c < 2; ++c)
//  {
//    Figure::Color color = (Figure::Color)c;
//    Figure::Color ocolor = Figure::otherColor(color);
//    BitMask not_attacked = ~finfo_[ocolor].pawnAttacks_;
//    const int & oki_pos = finfo_[ocolor].king_pos_;
//		const BitMask & oki_mask = movesTable().caps(Figure::TypeKing, oki_pos);
//
//    BitMask bimask = board_->fmgr().bishop_mask(color);
//    for ( ; bimask; )
//    {
//      int from = clear_lsb(bimask);
//
//      BitMask bmob_mask = magic_ns::bishop_moves(from, mask_all_) & inv_mask_all_;
//
//      //mobility_masks_LSB(from, bmob_mask, betweenMasks().from_dir(from, nst::nw));
//      //mobility_masks_LSB(from, bmob_mask, betweenMasks().from_dir(from, nst::ne));
//      //mobility_masks_MSB(from, bmob_mask, betweenMasks().from_dir(from, nst::se));
//      //mobility_masks_MSB(from, bmob_mask, betweenMasks().from_dir(from, nst::sw));
//
//      finfo_[c].attack_mask_ |= bmob_mask;
//      bmob_mask &= not_attacked;
//
//      int ki_dist = distanceCounter().getDistance(from, oki_pos);
//      finfo_[c].bishopPressure_ += kingDistanceBonus_[Figure::TypeBishop][ki_dist];
//
//      int movesN = pop_count(bmob_mask);
//      finfo_[c].bishopMobility_ += mobilityBonus_[Figure::TypeBishop][movesN];
//
//			// is attacking fields near king
//			finfo_[c].attackersN_[Figure::TypeBishop] += isAttackingFields(from, oki_mask);
//    }
//  }
//
//  score -= finfo_[0].bishopPressure_;
//  score += finfo_[1].bishopPressure_;
//
//  score -= finfo_[0].bishopMobility_;
//  score += finfo_[1].bishopMobility_;
//
//  return score;
//}
//
//void Evaluator::evaluateRooks(bool eval_open)
//{
//  BitMask rattack_mask[2] = { 0, 0 };
//
//  for (int c = 0; c < 2; ++c)
//  {
//    Figure::Color color = (Figure::Color)c;
//    Figure::Color ocolor = Figure::otherColor(color);
//    BitMask not_attacked = ~finfo_[ocolor].attack_mask_;
//    const int & oki_pos = finfo_[ocolor].king_pos_;
//		const BitMask & oki_mask = movesTable().caps(Figure::TypeKing, oki_pos);
//
//    Index oki_p(oki_pos);
//    int okx = oki_p.x();
//    int oky = oki_p.y();
//
//    const uint8 * pmsk_t  = (const uint8*)&board_->fmgr().pawn_mask_t(color);
//    const uint8 * opmsk_t = (const uint8*)&board_->fmgr().pawn_mask_t(ocolor);
//    BitMask ro_mask = board_->fmgr().rook_mask(color);
//    for ( ; ro_mask; )
//    {
//      int from = clear_lsb(ro_mask);
//
//      BitMask rmob_mask = magic_ns::rook_moves(from, mask_all_) & inv_mask_all_;;
//
//      //mobility_masks_LSB(from, rmob_mask, betweenMasks().from_dir(from, nst::no));
//      //mobility_masks_LSB(from, rmob_mask, betweenMasks().from_dir(from, nst::ea));
//      //mobility_masks_MSB(from, rmob_mask, betweenMasks().from_dir(from, nst::so));
//      //mobility_masks_MSB(from, rmob_mask, betweenMasks().from_dir(from, nst::we));
//
//      rattack_mask[c] |= rmob_mask;
//      rmob_mask &= not_attacked;
//
//      int ki_dist = distanceCounter().getDistance(from, oki_pos);
//      finfo_[c].rookPressure_ += kingDistanceBonus_[Figure::TypeRook][ki_dist];
//
//      int movesN = pop_count(rmob_mask);
//      finfo_[c].rookMobility_ += mobilityBonus_[Figure::TypeRook][movesN];
//
//			// is attacking fields near king
//			finfo_[c].attackersN_[Figure::TypeRook] += isAttackingFields(from, oki_mask);
//
//      // rook on open column
//      if ( eval_open )
//      {
//        Index rp(from);
//        int x = rp.x();
//        int y = rp.y();
//
//        // no pawns of some color
//        if ( !opmsk_t[rp.x()] || !pmsk_t[rp.x()] )
//        {
//          finfo_[c].rookOpenScore_ += semiopenRook_;
//
//          // near opponent's king
//          if ( x == okx || x == okx-1 || x == okx+1 )
//            finfo_[c].rookOpenScore_ += rookToKingBonus_;
//        }
//
//        // no pawns at all
//        if ( !(opmsk_t[x] | pmsk_t[x]) )
//          finfo_[c].rookOpenScore_ += openRook_;
//
//        // rooks to the opponent king
//        if ( y == oky || y == oky-1 || y == oky+1 )
//          finfo_[c].rookOpenScore_ += rookToKingBonus_;
//      }
//    }
//  }
//
//  finfo_[0].attack_mask_ |= rattack_mask[0];
//  finfo_[1].attack_mask_ |= rattack_mask[1];
//}
//
//void Evaluator::evaluateQueens()
//{
//  BitMask qattack_mask[2] = { 0, 0 };
//
//  for (int c = 0; c < 2; ++c)
//  {
//    Figure::Color color = (Figure::Color)c;
//    Figure::Color ocolor = Figure::otherColor(color);
//    BitMask not_attacked = ~finfo_[ocolor].attack_mask_;
//    const int & oki_pos = finfo_[ocolor].king_pos_;
//		const BitMask & oki_mask = movesTable().caps(Figure::TypeKing, oki_pos);
//
//    BitMask q_mask = board_->fmgr().queen_mask(color);
//    for ( ; q_mask; )
//    {
//      int from = clear_lsb(q_mask);
//
//      BitMask qmob_mask = magic_ns::queen_moves(from, mask_all_) & inv_mask_all_;
//
//      //mobility_masks_LSB(from, qmob_mask, betweenMasks().from_dir(from, nst::nw));
//      //mobility_masks_LSB(from, qmob_mask, betweenMasks().from_dir(from, nst::ne));
//      //mobility_masks_MSB(from, qmob_mask, betweenMasks().from_dir(from, nst::se));
//      //mobility_masks_MSB(from, qmob_mask, betweenMasks().from_dir(from, nst::sw));
//
//      //mobility_masks_LSB(from, qmob_mask, betweenMasks().from_dir(from, nst::no));
//      //mobility_masks_LSB(from, qmob_mask, betweenMasks().from_dir(from, nst::ea));
//      //mobility_masks_MSB(from, qmob_mask, betweenMasks().from_dir(from, nst::so));
//      //mobility_masks_MSB(from, qmob_mask, betweenMasks().from_dir(from, nst::we));
//
//      qattack_mask[c] |= qmob_mask;
//      qmob_mask &= not_attacked;
//
//      int ki_dist = distanceCounter().getDistance(from, oki_pos);
//      finfo_[c].queenPressure_ += kingDistanceBonus_[Figure::TypeQueen][ki_dist];
//
//      int movesN = pop_count(qmob_mask);
//      finfo_[c].queenMobility_ += mobilityBonus_[Figure::TypeQueen][movesN];
//
//			// is attacking fields near king
//			finfo_[c].attackersN_[Figure::TypeQueen] += isAttackingFields(from, oki_mask);
//    }
//  }
//
//  finfo_[0].attack_mask_ |= qattack_mask[0];
//  finfo_[1].attack_mask_ |= qattack_mask[1];
//}
//
////////////////////////////////////////////////////////////////////////////
//ScoreType Evaluator::evaluateKingPressure() const
//{
//	ScoreType score = 0;
//	for (int c = 0; c < 2; ++c)
//	{
//		int num = 0;
//		int coef = 1;
//		for (int t = Figure::TypePawn; t < Figure::TypesNum; ++t)
//		{
//			num += finfo_[c].attackersN_[t];
//			if ( !finfo_[c].attackersN_[t] )
//				continue;
//			if ( t == Figure::TypeRook )
//				coef = 2;
//			else if ( t == Figure::TypeQueen )
//				coef = 3;
//		}
//
//		if ( c )
//			score += attackerNumberBonus_[num & 7] * coef;
//		else
//			score -= attackerNumberBonus_[num & 7] * coef;
//	}
//	return score;
//}
//
//int Evaluator::isAttackingFields(int from, const BitMask & mask) const
//{
//	X_ASSERT( !board_ || !board_->getField(from), "no figure on field in isAttackingFields()" );
//
//	Figure::Type t = board_->getField(from).type();
//	Figure::Color c = board_->getField(from).color();
//
//	if ( t == Figure::TypePawn )
//	{
//		const BitMask & p_caps = movesTable().pawnCaps_o(t, from);
//		return (int)((p_caps & mask ) != 0);
//	}
//	
//	const BitMask & f_caps = movesTable().caps(t, from);
//	if ( t == Figure::TypeKnight || t == Figure::TypeKing )
//		return (int)((f_caps & mask ) != 0);
//
//	BitMask a_mask = mask & f_caps;
//	/// not attacked
//	if ( !a_mask )
//		return 0;
//
//	BitMask xray_mask = board_->fmgr().queen_mask(c);
//	if ( t == Figure::TypeQueen )
//	{
//		xray_mask |= board_->fmgr().bishop_mask(c);
//		xray_mask |= board_->fmgr().rook_mask(c);
//	}
//	else if ( t == Figure::TypeBishop )
//		xray_mask |= board_->fmgr().bishop_mask(c);
//	else if ( t == Figure::TypeRook )
//		xray_mask |= board_->fmgr().rook_mask(c);
//
//	for ( ; a_mask; )
//	{
//		int to = clear_lsb(a_mask);
//		if ( board_->is_nothing_between(from, to, inv_mask_all_) )
//			return 1;
//
//		/// only x-ray figures between
//		const BitMask & btw_mask = betweenMasks().between(from, to);
//		if ( (btw_mask & xray_mask) == (btw_mask & mask_all_) )
//			return 1;
//	}
//
//	return 0;
//}
//
////////////////////////////////////////////////////////////////////////////
/// common  part

//
//ScoreType Evaluator::evaluateFianchetto() const
//{
//  ScoreType score = 0;
//
//  const Field & fb2 = board_->getField(B2);
//  const Field & fb3 = board_->getField(B3);
//
//  const Field & fg2 = board_->getField(G2);
//  const Field & fg3 = board_->getField(G3);
//
//  const Field & fb7 = board_->getField(B7);
//  const Field & fb6 = board_->getField(B6);
//
//  const Field & fg7 = board_->getField(G7);
//  const Field & fg6 = board_->getField(G6);
//
//  // white
//  if ( fb3.color() && fb3.type() == Figure::TypePawn && fb2.color() && fb2.type() == Figure::TypeBishop )
//    score += fianchettoBonus_;
//
//  if ( fg3.color() && fg3.type() == Figure::TypePawn && fg2.color() && fg2.type() == Figure::TypeBishop )
//    score += fianchettoBonus_;
//
//  // black
//  if ( !fb6.color() && fb6.type() == Figure::TypePawn && !fb7.color() && fb7.type() == Figure::TypeBishop )
//    score -= fianchettoBonus_;
//
//  if ( !fg6.color() && fg6.type() == Figure::TypePawn && !fg7.color() && fg7.type() == Figure::TypeBishop )
//    score -= fianchettoBonus_;
//
//  return score;
//}
//
//ScoreType Evaluator::evaluateForks(Figure::Color color)
//{
//  Figure::Color ocolor = Figure::otherColor(color);
//
//  BitMask o_rq_mask = board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
//  BitMask o_mask = board_->fmgr().knight_mask(ocolor) | board_->fmgr().bishop_mask(ocolor) | o_rq_mask;
//
//  BitMask pawn_fork = o_mask & finfo_[color].pawnAttacks_;
//  int pawnsN = pop_count(pawn_fork);
//  if ( pawnsN > 1 )
//    return forkBonus_;
//
//  BitMask kn_fork = o_rq_mask & finfo_[color].kn_attack_mask_;
//  int knightsN = pop_count(kn_fork);
//  if ( pawnsN+knightsN > 1 )
//    return forkBonus_;
//
//  if ( pawnsN+knightsN > 0 && color == board_->getColor() )
//    return attackedByWeakBonus_;
//
//  return 0;
//}
//
//
//
//
//
//ScoreType Evaluator::evaluatePasserAdditional(GamePhase phase, Figure::Color color, ScoreType & pw_score_eg, int & most_adv_y)
//{
//  const FiguresManager & fmgr = board_->fmgr();
//  Figure::Color ocolor = Figure::otherColor(color);
//
//  most_adv_y = -1;
//
//  const BitMask & opmsk_t = fmgr.pawn_mask_t(ocolor);
//  const BitMask & pmsk_t = fmgr.pawn_mask_t(color);
//  if ( !pmsk_t )
//    return 0;
//
//  ScoreType score = 0, score_eg = 0;
//
//  static int delta_y[] = { -1, 1 };
//  static int promo_y[] = {  0, 7 };
//  
//  int py = promo_y[color];
//  int dy = delta_y[color];
//
//  nst::dirs dir_behind[] = {nst::no, nst::so};
//
//  BitMask pawn_mask = fmgr.pawn_mask_o(color);
//  for ( ; pawn_mask; )
//  {
//    int n = clear_lsb(pawn_mask);
//
//    int x = n & 7;
//    int y = n >>3;
//
//    const uint64 & passmsk = pawnMasks().mask_passed(color, n);
//    const uint64 & blckmsk = pawnMasks().mask_blocked(color, n);
//
//    if ( !(opmsk_t & passmsk) && !(pmsk_t & blckmsk) )
//    {
//      int promo_pos = x | (py<<3);
//      int cy = color ? y : 7-y;
//
//      bool king_far = false;
//      int pawn_dist_promo = py - y;
//      if ( pawn_dist_promo < 0 )
//        pawn_dist_promo = -pawn_dist_promo;
//      int o_dist_promo = distanceCounter().getDistance(finfo_[ocolor].king_pos_, promo_pos);
//      o_dist_promo -= board_->color_ == ocolor;
//      king_far = pawn_dist_promo < o_dist_promo;
//
//      // have bishop with color the same as promotion field's color
//      Figure::Color pcolor = ((Figure::Color)FiguresCounter::s_whiteColors_[promo_pos]);
//      if ( pcolor && fmgr.bishops_w(color) || !pcolor && fmgr.bishops_b(color) )
//        score += assistantBishop_;
//
//      // my rook behind passed pawn - give bonus
//      BitMask behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & fmgr.rook_mask(color) ;
//      BitMask o_behind_msk = betweenMasks().from_dir(n, dir_behind[color]) & fmgr.rook_mask(ocolor) ;
//      if ( behind_msk )
//      {
//        int rpos = color ? _msb64(behind_msk) : _lsb64(behind_msk);
//        if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
//          score += rookBehindBonus_;
//      }
//      else if ( o_behind_msk ) // may be opponent's rook - give penalty
//      {
//        int rpos = color ? _msb64(o_behind_msk) : _lsb64(o_behind_msk);
//        if ( board_->is_nothing_between(n, rpos, inv_mask_all_) )
//          score -= rookBehindBonus_;
//      }
//
//      // pawn can go to the next line
//      int next_pos = x | ((y+dy)<<3);
//      X_ASSERT( ((y+dy)) > 7 || ((y+dy)) < 0, "pawn goes to invalid line" );
//
//      bool can_go = false;
//
//      // field is empty
//      // check is it attacked by opponent
//      if ( !board_->getField(next_pos) )
//      {
//        BitMask next_mask = set_mask_bit(next_pos);
//        if ( (next_mask & finfo_[ocolor].attack_mask_) == 0 )
//          can_go = true;
//        else
//        {
//          Move move;
//          Figure::Type type = Figure::TypeNone;
//          if ( next_pos == promo_pos )
//            type = Figure::TypeQueen;
//
//          move.set(n, next_pos, type, false);
//          ScoreType see_score = board_->see(move);
//          can_go = see_score >= 0;
//        }
//      }
//
//      // additional bonus if opponent's king can't go to my pawn's promotion field
//      if ( can_go )
//      {
//        score += pawnPassed_[cy] / 3;
//
//        if ( phase != Opening && (king_far || !findRootToPawn(color, promo_pos, pawn_dist_promo)) )
//        {
//          if ( cy > most_adv_y )
//            most_adv_y = cy;
//
//          score_eg += unstoppablePawn_;
//        }
//      }
//    }
//  }
//
//  if ( !color )
//    score_eg = -score_eg;
//
//  pw_score_eg += score_eg;
//
//  return score;
//}
//
//
//bool Evaluator::isSpecialCase() const
//{
//  return board_->isWinnerLoser() || findSpecialCase() != SC_None;
//}
//
////////////////////////////////////////////////////////////////////////////
///// special cases
//
//// not winner-loser
//Evaluator::SpecialCases Evaluator::findSpecialCase() const
//{
//  const FiguresManager & fmgr = board_->fmgr();
//
//  // no queens
//  if ( fmgr.queens(Figure::ColorBlack)+fmgr.queens(Figure::ColorWhite) > 0 )
//  {
//    return SC_None;
//  }
//
//  // white rooks == black rooks > 0, white has 1 pawn black 1 figure
//  if (fmgr.rooks(Figure::ColorWhite) == fmgr.rooks(Figure::ColorBlack) &&
//      fmgr.rooks(Figure::ColorWhite) > 0 &&
//      fmgr.pawns(Figure::ColorWhite) == 1 &&
//      fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) == 0 &&
//      fmgr.pawns(Figure::ColorBlack) == 0 &&
//      fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) == 1)
//  {
//    return SC_RPRF_W;
//  }
//
//  // white rooks == black rooks > 0, black has 1 pawn white 1 figure
//  if (fmgr.rooks(Figure::ColorBlack) == fmgr.rooks(Figure::ColorWhite) &&
//      fmgr.rooks(Figure::ColorBlack) > 0 &&
//      fmgr.pawns(Figure::ColorBlack) == 1 &&
//      fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) == 0 &&
//      fmgr.pawns(Figure::ColorWhite) == 0 &&
//      fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) == 1)
//  {
//    return SC_RPRF_B;
//  }
//
//  // white rook against figure + pawn
//  if ( fmgr.rooks(Figure::ColorWhite) == 1 &&
//       fmgr.rooks(Figure::ColorBlack) == 0 && 
//       fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) + fmgr.pawns(Figure::ColorWhite) == 0 &&
//       fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) == 1 && 
//       fmgr.pawns(Figure::ColorBlack) == 1 )
//  {
//    return SC_RFP_W;
//  }
//
//  // black rook against figure + pawn
//  if ( fmgr.rooks(Figure::ColorBlack) == 1 &&
//       fmgr.rooks(Figure::ColorWhite) == 0 &&
//       fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) + fmgr.pawns(Figure::ColorBlack) == 0 &&
//       fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) == 1 && 
//       fmgr.pawns(Figure::ColorWhite) == 1 )
//  {
//    return SC_RFP_B;
//  }
//
//  if ( fmgr.rooks(Figure::ColorBlack) + fmgr.rooks(Figure::ColorWhite) == 0 )
//  {
//    // 2 white knights against pawn or 2
//    if ( fmgr.knights(Figure::ColorWhite) == 2 &&
//         fmgr.bishops(Figure::ColorWhite) == 0 &&
//         fmgr.pawns(Figure::ColorWhite) == 0 &&
//         fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack) == 0 &&
//         (fmgr.pawns(Figure::ColorBlack) == 1 || fmgr.pawns(Figure::ColorBlack) == 2) )
//    {
//      return SC_2NP_W;
//    }
//
//    // 2 black knights against pawn or 2
//    if ( fmgr.knights(Figure::ColorBlack) == 2 &&
//         fmgr.bishops(Figure::ColorBlack) == 0 &&
//         fmgr.pawns(Figure::ColorBlack) == 0 &&
//         fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite) == 0 &&
//         (fmgr.pawns(Figure::ColorWhite) == 1 || fmgr.pawns(Figure::ColorWhite) == 2) )
//    {
//      return SC_2NP_B;
//    }
//  }
//
//  // remaining cases suppose that no pawns on the board
//  if ( fmgr.pawns(Figure::ColorWhite) + fmgr.pawns(Figure::ColorBlack) > 0 )
//    return SC_None;
//
//  // black side has 1 rook more, white has 1 figure
//  if (fmgr.rooks(Figure::ColorBlack) - fmgr.rooks(Figure::ColorWhite) == 1 &&
//      fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) == 0 &&
//      fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) == 1)
//  {
//    return SC_RF_B;
//  }
//
//  // white side has 1 rook more, black has 1 figure
//  if (fmgr.rooks(Figure::ColorWhite) - fmgr.rooks(Figure::ColorBlack) == 1 &&
//      fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) == 0 &&
//      fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) == 1)
//  {
//    return SC_RF_W;
//  }
//
//  // black side has 1 rook, white has 2 figures
//  if (fmgr.rooks(Figure::ColorBlack) == 1 &&
//      fmgr.rooks(Figure::ColorWhite) == 0 &&
//      fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) == 0 &&
//      fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) == 2 )
//  {
//    return SC_R2F_B;
//  }
//
//  // white side has 1 rook, black has 2 figures
//  if (fmgr.rooks(Figure::ColorWhite) == 1 &&
//      fmgr.rooks(Figure::ColorBlack) == 0 &&
//      fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) == 0 &&
//      fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) == 2 )
//  {
//    return SC_R2F_W;
//  }
//
//  // it is supposed for all remaining cases that each side has the same number of rooks and one side has only 1 light figure
//  if (fmgr.rooks(Figure::ColorBlack) == 0 || fmgr.rooks(Figure::ColorBlack) != fmgr.rooks(Figure::ColorWhite))
//  {
//    return SC_None;
//  }
//
//  if ( fmgr.bishops(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 0 && 
//       fmgr.bishops(Figure::ColorWhite) == 1 && fmgr.knights(Figure::ColorWhite) == 0 )
//  {
//    return SC_RBR_W;
//  }
//
//  if ( fmgr.bishops(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 0 && 
//       fmgr.bishops(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 1 )
//  {
//    return SC_RNR_W;
//  }
//
//  if ( fmgr.bishops(Figure::ColorBlack) == 1 && fmgr.knights(Figure::ColorBlack) == 0 && 
//       fmgr.bishops(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 0 )
//  {
//    return SC_RBR_B;
//  }
//
//  if ( fmgr.bishops(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 1 && 
//       fmgr.bishops(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 0 )
//  {
//    return SC_RNR_B;
//  }
//
//  return SC_None;
//}
//
//ScoreType Evaluator::evaluateSpecial(SpecialCases sc) const
//{
//  ScoreType score = 0;
//
//  bool white_winner = true;
//  const int & kpb = finfo_[0].king_pos_;
//  const int & kpw = finfo_[1].king_pos_;
//
//  switch ( sc )
//  {
//  case SC_RBR_B:
//  case SC_RNR_B:
//    white_winner = false;
//    score = -Figure::figureWeight_[Figure::TypePawn] >> 1;
//    break;
//
//  case SC_RFP_B:
//    white_winner = false;
//    score = -25;
//    break;
//
//  case SC_R2F_W:
//  case SC_2NP_W:
//    white_winner = false;
//    score = -35;
//    break;
//
//  case SC_RBR_W:
//  case SC_RNR_W:
//    score = +Figure::figureWeight_[Figure::TypePawn] >> 1;
//    break;
//
//  case SC_RFP_W:
//    score = 25;
//    break;
//
//  case SC_R2F_B:
//  case SC_2NP_B:
//    score = 35;
//    break;
//
//  case SC_RPRF_W: // white have pawn
//    score = -25;
//    white_winner = false;
//    break;
//
//  case SC_RPRF_B: // black have pawn
//    score = 25;
//    break;
//
//  case SC_RF_B:
//    score = -35;
//    white_winner = false;
//    break;
//
//  case SC_RF_W:
//    score = 35;
//    break;
//
//  default:
//    X_ASSERT(true, "invalid special case given");
//    break;
//  }
//
//  const FiguresManager & fmgr = board_->fmgr();
//  int pos = 0;
//  // opponent's NB should be as far as possible from my king
//  switch ( sc )
//  {
//  case SC_RBR_W:
//    pos = _lsb64(fmgr.bishop_mask(Figure::ColorWhite));
//    break;
//
//  case SC_RNR_W:
//    pos = _lsb64(fmgr.knight_mask(Figure::ColorWhite));
//    break;
//
//  case SC_RBR_B:
//    pos = _lsb64(fmgr.bishop_mask(Figure::ColorBlack));
//    break;
//
//  case SC_RNR_B:
//    pos = _lsb64(fmgr.knight_mask(Figure::ColorBlack));
//    break;
//  }
//
//  if ( white_winner )
//  {
//    // loser king's position should be near to center
//    score -= positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpb);
//    // winner king should be near to loser king
//    int ki_dist = distanceCounter().getDistance(kpw, kpb);
//    score -= ki_dist;
//  }
//  else
//  {
//    // loser king's position should be near to center
//    score += positionEvaluation(1, Figure::ColorWhite, Figure::TypeKing, kpw);
//    // winner king should be near to loser king
//    int ki_dist = distanceCounter().getDistance(kpw, kpb);
//    score += ki_dist;
//  }
//
//  if ( sc == SC_RBR_W || sc == SC_RNR_W ) // white is winner
//  {
//    int dist = distanceCounter().getDistance(pos, kpb);
//    score -= dist << 1;
//  }
//  else if ( sc == SC_RBR_B || sc == SC_RNR_B ) // back is winner
//  {
//    int dist = distanceCounter().getDistance(pos, kpw);
//    score += dist << 1;
//  }
//
//  if ( Figure::ColorBlack  == board_->getColor() )
//    score = -score;
//
//  return score;
//}
//
//// winner-loser
//ScoreType Evaluator::evaluateWinnerLoser()
//{
//  ScoreType score = 0;
//  if ( !evaluateWinnerLoserSpecial(score) )
//  {
//    score = evaluateTrueWinnerLoser();
//  }
//
//  if ( Figure::ColorBlack  == board_->getColor() )
//    score = -score;
//
//  return score;
//}
//
//bool Evaluator::evaluateWinnerLoserSpecial(ScoreType & score)
//{
//  const FiguresManager & fmgr = board_->fmgr();
//  score = 0;
//
//  if ( fmgr.pawns(Figure::ColorWhite) + fmgr.rooks(Figure::ColorWhite) + fmgr.queens(Figure::ColorWhite) + 
//       fmgr.pawns(Figure::ColorBlack) + fmgr.rooks(Figure::ColorBlack) + fmgr.queens(Figure::ColorBlack) > 0 )
//  {
//    return false;
//  }
//
//  // 1. special case - both sides have one bishop|knight
//  if ( (fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite) == 1) && 
//       (fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack) == 1) &&
//       !board_->can_win_[0] && !board_->can_win_[1] )
//  {
//    // return negative value to force exchange
//    score = (Figure::ColorBlack == board_->getColor()) ? 10 : -10;
//    return true;
//  }
//
//  Index kpw = board_->kingPos(Figure::ColorWhite);
//  Index kpb = board_->kingPos(Figure::ColorBlack);
//
//  // 2. special case - one side has 2 knight|bishop and another has only one or zero
//  // black wins
//  // bishop + knight agains 1 bishop|knight
//  if ( (fmgr.bishops(Figure::ColorBlack) == 1 && fmgr.knights(Figure::ColorBlack) == 1) && 
//       (fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite) == 1) )
//  {
//    // force exchange
//    score = -20;
//    score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpw);
//    return true;
//  }
//  // 2 knights agains 1 bishop|knight
//  if ( (fmgr.bishops(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 2) && 
//       (fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite) == 1) )
//  {
//    // force exchange
//    score = -10;
//    score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpw) >> 2;
//    return true;
//  }
//  // 2 bishop agains 1 bishop
//  if ( (fmgr.bishops(Figure::ColorBlack) == 2 && fmgr.knights(Figure::ColorBlack) == 0) && 
//       (fmgr.bishops(Figure::ColorWhite) == 1 && fmgr.knights(Figure::ColorWhite) == 0) )
//  {
//    // force exchange
//    score = -30;
//    score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpw);
//    return true;
//  }
//  // 2 bishop agains 1 knight
//  if ( (fmgr.bishops(Figure::ColorBlack) == 2 && fmgr.knights(Figure::ColorBlack) == 0) && 
//       (fmgr.bishops_b(Figure::ColorBlack) == 1 && fmgr.bishops_w(Figure::ColorBlack) == 1) &&
//       (fmgr.bishops(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 1) )
//  {
//    // can lose
//    score = -75;
//    score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpw);
//    // knight close to center
//    int npos = _lsb64(fmgr.knight_mask(Figure::ColorWhite));
//    score -= distanceCounter().getDistance(npos, 32);
//    return true;
//  }
//
//  // usualy non-winning figures against 0
//  if ( ((fmgr.knights(Figure::ColorBlack) == 2 && fmgr.bishops(Figure::ColorBlack) == 0) ||
//        (fmgr.bishops_b(Figure::ColorBlack) > 1 && fmgr.bishops_w(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 0) ||
//        (fmgr.bishops_w(Figure::ColorBlack) > 1 && fmgr.bishops_b(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 0)) &&
//        (fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) == 0) )
//  {
//    score = -10;
//    score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpw) >> 2;
//    return true;
//  }
//
//  // white wins
//  // bishop + knight agains 1 bishop|knight
//  if ( (fmgr.bishops(Figure::ColorWhite) == 1 && fmgr.knights(Figure::ColorWhite) == 1) && 
//       (fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack) == 1) )
//  {
//    // force exchange
//    score = +20;
//    score -= positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpb);
//    return true;
//  }
//  // 2 knights agains 1 bishop|knight
//  if ( (fmgr.bishops(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 2) && 
//       (fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack) == 1) )
//  {
//    // force exchange
//    score = +10;
//    score -= positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpb) >> 2;
//    return true;
//  }
//  // 2 bishop agains 1 bishop
//  if ( (fmgr.bishops(Figure::ColorWhite) == 2 && fmgr.knights(Figure::ColorWhite) == 0) && 
//       (fmgr.bishops(Figure::ColorBlack) == 1 && fmgr.knights(Figure::ColorBlack) == 0) )
//  {
//    // force exchange
//    score = +30;
//    score -= positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpb);
//    return true;
//  }
//  // 2 bishop agains 1 knight
//  if ( (fmgr.bishops(Figure::ColorWhite) == 2 && fmgr.knights(Figure::ColorWhite) == 0) && 
//       (fmgr.bishops_b(Figure::ColorWhite) == 1 && fmgr.bishops_w(Figure::ColorWhite) == 1) &&
//       (fmgr.bishops(Figure::ColorBlack) == 0 && fmgr.knights(Figure::ColorBlack) == 1) )
//  {
//    // can lose
//    score = +75;
//    score -= positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpb);
//    // knight close to center
//    int npos = _lsb64(fmgr.knight_mask(Figure::ColorBlack));
//    score += distanceCounter().getDistance(npos, 32);
//    return true;
//  }
//
//  // usualy non-winning figures against 0
//  if ( ((fmgr.knights(Figure::ColorWhite) == 2 && fmgr.bishops(Figure::ColorWhite) == 0) ||
//        (fmgr.bishops_b(Figure::ColorWhite) > 1 && fmgr.bishops_w(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 0) ||
//        (fmgr.bishops_w(Figure::ColorWhite) > 1 && fmgr.bishops_b(Figure::ColorWhite) == 0 && fmgr.knights(Figure::ColorWhite) == 0)) &&
//        (fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) == 0) )
//  {
//    score = 10;
//    score -= positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, kpb);
//    return true;
//  }
//
//  return false;
//}
//

} //NEngine
