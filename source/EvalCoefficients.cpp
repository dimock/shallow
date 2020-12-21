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
  ScoreType32 EvalCoefficients::rookBehindBonus_{ 5, 5 };
  ScoreType32 EvalCoefficients::protectedPawnPressure_{ 1, 1 };
  ScoreType32 EvalCoefficients::unprotectedPawnPressure_{ 6, 6 };
  ScoreType32 EvalCoefficients::semiprotectedPawnPressure_{ 2, 2 };
  ScoreType32 EvalCoefficients::unprotectedPawnBishopTreat_{ 0, 3 };
  ScoreType32 EvalCoefficients::farKingPawn_ = { 5, 20 };

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
  ScoreType32 EvalCoefficients::doubledPawn_ = { -6, -5 };
  ScoreType32 EvalCoefficients::isolatedPawn_ = { -12, -11 };
  ScoreType32 EvalCoefficients::backwardPawn_ = { -12, -11 };
  ScoreType32 EvalCoefficients::blockedPawn_ = { -7, -5 };
  ScoreType32 EvalCoefficients::unguardedPawn_ = { -6, -3 };
  ScoreType32 EvalCoefficients::unprotectedPawn_ = { -5, -3 };
  ScoreType32 EvalCoefficients::hasneighborPawn_ = { 3, 2 };

  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 10, 5, 1, 0, 0, 0 };

  int EvalCoefficients::pawnShieldA_[4] = { 20, 16, 5, 0 };
  int EvalCoefficients::pawnShieldB_[4] = { 20, 16, 5, 0 };
  int EvalCoefficients::pawnShieldC_[4] = { 15, 12, 3, 0 };
  int EvalCoefficients::pawnShieldAbove_[4] = { 10, 5, 1, 0 };

  // rook on open column
  ScoreType32 EvalCoefficients::openRook_[2] = { {20, 8}, {8, 3} };

  // material diff
  // endgame, opening
  ScoreType32 EvalCoefficients::doubleBishopBonus_[10] = { {0, 0}, {12, 12}, {17, 17}, {17, 17}, {17, 17}, {17, 17}, {17, 17}, {17, 17}, {17, 17}, {17, 17} };
  ScoreType32 EvalCoefficients::doubleKnightBonus_[10] = { {0, 0}, {3, 3}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5} };
  ScoreType32 EvalCoefficients::twoKnightsBonus_[10] = { {0, 0}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5} };
  ScoreType32 EvalCoefficients::twoBishopsBonus_[10] = { {5, 5}, {20, 25}, {30, 35}, {40, 45}, {40, 45}, {40, 45}, {40, 45}, {40, 45}, {40, 45}, {40, 45} };
  ScoreType32 EvalCoefficients::twoRooksBonus_[10] = { {0, 0}, {5, 5}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10} };
  ScoreType32 EvalCoefficients::figureAgainstPawnBonus_[10] = { {0, 0}, {10, 5}, {20, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20} };
  ScoreType32 EvalCoefficients::figuresAgainstRookBonus_[10] = { {0, 0}, {15, 15}, {20, 20}, {20, 20}, {20, 20}, {20, 20}, {20, 20}, {20, 20}, {20, 20}, {20, 20} };
  ScoreType32 EvalCoefficients::knightsAgainstRookBonus_[10] = { {0, 0}, {7, 7}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10} };
  ScoreType32 EvalCoefficients::rookAgainstPawnBonus_[10] = { {0, 0}, {10, 10}, {20, 20}, {20, 20}, {20, 20}, {20, 20}, {20, 20}, {20, 20}, {20, 20}, {20, 20} };
  ScoreType32 EvalCoefficients::rookAgainstFigureBonus_[10][10] = {
    { {-40, -50}, {-40, -50}, {-40, -50}, {-40, -50}, {-40, -50}, {-40, -50}, {-40, -50}, {-40, -50}, {-40, -50}, { 0, 0} },
    { {-12, -15}, {-15, -20}, {-20, -25}, {-25, -30}, {-25, -30}, {-25, -30}, {-25, -30}, {-25, -30}, {-25, -30}, { 0, 0} },
    { {  0,   0}, {  0,   0}, { -7, -12}, {-12, -15}, {-12, -15}, {-12, -15}, {-12, -15}, {-12, -15}, {-12, -15}, { 0, 0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, { -7, -12}, {-12, -15}, {-12, -15}, {-12, -15}, {-12, -15}, {-12, -15}, { 0, 0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, { -7, -12}, {-12, -15}, {-12, -15}, {-12, -15}, {-12, -15}, { 0, 0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, { -7, -12}, {-12, -15}, {-12, -15}, {-12, -15}, { 0, 0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, { -7, -12}, {-12, -15}, {-12, -15}, { 0, 0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, { -7, -12}, {-12, -15}, { 0, 0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, { -7, -12}, { 0, 0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, { 0, 0} }
  };

  ScoreType32 EvalCoefficients::noKnightsPenalty_ = {5, 5};
  ScoreType32 EvalCoefficients::noBishopsPenalty_ = {8, 8};
  ScoreType32 EvalCoefficients::noRooksPenalty_ = {10, 10};
  ScoreType32 EvalCoefficients::noQueensPenalty_ = {15, 15};

  // arrays
  ScoreType32 EvalCoefficients::passerPawn_[8]  = { {  0,   0}, {  1,   1}, {  5,   5}, { 10,  10}, { 17,  17}, { 35,  35}, { 47,  47}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawn2_[8] = { {  0,   0}, {  0,   0}, {  2,   2}, {  5,   5}, {  8,   8}, { 17,  17}, { 23,  23}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawn4_[8] = { {  0,   0}, {  0,   0}, {  1,   1}, {  2,   2}, {  4,   4}, {  8,   8}, { 11,  11}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnEx_[2][8] = {
    { {  0,   0}, {  4,   4}, { 15,  15}, { 30,  30}, { 52,  52}, {105, 105}, {142, 142}, {  0,   0} },
    { {  0,   0}, {  1,   1}, {  5,   5}, { 10,  10}, { 17,  17}, { 35,  35}, { 47,  47}, {  0,   0} }
  };
  ScoreType32 EvalCoefficients::kingToPasserDistanceBonus_[8]  = { {  0,   0}, {  0,   0}, {  0,   2}, {  0,   4}, {  0,   7}, {  0,  14}, {  0,  19}, {  0,   0} };
  ScoreType32 EvalCoefficients::okingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   1}, {  0,   4}, {  0,   8}, {  0,  14}, {  0,  28}, {  0,  38}, {  0,   0} };

  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  ScoreType32 EvalCoefficients::knightMobility_[16] = { {-35, -35}, {-15, -15}, { 3,  3},  { 5,  5},  { 7,  7},  { 8,  8}, {10, 10}, {12, 12}, {14, 14} };
  ScoreType32 EvalCoefficients::bishopMobility_[16] = { {-35, -35}, {-15, -15}, { 3,  3},  { 5,  5},  { 7,  7},  { 9,  9}, {12, 12}, {13, 13}, {14, 14}, {16, 16}, {20, 20}, {22, 22}, {23, 23}, {24, 24}, {25, 25}, {26, 26} };
  ScoreType32 EvalCoefficients::rookMobility_[16] =   { {-35, -30}, {-15, -15}, { 5,  5},  { 8,  8},  {12, 12},  {16, 16}, {21, 21}, {25, 25}, {27, 27}, {29, 29}, {30, 30}, {31, 31}, {32, 32}, {33, 33}, {34, 34}, {35, 35} };
  ScoreType32 EvalCoefficients::queenMobility_[32] =  { {-65, -65}, {-30, -30}, { 6,  6},  {10, 10},  {13, 13},  {18, 18}, {23, 23}, {27, 27}, {30, 30}, {33, 33}, {36, 36}, {40, 40}, {43, 43}, {45, 45}, {47, 47}, {48, 48},
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
        {  5,   5}, {  5,   5}, {  5,   6}, {  5,   7}, {  5,   7}, {  5,   6}, {  5,   5}, {  5,   5},
        {  3,   4}, {  3,   4}, {  5,   4}, { 10,   6}, { 10,   6}, {  5,   5}, {  3,   4}, {  3,   4},
        {  1,   3}, {  1,   3}, {  9,   4}, { 13,   5}, { 13,   5}, {  9,   4}, {  1,   3}, {  1,   3},
        { -2,   2}, { -2,   2}, {  7,   3}, { 13,   4}, { 13,   4}, {  7,   3}, { -2,   2}, { -2,   2},
        {  4,   1}, {  4,   1}, {  4,   2}, { 10,   3}, { 10,   3}, {  4,   2}, {  4,   1}, {  4,   1},
        {  5,  -1}, {  5,  -2}, {  5,  -3}, {-10,  -4}, {-10,  -4}, {  5,  -3}, {  5,  -1}, {  5,  -1},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}
      },
      {
        {-35, -20}, {-20, -12}, {-10, -10}, {-10, -10}, {-10, -10}, {-10, -10}, {-20, -10}, {-35, -20},
        {-20, -12}, { -4,  -4}, { -2,   5}, {  0,   5}, {  0,   5}, { -2,   5}, { -4,  -4}, {-20, -10},
        { -7, -10}, {  8,   5}, {  9,   8}, { 10,   8}, { 10,   8}, {  9,   8}, {  8,   5}, { -7, -10},
        { -5, -10}, {  7,   5}, {  8,   8}, { 12,  10}, { 12,  10}, {  8,   8}, {  7,   5}, { -5, -10},
        {-12, -10}, {  3,   5}, {  8,   8}, { 12,  10}, { 12,  10}, {  8,   8}, {  3,   5}, {-12, -10},
        {-12, -10}, { -2,   5}, {  2,   8}, {  3,   8}, {  3,   8}, {  2,   8}, { -2,   5}, {-12, -10},
        {-12, -12}, { -4,  -4}, { -3,   5}, { -1,   5}, { -1,   5}, { -3,   5}, { -4,  -4}, {-12, -12},
        {-25, -20}, {-18, -12}, {-12, -10}, {-10, -10}, {-10, -10}, {-12, -10}, {-18, -12}, {-25, -20}
      },
      {
        {-15,  -8}, { -8,  -3}, { -8,  -2}, { -8,  -1}, { -8,  -1}, { -8,  -2}, { -8,  -3}, {-15,  -8},
        { -2,  -3}, { -5,   4}, {  3,   3}, {  0,   3}, {  0,   3}, {  3,   3}, { -5,   4}, { -2,  -3},
        {  0,  -2}, {  7,   3}, {  5,   5}, {  5,   5}, {  5,   5}, {  5,   5}, {  7,   3}, {  0,  -2},
        { -4,  -1}, {  6,   3}, {  8,   5}, { 12,   6}, { 12,   6}, {  8,   5}, {  6,   3}, { -4,  -1},
        { -2,  -1}, {  6,   3}, {  8,   5}, { 12,   6}, { 12,   6}, {  8,   5}, {  6,   3}, { -2,  -1},
        {  0,  -2}, {  7,   3}, { 10,   5}, {  8,   5}, {  8,   5}, { 10,   5}, {  7,   3}, {  0,  -2},
        {  0,  -3}, {  8,   4}, {  5,   3}, {  3,   3}, {  3,   3}, {  5,   3}, {  8,   4}, {  0,  -3},
        {-15,  -8}, { -5,  -3}, { -8,  -2}, { -9,  -1}, { -9,  -1}, { -8,  -2}, { -5,  -3}, {-15,  -8}
      },
      {
        { -5,  -3}, {  2,   2}, {  5,   4}, {  7,   4}, {  7,   4}, {  5,   4}, {  2,   2}, { -5,  -3},
        {  0,   1}, {  4,   2}, { 10,   5}, { 12,   5}, { 12,   5}, { 10,   5}, {  4,   2}, {  0,   1},
        {  2,   2}, {  2,   2}, {  6,   5}, {  8,   6}, {  8,   6}, {  6,   5}, {  2,   2}, {  2,   2},
        {  2,   2}, {  2,   3}, {  4,   6}, {  7,   6}, {  7,   6}, {  4,   6}, {  2,   3}, {  2,   2},
        {  2,   2}, {  2,   3}, {  3,   6}, {  5,   6}, {  5,   6}, {  3,   6}, {  2,   3}, {  2,   2},
        {  2,   2}, {  2,   3}, {  2,   5}, {  4,   6}, {  4,   6}, {  2,   5}, {  2,   3}, {  2,   2},
        { -5,   1}, { -2,   2}, {  1,   5}, {  2,   5}, {  2,   5}, {  1,   5}, { -2,   2}, { -5,   1},
        { -7,  -3}, { -7,   2}, {  0,   4}, {  1,   4}, {  1,   4}, {  0,   4}, { -7,   2}, { -7,  -3}
      },
      {
        { -8,  -8}, { -2,  -3}, {  1,  -2}, {  3,  -2}, {  3,  -2}, {  1,  -2}, { -2,  -3}, { -8,  -8},
        { -2,  -5}, {  2,  -2}, {  4,   1}, {  4,   3}, {  4,   3}, {  4,   1}, {  2,  -2}, { -2,  -5},
        { -2,  -5}, {  2,   1}, {  7,   6}, {  6,   6}, {  6,   6}, {  7,   6}, {  2,   1}, { -2,  -5},
        { -2,  -2}, {  2,   3}, {  5,   6}, {  6,   8}, {  6,   8}, {  5,   6}, {  2,   3}, { -2,  -2},
        { -2,  -2}, {  2,   3}, {  5,   6}, {  5,   8}, {  5,   8}, {  5,   6}, {  2,   3}, { -2,  -2},
        { -2,  -5}, {  2,   1}, {  3,   6}, {  4,   6}, {  4,   6}, {  3,   6}, {  2,   1}, { -2,  -5},
        { -3,  -5}, {  1,  -2}, {  1,   1}, {  0,   3}, {  0,   3}, {  1,   1}, {  1,  -2}, { -3,  -5},
        { -8,  -8}, { -2,  -3}, { -2,  -2}, { -2,  -2}, { -2,  -2}, { -2,  -2}, { -2,  -3}, { -8,  -8}
      },
      {
        {-60, -37}, {-60, -18}, {-60, -13}, {-60,  -9}, {-60,  -9}, {-60, -13}, {-60, -18}, {-60, -37},
        {-46, -25}, {-47, -13}, {-48,   2}, {-50,   2}, {-50,   2}, {-48,   2}, {-47, -13}, {-46, -25},
        {-37, -15}, {-39,   2}, {-41,  10}, {-43,  15}, {-43,  15}, {-41,  10}, {-39,   2}, {-37, -15},
        {-28, -15}, {-30,   2}, {-32,  15}, {-34,  20}, {-34,  20}, {-32,  15}, {-30,   2}, {-28, -15},
        {-20, -15}, {-23,   2}, {-27,  15}, {-29,  20}, {-29,  20}, {-27,  15}, {-23,   2}, {-20, -15},
        { -8, -15}, { -6,   2}, {-15,  10}, {-19,  15}, {-19,  15}, {-12,  10}, { -5,   2}, { -7, -15},
        { 12, -25}, { 18, -13}, {  2,   2}, {-14,   2}, {-14,   2}, {  3,   2}, { 22, -13}, { 15, -20},
        { 16, -40}, { 23, -20}, {  6, -15}, { -8, -10}, { -8, -10}, {  7, -15}, { 29, -20}, { 20, -40}
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
