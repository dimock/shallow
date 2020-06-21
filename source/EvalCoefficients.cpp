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
  // pawns
  int EvalCoefficients::pawnEndgameBonus_{ 15 };
  int EvalCoefficients::rookBehindBonus_{ 5 };
  int EvalCoefficients::protectedPawnPressure_{ 1 };
  int EvalCoefficients::unprotectedPawnPressure_{ 4 };
  int EvalCoefficients::protectedPawnBishopTreat_{ 1 };
  int EvalCoefficients::unprotectedPawnBishopTreat_{ 4 };
  int EvalCoefficients::kingToPasserDistanceBonus_{ 2 };

  // forks
  int EvalCoefficients::bishopsAttackBonus_{ 25 };
  int EvalCoefficients::forkBonus_{ 50 };
  int EvalCoefficients::doublePawnAttack_{ 60 };

#ifdef EVALUATE_DANGEROUS_ATTACKS
  int EvalCoefficients::dangerousAttacksOnPawn_{6};
  int EvalCoefficients::dangerousAttacksOnKnight_{11};
  int EvalCoefficients::dangerousAttacksOnBishop_{10};
  int EvalCoefficients::dangerousAttacksOnRook_{15};
  int EvalCoefficients::dangerousAttacksOnQueen_{23};
#endif // EVALUATE_DANGEROUS_ATTACKS

  // king
  int EvalCoefficients::fakeCastle_{ -40 };
  int EvalCoefficients::kingIsUnsafe_{ -25 };
  int EvalCoefficients::pawnPenaltyA_{ -14 };
  int EvalCoefficients::pawnPenaltyB_{ -16 };
  int EvalCoefficients::pawnPenaltyC_{ -5 };

  // blocked figure
  int EvalCoefficients::knightBlocked_{ 80 };
  int EvalCoefficients::bishopBlocked_{ 80 };
  int EvalCoefficients::rookBlocked_{ 80 };

  // king attacks
  int EvalCoefficients::pawnKingAttack_{ 3 };
  int EvalCoefficients::knightKingAttack_{ 10 };
  int EvalCoefficients::bishopKingAttack_{ 10 };
  int EvalCoefficients::rookKingAttack_{ 10 };
  int EvalCoefficients::queenKingAttack_{ 10 };

  int EvalCoefficients::unprotectedKingMultiattacksCoeff_{15};
  int EvalCoefficients::unprotectedKingAttacksCoeff_{10};
  int EvalCoefficients::allKingAttacksCoeff_{5};
  int EvalCoefficients::generalKingPressure_{ 2 };
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
  int EvalCoefficients::doubledPawn_[2] = { -12, -8 };
  int EvalCoefficients::isolatedPawn_[2] = { -15, -12 };
  int EvalCoefficients::backwardPawn_[2] = { -10, -8 };
  int EvalCoefficients::unguardedPawn_[2] = {-10, -8 };
  int EvalCoefficients::protectedPawn_[2] = { 10, 8};
  int EvalCoefficients::hasneighborPawn_[2] = { 7, 5};
  int EvalCoefficients::weakHalfopenPawn_[2] = { -6, 0 };
  
  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 1, 0, 0, 0 };

  int EvalCoefficients::pawnShieldA_[2] = {14, 12};
  int EvalCoefficients::pawnShieldB_[2] = {14, 12};
  int EvalCoefficients::pawnShieldC_[2] = { 6, 5};

  // rook on open column
  int EvalCoefficients::openRook_[4] = { 0, 10, 30, 30};

  // material diff
  // endgame, opening
  int EvalCoefficients::knightBonus_[2][2] = { {0, 15}, {-5, 5} };
  int EvalCoefficients::bishopBonus_[2][2] = { {12, 75}, {5, 50} };
  int EvalCoefficients::rookBonus_[2][2] = { {0, 10}, {0, 10} };
  int EvalCoefficients::figureAgainstPawnBonus_[2][2] = { {25, 50}, {10, 40} };
  int EvalCoefficients::rookAgainstFigureBonus_[2][2] = { {10, 25}, {-10, 15} };
  int EvalCoefficients::figuresAgainstRookBonus_[2][2] = { {20, 65}, {0, 55} };
  int EvalCoefficients::rookAgainstPawnBonus_[2][2] = { {15, 35}, {0, 30} };

  // arrays
  int EvalCoefficients::passerPawn_[8] = { 0, 7, 11, 23, 44, 73, 110, 0 };
  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::farKingPawn_[8] = { 0, 3, 5, 12, 24, 40, 60, 0 };
  int EvalCoefficients::canpromotePawn_[8] = { 0, 7, 11, 23, 44, 73, 110, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  int EvalCoefficients::knightMobility_[16] = { -35, -18, 0, 6, 10, 14, 18, 22, 26 };
  int EvalCoefficients::bishopMobility_[16] = { -35, -18, 0, 6, 10, 14, 18, 22, 26, 27, 29, 30, 30, 30, 30, 30 };
  int EvalCoefficients::rookMobility_[16]   = { -35, -18, 0, 6, 10, 14, 18, 22, 26, 27, 29, 30, 30, 30, 30, 30 };
  int EvalCoefficients::queenMobility_[32]  = { -60, -30, 0, 9, 13, 17, 20, 26, 29, 32, 36, 37, 38, 39, 40, 40, 40,
                                                 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40 };

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
    { 0, 12, 8, 6, 3, 1, 0, -5 },
    { 0, 12, 8, 6, 3, 1, 0, -5 },
    { 0, 12, 8, 6, 3, 1, 0, -5 },
    { 0, 12, 8, 6, 3, 1, 0, -5 },
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
          0,   1,   4,  13,  13,   4,   1,   0,
         -5,   0,   3,  13,  13,   3,   0,  -5,
         -3,   0,   2,   5,   5,   2,   0,  -3,
          4,   3,   2,  -8,  -8,   2,   3,   4,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
        -16, -13,  -7,  -7,  -7,  -7, -13, -16,
        -13,  -5,  -1,  -1,  -1,  -1,  -5, -13,
         -7,   5,   5,   5,   5,   5,   5,  -7,
         -2,   8,   9,  13,  13,   9,   8,  -2,
         -2,   0,   9,  13,  13,   9,   0,  -2,
         -7,   0,   6,   4,   4,   6,   0,  -7,
        -13,  -7,   2,   2,   2,   2,  -7, -13,
        -16, -13,  -7,  -2,  -2,  -7, -13, -16
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
         -7,   0,   5,   8,   8,   5,   0,  -7,
         -5,  -5,   5,   8,   8,   5,  -5,  -5
      },
      {
        -10,  -7,  -6,  -5,  -5,  -6,  -7, -10,
         -5,  -1,  -1,  -1,  -1,  -1,  -1,  -5,
         -2,  -1,   0,   0,   0,   0,  -1,  -2,
         -2,   2,   5,   4,   4,   5,   2,  -2,
         -5,   0,   5,   4,   4,   5,   0,  -5,
         -5,  -1,   5,   5,   5,   5,  -1,  -5,
         -7,  -1,   0,   2,   2,   0,  -1,  -7,
        -10,  -7,  -6,  -5,  -5,  -6,  -7, -10
      },
      {
        -48, -48, -48, -48, -48, -48, -48, -48,
        -41, -41, -41, -41, -41, -41, -41, -41,
        -38, -38, -38, -38, -38, -38, -38, -38,
        -35, -35, -35, -35, -35, -35, -35, -35,
        -25, -28, -32, -35, -35, -32, -28, -25,
        -11, -16, -24, -32, -32, -24, -16, -11,
          0,   0,  -8, -28, -28,  -8,   0,   0,
         24,  32,   9, -20, -20,  14,  40,  32
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
          0,   8,  11,  17,  17,  11,   8,   0,
         -2,   6,   8,  15,  15,   8,   6,  -2,
         -4,   4,   6,  13,  13,   6,   4,  -4,
         -6,   2,   4,  11,  11,   4,   2,  -6,
        -11,   0,   0,   0,   0,   0,   0, -11,
        -13,  -4,  -2, -11, -11,  -2,  -4, -13,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
        -16, -11, -11,  -8,  -8, -11, -11, -16,
         -8,  -5,   2,   2,   2,   2,  -5,  -8,
         -3,  -1,   3,   4,   4,   3,  -1,  -3,
         -2,   0,   4,   8,   8,   4,   0,  -2,
         -2,   0,   4,   8,   8,   4,   0,  -2,
         -3,   0,   3,   4,   4,   3,   0,  -3,
        -11,   0,   2,   2,   2,   2,   0,  -8,
        -16, -11, -11,  -8,  -8, -11, -11, -16
      },
      {
        -10,  -6,  -5,  -4,  -4,  -5,  -6, -10,
         -6,   0,   3,   3,   3,   3,   0,  -6,
         -5,   0,   4,   5,   5,   4,   0,  -5,
         -4,   0,   3,   6,   6,   3,   0,  -4,
         -4,   0,   3,   6,   6,   3,   0,  -4,
         -5,   0,   4,   5,   5,   4,   0,  -5,
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
        -35, -28, -23, -21, -21, -23, -28, -35,
        -28, -16, -11,  -7,  -7, -11, -16, -16,
        -23, -11,  14,  16,  16,  14, -11, -23,
        -21,  -7,  16,  28,  28,  16,  -7, -21,
        -21,  -7,  16,  28,  28,  14,  -7, -21,
        -23, -11,  14,  16,  16,  14, -11, -23,
        -28, -16, -11,  -7,  -7, -11, -16, -28,
        -35, -28, -23, -21, -21, -23, -28, -35
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
