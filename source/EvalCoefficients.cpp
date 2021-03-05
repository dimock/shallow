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
  // pawns
  ScoreType32 EvalCoefficients::pawnPressureStrong_{ 8, 25 };
  ScoreType32 EvalCoefficients::pawnPressureWeak_{ 3, 9 };
  ScoreType32 EvalCoefficients::pawnBishopTreat_{ 0, 3 };

  // forks
  ScoreType EvalCoefficients::bishopsAttackBonus_{ 22 };
  ScoreType EvalCoefficients::knightAttack_{ 32 };
  ScoreType EvalCoefficients::pawnAttack_{ 40 };
  ScoreType EvalCoefficients::rookQueenAttackedBonus_{ 30 };
  ScoreType EvalCoefficients::queenUnderRookAttackBonus_{ 20 };
  ScoreType EvalCoefficients::multiattackedBonus_{ 25 };

  // immobility
  ScoreType32 EvalCoefficients::immobileAttackBonus_{ 30, 30 };

  // check & mat treat
  ScoreType32 EvalCoefficients::discoveredCheckBonus_{ 20, 20 };

  // king
  ScoreType32 EvalCoefficients::fakeCastle_{ -60, 0 };

  // blocked figure
  ScoreType32 EvalCoefficients::knightBlocked_{ 60, 50 };
  ScoreType32 EvalCoefficients::bishopBlocked_{ 60, 50 };
  ScoreType32 EvalCoefficients::rookBlocked_{ 60, 60 };

  // king attacks
  int EvalCoefficients::pawnKingAttack_{ 7 };

#ifdef KING_EVAL_ATTACKED_MULTI_FIELD
  int EvalCoefficients::knightKingAttack_{ 10 };
  int EvalCoefficients::bishopKingAttack_{ 9 };
  int EvalCoefficients::rookKingAttack_{ 10 };
  int EvalCoefficients::queenKingAttack_{ 13 };
  int EvalCoefficients::basicAttack_{ 2 };
#else
  int EvalCoefficients::knightKingAttack_{ 16 };
  int EvalCoefficients::bishopKingAttack_{ 16 };
  int EvalCoefficients::rookKingAttack_{ 17 };
  int EvalCoefficients::queenKingAttack_{ 19 };
  int EvalCoefficients::basicAttack_{ 3 };
#endif

  int EvalCoefficients::generalKingPressure_{ 6 };
  int EvalCoefficients::generalKingAuxPressure_{ 3 };

  // king threat
  int EvalCoefficients::knightChecking_{ 40 };
  int EvalCoefficients::bishopChecking_{ 40 };
  int EvalCoefficients::rookChecking_{ 80 };
  int EvalCoefficients::queenChecking_{ 90 };
  int EvalCoefficients::weakChecking_{ 9 };
  
  int EvalCoefficients::attackedNearKingCoeff_{ 30 };
  int EvalCoefficients::kingWeakCheckersCoefficients_{ 16 };
  int EvalCoefficients::kingCheckersCoefficients_[8]  = { 0, 32, 48, 64, 64, 64, 64, 64 };
  int EvalCoefficients::kingAttackersCoefficients_[8] = { 0,  0, 15, 32, 48, 56, 60, 62 };
  int EvalCoefficients::kingPossibleMovesCoefficients_[10] = { 18,  12, 4, 2, 1, 0, 0, 0, 0, 0 };

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 3 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 3 };

  // arrays
  ScoreType32 EvalCoefficients::doubledPawn_ = {-7, -5};
  ScoreType32 EvalCoefficients::isolatedPawn_ = {-16, -12};
  ScoreType32 EvalCoefficients::backwardPawn_[8] = { {0, 0}, {-16, -12}, {-14, -10}, {-12, -10}, {-10, -8}, {-10, -8}, {0, 0}, {0, 0} };
  ScoreType32 EvalCoefficients::protectedPawn_[8] = { {0, 0}, {0, 0}, {4, 3}, {6, 4}, {8, 6}, {10, 8}, {10, 8}, {0, 0} };
  ScoreType32 EvalCoefficients::hasneighborPawn_[8] = { {0, 0}, {3, 2}, {3, 2}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {0, 0} };

  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 7, 1, 0, 0 };

  int EvalCoefficients::pawnShieldA_[4] = { 27, 23, 5, 0 };
  int EvalCoefficients::pawnShieldB_[4] = { 27, 23, 5, 0 };
  int EvalCoefficients::pawnShieldC_[4] = { 20, 16, 3, 0 };
  int EvalCoefficients::pawnShieldAbove_[4] = { 12, 9, 2, 0 };

  // rook on open column
  ScoreType32 EvalCoefficients::openRook_[2] = { {20, 8}, {10, 4} };

  // material diff
  // opening, endgame
  ScoreType32 EvalCoefficients::doubleBishopBonus_[10] = { {0, 0}, {10, 10}, {12, 12}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15} };
  ScoreType32 EvalCoefficients::doubleKnightBonus_[10] = { {0, 0}, {3, 3}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5} };
  ScoreType32 EvalCoefficients::twoKnightsBonus_[10] = { {0, 0}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5} };
  ScoreType32 EvalCoefficients::twoBishopsBonus_[10] = { {5, 5}, {25, 25}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35} };
  ScoreType32 EvalCoefficients::twoRooksBonus_[10] = { {0, 0}, {5, 5}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10} };
  ScoreType32 EvalCoefficients::figureAgainstPawnBonus_[10] = { {0, 0}, {20, 10}, {30, 20}, {35, 20}, {35, 20}, {35, 20}, {35, 20}, {35, 20}, {35, 20}, {35, 20} };
  ScoreType32 EvalCoefficients::figuresAgainstRookBonus_[10] = { {0, 0}, {30, 20}, {50, 40}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50} };
  ScoreType32 EvalCoefficients::knightsAgainstRookBonus_[10] = { {0, 0}, {12, 12}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20} };
  ScoreType32 EvalCoefficients::rookAgainstFigureBonus_[10][10] = {
    { { 70,  30}, { 67,  27}, { 65,  25}, { 63,  23}, { 62,  22}, { 61,  21}, { 60,  20}, { 60,  20}, { 60,  20}, {  0,   0} },
    { { 60,  22}, { 59,  21}, { 58,  20}, { 57,  19}, { 56,  18}, { 56,  18}, { 55,  17}, { 55,  17}, { 55,  17}, {  0,   0} },
    { { 52,  16}, { 52,  16}, { 51,  16}, { 51,  16}, { 51,  15}, { 51,  15}, { 51,  15}, { 51,  15}, { 51,  15}, {  0,   0} },
    { { 45,  11}, { 46,  12}, { 46,  12}, { 46,  13}, { 47,  13}, { 47,  13}, { 47,  13}, { 47,  13}, { 47,  13}, {  0,   0} },
    { { 40,   7}, { 41,   8}, { 42,   9}, { 43,  10}, { 43,  11}, { 44,  11}, { 44,  12}, { 44,  12}, { 45,  12}, {  0,   0} },
    { { 35,   4}, { 37,   5}, { 38,   7}, { 40,   8}, { 41,   9}, { 41,  10}, { 42,  10}, { 42,  11}, { 42,  11}, {  0,   0} },
    { { 32,   1}, { 34,   3}, { 36,   5}, { 37,   7}, { 39,   8}, { 40,   9}, { 40,  10}, { 41,  10}, { 41,  10}, {  0,   0} },
    { { 30,   0}, { 32,   2}, { 34,   4}, { 36,   6}, { 37,   7}, { 38,   8}, { 39,   9}, { 40,  10}, { 40,  10}, {  0,   0} },
    { { 30,   0}, { 32,   2}, { 34,   4}, { 36,   6}, { 37,   7}, { 38,   8}, { 39,   9}, { 39,   9}, { 40,  10}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };

  ScoreType32 EvalCoefficients::noKnightsPenalty_ = {5, 5};
  ScoreType32 EvalCoefficients::noBishopsPenalty_ = {8, 8};
  ScoreType32 EvalCoefficients::noRooksPenalty_ = {10, 10};
  ScoreType32 EvalCoefficients::noQueensPenalty_ = {15, 15};

  // passer pawns
  ScoreType32 EvalCoefficients::passerPawn_[8] = { {  0,   0}, {  2,   2}, {  5,   5}, { 13,  13}, { 28,  28}, { 48,  48}, { 75,  75}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawn2_[8] = { {  0,   0}, {  1,   1}, {  2,   2}, {  6,   6}, { 14,  14}, { 24,  24}, { 37,  37}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawn4_[8] = { {  0,   0}, {  0,   0}, {  1,   1}, {  3,   3}, {  7,   7}, { 12,  12}, { 18,  18}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnEx_[8] = { {  0,   0}, {  0,   3}, {  1,   8}, {  3,  22}, {  7,  45}, { 13,  78}, { 24, 144}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnExP_[8] = { {  0,   0}, {  2,   1}, {  5,   3}, { 14,   9}, { 30,  18}, { 52,  32}, { 80,  50}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnBasic_[8] = { {  0,   0}, {  0,   0}, {  1,   1}, {  2,   3}, {  5,   7}, {  9,  13}, { 15,  20}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnExS_[8][8] = {
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  0,   2}, {  0,   2}, {  0,   2}, {  0,   3}, {  0,   3}, {  0,   3}, {  0,   0} },
    { {  0,   0}, {  0,   4}, {  0,   5}, {  1,   6}, {  1,   7}, {  1,   8}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  2,  13}, {  2,  16}, {  3,  19}, {  3,  22}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  5,  30}, {  6,  37}, {  7,  45}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  9,  58}, { 13,  78}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 24, 144}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };
  ScoreType32 EvalCoefficients::passerPawnExSp_[8][8] = {
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  1,   0}, {  1,   1}, {  1,   1}, {  2,   1}, {  2,   1}, {  2,   1}, {  0,   0} },
    { {  0,   0}, {  3,   2}, {  3,   2}, {  4,   2}, {  4,   3}, {  5,   3}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  9,   5}, { 11,   6}, { 12,   8}, { 14,   9}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 20,  12}, { 25,  15}, { 30,  18}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 39,  24}, { 52,  32}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 80,  50}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };
  ScoreType32 EvalCoefficients::kingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   1}, {  0,   3}, {  0,   6}, {  0,  10}, {  0,   0} };
  ScoreType32 EvalCoefficients::okingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   0}, {  0,   1}, {  0,   3}, {  0,   7}, {  0,  13}, {  0,  20}, {  0,   0} };
  ScoreType32 EvalCoefficients::okingFarFromPasserBonus_[8] = { {  0,   0}, {  0,   1}, {  0,   3}, {  0,   9}, {  0,  18}, {  0,  32}, {  0,  50}, {  0,   0} };
  // end of passer pawns


  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  ScoreType32 EvalCoefficients::knightMobility_[16] = { {-40, -40}, {-20, -22}, { -2, -3},  { 5,  6},  { 8,  9},  {11, 12}, {13, 14}, {15, 16}, {17, 18} };
  ScoreType32 EvalCoefficients::bishopMobility_[16] = { {-40, -40}, {-18, -20}, { -2, -3},  { 5,  6},  { 8,  9},  {10, 11}, {12, 13}, {13, 14}, {14, 15}, {15, 16}, {16, 17}, {17, 18}, {17, 18}, {18, 18}, {18, 18}, {18, 18} };
  ScoreType32 EvalCoefficients::rookMobility_[16] =   { {-45, -45}, {-18, -20}, { -2, -3},  { 5,  9},  { 8, 13},  {10, 16}, {12, 18}, {13, 20}, {14, 21}, {15, 22}, {16, 23}, {17, 24}, {17, 24}, {18, 24}, {18, 24}, {18, 24} };
  ScoreType32 EvalCoefficients::queenMobility_[32] =  { {-55, -55}, {-25, -25}, { -2, -3},  { 5,  9},  { 8, 13},  {10, 16}, {12, 18}, {14, 20}, {16, 22}, {18, 24}, {20, 26}, {22, 28}, {24, 30}, {26, 32}, {28, 34}, {30, 36},
                                                        { 32,  38}, { 34,  40}, { 36, 42},  {38, 44},  {40, 46},  {42, 48}, {44, 50}, {46, 52}, {48, 54}, {49, 55}, {50, 56}, {50, 56}, {50, 56}, {50, 56}, {50, 56}, {50, 56} };

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
        {  4,   4}, {  3,   4}, {  5,   4}, { 10,   6}, { 10,   6}, {  5,   5}, {  3,   4}, {  4,   4},
        {  4,   3}, {  3,   3}, {  9,   4}, { 12,   5}, { 12,   5}, {  9,   4}, {  3,   3}, {  4,   3},
        {  0,   2}, {  0,   2}, {  7,   3}, { 12,   4}, { 12,   4}, {  7,   3}, {  0,   2}, {  0,   2},
        {  4,   1}, {  4,   1}, {  5,   2}, { 10,   3}, { 10,   3}, {  5,   2}, {  4,   1}, {  4,   1},
        {  5,  -1}, {  5,  -2}, {  5,  -3}, { -8,  -4}, { -8,  -4}, {  5,  -3}, {  5,  -1}, {  5,  -1},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}
      },
      {
        {-60, -50}, {-30, -30}, {-18, -10}, {-12, -10}, {-12, -10}, {-18, -10}, {-30, -30}, {-60, -50},
        {-24, -25}, { -8, -10}, { -2,  -5}, {  6,   0}, {  6,   0}, { -2,  -5}, { -8, -10}, {-24, -25},
        { -9, -10}, { 12,  -5}, { 14,  10}, { 14,  10}, { 14,  10}, { 14,  10}, { 12,  -5}, { -9, -10},
        { -6, -10}, {  9,  -2}, { 10,   8}, { 18,  12}, { 18,  12}, { 10,   8}, {  9,  -2}, { -6, -10},
        { -6, -10}, {  3,   0}, {  9,   8}, { 18,  10}, { 18,  10}, {  9,   8}, {  3,   0}, { -6, -10},
        {-18, -10}, { -9,  -7}, { -6,  -4}, {  7,   6}, {  7,   6}, { -6,  -4}, { -9,  -7}, {-18, -10},
        {-24, -20}, {-10, -10}, { -9,  -6}, { -6,  -3}, { -6,  -3}, { -9,  -6}, {-10, -10}, {-24, -20},
        {-48, -30}, {-36, -15}, {-24, -10}, {-18, -10}, {-18, -10}, {-24, -10}, {-36, -15}, {-48, -30}
      },
      {
        {-15,  -8}, { -8,  -3}, { -8,  -2}, { -8,  -1}, { -8,  -1}, { -8,  -2}, { -8,  -3}, {-15,  -8},
        { -2,  -3}, {  5,   4}, {  3,   3}, {  0,   3}, {  0,   3}, {  3,   3}, {  5,   4}, { -2,  -3},
        {  0,  -2}, {  8,   3}, {  7,   5}, {  5,   5}, {  5,   5}, {  7,   5}, {  8,   3}, {  0,  -2},
        { -4,  -1}, {  7,   3}, { 10,   5}, { 12,   6}, { 12,   6}, { 10,   5}, {  7,   3}, { -4,  -1},
        { -2,  -1}, {  6,   3}, {  9,   5}, { 12,   6}, { 12,   6}, {  9,   5}, {  6,   3}, { -2,  -1},
        {  0,  -2}, {  8,   3}, { 10,   5}, {  8,   5}, {  8,   5}, { 10,   5}, {  8,   3}, {  0,  -2},
        {  0,  -3}, { 10,   4}, {  5,   3}, {  2,   3}, {  2,   3}, {  5,   3}, { 10,   4}, {  0,  -3},
        {-15,  -8}, { -5,  -3}, { -8,  -2}, { -9,  -1}, { -9,  -1}, { -8,  -2}, { -5,  -3}, {-15,  -8}
      },
      {
        {  2,  -3}, {  2,   2}, {  5,   4}, {  7,   4}, {  7,   4}, {  5,   4}, {  2,   2}, {  2,  -3},
        {  4,   1}, {  6,   2}, { 10,   5}, { 12,   5}, { 12,   5}, { 10,   5}, {  6,   2}, {  4,   1},
        {  2,   2}, {  2,   2}, {  6,   5}, {  8,   6}, {  8,   6}, {  6,   5}, {  2,   2}, {  2,   2},
        {  2,   2}, {  2,   3}, {  4,   6}, {  7,   6}, {  7,   6}, {  4,   6}, {  2,   3}, {  2,   2},
        {  2,   2}, {  2,   3}, {  3,   6}, {  5,   6}, {  5,   6}, {  3,   6}, {  2,   3}, {  2,   2},
        {  2,   2}, {  2,   3}, {  2,   5}, {  4,   6}, {  4,   6}, {  2,   5}, {  2,   3}, {  2,   2},
        { -5,   1}, { -2,   2}, {  1,   5}, {  2,   5}, {  2,   5}, {  1,   5}, { -2,   2}, { -5,   1},
        { -7,  -3}, { -7,   2}, { -5,   4}, { -3,   4}, { -3,   4}, { -5,   4}, { -7,   2}, { -7,  -3}
      },
      {
        {  0, -12}, {  1,  -6}, {  1,  -2}, {  3,  -2}, {  3,  -2}, {  1,  -2}, {  1,  -6}, {  0, -12},
        {  2,  -5}, {  3,  -2}, {  4,   1}, {  4,   3}, {  4,   3}, {  4,   1}, {  3,  -2}, {  2,  -5},
        { -1,  -6}, {  2,   0}, {  3,   5}, {  4,   6}, {  4,   6}, {  3,   5}, {  2,   0}, { -1,  -6},
        { -2,  -4}, {  2,   1}, {  4,   6}, {  5,   8}, {  5,   8}, {  4,   6}, {  2,   1}, { -2,  -4},
        { -2,  -4}, {  2,  -1}, {  4,   5}, {  5,   8}, {  5,   8}, {  4,   5}, {  2,  -1}, { -2,  -4},
        { -2,  -6}, {  2,  -2}, {  3,   3}, {  4,   4}, {  4,   4}, {  3,   3}, {  2,  -2}, { -2,  -6},
        { -3,  -8}, {  1,  -5}, {  1,  -1}, {  0,   1}, {  0,   1}, {  1,  -1}, {  1,  -5}, { -3,  -8},
        { -4, -15}, { -2,  -8}, { -1,  -5}, {  0,  -4}, {  0,  -4}, { -1,  -5}, { -2,  -8}, { -4, -15}
      },
      {
        {-60, -37}, {-60, -18}, {-60, -13}, {-60,  -9}, {-60,  -9}, {-60, -13}, {-60, -18}, {-60, -37},
        {-46, -25}, {-47, -13}, {-48,   2}, {-50,   4}, {-50,   4}, {-48,   2}, {-47, -13}, {-46, -25},
        {-37, -15}, {-39,   2}, {-41,  10}, {-43,  15}, {-43,  15}, {-41,  10}, {-39,   2}, {-37, -15},
        {-28, -15}, {-30,   2}, {-32,  15}, {-34,  20}, {-34,  20}, {-32,  15}, {-30,   2}, {-28, -15},
        {-20, -15}, {-23,   2}, {-27,  15}, {-29,  20}, {-29,  20}, {-27,  15}, {-23,   2}, {-20, -15},
        { -5, -15}, { -7,   2}, {-14,  10}, {-19,  15}, {-19,  15}, {-14,  10}, { -7,   2}, { -5, -15},
        { 22, -20}, { 28, -10}, {  2,   2}, {-14,   5}, {-14,   5}, {  2,   2}, { 28, -10}, { 22, -20},
        { 25, -40}, { 33, -25}, {  6, -15}, { -8, -10}, { -8, -10}, {  6, -15}, { 33, -25}, { 25, -40}
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
