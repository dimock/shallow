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
  int EvalCoefficients::additionalMatBonus_{ 150 };
  int EvalCoefficients::queenRookVsPwPenalty_{ 80 };
  // pawns
  int EvalCoefficients::pawnEndgameBonus_{ 20 };
  int EvalCoefficients::rookBehindBonus_{ 5 };
  int EvalCoefficients::protectedPawnPressure_{ 1 };
  int EvalCoefficients::unprotectedPawnPressure_{ 6 };
  int EvalCoefficients::semiprotectedPawnPressure_{ 2 };
  int EvalCoefficients::unprotectedPawnBishopTreat_{ 3 };
  int EvalCoefficients::kingToPasserDistanceBonus_{ 3 };

  // forks
  int EvalCoefficients::bishopsAttackBonus_{ 65 };
  int EvalCoefficients::knightForkBonus_{ 65 };
  int EvalCoefficients::doublePawnAttack_{ 65 };
  int EvalCoefficients::generalAttackBonus_{ 10 };
  int EvalCoefficients::queenUnderRookAttackBonus_{ 30 };
  int EvalCoefficients::immobileAttackBonus_{ 30 };
  int EvalCoefficients::discoveredCheckBonus_{ 20 };
  int EvalCoefficients::possibleCheckBonus_{ 5 };
  int EvalCoefficients::matTreatBonus_{ 150 };

  // king
  int EvalCoefficients::fakeCastle_{ -15 };
  int EvalCoefficients::kingIsUnsafe_{ -25 };
  int EvalCoefficients::pawnPenaltyA_{ -13 };
  int EvalCoefficients::pawnPenaltyB_{ -15 };
  int EvalCoefficients::pawnPenaltyC_{ -3 };

  // blocked figure
  int EvalCoefficients::knightBlocked_{ 80 };
  int EvalCoefficients::bishopBlocked_{ 80 };
  int EvalCoefficients::rookBlocked_{ 80 };

  // king attacks
  int EvalCoefficients::pawnKingAttack_{ 8 };
  int EvalCoefficients::knightKingAttack_{ 8 };
  int EvalCoefficients::bishopKingAttack_{ 8 };
  int EvalCoefficients::rookKingAttack_{ 8 };
  int EvalCoefficients::queenKingAttack_{ 10 };
  int EvalCoefficients::basicAttack_{ 2 };

  int EvalCoefficients::generalKingPressure_{ 5 };
  int EvalCoefficients::generalOpponentPressure_{ 2 };

  // king threat
  int EvalCoefficients::knightChecking_{ 45 };
  int EvalCoefficients::bishopChecking_{ 45 };
  int EvalCoefficients::rookChecking_{ 65 };
  int EvalCoefficients::queenChecking_{ 85 };
  int EvalCoefficients::checkedFieldBonus_{ 12 };
  int EvalCoefficients::attackedNearKing_{20};

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 2 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 2 };

  // arrays
  int EvalCoefficients::doubledPawn_[2] = { -12, -10 };
  int EvalCoefficients::isolatedPawn_[2] = { -15, -13 };
  int EvalCoefficients::backwardPawn_[2] = { -10, -9 };
  int EvalCoefficients::unguardedPawn_[2] = { -6, -3 };
  int EvalCoefficients::unprotectedPawn_[2] = { -5, -3 };
  int EvalCoefficients::hasneighborPawn_[2] = { 3, 2 };
  
  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 1, 0, 0, 0 };

  int EvalCoefficients::pawnShieldA_[2] = {15, 12};
  int EvalCoefficients::pawnShieldB_[2] = {15, 10};
  int EvalCoefficients::pawnShieldC_[2] = { 6, 4};

  // rook on open column
  int EvalCoefficients::openRook_[2][2] = { {20, 8}, {8, 3} };

  // material diff
  // endgame, opening
  int EvalCoefficients::doubleBishopBonus_[2][10] = { {10, 15, 25, 30, 30, 30, 30, 30, 30, 30}, {5, 10, 15, 15, 15, 15, 15, 15, 15, 15} };
  int EvalCoefficients::twoKnightsBonus_[2][10] = { {0, 5, 10, 15, 15, 15, 15, 15, 15, 15}, {0, 2, 5, 5, 5, 5, 5, 5, 5, 5} };
  int EvalCoefficients::oneBishopBonus_[2][10] = { {2, 5, 10, 10, 10, 10, 10, 10, 10, 10}, {0, 2, 5, 5, 5, 5, 5, 5, 5, 5} };
  int EvalCoefficients::twoBishopsBonus_[2][10] = { {15, 40, 60, 60, 60, 60, 60, 60, 60, 60}, {10, 20, 40, 40, 40, 40, 40, 40, 40, 40} };
  int EvalCoefficients::twoRooksBonus_[2][10] = { {0, 5, 10, 10, 10, 10, 10, 10, 10, 10}, {0, 5, 10, 10, 10, 10, 10, 10, 10, 10} };
  int EvalCoefficients::figureAgainstPawnBonus_[2][10] = { {0, 15, 45, 45, 45, 45, 45, 45, 45, 45}, {0, 5, 20, 20, 20, 20, 20, 20, 20, 20} };
  int EvalCoefficients::rookAgainstFigureBonus_[2][10] = { {0, 5, 10, 10, 10, 10, 10, 10, 10, 10}, {0, 5, 10, 10, 10, 10, 10, 10, 10, 10} };
  int EvalCoefficients::figuresAgainstRookBonus_[2][10] = { {0, 20, 30, 30, 30, 30, 30, 30, 30, 30}, {0, 15, 20, 20, 20, 20, 20, 20, 20, 20} };
  int EvalCoefficients::rookAgainstPawnBonus_[2][10] = { {10, 30, 40, 40, 40, 40, 40, 40, 40, 40}, {10, 30, 40, 40, 40, 40, 40, 40, 40, 40} };

  // arrays
  int EvalCoefficients::passerPawn_[8] = { 0, 5, 9, 20, 38, 65, 98, 0 };
  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::farKingPawn_[8] = { 0, 3, 4, 8, 14, 23, 35, 0 };
  int EvalCoefficients::canpromotePawn_[8] = { 0, 5, 9, 23, 45, 75, 115, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  int EvalCoefficients::knightMobility_[16] = { -30, -15,  3,  5,  7,  8, 10, 12, 14 };
  int EvalCoefficients::bishopMobility_[16] = { -30, -15,  3,  5,  7,  9, 12, 13, 14, 16, 20, 22, 23, 24, 25, 26 };
  int EvalCoefficients::rookMobility_[16]   = { -30, -15,  5,  8,  12,  16, 21, 25, 27, 29, 30, 31, 32, 33, 34, 35 };
  int EvalCoefficients::queenMobility_[32]  = { -60, -30,  6,  10, 13,  18, 23, 27, 30, 33, 36, 40, 43, 45, 47, 48,
                                                 49,  50, 51,  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64 };

  // king position eval for BN-mat
  int EvalCoefficients::bishopKnightMat_[64] =
  {
     30,  20,  10,  -1,  -5, -10, -20, -30,
     20,  16,   8,  -1,  -6,  -8, -16, -20,
     10,   8,   6,  -2,  -8,  -6,  -8, -10,
     -1,  -1,  -2, -15, -15,  -8,  -6,  -5,
     -5,  -6,  -8, -15, -15,  -2,  -1,  -1,
    -10,  -8,  -6,  -8,  -2,   6,   8,  10,
    -20, -16,  -8,  -6,  -1,   8,  16,  20,
    -30, -20, -10,  -5,  -1,  10,  20,  30
  };

  int EvalCoefficients::kingDistanceBonus_[8][8] =
  {
    {},
    {},
    { 0, 10, 7, 5, 3, 1, 0, -5 },
    { 0, 10, 7, 5, 3, 1, 0, -5 },
    { 0, 10, 7, 5, 3, 1, 0, -5 },
    { 0, 10, 7, 5, 3, 1, 0, -5 },
    {},
    {}
  };

  int EvalCoefficients::positionEvaluations_[2][8][64] =
  {
    {
      {
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
          0,   0,   0,   0,   0,   0,   0,   0,
          5,   5,   5,   5,   5,   5,   5,   5,
          3,   3,   5,  10,  10,   5,   3,   3,
          2,   2,   4,  12,  12,   4,   2,   2,
          0,   0,   3,  12,  12,   3,   0,   0,
         -1,  -1,   0,  10,  10,   0,  -1,  -1,
         -2,  -2,  -2,  -8,  -8,  -2,  -2,  -2,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
        -15, -12, -10, -10, -10, -10, -12, -15,
        -12,  -2,   0,   0,   0,   0,  -2, -12,
        -10,   0,   5,   7,   7,   5,   0, -10,
         -8,   0,   8,  12,  12,   8,   0,  -8,
         -8,   0,   8,  12,  12,   8,   0,  -8,
        -10,  -2,   5,   7,   7,   5,  -2, -10,
        -12,  -8,  -3,  -1,  -1,  -3,  -8, -12,
        -15, -12,  -8,  -8,  -8,  -8, -12, -15
      },
      {
        -15,  -8,  -8,  -8,  -8,  -8,  -8, -15,
         -9,   5,   3,   0,   0,   3,   5,  -9,
         -1,   7,   5,   5,   5,   5,   7,  -1,
         -1,   6,   7,  10,  10,   7,   6,  -1,
         -1,   6,   8,  10,  10,   8,   6,  -1,
         -1,   9,  10,   8,   8,  10,   9,  -1,
         -5,   2,   9,   6,   6,   9,   2,  -5,
        -15,  -6,  -8,  -6,  -6,  -8,  -6, -15
      },
      {
         -5,   2,   5,   7,   7,   5,   2,  -5,
          0,   0,   8,  10,  10,   8,   0,   0,
          2,   2,   3,   4,   4,   3,   2,   2,
          2,   2,   3,   4,   4,   3,   2,   2,
          2,   2,   3,   4,   4,   3,   2,   2,
          2,   2,   3,   4,   4,   3,   2,   2,
         -5,  -5,   3,   4,   4,   3,  -5,  -5,
         -5,  -5,   3,   4,   4,   3,  -5,  -5
      },
      {
         -5,  -2,   1,   3,   3,   1,  -2,  -5,
         -2,   2,   2,   2,   2,   2,   2,  -2,
         -2,   2,   3,   3,   3,   3,   2,  -2,
         -2,   2,   5,   7,   7,   5,   2,  -2,
         -2,   2,   5,   7,   7,   5,   2,  -2,
         -2,   2,   3,   4,   4,   3,   2,  -2,
         -2,   1,   1,   2,   2,   1,   1,  -2,
         -5,  -2,  -2,  -2,  -2,  -2,  -2,  -5
      },
      {
        -30, -30, -30, -30, -30, -30, -30, -30,
        -26, -26, -26, -26, -26, -26, -26, -26,
        -24, -24, -24, -24, -24, -24, -24, -24,
        -22, -22, -22, -22, -22, -22, -22, -20,
        -15, -15, -20, -22, -22, -18, -15, -15,
          1,  -5, -10, -18, -18,  -8,  -3,   2,
         10,  12,   8, -12, -12,  10,  15,  12,
         13,  17,  12, -10, -10,  15,  20,  16
      },
      {
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0
      }
    },
    {
      {
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
          0,   0,   0,   0,   0,   0,   0,   0,
          4,   4,   5,   5,   5,   5,   4,   4,
          3,   4,   5,   5,   5,   5,   4,   3,
          2,   3,   4,   4,   4,   4,   3,   2,
          1,   2,   3,   4,   4,   3,   2,   1,
          0,   1,   2,   3,   3,   2,   1,   0,
          0,   0,   0,  -8,  -8,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
        -12, -10,  -8,  -8,  -8,  -8, -10, -12,
        -10,   0,   5,   5,   5,   5,   0, -10,
         -8,   5,   8,   8,   8,   8,   5,  -8,
         -8,   5,   8,  10,  10,   8,   5,  -8,
         -8,   5,   8,  10,  10,   8,   5,  -8,
         -8,   5,   8,   8,   8,   8,   5,  -8,
        -10,   0,   5,   5,   5,   5,   0, -10,
        -12, -10,  -8,  -8,  -8,  -8, -10, -12
      },
      {
         -8,  -3,  -2,  -1,  -1,  -2,  -3,  -8,
         -3,   4,   3,   3,   3,   3,   4,  -3,
         -2,   3,   5,   5,   5,   5,   3,  -2,
         -1,   3,   5,   6,   6,   5,   3,  -1,
         -1,   3,   5,   6,   6,   5,   3,  -1,
         -2,   3,   5,   5,   5,   5,   3,  -2,
         -3,   4,   3,   3,   3,   3,   4,  -3,
         -8,  -3,  -2,  -1,  -1,  -2,  -3,  -8
      },
      {
         -3,   2,   4,   4,   4,   4,   2,  -3,
          1,   2,   5,   5,   5,   5,   2,   1,
          2,   2,   5,   6,   6,   5,   2,   2,
          2,   3,   6,   6,   6,   6,   3,   2,
          2,   3,   6,   6,   6,   6,   3,   2,
          2,   3,   5,   6,   6,   5,   3,   2,
          1,   2,   5,   5,   5,   5,   2,   1,
         -3,   2,   4,   4,   4,   4,   2,  -3
      },
      {
         -8,  -3,  -2,  -2,  -2,  -2,  -3,  -8,
         -5,  -2,   1,   3,   3,   1,  -2,  -5,
         -5,   1,   8,   8,   8,   8,   1,  -5,
         -2,   3,   8,  10,  10,   8,   3,  -2,
         -2,   3,   8,  10,  10,   8,   3,  -2,
         -5,   1,   8,   8,   8,   8,   1,  -5,
         -5,  -2,   1,   3,   3,   1,  -2,  -5,
         -8,  -3,  -2,  -2,  -2,  -2,  -3,  -8
      },
      {
        -25, -15, -10, -10, -10, -10, -15, -25,
        -15,   0,   0,   0,   0,   0,   0, -15,
        -10,   0,  12,  12,  12,  12,   0, -10,
        -10,   0,  12,  20,  20,  12,   0, -10,
        -10,   0,  12,  20,  20,  12,   0, -10,
        -10,   0,  12,  12,  12,  12,   0, -10,
        -15,   0,   0,   0,   0,   0,   0, -15,
        -25, -20, -10, -10, -10, -10, -15, -25
      },
      {
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0
      }
    }
  };

}
