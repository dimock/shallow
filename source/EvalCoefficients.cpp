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
  int EvalCoefficients::pawnEndgameBonus_{ 15 };
  int EvalCoefficients::rookBehindBonus_{ 5 };
  int EvalCoefficients::protectedPawnPressure_{ 1 };
  int EvalCoefficients::unprotectedPawnPressure_{ 4 };
  int EvalCoefficients::protectedPawnBishopTreat_{ 1 };
  int EvalCoefficients::unprotectedPawnBishopTreat_{ 4 };
  int EvalCoefficients::kingToPasserDistanceBonus_{ 2 };

  // forks
  int EvalCoefficients::bishopsAttackBonus_{ 30 };
  int EvalCoefficients::knightForkBonus_{ 50 };
  int EvalCoefficients::doublePawnAttack_{ 50 };
  int EvalCoefficients::queenUnderRookAttackBonus_{ 8 };
  int EvalCoefficients::multiAttackBonus_{ 15 };
  int EvalCoefficients::pinnedAttackBonus_{ 20 };

#ifdef EVALUATE_DANGEROUS_ATTACKS
  int EvalCoefficients::dangerousAttacksOnPawn_{6};
  int EvalCoefficients::dangerousAttacksOnKnight_{11};
  int EvalCoefficients::dangerousAttacksOnBishop_{10};
  int EvalCoefficients::dangerousAttacksOnRook_{15};
  int EvalCoefficients::dangerousAttacksOnQueen_{23};
#endif // EVALUATE_DANGEROUS_ATTACKS

  // king
  int EvalCoefficients::fakeCastle_{ -20 };
  int EvalCoefficients::kingIsUnsafe_{ -25 };
  int EvalCoefficients::pawnPenaltyA_{ -14 };
  int EvalCoefficients::pawnPenaltyB_{ -16 };
  int EvalCoefficients::pawnPenaltyC_{ -6 };

  // blocked figure
  int EvalCoefficients::knightBlocked_{ 80 };
  int EvalCoefficients::bishopBlocked_{ 80 };
  int EvalCoefficients::rookBlocked_{ 80 };

  // king attacks
  int EvalCoefficients::pawnKingAttack_{ 3 };
  int EvalCoefficients::knightKingAttack_{ 15 };
  int EvalCoefficients::bishopKingAttack_{ 15 };
  int EvalCoefficients::rookKingAttack_{ 15 };
  int EvalCoefficients::queenKingAttack_{ 15 };

  int EvalCoefficients::unprotectedKingMultiattacksCoeff_{15};
  int EvalCoefficients::unprotectedKingAttacksCoeff_{10};
  int EvalCoefficients::allKingAttacksCoeff_{5};
  int EvalCoefficients::generalKingPressure_{ 3 };
  int EvalCoefficients::generalOpponentPressure_{ 1 };

  // king threat
  int EvalCoefficients::knightChecking_{ 45 };
  int EvalCoefficients::bishopChecking_{ 40 };
  int EvalCoefficients::rookChecking_{ 55 };
  int EvalCoefficients::queenChecking_{ 65 };

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 2 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 2 };

  // arrays
  int EvalCoefficients::doubledPawn_[2] = { -10, -8 };
  int EvalCoefficients::isolatedPawn_[2] = { -13, -12 };
  int EvalCoefficients::backwardPawn_[2] = { -9, -7 };
  int EvalCoefficients::unguardedPawn_[2] = {-8, -7 };
  int EvalCoefficients::protectedPawn_[2] = { 5, 4};
  int EvalCoefficients::hasneighborPawn_[2] = { 3, 2};
  
  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 1, 0, 0, 0 };

  int EvalCoefficients::pawnShieldA_[2] = {14, 10};
  int EvalCoefficients::pawnShieldB_[2] = {14, 10};
  int EvalCoefficients::pawnShieldC_[2] = { 6, 4};

  // rook on open column
  int EvalCoefficients::openRook_[4] = { 0, 7, 30, 30};

  // material diff
  // endgame, opening
  int EvalCoefficients::doubleBishopBonus_[2][2] = { {25, 45}, {15, 35} };
  int EvalCoefficients::twoKnightsBonus_[2][2] = { {0, 15}, {0, 5} };
  int EvalCoefficients::oneBishopBonus_[2][2] = { {5, 10}, {5, 10} };
  int EvalCoefficients::twoBishopsBonus_[2][2] = { {20, 50}, {15, 40} };
  int EvalCoefficients::twoRooksBonus_[2][2] = { {5, 10}, {5, 10} };
  int EvalCoefficients::figureAgainstPawnBonus_[2][2] = { {25, 50}, {10, 40} };
  int EvalCoefficients::rookAgainstFigureBonus_[2][2] = { {15, 35}, {0, 20} };
  int EvalCoefficients::figuresAgainstRookBonus_[2][2] = { {20, 50}, {0, 30} };
  int EvalCoefficients::rookAgainstPawnBonus_[2][2] = { {15, 30}, {10, 30} };

  // arrays
  int EvalCoefficients::passerPawn_[8] = { 0, 5, 8, 18, 35, 60, 90, 0 };
  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::farKingPawn_[8] = { 0, 3, 5, 12, 24, 40, 60, 0 };
  int EvalCoefficients::canpromotePawn_[8] = { 0, 7, 11, 23, 44, 73, 110, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  int EvalCoefficients::knightMobility_[16] = { -35, -15, 0, 5, 10, 12, 16, 19, 21 };
  int EvalCoefficients::bishopMobility_[16] = { -35, -15, 0, 5, 10, 12, 16, 19, 21, 21, 21, 21, 21, 21, 21, 21 };
  int EvalCoefficients::rookMobility_[16]   = { -35, -15, 0, 5, 10, 12, 16, 19, 21, 21, 21, 21, 21, 21, 21, 21 };
  int EvalCoefficients::queenMobility_[32]  = { -60, -30, 0, 7, 13, 17, 20, 23, 27, 30, 31, 32, 32, 32, 32, 32, 32,
                                                 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32 };

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
          5,   6,   7,   8,   8,   7,   6,   5,
          0,   1,   4,  12,  12,   4,   1,   0,
         -5,   0,   3,  12,  12,   3,   0,  -5,
         -3,   0,   2,   5,   5,   2,   0,  -3,
          4,   3,   2,  -8,  -8,   2,   3,   4,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
        -12, -10,  -8,  -8,  -8,  -8, -10, -12,
        -10,  -5,  -1,  -1,  -1,  -1,  -5, -10,
         -8,   5,   5,   5,   5,   5,   5,  -8,
         -8,   8,   9,  12,  12,   9,   8,  -8,
         -8,   0,   9,  12,  12,   9,   0,  -8,
         -8,   0,   6,   4,   4,   6,   0,  -8,
        -10,  -3,   2,   2,   2,   2,  -3, -10,
        -12, -10,  -8,  -8,  -8,  -8, -10, -12
      },
      {
        -15, -10,  -8,  -8,  -8,  -8, -10, -15,
         -9,  -1,  -1,  -1,  -1,  -1,  -1,  -9,
         -7,  -1,  -1,  -1,  -1,  -1,  -1,  -7,
         -6,   2,   7,   5,   5,   7,   2,  -6,
         -6,   6,   9,   9,   9,   9,   6,  -6,
         -7,   7,   9,   6,   6,   9,   7,  -7,
         -9,   9,   6,   3,   3,   6,   8,  -9,
        -15, -10, -10,  -6,  -6, -10, -10, -15
      },
      {
          7,   7,  10,  15,  15,  10,   7,   7,
          7,   7,  10,  15,  15,  10,   7,   7,
         -3,   0,   5,   8,   8,   5,   0,  -3,
         -3,   0,   5,   8,   8,   5,   0,  -3,
         -3,   0,   5,   8,   8,   5,   0,  -3,
         -3,   0,   5,   8,   8,   5,   0,  -3,
         -3,   0,   5,   8,   8,   5,   0,  -3,
         -5,  -5,   5,   8,   8,   5,  -5,  -5
      },
      {
        -10,  -7,  -6,  -5,  -5,  -6,  -7, -10,
         -5,  -1,  -1,  -1,  -1,  -1,  -1,  -5,
         -2,  -1,   0,   0,   0,   0,  -1,  -2,
         -2,   2,   5,   4,   4,   5,   2,  -2,
         -5,   0,   5,   4,   4,   5,   0,  -5,
         -5,  -1,   5,   5,   5,   5,  -1,  -5,
         -7,  -1,   2,   2,   2,   2,  -1,  -7,
        -10,  -7,  -6,  -5,  -5,  -6,  -7, -10
      },
      {
        -39, -39, -39, -39, -39, -39, -39, -39,
        -33, -33, -33, -33, -33, -33, -33, -33,
        -31, -31, -31, -31, -31, -31, -31, -31,
        -28, -28, -28, -28, -28, -28, -28, -26,
        -20, -23, -26, -28, -28, -23, -20, -18,
         -9, -13, -19, -26, -26, -15, -10,  -9,
          7,   7,  -5, -23, -23,  -2,  10,  10,
         16,  22,  11, -16, -16,  15,  26,  20
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
          0,   8,  10,  16,  16,  10,   8,   0,
         -2,   6,   8,  14,  14,   8,   6,  -2,
         -4,   4,   6,  12,  12,   6,   4,  -4,
         -6,   2,   4,  10,  10,   4,   2,  -6,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -12,  -4,  -2, -10, -10,  -2,  -4, -12,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
        -12, -10,  -8,  -8,  -8,  -8, -10, -12,
        -10,  -3,  -1,   0,   0,  -1,  -3, -10,
         -8,  -1,   5,   5,   5,   5,  -1,  -8,
         -8,   0,   5,   8,   8,   5,   0,  -8,
         -8,   0,   5,   8,   8,   5,   0,  -8,
         -8,  -1,   5,   5,   5,   5,  -1,  -8,
        -10,  -3,  -1,   0,   0,  -1,  -3, -10,
        -12, -10,  -8,  -8,  -8,  -8, -10, -12
      },
      {
        -10,  -6,  -5,  -4,  -4,  -5,  -6, -10,
         -6,   0,   3,   3,   3,   3,   0,  -6,
         -5,   3,   4,   5,   5,   4,   3,  -5,
         -4,   3,   5,   6,   6,   5,   3,  -4,
         -4,   3,   5,   6,   6,   5,   3,  -4,
         -5,   3,   4,   5,   5,   4,   3,  -5,
         -6,   0,   3,   3,   3,   3,   0,  -6,
        -10,  -6,  -5,  -4,  -4,  -5,  -6, -10
      },
      {
         -5,  -3,   2,   2,   2,   2,  -3,  -5,
         -5,  -3,   3,   3,   3,   3,  -3,  -5,
         -3,   0,   3,   5,   5,   3,   0,  -3,
         -3,   0,   5,   5,   5,   5,   0,  -3,
         -3,   0,   5,   5,   5,   5,   0,  -3,
         -3,   0,   3,   5,   5,   3,   0,  -3,
         -5,   0,   3,   3,   3,   3,   0,  -5,
         -5,  -3,   2,   2,   2,   2,  -3,  -5
      },
      {
         -5,  -3,   2,   2,   2,   2,  -3,  -5,
         -5,  -3,   3,   3,   3,   3,  -3,  -5,
         -3,   0,   3,   5,   5,   3,   0,  -3,
         -3,   0,   5,   5,   5,   5,   0,  -3,
         -3,   0,   5,   5,   5,   5,   0,  -3,
         -3,   0,   3,   5,   5,   3,   0,  -3,
         -5,   0,   3,   3,   3,   3,   0,  -5,
         -5,  -3,   2,   2,   2,   2,  -3,  -5
      },
      {
        -25, -20, -17, -15, -15, -17, -20, -25,
        -20, -12,  -8,  -5,  -5,  -8, -12, -12,
        -17,  -8,  10,  12,  12,  10,  -8, -17,
        -15,  -5,  12,  20,  20,  12,  -5, -15,
        -15,  -5,  12,  20,  20,  10,  -5, -15,
        -17,  -8,  10,  12,  12,  10,  -8, -17,
        -20, -12,  -8,  -5,  -5,  -8, -12, -20,
        -25, -20, -17, -15, -15, -17, -20, -25
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
