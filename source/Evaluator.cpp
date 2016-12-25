/*************************************************************
  Evaluator.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Evaluator.h>
#include <Board.h>
#include <HashTable.h>
#include <xindex.h>
#include <magicbb.h>

namespace NEngine
{

  enum {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
  };
  
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
  //// king position eval for BN-mat
  //const ScoreType Evaluator::bishopKnightMat_[64] =
  //{
  //  16,   10,  6,  1, -2, -5,  -12,  -16,
  //  10,   12,  5, -1, -3, -6,  -14,  -12,
  //   5,    5,  4, -2, -4, -8,   -8,  -10,
  //  -1,   -1, -2, -6, -6, -6,   -5,   -4,
  //  -4,   -5, -6, -6, -6, -2,   -1,   -1,
  //  -10,  -8, -8, -4, -2,  4,    5,    5,
  //  -12, -14, -6, -3, -1,  5,   12,   10,
  //  -16, -12, -5, -2,  1,  6,   10,   16
  //};
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

    finfo_[0] = {};
    finfo_[1] = {};

    finfo_[0].king_pos_ = board_->kingPos(Figure::ColorBlack);
    finfo_[1].king_pos_ = board_->kingPos(Figure::ColorWhite);


    // pawns attacks
    {
      const BitMask & pawn_msk_w = board_->fmgr().pawn_mask(Figure::ColorWhite);
      const BitMask & pawn_msk_b = board_->fmgr().pawn_mask(Figure::ColorBlack);

      finfo_[Figure::ColorWhite].pw_attack_mask_ = ((pawn_msk_w << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_w << 7) & Figure::pawnCutoffMasks_[1]);
      finfo_[Figure::ColorBlack].pw_attack_mask_ = ((pawn_msk_b >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_b >> 9) & Figure::pawnCutoffMasks_[1]);

      finfo_[0].attack_mask_ = finfo_[0].pw_attack_mask_;
      finfo_[1].attack_mask_ = finfo_[1].pw_attack_mask_;
    }

    // attacked fields near king
  {
    const BitMask & ki_mask_b = movesTable().caps(Figure::TypeKing, finfo_[0].king_pos_);
    const BitMask & ki_mask_w = movesTable().caps(Figure::TypeKing, finfo_[1].king_pos_);

    finfo_[0].attackersN_[Figure::TypePawn] = (int)((finfo_[Figure::ColorBlack].pw_attack_mask_ & ki_mask_w) != 0);
    finfo_[1].attackersN_[Figure::TypePawn] = (int)((finfo_[Figure::ColorWhite].pw_attack_mask_ & ki_mask_b) != 0);

    finfo_[0].attackersN_[Figure::TypeKing] = finfo_[1].attackersN_[Figure::TypeKing] = (int)((ki_mask_b & ki_mask_w) != 0);
  }

  alpha_ = -ScoreMax;
  betta_ = +ScoreMax;
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
      return -Figure::MatScore;
    else if(board_->drawState())
      return Figure::DrawScore;

    //ScoreType score = board_->fmgr().weight();
    //for(Figure::Color color : { Figure::ColorBlack, Figure::ColorWhite})
    //{
    //  int k = color*2 - 1;
    //  for(int type = Figure::TypePawn; type <= Figure::TypeQueen; ++type)
    //  {
    //    BitMask fg_mask = board_->fmgr().type_mask((Figure::Type)type, color);
    //    score += Figure::figureWeight_[type] * pop_count(fg_mask) * k;
    //  }
    //}

    //// consider current move side
    //if(Figure::ColorBlack  == board_->getColor())
    //  score = -score;

    prepare();

    // prepare lazy evaluation
    if(alpha > -Figure::MatScore)
      alpha_ = alpha - lazyThreshold_;

    if(betta < +Figure::MatScore)
      betta_ = betta + lazyThreshold_;

    ScoreType score = -ScoreMax;

    //if ( board_->isWinnerLoser() )
    //  score = evaluateWinnerLoser();
    //else
    //{
    //  SpecialCases sc = findSpecialCase();
    //  if ( sc != SC_None )
    //    score = evaluateSpecial(sc);
    //  else
    score = evaluate();
    //}

    X_ASSERT(score <= -ScoreMax || score >= ScoreMax, "invalid score");

    return score;
  }
  ////////////////////////////////////////////////////////////////////////////
  //ScoreType Evaluator::express() const
  //{
  //	ScoreType score = board_->fmgr().weight();
  //
  //	// consider current move side
  //	if ( Figure::ColorBlack  == board_->getColor() )
  //		score = -score;
  //
  //	return score;
  //}

  ScoreType Evaluator::evaluate()
  {
    const FiguresManager & fmgr = board_->fmgr();

    // 1. evaluate figures weight
    ScoreType score = fmgr.weight();

    //score += evaluateMaterialDiff();

    // take pawns eval. from hash if possible
    auto pawnScore = hashedEvaluation();

    // basic king safety - pawn shield, castle
    auto kingScore = evaluateKingSafety();

    score += pawnScore.common_;
    score += kingScore.common_;

    //// blocked by opponent pawns
    //score += evaluateBlockedBishops();
    //score += evaluateBlockedKnights();

    // 2. determine game phase (opening, middle or end game)
    auto phaseInfo = detectPhase();

    ScoreType score_o = 0, score_e = 0;

    // opening part
    if(phaseInfo.phase_ != EndGame)
    {
      //// PSQ - evaluation
      //score_o -= fmgr.eval(Figure::ColorBlack, 0);
      //score_o += fmgr.eval(Figure::ColorWhite, 0);

      //ScoreType score_king = evaluateCastlePenalty(Figure::ColorWhite) - evaluateCastlePenalty(Figure::ColorBlack);
      //score_king += score_ps;
      //score_o += score_king;

      //score_o += evaluateFianchetto();

      // pawns
      score_o += pawnScore.opening_;

      // king
      score_o += kingScore.opening_;
    }

    if(phaseInfo.phase_ != Opening)
    {
      //score_e -= fmgr.eval(Figure::ColorBlack, 1);
      //score_e += fmgr.eval(Figure::ColorWhite, 1);

      // pawns
      score_e += pawnScore.endGame_;

      // king
      score_e += kingScore.endGame_;
    }

    if(phaseInfo.phase_ == Opening)
      score += score_o;
    else if(phaseInfo.phase_ == EndGame)
      score += score_e;
    else // middle game
      score = score + (score_o * phaseInfo.opening_ + score_e * phaseInfo.endGame_) / weightOEDiff_;

    // consider current move side
    if(Figure::ColorBlack  == board_->getColor())
      score = -score;

    /// use lazy evaluation
    if(score < alpha_ || score > betta_)
      return score;

    //ScoreType score_ex = evaluateExpensive(phase, coef_o, coef_e);
    ////if ( abs(score_ex) > score_ex_max_ )
    ////  score_ex_max_ = abs(score_ex);

    //score += score_ex;

    return score;
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

  Evaluator::PawnsScore Evaluator::hashedEvaluation()
  {
    HEval * heval = 0;
    const uint64 & code = board_->pawnCode();
    uint32 hkey = (uint32)(code >> 32);

    if(ehash_)
    {
      heval = ehash_->get(code);

      if(heval->hkey_ == hkey && heval->initizalized_)
      {
        PawnsScore score;
        score.common_ = heval->common_;
        score.opening_ = heval->opening_;
        score.endGame_ = heval->endGame_;
        return score;
      }
    }

    auto score = evaluatePawns(Figure::ColorWhite);
    score -= evaluatePawns(Figure::ColorBlack);
    score >>= EvalCoefficients::shift_divider;

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
  BitMask blockers_mask = (finfo_[ocolor].pw_attack_mask_ & (~finfo_[color].pw_attack_mask_)) | opmsk;
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

Evaluator::PawnsScore Evaluator::evaluatePawns(Figure::Color color) const
{
  const FiguresManager & fmgr = board_->fmgr();
  const BitMask & pmask = fmgr.pawn_mask(color);
  if(!pmask)
    return{};

//  std::string sfen = toFEN(*board_);

  PawnsScore score;
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

    // 1. center pawns
    if(x == 3 || x == 4) // D, E columns
    {
      score.opening_ += coeffs_->centerPawn_[cy];
    }

    // 2. passed pawn
    {
      auto coeffPassed = coeffs_->passedPawn_ * cy;
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
      // 5. backward || forward but not supported
      if(((pawnMasks().mask_backward(color, n) & pmask) == 0ULL)
         || !couldBeSupported(idx, color, ocolor, pmask, opmsk))
      {
        // TODO: optimization required
        int closest_y = closestToBackward(idx.x(), idx.y(), pmask, color);
        X_ASSERT(closest_y < 0 || closest_y > 7, "backward mask calculation error - invalid next y position");
        BitMask pmask_after = betweenMasks().between(n, Index(idx.x(), closest_y));
        bool is_blocked  = (finfo_[ocolor].pw_attack_mask_ | opmsk) & pmask_after;
        score.common_ += is_blocked * coeffs_->backwardPawn_;
      }
    }
  }

  // 5. additional bonus for passers on neighbour columns
  {
    auto multi = pawnMasks().mask_multi_passer(x_passers) & pmask;
    if(multi)
    {
      int y_min = color ? _lsb64(multi) : _msb64(multi);
      y_min >>= 3;
      int cy = colored_y_[color][y_min];
      score.common_ += (coeffs_->passedPawn_ * cy) >> 1;
    }
  }

  return score;
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

Evaluator::PawnsScore Evaluator::evaluateKingSafety() const
{
  auto score = evaluateKingSafety(Figure::ColorWhite);
  score -= evaluateKingSafety(Figure::ColorBlack);
  return score;
}

Evaluator::PawnsScore Evaluator::evaluateKingSafety(Figure::Color color) const
{
  const FiguresManager & fmgr = board_->fmgr();
  auto pmask  = board_->fmgr().pawn_mask(color);

  PawnsScore score;
  Figure::Color ocolor = Figure::otherColor((Figure::Color)color);
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

  if(ctype >= 0)
  {
    int shield_bonus = ((pawns_shield_mask[color][ctype][0] & pmask) != 0) * coeffs_->pawnShieldA_
      + ((pawns_shield_mask[color][ctype][1] & pmask) != 0) * coeffs_->pawnShieldB_
      + ((pawns_shield_mask[color][ctype][2] & pmask) != 0) * coeffs_->pawnShieldC_;
    score.opening_ += shield_bonus;
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







//
//ScoreType Evaluator::evaluateBlockedBishops()
//{
//  ScoreType score_b = 0, score_w = 0;
//
//  BitMask bimask_w = board_->fmgr().bishop_mask(Figure::ColorWhite);
//  for ( ; bimask_w; )
//  {
//    int n = clear_lsb(bimask_w);
//    switch ( n )
//    {
//    case A7:
//      if ( board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn) )
//        score_w -= bishopBlocked_;
//      break;
//
//    case B8:
//      if ( board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn) &&
//            (board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn) ||
//             board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn)) )
//        score_w -= bishopBlocked_;
//      break;
//
//    case H7:
//      if ( board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn) )
//        score_w -= bishopBlocked_;
//      break;
//
//    case G8:
//      if ( board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn) &&
//            (board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn) ||
//             board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn)) )
//        score_w -= bishopBlocked_;
//      break;
//
//    case A6:
//      if ( board_->isFigure(B5, Figure::ColorBlack, Figure::TypePawn) &&
//           board_->isFigure(C6, Figure::ColorBlack, Figure::TypePawn) )
//        score_w -= bishopBlocked_;
//      break;
//
//    case H6:
//      if ( board_->isFigure(G5, Figure::ColorBlack, Figure::TypePawn) &&
//           board_->isFigure(F6, Figure::ColorBlack, Figure::TypePawn) )
//        score_w -= bishopBlocked_;
//      break;
//    }
//  }
//
//  BitMask bimask_b = board_->fmgr().bishop_mask(Figure::ColorBlack);
//  for ( ; bimask_b; )
//  {
//    int n = clear_lsb(bimask_b);
//
//    switch ( n )
//    {
//    case A2:
//      if ( board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn) )
//        score_b -= bishopBlocked_;
//      break;
//
//    case B1:
//      if ( board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn) &&
//            (board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn) ||
//             board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn)) )
//        score_b -= bishopBlocked_;
//      break;
//
//    case H2:
//      if ( board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn) )
//        score_b -= bishopBlocked_;
//      break;
//
//    case G1:
//      if ( board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn) &&
//            (board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn) ||
//             board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn)) )
//        score_b -= bishopBlocked_;
//      break;
//
//    case A3:
//      if ( board_->isFigure(B4, Figure::ColorWhite, Figure::TypePawn) &&
//           board_->isFigure(C3, Figure::ColorWhite, Figure::TypePawn) )
//        score_b -= bishopBlocked_;
//      break;
//
//    case H3:
//      if ( board_->isFigure(G4, Figure::ColorWhite, Figure::TypePawn) &&
//           board_->isFigure(F3, Figure::ColorWhite, Figure::TypePawn) )
//        score_b -= bishopBlocked_;
//      break;
//    }
//  }
//
//  ScoreType score = score_w - score_b;
//  return score;
//}
//
//ScoreType Evaluator::evaluateBlockedKnights()
//{
//  ScoreType score_b = 0, score_w = 0;
//
//  BitMask knight_w = board_->fmgr().knight_mask(Figure::ColorWhite);
//  for ( ; knight_w; )
//  {
//    int n = clear_lsb(knight_w);
//
//    switch ( n )
//    {
//    case A8:
//      if ( board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn) ||
//           board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn) )
//        score_w -= knightBlocked_;
//      break;
//
//    case A7:
//      if ( board_->isFigure(A6, Figure::ColorBlack, Figure::TypePawn) &&
//           board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn) )
//        score_w -= knightBlocked_;
//      break;
//
//    case H8:
//      if ( board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn) ||
//           board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn) )
//        score_w -= knightBlocked_;
//      break;
//
//    case H7:
//      if ( board_->isFigure(H6, Figure::ColorBlack, Figure::TypePawn) &&
//           board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn) )
//        score_w -= knightBlocked_;
//      break;
//    }
//  }
//
//  BitMask knight_b = board_->fmgr().knight_mask(Figure::ColorBlack);
//  for ( ; knight_b; )
//  {
//    int n = clear_lsb(knight_b);
//
//    switch ( n )
//    {
//    case A1:
//      if ( board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn) ||
//           board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn) )
//        score_b -= knightBlocked_;
//      break;
//
//    case A2:
//      if ( board_->isFigure(A3, Figure::ColorWhite, Figure::TypePawn) &&
//           board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn) )
//        score_b -= knightBlocked_;
//      break;
//
//    case H1:
//      if ( board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn) ||
//           board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn) )
//        score_b -= knightBlocked_;
//      break;
//
//    case H2:
//      if ( board_->isFigure(H3, Figure::ColorWhite, Figure::TypePawn) &&
//           board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn) )
//        score_b -= knightBlocked_;
//      break;
//    }
//  }
//
//  ScoreType score = score_w - score_b;
//  return score;
//}
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
//    BitMask not_occupied = ~finfo_[ocolor].pw_attack_mask_ & inv_mask_all_;
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
//    BitMask not_attacked = ~finfo_[ocolor].pw_attack_mask_;
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
//  BitMask pawn_fork = o_mask & finfo_[color].pw_attack_mask_;
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
//ScoreType Evaluator::evaluateMaterialDiff()
//{
//  ScoreType score = 0;
//
//  const FiguresManager & fmgr = board_->fmgr();
//
//  // 1. bonus for bishop. only if we have other figures
//  if ( fmgr.bishops(Figure::ColorWhite) + fmgr.knights(Figure::ColorWhite) + fmgr.rooks(Figure::ColorWhite) + fmgr.queens(Figure::ColorWhite) > 1 )
//    score += fmgr.bishops(Figure::ColorWhite)*bishopBonus_;
//  if ( fmgr.bishops(Figure::ColorBlack) + fmgr.knights(Figure::ColorBlack) + fmgr.rooks(Figure::ColorBlack) + fmgr.queens(Figure::ColorBlack) > 1 )
//    score -= fmgr.bishops(Figure::ColorBlack)*bishopBonus_;
//
//  // 2. additional bonus for double bishop
//  if ( fmgr.bishops_b(Figure::ColorWhite) && fmgr.bishops_w(Figure::ColorWhite) )
//    score += bishopBonus_;
//  if ( fmgr.bishops_b(Figure::ColorBlack) && fmgr.bishops_w(Figure::ColorBlack) )
//    score -= bishopBonus_;
//
//  // 3. Knight or Bishop against 3 pawns
//  int figuresDiff = (fmgr.bishops(Figure::ColorWhite)+fmgr.knights(Figure::ColorWhite)) -
//    (fmgr.bishops(Figure::ColorBlack)+fmgr.knights(Figure::ColorBlack));
//
//  int pawnsDiff  = fmgr.pawns(Figure::ColorWhite) - fmgr.pawns(Figure::ColorBlack);
//  int rooksDiff  = fmgr.rooks(Figure::ColorWhite) - fmgr.rooks(Figure::ColorBlack);
//  int queensDiff = fmgr.queens(Figure::ColorWhite) - fmgr.queens(Figure::ColorBlack);
//
//  if ( figuresDiff*pawnsDiff < 0 && !rooksDiff && !queensDiff )
//  {
//    Figure::Color strongColor = (Figure::Color)(figuresDiff > 0);
//    int zeroPawns = (fmgr.pawns(strongColor) != 0) & 1;
//    score += figuresDiff * figureAgainstPawnBonus_[zeroPawns];
//  }
//
//  // 4. Knight|Bishop+2Pawns vs. Rook
//  else if (!queensDiff && ((rooksDiff == -1 && figuresDiff == 1) || (rooksDiff == 1 && figuresDiff == -1)))
//  {
//    Figure::Color strongColor = (Figure::Color)(rooksDiff > 0);
//    int zeroPawns = (fmgr.pawns(strongColor) != 0) & 1;
//    score += rooksDiff * rookAgainstFigureBonus_[zeroPawns];
//  }
//  // 5. pawns against rook
//  else if ( !queensDiff && rooksDiff*pawnsDiff < 0 )
//  {
//    Figure::Color strongColor = (Figure::Color)(rooksDiff > 0);
//    int zeroPawns = (fmgr.pawns(strongColor) != 0) & 1;
//    score += rooksDiff * rookAgainstPawnsBonus_[zeroPawns];
//  }
//
//  return score;
//}
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
//// idea from CCRL
//bool Evaluator::findRootToPawn(Figure::Color color, int promo_pos, int stepsMax) const
//{
//  Figure::Color ocolor = Figure::otherColor(color);
//  int oki_pos = finfo_[ocolor].king_pos_;
//
//  if ( oki_pos == promo_pos )
//  {
//    X_ASSERT(finfo_[color].attack_mask_ & set_mask_bit(promo_pos), "king is on attacked field");
//    return true;
//  }
//
//  BitMask to_mask = set_mask_bit(promo_pos);
//
//  // promotion field is attacked
//  if ( finfo_[color].attack_mask_ & to_mask )
//    return false;
//
//  BitMask from_mask = set_mask_bit(oki_pos);
//  BitMask path_mask = (inv_mask_all_ & ~finfo_[color].attack_mask_) | from_mask | to_mask;
//
//  const BitMask cut_le = ~0x0101010101010101;
//  const BitMask cut_ri = ~0x8080808080808080;
//
//  for (int i = 0; i < stepsMax; ++i)
//  {
//    X_ASSERT( i > 64, "infinite loop in path finder" );
//
//    BitMask mask_le = ((from_mask << 1) | (from_mask << 9) | (from_mask >> 7) | (from_mask << 8)) & cut_le;
//    BitMask mask_ri = ((from_mask >> 1) | (from_mask >> 9) | (from_mask << 7) | (from_mask >> 8)) & cut_ri;
//    BitMask next_mask = from_mask | ((mask_le | mask_ri) & path_mask);
//
//    if ( next_mask & to_mask )
//      return true;
//
//    if ( next_mask == from_mask )
//      break;
//
//    from_mask = next_mask;
//  }
//
//  return false;
//}
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
//ScoreType Evaluator::evaluateTrueWinnerLoser()
//{
//  const FiguresManager & fmgr = board_->fmgr();
//  Figure::Color win_color = board_->can_win_[0] ? Figure::ColorBlack : Figure::ColorWhite;
//  Figure::Color lose_color = Figure::otherColor(win_color);
//
//  ScoreType score = fmgr.weight(win_color);
//
//  // bonus for pawns
//  score += fmgr.pawns(win_color)*pawnEndgameBonus_;
//
//  Index king_pos_w = board_->kingPos(win_color);
//  Index king_pos_l = board_->kingPos(lose_color);
//
//  bool eval_pawns = true;
//
//  if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) > 0 )
//  {
//    int num_lose_figs = fmgr.knights(lose_color) + fmgr.bishops(lose_color);
//    ScoreType weight_lose_fig = 10;
//
//    // if winner doesn't have light figure and loser has more than 1 figure we don't want to evaluate them less than a pawn
//    if ( fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 1 && fmgr.knights(win_color) +fmgr.bishops(win_color) == 0 )
//      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn] + (MAX_PASSED_SCORE) + pawnEndgameBonus_;
//
//    // if winner has more pawns than loser and also has some figure he must exchange all loser figures to pawns
//    else if ( fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0  &&
//         fmgr.knights(win_color) +fmgr.bishops(win_color) > 0 &&
//         fmgr.knights(lose_color)+fmgr.bishops(lose_color) < fmgr.pawns(win_color) )
//    {
//      weight_lose_fig = Figure::figureWeight_[Figure::TypePawn] + (MAX_PASSED_SCORE) + pawnEndgameBonus_;
//      eval_pawns = false;
//    }
//
//    score -= num_lose_figs * weight_lose_fig;
//  }
//  else
//    score -= fmgr.weight(lose_color);
//
//  // add small bonus for winner-loser state to force it
//  score += winloseBonus_;
//
//  // BN-mat case
//  if ( fmgr.weight(lose_color) == 0 && fmgr.knights(win_color) == 1 && fmgr.bishops(win_color) == 1 &&
//       fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 0 )
//  {
//    int dist  = distanceCounter().getDistance(king_pos_w, king_pos_l);
//    score -= dist;
//
//    int kp = king_pos_l;
//    if ( fmgr.bishops_w(win_color) )
//    {
//      int kx = king_pos_l.x();
//      int ky = king_pos_l.y();
//      kp = ((7-ky)<<3)| kx;
//    }
//    score += bishopKnightMat_[kp];
//
//    uint64 n_mask = fmgr.knight_mask(win_color);
//    int np = clear_lsb(n_mask);
//    X_ASSERT( (unsigned)np > 63, "no knigt found" );
//    int ndist = distanceCounter().getDistance(np, king_pos_l);
//    score -= ndist >> 1;
//
//    // add more bonus to be sure that we go to this state
//    score += winloseBonus_;
//  }
//  else
//  {
//    // some special almost-draw cases
//    if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 0 &&
//         fmgr.weight(win_color)-fmgr.weight(lose_color) < Figure::figureWeight_[Figure::TypeBishop]+Figure::figureWeight_[Figure::TypeKnight] )
//    {
//      score = 10;
//    }
//    else if ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.pawns(win_color) == 1 &&
//              fmgr.knights(lose_color)+fmgr.bishops(lose_color) > 0 )
//    {
//      if ( fmgr.knights(win_color)+fmgr.bishops(win_color) <= fmgr.knights(lose_color)+fmgr.bishops(lose_color) )
//      {
//        score = (MAX_PASSED_SCORE);
//        uint64 pwmask = fmgr.pawn_mask_o(win_color);
//        int pp = clear_lsb(pwmask);
//        int x = pp & 7;
//        int y = pp >> 3;
//        if ( !win_color )
//          y = 7-y;
//        if ( y < 6 )
//        {
//          int ep = x | (win_color ? A8 : A1);
//          int8 pwhite = FiguresCounter::s_whiteColors_[ep];
//          if ( pwhite && fmgr.bishops_w(lose_color) || !pwhite && fmgr.bishops_b(lose_color) > 0 || y < 5 )
//            score = 10;
//        }
//        else if ( board_->color_ == win_color )
//          score = Figure::figureWeight_[Figure::TypePawn];
//        score += y << 1;
//        eval_pawns = false;
//      }
//    }
//    // Rook against Knight|Bishop
//    else if ( fmgr.queens(win_color) == 0 && fmgr.bishops(win_color) == 0 &&
//              fmgr.knights(win_color) == 0 && fmgr.pawns(win_color) == 0 && fmgr.rooks(win_color) == 1 &&
//              fmgr.knights(lose_color)+fmgr.bishops(lose_color) == 1 )
//    {
//      score = 35;
//    }
//
//    if ( fmgr.pawns(win_color) == 1 )
//    {
//      uint64 pwmsk = fmgr.pawn_mask_o(win_color);
//      int pp = clear_lsb(pwmsk);
//      X_ASSERT( (unsigned)pp > 63, "no pawn found" );
//
//      int ykl = king_pos_l.y();
//      int ykw = king_pos_w.y();
//
//      int xkl = king_pos_l.x();
//      int xkw = king_pos_w.x();
//
//      int x = pp & 7;
//      int y = pp >> 3;
//      int y_under = y;
//      if ( win_color )
//        y_under++;
//      else
//        y_under--;
//      int pp_under = (x | (y_under << 3)) & 63;
//
//      if ( !win_color )
//      {
//        y = 7-y;
//        ykl = 7-ykl;
//        ykw = 7-ykw;
//      }
//
//      int pr_pos = x | (win_color ? A8 : A1);
//      Figure::Color pr_color = (Figure::Color)FiguresCounter::s_whiteColors_[pr_pos];
//
//      int pr_moves = 7-y;
//
//      int wk_pr_dist = distanceCounter().getDistance(king_pos_w, pr_pos);
//      int lk_pr_dist = distanceCounter().getDistance(king_pos_l, pr_pos);
//
//      int wdist = distanceCounter().getDistance(king_pos_w, pp);
//      int ldist = distanceCounter().getDistance(king_pos_l, pp);
//
//      int wudist = distanceCounter().getDistance(king_pos_w, pp_under);
//      int ludist = distanceCounter().getDistance(king_pos_l, pp_under);
//
//      // special case KPK
//      if ( (fmgr.weight(win_color) == Figure::figureWeight_[Figure::TypePawn] && fmgr.weight(lose_color) == 0) )
//      {
//        bool almost_draw = false;
//        if ( x == 0 || x == 7 )
//        {
//          if ( (lk_pr_dist + ludist <= wk_pr_dist + wudist) ||
//               ((pr_moves >= lk_pr_dist && y > 1 || pr_moves > lk_pr_dist && y == 1) && wudist >= ludist) )
//            almost_draw = true;
//        }
//        else
//        {
//          if ( (pr_moves >= lk_pr_dist && y > 1 || pr_moves > lk_pr_dist && y == 1) &&
//               ( (wudist > ludist || wudist == ludist && lose_color == board_->color_) && y >= ykw ||
//                 (wudist > ludist+1 && y < ykw) ) )
//            almost_draw = true;
//        }
//
//        if ( almost_draw )
//        {
//          score = 30 + (y<<1);
//          eval_pawns = false;
//        }
//      }
//      // KPBK. bishop color differs from promotion field color
//      else if ( ( fmgr.rooks(win_color) == 0 && fmgr.queens(win_color) == 0 && fmgr.knights(win_color) == 0 &&
//                  fmgr.bishops(win_color) && (x == 0 || x == 7) &&
//                 (!fmgr.bishops_w(win_color) && pr_color || !fmgr.bishops_b(win_color) && !pr_color) ) )
//      {
//        if ( (pr_moves > lk_pr_dist && lk_pr_dist <= wk_pr_dist) || (lk_pr_dist < 2 && pr_moves > 0) )
//        {
//          score = 30 + (y<<1);
//          eval_pawns = false;
//        }
//      }
//
//      // opponent's king should be as far as possible from my pawn
//      score -= (7-ldist);
//
//      // my king should be as near as possible to my pawn
//      score -= wdist;
//    }
//    else
//    {
//      int dist  = distanceCounter().getDistance(king_pos_w, king_pos_l);
//      score -= dist << 1;
//      score -= positionEvaluation(1, lose_color, Figure::TypeKing, king_pos_l);
//      score += positionEvaluation(1, Figure::ColorBlack, Figure::TypeKing, king_pos_w) >> 1;
//    }
//  }
//
//  if ( win_color == Figure::ColorBlack )
//    score = -score;
//
//  if ( eval_pawns )
//  {
//    ScoreType pwscore = -ScoreMax, pwscore_eg = -ScoreMax, score_ps = -ScoreMax;
//    hashedEvaluation(pwscore, pwscore_eg, score_ps);
//
//    /// calculate attacked fields
//    evaluateKnights();
//    evaluateBishops();
//    evaluateRooks(false);
//    evaluateQueens();
//
//    int most_adv_yb = -1, most_adv_yw = -1;
//    ScoreType score_eg = 0;
//    score -= evaluatePasserAdditional(EndGame, Figure::ColorBlack, score_eg, most_adv_yb);
//    score += evaluatePasserAdditional(EndGame, Figure::ColorWhite, score_eg, most_adv_yw);
//
//    score += score_eg;
//    score += pwscore;
//  }
//
//  return score;
//}

} //NEngine
