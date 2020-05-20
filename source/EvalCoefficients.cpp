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
  int EvalCoefficients::rookBehindBonus_{ 5 };
  int EvalCoefficients::protectedPawnPressure_{ 1 };
  int EvalCoefficients::unprotectedPawnPressure_{ 4 };
  int EvalCoefficients::protectedPawnBishopTreat_{ 1 };
  int EvalCoefficients::unprotectedPawnBishopTreat_{ 4 };

  // forks
  int EvalCoefficients::forkBonus_{ 60 };
  int EvalCoefficients::doublePawnAttack_{ 50 };

#ifdef EVALUATE_DANGEROUS_ATTACKS
  int EvalCoefficients::dangerousAttacksOnPawn_{6};
  int EvalCoefficients::dangerousAttacksOnKnight_{11};
  int EvalCoefficients::dangerousAttacksOnBishop_{10};
  int EvalCoefficients::dangerousAttacksOnRook_{15};
  int EvalCoefficients::dangerousAttacksOnQueen_{23};
#endif // EVALUATE_DANGEROUS_ATTACKS

  // king
  int EvalCoefficients::castleImpossible_{ 15 };
  int EvalCoefficients::fakeCastle_{ -30 };
  int EvalCoefficients::castleBonus_{ 10 };
  int EvalCoefficients::kingIsUnsafe_{ -20 };
  int EvalCoefficients::pawnPenaltyA_{ -10 };
  int EvalCoefficients::pawnPenaltyB_{ -12 };
  int EvalCoefficients::pawnPenaltyC_{ -4 };

  // blocked figure
  int EvalCoefficients::knightBlocked_{ 70 };
  int EvalCoefficients::bishopBlocked_{ 70 };
  int EvalCoefficients::rookBlocked_{ 70 };

  // king attacks
  int EvalCoefficients::pawnKingAttack_{ 2 };
  int EvalCoefficients::knightKingAttack_{ 15 };
  int EvalCoefficients::bishopKingAttack_{ 15 };
  int EvalCoefficients::rookKingAttack_{ 15 };
  int EvalCoefficients::queenKingAttack_{ 15 };

  int EvalCoefficients::unprotectedKingMultiattacksCoeff_{15};
  int EvalCoefficients::unprotectedKingAttacksCoeff_{10};
  int EvalCoefficients::allKingAttacksCoeff_{5};
  int EvalCoefficients::generalKingPressure_{ 2 };
  int EvalCoefficients::generalOpponentPressure_{ 1 };

  // king threat
  int EvalCoefficients::knightChecking_{ 45 };
  int EvalCoefficients::bishopChecking_{ 35 };
  int EvalCoefficients::rookChecking_{ 55 };
  int EvalCoefficients::queenChecking_{ 65 };

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 2 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 2 };

  // arrays
  int EvalCoefficients::doubledPawn_[2] = { -10, -8 };
  int EvalCoefficients::isolatedPawn_[2] = { -14, -10 };
  int EvalCoefficients::backwardPawn_[2] = { -12, -8 };
  int EvalCoefficients::unsupportedPawn_[2] = { -8, 8 };
  int EvalCoefficients::weakHalfopenPawn_[2] = { -6, 0 };
  
  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 1, 0, 0, 0 };

  int EvalCoefficients::pawnShieldA_[2] = {10, 8};
  int EvalCoefficients::pawnShieldB_[2] = {12, 10};
  int EvalCoefficients::pawnShieldC_[2] = { 6, 5};

  // rook on open column
  int EvalCoefficients::openRook_[4] = { 0, 10, 20, 20};

  // material diff
  // endgame, opening
  int EvalCoefficients::knightBonus_[2][2] = { {0, 15}, {-20, 0} };
  int EvalCoefficients::bishopBonus_[2][2] = { {12, 75}, {5, 25} };
  int EvalCoefficients::rookBonus_[2][2] = { {0, 0}, {0, 0} };
  int EvalCoefficients::figureAgainstPawnBonus_[2][2] = { {25, 45}, {10, 25} };
  int EvalCoefficients::rookAgainstFigureBonus_[2][2] = { {15, 30}, {-25, 15} };
  int EvalCoefficients::figuresAgainstRookBonus_[2][2] = { {20, 65}, {0, 55} };
  int EvalCoefficients::rookAgainstPawnBonus_[2][2] = { {10, 35}, {0, 10} };

  // arrays
  int EvalCoefficients::passerPawn_[8] = { 0, 7, 11, 21, 39, 63, 95, 0 };
  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::farKingPawn_[8] = { 0, 3, 4, 8, 15, 23, 35, 0 };
  int EvalCoefficients::canpromotePawn_[8] = { 0, 7, 11, 21, 39, 63, 95, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  int EvalCoefficients::knightMobility_[16] = { -30, -15, 0, 3, 6, 9, 12, 15, 18 };
  int EvalCoefficients::bishopMobility_[16] = { -30, -15, 0, 3, 6, 9, 12, 15, 18, 18, 18, 18, 18, 18, 18, 18 };
  int EvalCoefficients::rookMobility_[16]   = { -25, -10, 0, 2, 5, 7, 9, 11, 13, 15, 17, 19, 19, 21, 21, 21 };
  int EvalCoefficients::queenMobility_[32]  = { -50, -25, 0, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 36, 36,
                                                 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36, 36 };

  // king position eval for BN-mat
  int EvalCoefficients::bishopKnightMat_[64] =
  {
     20,  16,  8, -1, -2, -8, -16, -20,
     16,  16,  8, -1, -3, -8, -16, -16,
      8,   8,  6, -2, -4, -6, -8,   -8,
     -1,  -1, -2, -8, -8, -4, -3,   -2,
     -2,  -3, -4, -8, -8, -2, -1,   -1,
     -8,  -8, -6, -4, -2,  6,  8,    8,
    -16, -16, -8, -3, -1,  8,  16,  16,
    -20, -16, -8, -2, -1,  8,  16,  20
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
          1,   1,   1,   3,   3,   1,   1,   1,
          0,   0,   2,  11,  11,   2,   0,   0,
          0,   0,   3,  11,  11,   3,   0,   0,
          0,   0,   2,   5,   5,   2,   0,   0,
          4,   4,   4,  -8,  -8,   4,   4,   4,
          0,   0,   0,   0,   0,   0,   0,   0
      },
      {
        -16, -13,  -7,  -7,  -7,  -7, -13, -16,
        -13,  -5,  -1,  -1,  -1,  -1,  -5, -13,
         -7,  -1,  -1,  -1,  -1,  -1,  -1,  -7,
         -2,   6,   7,  11,  11,   7,   6,  -2,
         -2,   0,   7,  11,  11,   7,   0,  -2,
         -7,  -1,   5,   3,   3,   5,  -1,  -7,
        -13,  -7,   2,   2,   2,   2,  -7, -13,
        -16, -13,  -7,  -2,  -2,  -7, -13, -16
      },
      {
        -15, -10,  -8,  -8,  -8,  -8, -10, -15,
         -9,  -1,  -1,  -1,  -1,  -1,  -1,  -9,
         -7,  -1,  -1,  -1,  -1,  -1,  -1,  -7,
         -6,   2,   3,   5,   5,   3,   2,  -6,
         -6,   0,   3,   5,   5,   3,   0,  -6,
         -7,   0,   5,   5,   5,   5,   0,  -7,
         -9,   6,   3,   3,   3,   3,   6,  -9,
        -15,  -8,  -8,  -6,  -6,  -8,  -8, -15
      },
      {
         -5,  -3,   5,   5,   5,   5,  -3,  -5,
         -5,  -3,   5,   5,   5,   5,  -3,  -5,
         -3,   0,   2,   5,   5,   2,   0,  -3,
         -3,   0,   2,   5,   5,   2,   0,  -3,
         -3,   0,   2,   5,   5,   2,   0,  -3,
         -3,   0,   2,   5,   5,   2,   0,  -3,
         -7,   0,   2,   5,   5,   2,   0,  -7,
        -10,  -8,   2,   5,   5,   2,  -8, -10
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
        -75, -75, -75, -75, -75, -75, -75, -75,
        -65, -65, -65, -65, -65, -65, -65, -65,
        -60, -60, -60, -60, -60, -60, -60, -60,
        -55, -55, -55, -55, -55, -55, -55, -55,
        -40, -45, -50, -55, -55, -50, -45, -40,
        -12, -25, -25, -35, -35, -25, -25, -12,
         -7,  -7, -12, -30, -30, -12,  -7,  -7,
         30,  37,   0, -25, -25,   0,  37,  25
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
        -11,  -8,  -5,  -5,  -5,  -5,  -8, -11,
         -8,  -5,   2,   2,   2,   2,  -5,  -8,
         -3,  -1,   3,   3,   3,   3,  -1,  -3,
         -2,   0,   4,   5,   5,   4,   0,  -2,
         -2,   0,   4,   5,   5,   4,   0,  -2,
         -3,  -1,   3,   3,   3,   3,  -1,  -3,
         -8,  -5,   2,   2,   2,   2,  -5,  -8,
        -11,  -8,  -5,  -5,  -5,  -5,  -8, -11
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
         -7,   0,   3,   3,   3,   3,   0,  -7,
         -5,  -3,   2,   2,   2,   2,  -3,  -5
      },
      {
         -5,  -3,   2,   2,   2,   2,  -3,  -5,
         -5,  -3,   3,   3,   3,   3,  -3,  -5,
         -3,   0,   3,   5,   5,   3,   0,  -3,
         -3,   0,   5,   5,   5,   5,   0,  -3,
         -3,   0,   5,   5,   5,   5,   0,  -3,
         -3,   0,   3,   5,   5,   3,   0,  -3,
         -7,   0,   3,   3,   3,   3,   0,  -7,
         -5,  -3,   2,   2,   2,   2,  -3,  -5
      },
      {
        -32, -26, -22, -19, -19, -22, -26, -32,
        -19, -15,   0,   0,   0,   0, -15, -15,
        -15,   0,  10,  13,  13,  10,   0, -10,
        -13,   0,  13,  26,  26,  13,   0,  -7,
        -13,   0,  13,  26,  26,  13,   0,  -7,
        -15,   0,  10,  13,  13,  10,   0, -10,
        -19, -10,   0,   0,   0,   0, -10, -15,
        -32, -26, -22, -19, -19, -22, -26, -32
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
