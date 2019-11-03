/*************************************************************
EvalCoefficients.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <EvalCoefficients.h>
#include <fstream>
#include <set>
#include <algorithm>

namespace NEngine
{
  // single vars
  // pawns
  int EvalCoefficients::pawnEndgameBonus_{ 15 };
  int EvalCoefficients::doubledPawn_{ -12 };
  int EvalCoefficients::isolatedPawn_{ -14 };
  int EvalCoefficients::backwardPawn_{ -14 };
  int EvalCoefficients::unsupportedPawn_{ -12 };
  int EvalCoefficients::unprotectedPawn_{ -9 };
  int EvalCoefficients::rookBehindBonus_{ 5 };
  int EvalCoefficients::protectedPawnPressure_{ 2 };
  int EvalCoefficients::unprotectedPawnPressure_{ 5 };
  int EvalCoefficients::protectedPawnBishopTreat_{ 2 };
  int EvalCoefficients::unprotectedPawnBishopTreat_{ 5 };
  int EvalCoefficients::kingPressure_{ 3 };
  int EvalCoefficients::generalPressure_{ 1 };

  // forks
  int EvalCoefficients::forkBonus_{ 35 };
  int EvalCoefficients::doublePawnAttack_{ 50 };

  // king
  int EvalCoefficients::castleImpossible_{ -28 };
  int EvalCoefficients::fakeCastle_{ -30 };
  int EvalCoefficients::castleBonus_{ 10 };
  int EvalCoefficients::pawnPenaltyA_{ -14 };
  int EvalCoefficients::pawnPenaltyB_{ -14 };
  int EvalCoefficients::pawnPenaltyC_{ -4 };
  int EvalCoefficients::noPawnPenaltyA_{ -8 };
  int EvalCoefficients::noPawnPenaltyB_{ -8 };
  int EvalCoefficients::noPawnPenaltyC_{ -1 };

  // blocked figure
  int EvalCoefficients::knightBlocked_{ 80 };
  int EvalCoefficients::bishopBlocked_{ 80 };

  // king attacks
  int EvalCoefficients::pawnKingAttack_{ 1 };
  int EvalCoefficients::knightKingAttack_{ 2 };
  int EvalCoefficients::bishopKingAttack_{ 2 };
  int EvalCoefficients::rookKingAttack_{ 5 };
  int EvalCoefficients::queenKingAttack_{ 12 };

  // x-ray attacks
  int EvalCoefficients::rookKingAttackXray_{ 4 };
  int EvalCoefficients::queenKingAttackXray_{ 8 };

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 2 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 2 };

  // king threat
  int EvalCoefficients::knightChecking_{ 3 };
  int EvalCoefficients::bishopChecking_{ 3 };
  int EvalCoefficients::rookChecking_{ 5 };
  int EvalCoefficients::queenChecking_{ 12 };

  // pinned figures
  int EvalCoefficients::pinnedPawn_{ 4 };
  int EvalCoefficients::pinnedKnight_{ 7 };
  int EvalCoefficients::pinnedBishop_{ 7 };
  int EvalCoefficients::pinnedRook_{ 10 };
  int EvalCoefficients::pinnedQueen_{ 12 };

  // arrays
  int EvalCoefficients::opponentPawnPressure_[8] = { 0, 25, 20, 10, 5, 2, 1, 0 };

  int EvalCoefficients::pawnShieldA_[2] = {14, 10};
  int EvalCoefficients::pawnShieldB_[2] = {14, 10};
  int EvalCoefficients::pawnShieldC_[2] = { 6, 4};

  // rook on open column
  int EvalCoefficients::openRook_[4] = { 0, 10, 25, 25};

  // material diff
  int EvalCoefficients::bishopBonus_[4] = { 0, 10, 50, 50 };

  int EvalCoefficients::figureAgainstPawnBonus_[2] = { 25, 60 };
  int EvalCoefficients::rookAgainstFigureBonus_[2] = { 25, 60 };
  int EvalCoefficients::figuresAgainstRookBonus_[2] = { 10, 40 };
  int EvalCoefficients::rookAgainstPawnBonus_[2] = { 15, 40 };

  // arrays
  int EvalCoefficients::passerPawn_[8] = { 0, 10, 20, 30, 40, 60, 90, 0 };
  int EvalCoefficients::multipasserPawn_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::semipasserPawn_[8] = { 0, 3, 6, 10, 14, 18, 0, 0 };
  int EvalCoefficients::passerCandidatePawn_[8] = { 0, 2, 5, 10, 14, 18, 0, 0 };
  int EvalCoefficients::protectedPasser_[8] = { 0, 9, 12, 15, 17, 20, 26, 0 };
  int EvalCoefficients::farKingPawn_[8] = { 0, 15, 17, 20, 25, 35, 50, 0 };
  int EvalCoefficients::cangoPawn_[8] = { 0, 6, 7, 8, 10, 12, 16, 0 };
  int EvalCoefficients::canpromotePawn_[8] = { 0, 3, 6, 8, 10, 12, 15, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 5, 6, 7, 8, 9 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  int  EvalCoefficients::knightMobility_[16] = { -40, -10, 0, 3, 5, 7, 9, 11 };
  int  EvalCoefficients::bishopMobility_[16] = { -40, -10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 };
  int  EvalCoefficients::rookMobility_[16] = {-35, -8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
  int  EvalCoefficients::queenMobility_[32] = {
    -45, -35, -7,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
    13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28};

  // king position eval for BN-mat
  int EvalCoefficients::bishopKnightMat_[64] =
  {
    20, 16, 8, -1, -2, -8, -16, -20,
      16, 16, 8, -1, -3, -8, -16, -16,
      8, 8, 6, -2, -4, -6, -8, -8,
      -1, -1, -2, -8, -8, -4, -3, -2,
      -2, -3, -4, -8, -8, -2, -1, -1,
      -8, -8, -6, -4, -2, 6, 8, 8,
      -16, -16, -8, -3, -1, 8, 16, 16,
      -20, -16, -8, -2, -1, 8, 16, 20
  };

  int EvalCoefficients::kingDistanceBonus_[8][8] =
  {
    {},
    {},
    { 10, 8, 7, 5, 3, 1, 0, 0 },
    { 10, 8, 7, 5, 3, 1, 0, 0 },
    { 10, 10, 8, 6, 4, 2, 1, 0 },
    { 15, 15, 15, 8, 6, 3, 1, 0 },
    {},
    {}
  };

  int EvalCoefficients::positionEvaluations_[2][8][64] =
  {
    // begin
    {
      // empty
      {},

        // pawn
      {
        0, 0, 0, 0, 0, 0, 0, 0,
        5, 5, 5, 5, 5, 5, 5, 5,
        0, 0, 3, 8, 8, 3, 0, 0,
        0, 0, 2, 7, 7, 2, 0, 0,
        0, 0, 1, 8, 8, 1, 0, 0,
        3, 0, 0, 0, 0, 0, 0, 3,
        3, 4, 4, -10, -10, 4, 4, 3,
        0, 0, 0, 0, 0, 0, 0, 0
      },

      // knight
      {
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, 0, 0, 0, 0, -8, -8,
        -5, 0, 3, 4, 4, 3, 0, -5,
        0, 5, 5, 5, 5, 5, 5, 0,
        -7, 0, 4, 5, 5, 4, 0, -7,
        -8, 2, 4, 4, 4, 4, 2, -8,
        -8, -8, 0, 2, 2, 0, -8, -8,
        -8, -12, -5, -5, -5, -5, -12, -8
      },

      // bishop
      {
        -8, -4, -4, -4, -4, -4, -4, -8,
        -2, 0, 0, 0, 0, 0, 0, -2,
        2, 0, 2, 6, 6, 2, 0, 2,
        -2, 2, 2, 6, 6, 2, 2, -2,
        -2, 0, 6, 6, 6, 6, 0, -2,
        0, 6, 6, 6, 6, 6, 6, 0,
        -2, 4, 0, 0, 0, 0, 4, -2,
        -5, -4, -12, -4, -4, -12, -4, -5
      },

      // rook
      {
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        -4, 0, 0, 0, 0, 0, 0, -4,
        -8, 0, 0, 0, 0, 0, 0, -8,
        -5, -10, 0, 5, 5, 0, -10, -5
      },

      // queen
      {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        -2, 0, 2, 2, 2, 2, 0, -2,
        -3, 0, 2, 3, 3, 2, 0, -3,
        -2, 0, 2, 3, 3, 2, 0, -2,
        -3, 0, 2, 2, 2, 2, 0, -3,
        -4, 0, 0, 1, 1, 0, 0, -4,
        -5, -5, -5, -5, -5, -5, -5, -5
      },


      // king
      {
        -30, -30, -30, -30, -30, -30, -30, -30,
        -26, -26, -26, -26, -26, -26, -26, -26,
        -24, -24, -24, -24, -24, -24, -24, -24,
        -22, -22, -22, -22, -22, -22, -22, -22,
        -16, -18, -20, -22, -22, -20, -18, -16,
        -10, -14, -14, -14, -14, -14, -14, -10,
        0, 0, -10, -10, -10, -6, 0, 0,
        4, 12, 0, -8, -8, 0, 16, 5
      },

      {}
    },

    // end
    {
      // empty
      {},

      // pawn
      {
        0, 0, 0, 0, 0, 0, 0, 0,
        10, 10, 10, 10, 10, 10, 10, 10,
        8, 8, 8, 8, 8, 8, 8, 8,
        6, 6, 6, 6, 6, 6, 6, 6,
        4, 4, 4, 4, 4, 4, 4, 4,
        2, 2, 2, 2, 2, 2, 2, 2,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
      },

      // knight
      {},

      // bishop
      {},

      // rook
      {},

      // queen
      {},

      // king
      {
        -12, -10, -8, -8, -8, -8, -10, -12,
        -10, -4, 0, 0, 0, 0, -4, -10,
        -8, 0, 6, 6, 6, 6, 0, -8,
        -6, 0, 6, 8, 8, 6, 0, -6,
        -6, 0, 6, 8, 8, 6, 0, -6,
        -8, 0, 6, 6, 6, 6, 0, -8,
        -10, -4, 0, 0, 0, 0, -4, -10,
        -12, -10, -8, -8, -8, -8, -10, -12
      },

      {}
    }
  };

}
