/*************************************************************
EvalCoefficients.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <Figure.h>
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
  ScoreType32 EvalCoefficients::rookBehindBonus_{ 5, 5 };
  ScoreType32 EvalCoefficients::protectedPawnPressure_{ 1, 1 };
  ScoreType32 EvalCoefficients::unprotectedPawnPressure_{ 6, 6 };
  ScoreType32 EvalCoefficients::semiprotectedPawnPressure_{ 2, 2 };
  ScoreType32 EvalCoefficients::unprotectedPawnBishopTreat_{ 0, 3 };
  ScoreType32 EvalCoefficients::kingToPasserDistanceBonus_{ 0, 3 };

  // forks
  ScoreType EvalCoefficients::bishopsAttackBonus_{ 65 };
  ScoreType EvalCoefficients::knightForkBonus_{ 65 };
  ScoreType EvalCoefficients::doublePawnAttack_{ 65 };
  ScoreType EvalCoefficients::generalAttackBonus_{ 10 };
  ScoreType EvalCoefficients::generalAttackBonus4_{ 2 };
  ScoreType EvalCoefficients::queenUnderRookAttackBonus_{ 30 };

  // immobility
  ScoreType32 EvalCoefficients::immobileAttackBonus_{ 30, 30 };

  // check & mat treat
  ScoreType32 EvalCoefficients::discoveredCheckBonus_{ 20, 20 };
  ScoreType32 EvalCoefficients::discoveredCheckBonus4_{ 5, 5 };
  ScoreType EvalCoefficients::discoveredCheckBonus1_{ 20 };
  ScoreType EvalCoefficients::possibleCheckBonus_{ 5 };
  ScoreType EvalCoefficients::matTreatBonus_{ 150 };

  // king
  int EvalCoefficients::fakeCastle_{ -15 };
  int EvalCoefficients::kingIsUnsafe_{ -25 };
  int EvalCoefficients::pawnPenaltyA_{ -13 };
  int EvalCoefficients::pawnPenaltyB_{ -15 };
  int EvalCoefficients::pawnPenaltyC_{ -3 };

  // blocked figure
  ScoreType32 EvalCoefficients::knightBlocked_{ 80, 80 };
  ScoreType32 EvalCoefficients::bishopBlocked_{ 80, 80 };
  ScoreType32 EvalCoefficients::rookBlocked_{ 80, 80 };

  // king attacks
  ScoreType EvalCoefficients::pawnKingAttack_{ 8 };
  ScoreType EvalCoefficients::knightKingAttack_{ 8 };
  ScoreType EvalCoefficients::bishopKingAttack_{ 8 };
  ScoreType EvalCoefficients::rookKingAttack_{ 8 };
  ScoreType EvalCoefficients::queenKingAttack_{ 10 };
  ScoreType EvalCoefficients::basicAttack_{ 2 };

  int EvalCoefficients::generalKingPressure_{ 5 };
  int EvalCoefficients::generalOpponentPressure_{ 2 };

  // king threat
  int EvalCoefficients::knightChecking_{ 45 };
  int EvalCoefficients::bishopChecking_{ 45 };
  int EvalCoefficients::rookChecking_{ 65 };
  int EvalCoefficients::queenChecking_{ 85 };
  int EvalCoefficients::checkedFieldBonus_{ 12 };
  int EvalCoefficients::attackedNearKing_{ 20 };

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 2 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 2 };

  // arrays
  ScoreType32 EvalCoefficients::doubledPawn_ = { -12, -10 };
  ScoreType32 EvalCoefficients::isolatedPawn_ = { -15, -13 };
  ScoreType32 EvalCoefficients::backwardPawn_ = { -10, -9 };
  ScoreType32 EvalCoefficients::unguardedPawn_ = { -6, -3 };
  ScoreType32 EvalCoefficients::unprotectedPawn_ = { -5, -3 };
  ScoreType32 EvalCoefficients::hasneighborPawn_ = { 3, 2 };

  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 1, 0, 0, 0 };

  int EvalCoefficients::pawnShieldA_[2] = { 15, 12 };
  int EvalCoefficients::pawnShieldB_[2] = { 15, 10 };
  int EvalCoefficients::pawnShieldC_[2] = { 6, 4 };

  // rook on open column
  ScoreType32 EvalCoefficients::openRook_[2] = { {20, 8}, {8, 3} };

  // material diff
  // endgame, opening
  ScoreType32 EvalCoefficients::doubleBishopBonus_[10] = { {10, 5}, {15, 10}, {25, 15}, {30, 15}, {30, 15}, {30, 15}, {30, 15}, {30, 15}, {30, 15}, {30, 15} };
  ScoreType32 EvalCoefficients::twoKnightsBonus_[10] = { {0, 0}, {5, 2}, {10, 5}, {15, 5}, {15, 5}, {15, 5}, {15, 5}, {15, 5}, {15, 5}, {15, 5} };
  ScoreType32 EvalCoefficients::oneBishopBonus_[10] = { {2, 0}, {5, 2}, {10, 5}, {10, 5}, {10, 5}, {10, 5}, {10, 5}, {10, 5}, {10, 5}, {10, 5} };
  ScoreType32 EvalCoefficients::twoBishopsBonus_[10] = { {15, 10}, {40, 20}, {60, 40}, {60, 40}, {60, 40}, {60, 40}, {60, 40}, {60, 40}, {60, 40}, {60, 40} };
  ScoreType32 EvalCoefficients::twoRooksBonus_[10] = { {0, 0}, {5, 5}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10} };
  ScoreType32 EvalCoefficients::figureAgainstPawnBonus_[10] = { {0, 0}, {15, 5}, {45, 20}, {45, 20}, {45, 20}, {45, 20}, {45, 20}, {45, 20}, {45, 20}, {45, 20} };
  ScoreType32 EvalCoefficients::rookAgainstFigureBonus_[10] = { {0, 0}, {5, 5}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10} };
  ScoreType32 EvalCoefficients::figuresAgainstRookBonus_[10] = { {0, 0}, {20, 15}, {30, 20}, {30, 20}, {30, 20}, {30, 20}, {30, 20}, {30, 20}, {30, 20}, {30, 20} };
  ScoreType32 EvalCoefficients::rookAgainstPawnBonus_[10] = { {10, 10}, {30, 30}, {40, 40}, {40, 40}, {40, 40}, {40, 40}, {40, 40}, {40, 40}, {40, 40}, {40, 40} };

  // arrays
  ScoreType   EvalCoefficients::passerPawn1_[8] = { 0, 5, 9, 20, 38, 65, 98, 0 };
  ScoreType32 EvalCoefficients::passerPawn_[8]  = { {0, 0}, {5, 5}, {9, 9}, {20, 20}, {38, 38}, {65, 65}, {98, 98}, {0, 0} };
  ScoreType32 EvalCoefficients::passerPawn2_[8] = { {0, 0}, {2, 2}, {4, 4}, {10, 10}, {19, 19}, {32, 32}, {49, 49}, {0, 0} };
  ScoreType32 EvalCoefficients::passerPawn4_[8] = { {0, 0}, {1, 1}, {2, 2}, {5, 5}, {9, 9}, {16, 16}, {24, 24}, {0, 0} };
  ScoreType32 EvalCoefficients::passerPawn38_[8] = { {0, 0}, {2, 2}, {3, 3}, {7, 7}, {14, 14}, {24, 24}, {37, 37}, {0, 0} };
  ScoreType32 EvalCoefficients::passerPawnEx_[8] = { {0, 0}, {5, 10}, {9, 18}, {20, 40}, {38, 76}, {65, 130}, {98, 196}, {0, 0} };
  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::farKingPawn_[8] = { 0, 3, 4, 8, 14, 23, 35, 0 };
  ScoreType32 EvalCoefficients::canpromotePawn_[8] = { {0, 0}, {5, 5}, {9, 9}, {23, 23}, {45, 45}, {75, 75}, {115, 115}, {0, 0} };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  ScoreType32 EvalCoefficients::knightMobility_[16] = { {-30, -30}, {-15, -15}, { 3,  3},  { 5,  5},  { 7,  7},  { 8,  8}, {10, 10}, {12, 12}, {14, 14} };
  ScoreType32 EvalCoefficients::bishopMobility_[16] = { {-30, -30}, {-15, -15}, { 3,  3},  { 5,  5},  { 7,  7},  { 9,  9}, {12, 12}, {13, 13}, {14, 14}, {16, 16}, {20, 20}, {22, 22}, {23, 23}, {24, 24}, {25, 25}, {26, 26} };
  ScoreType32 EvalCoefficients::rookMobility_[16] =   { {-30, -30}, {-15, -15}, { 5,  5},  { 8,  8},  {12, 12},  {16, 16}, {21, 21}, {25, 25}, {27, 27}, {29, 29}, {30, 30}, {31, 31}, {32, 32}, {33, 33}, {34, 34}, {35, 35} };
  ScoreType32 EvalCoefficients::queenMobility_[32] =  { {-60, -60}, {-30, -30}, { 6,  6},  {10, 10},  {13, 13},  {18, 18}, {23, 23}, {27, 27}, {30, 30}, {33, 33}, {36, 36}, {40, 40}, {43, 43}, {45, 45}, {47, 47}, {48, 48},
                                                        { 49,  49}, { 50,  50}, {51, 51},  {52, 52},  {53, 53},  {54, 54}, {55, 55}, {56, 56}, {57, 57}, {58, 58}, {59, 59}, {60, 60}, {61, 61}, {62, 62}, {63, 63}, {64, 64} };

  ScoreType32 EvalCoefficients::knightPinned_ = { 15, 15 };
  ScoreType32 EvalCoefficients::bishopPinned_ = { 15, 15 };
  ScoreType32 EvalCoefficients::rookPinned_ = { 15, 15 };
  ScoreType32 EvalCoefficients::queenPinned_ = { 30, 30 };

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

  ScoreType32 EvalCoefficients::kingDistanceBonus_[8][8] =
  {
    {},
    {},
    { {0, 0}, {10, 0}, {7, 0}, {5, 0}, {3, 0}, {1, 0}, {0, 0}, {-5, 0} },
    { {0, 0}, {10, 0}, {7, 0}, {5, 0}, {3, 0}, {1, 0}, {0, 0}, {-5, 0} },
    { {0, 0}, {10, 0}, {7, 0}, {5, 0}, {3, 0}, {1, 0}, {0, 0}, {-5, 0} },
    { {0, 0}, {10, 0}, {7, 0}, {5, 0}, {3, 0}, {1, 0}, {0, 0}, {-5, 0} },
    {},
    {}
  };

  ScoreType32 EvalCoefficients::positionEvaluations_[2][8][64] =
  {
    {
      {
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}
      },
      {
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  5,   4}, {  5,   4}, {  5,   5}, {  5,   5}, {  5,   5}, {  5,   5}, {  5,   4}, {  5,   4},
        {  3,   3}, {  3,   4}, {  5,   5}, { 10,   5}, { 10,   5}, {  5,   5}, {  3,   4}, {  3,   3},
        {  2,   2}, {  2,   3}, {  4,   4}, { 12,   4}, { 12,   4}, {  4,   4}, {  2,   3}, {  2,   2},
        {  0,   1}, {  0,   2}, {  3,   3}, { 12,   4}, { 12,   4}, {  3,   3}, {  0,   2}, {  0,   1},
        { -1,   0}, { -1,   1}, {  0,   2}, { 10,   3}, { 10,   3}, {  0,   2}, { -1,   1}, { -1,   0},
        { -2,   0}, { -2,   0}, { -2,   0}, { -8,  -8}, { -8,  -8}, { -2,   0}, { -2,   0}, { -2,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}
      },
      {
        {-15, -12}, {-12, -10}, {-10,  -8}, {-10,  -8}, {-10,  -8}, {-10,  -8}, {-12, -10}, {-15, -12},
        {-12, -10}, { -2,   0}, {  0,   5}, {  0,   5}, {  0,   5}, {  0,   5}, { -2,   0}, {-12, -10},
        {-10,  -8}, {  0,   5}, {  5,   8}, {  7,   8}, {  7,   8}, {  5,   8}, {  0,   5}, {-10,  -8},
        { -8,  -8}, {  0,   5}, {  8,   8}, { 12,  10}, { 12,  10}, {  8,   8}, {  0,   5}, { -8,  -8},
        { -8,  -8}, {  0,   5}, {  8,   8}, { 12,  10}, { 12,  10}, {  8,   8}, {  0,   5}, { -8,  -8},
        {-10,  -8}, { -2,   5}, {  5,   8}, {  7,   8}, {  7,   8}, {  5,   8}, { -2,   5}, {-10,  -8},
        {-12, -10}, { -8,   0}, { -3,   5}, { -1,   5}, { -1,   5}, { -3,   5}, { -8,   0}, {-12, -10},
        {-15, -12}, {-12, -10}, { -8,  -8}, { -8,  -8}, { -8,  -8}, { -8,  -8}, {-12, -10}, {-15, -12}
      },
      {
        {-15,  -8}, { -8,  -3}, { -8,  -2}, { -8,  -1}, { -8,  -1}, { -8,  -2}, { -8,  -3}, {-15,  -8},
        { -9,  -3}, {  5,   4}, {  3,   3}, {  0,   3}, {  0,   3}, {  3,   3}, {  5,   4}, { -9,  -3},
        { -1,  -2}, {  7,   3}, {  5,   5}, {  5,   5}, {  5,   5}, {  5,   5}, {  7,   3}, { -1,  -2},
        { -1,  -1}, {  6,   3}, {  7,   5}, { 10,   6}, { 10,   6}, {  7,   5}, {  6,   3}, { -1,  -1},
        { -1,  -1}, {  6,   3}, {  8,   5}, { 10,   6}, { 10,   6}, {  8,   5}, {  6,   3}, { -1,  -1},
        { -1,  -2}, {  9,   3}, { 10,   5}, {  8,   5}, {  8,   5}, { 10,   5}, {  9,   3}, { -1,  -2},
        { -5,  -3}, {  2,   4}, {  9,   3}, {  6,   3}, {  6,   3}, {  9,   3}, {  2,   4}, { -5,  -3},
        {-15,  -8}, { -6,  -3}, { -8,  -2}, { -6,  -1}, { -6,  -1}, { -8,  -2}, { -6,  -3}, {-15,  -8}
      },
      {
        { -5,  -3}, {  2,   2}, {  5,   4}, {  7,   4}, {  7,   4}, {  5,   4}, {  2,   2}, { -5,  -3},
        {  0,   1}, {  0,   2}, {  8,   5}, { 10,   5}, { 10,   5}, {  8,   5}, {  0,   2}, {  0,   1},
        {  2,   2}, {  2,   2}, {  3,   5}, {  4,   6}, {  4,   6}, {  3,   5}, {  2,   2}, {  2,   2},
        {  2,   2}, {  2,   3}, {  3,   6}, {  4,   6}, {  4,   6}, {  3,   6}, {  2,   3}, {  2,   2},
        {  2,   2}, {  2,   3}, {  3,   6}, {  4,   6}, {  4,   6}, {  3,   6}, {  2,   3}, {  2,   2},
        {  2,   2}, {  2,   3}, {  3,   5}, {  4,   6}, {  4,   6}, {  3,   5}, {  2,   3}, {  2,   2},
        { -5,   1}, { -5,   2}, {  3,   5}, {  4,   5}, {  4,   5}, {  3,   5}, { -5,   2}, { -5,   1},
        { -5,  -3}, { -5,   2}, {  3,   4}, {  4,   4}, {  4,   4}, {  3,   4}, { -5,   2}, { -5,  -3}
      },
      {
        { -5,  -8}, { -2,  -3}, {  1,  -2}, {  3,  -2}, {  3,  -2}, {  1,  -2}, { -2,  -3}, { -5,  -8},
        { -2,  -5}, {  2,  -2}, {  2,   1}, {  2,   3}, {  2,   3}, {  2,   1}, {  2,  -2}, { -2,  -5},
        { -2,  -5}, {  2,   1}, {  3,   8}, {  3,   8}, {  3,   8}, {  3,   8}, {  2,   1}, { -2,  -5},
        { -2,  -2}, {  2,   3}, {  5,   8}, {  7,  10}, {  7,  10}, {  5,   8}, {  2,   3}, { -2,  -2},
        { -2,  -2}, {  2,   3}, {  5,   8}, {  7,  10}, {  7,  10}, {  5,   8}, {  2,   3}, { -2,  -2},
        { -2,  -5}, {  2,   1}, {  3,   8}, {  4,   8}, {  4,   8}, {  3,   8}, {  2,   1}, { -2,  -5},
        { -2,  -5}, {  1,  -2}, {  1,   1}, {  2,   3}, {  2,   3}, {  1,   1}, {  1,  -2}, { -2,  -5},
        { -5,  -8}, { -2,  -3}, { -2,  -2}, { -2,  -2}, { -2,  -2}, { -2,  -2}, { -2,  -3}, { -5,  -8}
      },
      {
        {-30, -25}, {-30, -15}, {-30, -10}, {-30, -10}, {-30, -10}, {-30, -10}, {-30, -15}, {-30, -25},
        {-26, -15}, {-26,   0}, {-26,   0}, {-26,   0}, {-26,   0}, {-26,   0}, {-26,   0}, {-26, -15},
        {-24, -10}, {-24,   0}, {-24,  12}, {-24,  12}, {-24,  12}, {-24,  12}, {-24,   0}, {-24, -10},
        {-22, -10}, {-22,   0}, {-22,  12}, {-22,  20}, {-22,  20}, {-22,  12}, {-22,   0}, {-20, -10},
        {-15, -10}, {-15,   0}, {-20,  12}, {-22,  20}, {-22,  20}, {-18,  12}, {-15,   0}, {-15, -10},
        {  1, -10}, { -5,   0}, {-10,  12}, {-18,  12}, {-18,  12}, { -8,  12}, { -3,   0}, {  2, -10},
        { 10, -15}, { 12,   0}, {  8,   0}, {-12,   0}, {-12,   0}, { 10,   0}, { 15,   0}, { 12, -15},
        { 13, -25}, { 17, -20}, { 12, -10}, {-10, -10}, {-10, -10}, { 15, -10}, { 20, -15}, { 16, -25}
      },
      {
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}
      }
    },
    {
    }
  };

  void EvalCoefficients::initialize()
  {
    // copy coefficients for white color with negative sign and flipped position
    for (int t = 0; t < 8; ++t)
    {
      for (int p = 0; p < 64; ++p)
      {
        positionEvaluations_[1][t][p] = -positionEvaluations_[0][t][Figure::mirrorIndex_[p]];
      }
    }
  }
}
