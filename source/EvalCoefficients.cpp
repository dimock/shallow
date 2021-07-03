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
  ScoreType32 EvalCoefficients::protectedPawnPressure_{ 1, 2 };
  ScoreType32 EvalCoefficients::pawnPressureStrong_{ 6, 16 };
  ScoreType32 EvalCoefficients::pawnPressureWeak_{ 3, 8 };
  ScoreType32 EvalCoefficients::pawnBishopTreat_{ 0, 3 };


  // outpost
  ScoreType32 EvalCoefficients::knightOutpost_{ 10, 3 };

  // forks
  ScoreType EvalCoefficients::bishopsAttackBonus_{ 22 };
  ScoreType EvalCoefficients::knightAttack_{ 32 };
  ScoreType EvalCoefficients::pawnAttack_{ 40 };
  ScoreType EvalCoefficients::rookQueenAttackedBonus_{ 20 };
  ScoreType EvalCoefficients::queenUnderRookAttackBonus_{ 30 };
  ScoreType EvalCoefficients::rookUnderRookAttackBonus_{ 20 };
  ScoreType EvalCoefficients::multiattackedBonus_{ 25 };
  ScoreType EvalCoefficients::attackedByKingBonus_{ 20 };
  ScoreType EvalCoefficients::pinnedFigureAttack_{ 10 };

  // immobility
  ScoreType32 EvalCoefficients::immobileAttackBonus_[4] = { { 30, 30 }, { 30, 30 }, { 30, 30 }, { 30, 30 } };

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
  int EvalCoefficients::knightKingAttack_{ 17 };
  int EvalCoefficients::bishopKingAttack_{ 17 };
  int EvalCoefficients::rookKingAttack_{ 18 };
  int EvalCoefficients::queenKingAttack_{ 21 };
  int EvalCoefficients::basicAttack_{ 3 };
#endif

  int EvalCoefficients::strongKingPressure_{ 7 };
  int EvalCoefficients::weakKingPressure_{ 2 };

  // king threat
  int EvalCoefficients::knightChecking_{ 40 };
  int EvalCoefficients::bishopChecking_{ 40 };
  int EvalCoefficients::rookChecking_{ 70 };
  int EvalCoefficients::queenChecking_{ 85 };
  int EvalCoefficients::weakChecking_{ 9 };
  int EvalCoefficients::discoveredChecking_{ 40 };
  
  int EvalCoefficients::checkMyMoveBonus_{ 15 };
  int EvalCoefficients::possibleMatTreat_{ 50 };
  int EvalCoefficients::attackedNearKingCoeff_{ 40 };
  int EvalCoefficients::kingWeakCheckersCoefficients_{ 16 };
  int EvalCoefficients::kingCheckersCoefficients_[8]  = { 0, 32, 48, 64, 64, 64, 64, 64 };
  int EvalCoefficients::kingAttackersCoefficients_[8] = { 0,  0, 16, 32, 48, 56, 60, 62 };
  int EvalCoefficients::kingPossibleMovesCoefficients_[10] = { 16,  8, 4, 2, 1, 0, 0, 0, 0, 0 };

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 3 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 3 };

  // arrays
  ScoreType32 EvalCoefficients::doubledPawn_ = {-10, -8};
  ScoreType32 EvalCoefficients::isolatedPawn_[2] = { {-10, -8}, {-16, -12} };
  ScoreType32 EvalCoefficients::backwardPawn_[8] = { {0, 0}, {-16, -12}, {-14, -10}, {-12, -10}, {-10, -8}, {-10, -8}, {0, 0}, {0, 0} };
  ScoreType32 EvalCoefficients::protectedPawn_[8] = { {0, 0}, {0, 0}, {4, 3}, {6, 4}, {8, 6}, {10, 8}, {10, 8}, {0, 0} };
  ScoreType32 EvalCoefficients::hasneighborPawn_[8] = { {0, 0}, {3, 2}, {3, 2}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {0, 0} };

  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 7, 1, 0, 0 };

  int EvalCoefficients::pawnShieldA_[4] = { 30, 26, 5, 0 };
  int EvalCoefficients::pawnShieldB_[4] = { 30, 26, 5, 0 };
  int EvalCoefficients::pawnShieldC_[4] = { 20, 16, 3, 0 };
  int EvalCoefficients::pawnShieldAbove_[4] = { 12, 5, 1, 0 };

  // rook on open column
  ScoreType32 EvalCoefficients::openRook_[2] = { {20, 8}, {10, 4} };

  // material diff
  // opening, endgame
  ScoreType32 EvalCoefficients::doubleBishopBonus_[10] = { {0, 0}, {10, 10}, {12, 12}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15} };
  ScoreType32 EvalCoefficients::doubleKnightBonus_[10] = { {0, 0}, {3, 3}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5} };
  ScoreType32 EvalCoefficients::twoKnightsBonus_[10] = { {0, 0}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5} };
  ScoreType32 EvalCoefficients::twoBishopsBonus_[4][10] = {
    { {5, 5}, {25, 25}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35} },
    { {5, 5}, {15, 15}, {25, 25}, {25, 25}, {25, 25}, {25, 25}, {25, 25}, {25, 25}, {25, 25}, {25, 25} },
    { {0, 0}, {10, 10}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15} },
    { {0, 0}, {10, 10}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15} }
  };
  ScoreType32 EvalCoefficients::twoRooksBonus_[10] = { {0, 0}, {5, 5}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10} };
  ScoreType32 EvalCoefficients::figureAgainstPawnBonus_[10] = { {0, 0}, {35, 30}, {50, 50}, {55, 55}, {55, 55}, {55, 55}, {55, 55}, {55, 55}, {55, 55}, {55, 55} };
  ScoreType32 EvalCoefficients::figuresAgainstRookBonus_[10] = { {0, 0}, {30, 20}, {50, 40}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50} };
  ScoreType32 EvalCoefficients::knightsAgainstRookBonus_[10] = { {0, 0}, {20, 20}, {30, 20}, {40, 30}, {40, 30}, {40, 30}, {40, 30}, {40, 30}, {40, 30}, {40, 30} };
  ScoreType32 EvalCoefficients::rookAgainstFigureBonus_[10][10] = {
    { { 40,  25}, { 38,  21}, { 37,  18}, { 36,  15}, { 36,  13}, { 35,  12}, { 35,  10}, { 35,  10}, { 35,  10}, {  0,   0} },
    { { 36,  21}, { 35,  18}, { 33,  15}, { 32,  13}, { 31,  11}, { 31,  10}, { 30,   9}, { 30,   9}, { 30,   8}, {  0,   0} },
    { { 33,  18}, { 31,  15}, { 30,  13}, { 29,  11}, { 28,  10}, { 27,   9}, { 26,   8}, { 26,   7}, { 26,   7}, {  0,   0} },
    { { 30,  15}, { 28,  13}, { 27,  11}, { 25,  10}, { 24,   9}, { 23,   8}, { 23,   7}, { 22,   7}, { 22,   6}, {  0,   0} },
    { { 28,  13}, { 26,  11}, { 24,  10}, { 23,   9}, { 22,   8}, { 21,   7}, { 20,   6}, { 20,   6}, { 20,   6}, {  0,   0} },
    { { 27,  12}, { 24,  10}, { 23,   9}, { 21,   8}, { 20,   7}, { 19,   6}, { 18,   6}, { 17,   5}, { 17,   5}, {  0,   0} },
    { { 25,  10}, { 23,   9}, { 21,   8}, { 20,   7}, { 18,   6}, { 17,   6}, { 16,   5}, { 16,   5}, { 16,   5}, {  0,   0} },
    { { 25,  10}, { 22,   9}, { 20,   7}, { 19,   7}, { 17,   6}, { 16,   5}, { 15,   5}, { 15,   5}, { 15,   5}, {  0,   0} },
    { { 25,  10}, { 22,   8}, { 20,   7}, { 18,   6}, { 17,   6}, { 16,   5}, { 15,   5}, { 15,   5}, { 15,   5}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };

  ScoreType32 EvalCoefficients::noKnightsPenalty_ = {5, 5};
  ScoreType32 EvalCoefficients::noBishopsPenalty_ = {8, 8};
  ScoreType32 EvalCoefficients::noRooksPenalty_ = {10, 10};
  ScoreType32 EvalCoefficients::noQueensPenalty_ = {15, 15};

  // passer pawns
  ScoreType32 EvalCoefficients::passerPawn_[8] = { {  0,   0}, {  2,   2}, {  5,   5}, { 13,  15}, { 28,  31}, { 48,  53}, { 75,  82}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawn2_[8] = { {  0,   0}, {  1,   1}, {  2,   2}, {  6,   7}, { 14,  15}, { 24,  26}, { 37,  41}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawn4_[8] = { {  0,   0}, {  0,   0}, {  1,   1}, {  3,   3}, {  7,   7}, { 12,  13}, { 18,  20}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnEx_[8] = { {  0,   0}, {  1,   3}, {  2,   6}, {  7,  18}, { 15,  37}, { 33,  84}, { 64, 160}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnFwd_[8] = { {  0,   0}, {  0,   0}, {  0,   1}, {  1,   2}, {  2,   5}, {  5,  12}, {  9,  24}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnNatt_[8] = { {  0,   0}, {  0,   0}, {  0,   1}, {  1,   2}, {  2,   5}, {  3,   9}, {  6,  15}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnMyMove_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   1}, {  1,   3}, {  2,   6}, {  4,  10}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnBasic_[8] = { {  0,   0}, {  0,   0}, {  0,   1}, {  2,   3}, {  4,   6}, {  7,  11}, { 12,  17}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnExS_[8][8] = {
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  0,   1}, {  0,   2}, {  0,   2}, {  1,   2}, {  1,   2}, {  1,   3}, {  0,   0} },
    { {  0,   0}, {  1,   4}, {  1,   4}, {  2,   5}, {  2,   6}, {  2,   6}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  4,  11}, {  5,  13}, {  6,  16}, {  7,  18}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 10,  25}, { 12,  31}, { 15,  37}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 25,  63}, { 33,  84}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 64, 160}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };
  ScoreType32 EvalCoefficients::kingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   1}, {  0,   3}, {  0,   6}, {  0,  10}, {  0,   0} };
  ScoreType32 EvalCoefficients::okingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   0}, {  0,   1}, {  0,   4}, {  0,   9}, {  0,  16}, {  0,  25}, {  0,   0} };
  // end of passer pawns


  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  ScoreType32 EvalCoefficients::knightMobility_[16] = { {-40, -40}, {-12, -15}, { 4,  5},  { 6,  7},  { 8, 10},  {11, 12}, {13, 14}, {15, 16}, {17, 18} };
  ScoreType32 EvalCoefficients::bishopMobility_[16] = { {-40, -40}, {-12, -15}, { 4,  5},  { 6,  7},  { 8, 10},  {10, 11}, {12, 13}, {13, 14}, {14, 15}, {15, 16}, {16, 17}, {17, 18}, {17, 18}, {18, 18}, {18, 18}, {18, 18} };
  ScoreType32 EvalCoefficients::rookMobility_[16] =   { {-45, -45}, {-12, -15}, { 4, 10},  { 6, 15},  { 8, 20},  {10, 25}, {12, 30}, {13, 35}, {14, 35}, {15, 40}, {16, 40}, {17, 40}, {17, 40}, {18, 40}, {18, 40}, {18, 40} };
  ScoreType32 EvalCoefficients::queenMobility_[32] =  { {-55, -55}, {-20, -20}, { 6, 10},  {10, 15},  {12, 20},  {14, 25}, {16, 30}, {18, 35}, {20, 35}, {22, 40}, {22, 40}, {22, 40}, {24, 40}, {26, 40}, {28, 40}, {30, 45},
                                                        { 32,  45}, { 34,  45}, {36, 50},  {38, 50},  {40, 50},  {42, 55}, {44, 55}, {46, 55}, {48, 60}, {49, 60}, {50, 60}, {50, 60}, {50, 60}, {50, 60}, {50, 60}, {50, 60} };

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
        {  4,   4}, {  4,   4}, {  5,   4}, { 10,   6}, { 10,   6}, {  5,   5}, {  4,   4}, {  4,   4},
        {  4,   3}, {  4,   3}, {  9,   4}, { 12,   5}, { 12,   5}, {  9,   4}, {  4,   3}, {  4,   3},
        {  3,   2}, {  3,   2}, {  7,   3}, { 12,   4}, { 12,   4}, {  7,   3}, {  3,   2}, {  3,   2},
        {  3,   1}, {  3,   1}, {  5,   2}, { 10,   3}, { 10,   3}, {  5,   2}, {  3,   1}, {  3,   1},
        {  2,  -1}, {  2,  -2}, {  2,  -3}, { -8,  -4}, { -8,  -4}, {  2,  -3}, {  2,  -1}, {  2,  -1},
        {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}
      },
      {
        {-60, -50}, {-30, -30}, {-18, -10}, {-12, -10}, {-12, -10}, {-18, -10}, {-30, -30}, {-60, -50},
        {-24, -25}, { -8, -10}, { -2,  -5}, {  6,   0}, {  6,   0}, { -2,  -5}, { -8, -10}, {-24, -25},
        { -9, -10}, { 12,  -5}, { 14,  10}, { 14,  10}, { 14,  10}, { 14,  10}, { 12,  -5}, { -9, -10},
        { -6, -10}, {  9,  -2}, { 10,   8}, { 18,  12}, { 18,  12}, { 10,   8}, {  9,  -2}, { -6, -10},
        { -6, -10}, {  3,   0}, {  9,   8}, { 18,  10}, { 18,  10}, {  9,   8}, {  3,   0}, { -6, -10},
        {-18, -10}, { -9,  -7}, { -6,  -4}, {  4,   6}, {  4,   6}, { -6,  -4}, { -9,  -7}, {-18, -10},
        {-24, -20}, {-10, -10}, { -9,  -6}, { -9,  -3}, { -9,  -3}, { -9,  -6}, {-10, -10}, {-24, -20},
        {-48, -30}, {-36, -15}, {-24, -10}, {-18, -10}, {-18, -10}, {-24, -10}, {-36, -15}, {-48, -30}
      },
      {
        {-15,  -8}, { -8,  -3}, { -8,  -2}, { -8,  -1}, { -8,  -1}, { -8,  -2}, { -8,  -3}, {-15,  -8},
        { -2,  -3}, {  5,   4}, {  3,   3}, {  0,   3}, {  0,   3}, {  3,   3}, {  5,   4}, { -2,  -3},
        {  2,  -2}, {  8,   3}, {  7,   5}, {  5,   5}, {  5,   5}, {  7,   5}, {  8,   3}, {  2,  -2},
        {  2,  -1}, {  7,   3}, { 10,   5}, { 10,   6}, { 10,   6}, { 10,   5}, {  7,   3}, {  2,  -1},
        {  0,  -1}, {  6,   3}, {  9,   5}, { 10,   6}, { 10,   6}, {  9,   5}, {  6,   3}, {  0,  -1},
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
        {-60, -40}, {-60, -25}, {-60, -15}, {-60, -10}, {-60, -10}, {-60, -15}, {-60, -25}, {-60, -40},
        {-46, -25}, {-47, -15}, {-48,   2}, {-50,   5}, {-50,   5}, {-48,   2}, {-47, -15}, {-46, -25},
        {-37, -15}, {-39,   2}, {-41,  10}, {-43,  15}, {-43,  15}, {-41,  10}, {-39,   2}, {-37, -15},
        {-28, -10}, {-30,   5}, {-32,  15}, {-34,  20}, {-34,  20}, {-32,  15}, {-30,   5}, {-28, -10},
        {-20, -10}, {-23,   5}, {-27,  15}, {-29,  20}, {-29,  20}, {-27,  15}, {-23,   5}, {-20, -10},
        { -5, -15}, { -7,   2}, {-14,  10}, {-19,  15}, {-19,  15}, {-14,  10}, { -7,   2}, { -5, -15},
        { 22, -25}, { 28, -15}, {  2,   2}, {-14,   5}, {-14,   5}, {  2,   2}, { 28, -15}, { 22, -25},
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
