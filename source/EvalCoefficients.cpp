/*************************************************************
EvalCoefficients.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include "Figure.h"
#include "fstream"
#include "set"
#include "algorithm"

namespace NEngine
{
  // single vars
  int EvalCoefficients::additionalMatBonus_{ 150 };

  // pawns
  ScoreType32 EvalCoefficients::protectedPawnPressure_{ 1, 2 };
  ScoreType32 EvalCoefficients::pawnPressureStrong_{ 13, 22 };
  ScoreType32 EvalCoefficients::pawnPressureMedium_{ 2, 5 };
  ScoreType32 EvalCoefficients::pawnPressureWeak_{ 1, 3 };
  ScoreType32 EvalCoefficients::pawnBishopTreat_{ 0, 3 };

  // outpost
  ScoreType32 EvalCoefficients::knightOutpost_[2] = { { 6, 2 }, { 10, 3 } };
  ScoreType32 EvalCoefficients::bishopOutpost_[2] = { {6, 2},  {10, 3} };

  // attacks
  ScoreType EvalCoefficients::bishopsAttackRQ_{ 40 };
  ScoreType EvalCoefficients::bishopsAttack_{ 30 };
  ScoreType EvalCoefficients::bishopsAttackWeak_{ 12 };
  ScoreType EvalCoefficients::knightAttackRQ_{ 40 };
  ScoreType EvalCoefficients::knightAttack_{ 30 };
  ScoreType EvalCoefficients::knightAttackWeak_{ 12 };
  ScoreType EvalCoefficients::pawnAttack_{ 60 };
  ScoreType EvalCoefficients::possibleKnightAttack_[4] = { 0, 8, 30, 40 };
  ScoreType EvalCoefficients::possiblePawnAttack_{ 12 };
  ScoreType EvalCoefficients::rookQueenAttackedBonus_{ 30 };
  ScoreType EvalCoefficients::queenUnderRookAttackBonus_{ 20 };
  ScoreType EvalCoefficients::multiattackedBonus_{ 25 };
  ScoreType EvalCoefficients::attackedByKingBonus_{ 20 };
  ScoreType EvalCoefficients::attackedThroughBonus_{ 20 };
  ScoreType EvalCoefficients::discoveredAttackBonus_{ 20 };

  // immobility
  ScoreType EvalCoefficients::immobileAttackBonus_ = 30;
  ScoreType EvalCoefficients::pinnedFigureBonus_ = 15;

  // check & mat treat
  ScoreType32 EvalCoefficients::discoveredCheckBonus_{ 20, 20 };

  // king
  ScoreType32 EvalCoefficients::fakeCastle_{ -60, 0 };

  // blocked figure
  ScoreType32 EvalCoefficients::knightBlocked_{ 60, 50 };
  ScoreType32 EvalCoefficients::bishopBlocked_{ 60, 50 };
  ScoreType32 EvalCoefficients::rookBlocked_{ 60, 60 };

  // king attacks
  int EvalCoefficients::pawnKingAttack_{ 15 };

  int EvalCoefficients::knightKingAttack_{ 20 };
  int EvalCoefficients::bishopKingAttack_{ 20 };
  int EvalCoefficients::rookKingAttack_{ 18 };
  int EvalCoefficients::queenKingAttack_{ 21 };

  int EvalCoefficients::generalKingPressure_{ 3 };

  // king threat
  int EvalCoefficients::knightChecking_{ 70 };
  int EvalCoefficients::bishopChecking_{ 30 };
  int EvalCoefficients::rookChecking_{ 85 };
  int EvalCoefficients::queenChecking_{ 85 };
  int EvalCoefficients::discoveredChecking_{ 40 };
  int EvalCoefficients::promotionChecking_{ 40 };
  int EvalCoefficients::weakChecking_{ 9 };
  int EvalCoefficients::queenCheckTreatBonus_{ 50 };
  
  int EvalCoefficients::attackedNearKingStrong_{ 35 };
  int EvalCoefficients::attackedNearKingWeak_{ 20 };
  int EvalCoefficients::attackedNearKingRem_{ 10 };
  int EvalCoefficients::attackedNearKingOther_{ 4 };
  int EvalCoefficients::attackedNearKingPawns_{ 3 };
  int EvalCoefficients::checkNearKingStrong_{ 35 };
  int EvalCoefficients::checkNearKingWeak_{ 20 };
  int EvalCoefficients::checkNearKingRem_{ 10 };
  int EvalCoefficients::checkNearKingOther_{ 4 };
  int EvalCoefficients::checkNearKingPawns_{ 3 };
  int EvalCoefficients::possibleMatTreat_{ 50 };
  int EvalCoefficients::possibleMatTreatMyMove_{ 50 };
  int EvalCoefficients::attackThroughPawn_{ 15 };

  int EvalCoefficients::kingWeakCheckersCoefficients_{ 16 };
  int EvalCoefficients::kingCheckersCoefficients_[8]  = { 0, 16, 32, 64, 64, 64, 64, 64 };
  int EvalCoefficients::kingAttackersCoefficients_[8] = { 0,  0,  4,  8, 16, 32, 56, 64 };

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 3 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 3 };

  // arrays
  ScoreType32 EvalCoefficients::doubledPawn_ = {-10, -8};
  ScoreType32 EvalCoefficients::isolatedPawn_[2] = { {-12, -10}, {-17, -12} };
  ScoreType32 EvalCoefficients::backwardPawn_[2] = { {-12, -10}, {-17, -12} };
  ScoreType32 EvalCoefficients::unprotectedPawn_ = {-6, -4};
  ScoreType32 EvalCoefficients::hasneighborPawn_ = {4, 4};
  ScoreType32 EvalCoefficients::attackingPawn_[8] = { { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 8, 10 }, { 14, 18 }, { 0, 0 }, { 0, 0 } };

  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 7, 1, 0, 0 };

  int EvalCoefficients::pawnShieldA_[4] = { 30, 26, 5, 0 };
  int EvalCoefficients::pawnShieldB_[4] = { 30, 26, 5, 0 };
  int EvalCoefficients::pawnShieldC_[4] = { 20, 16, 3, 0 };
  int EvalCoefficients::pawnShieldAbove_[4] = { 12, 5, 1, 0 };

  // new shield
  int EvalCoefficients::pawnsShields_[8][8] = {
    {-20, 32, 30, 15,-10, -20, -20, 0},
    {-20, 35, 30, 0, -15, -20, -20, 0},
    {-10, 32, 10, 0, -10, -10, -10, 0},
    {-10, 15,  5, 0, -10, -10, -10, 0},
    {-10, 15,  5, 0, -10, -10, -10, 0},
    {-10, 32, 10, 0, -10, -10, -10, 0},
    {-20, 35, 30, 0, -15, -20, -20, 0},
    {-20, 32, 30, 15,-10, -20, -20, 0}
  };
  int EvalCoefficients::opawnsShieldAttack_[2][8] = {
    {0, 3, 1, 0, 0, 0, 0, 0},
    {30, 20, 12, 8, 3, 2, 1, 0}
  };
  int EvalCoefficients::opawnsNearKing_[8] = { 0, 20, 15, 10, 6, 1, 0, 0 };
  int EvalCoefficients::opawnsAttackCoeffs_[8] = { 0, 32, 32, 32, 24, 16, 4, 0 };
  int EvalCoefficients::opawnAboveKing_[8] = { 0, 40, 25, 10, 0, 0, 0, 0 };

  // rook on open column
  ScoreType32 EvalCoefficients::openRook_[2] = { {20, 8}, {10, 4} };

  // material diff
  // opening, endgame
  ScoreType32 EvalCoefficients::doubleBishopBonus_[10] = { {0, 0}, {10, 10}, {12, 12}, {12, 12}, {12, 12}, {12, 12}, {12, 12}, {12, 12}, {12, 12}, {12, 12} };
  ScoreType32 EvalCoefficients::doubleKnightBonus_[10] = { {0, 0}, {3, 3}, {8, 8}, {8, 8}, {8, 8}, {8, 8}, {8, 8}, {8, 8}, {8, 8}, {8, 8} };
  ScoreType32 EvalCoefficients::twoKnightsBonus_[10] = { {0, 0}, {15, 10}, {20, 10}, {25, 15}, {25, 15}, {25, 15}, {25, 15}, {25, 15}, {25, 15}, {25, 15} };
  ScoreType32 EvalCoefficients::twoBishopsBonus_[10] = { {5, 5}, {25, 25}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35} };
  ScoreType32 EvalCoefficients::twoRooksBonus_[10] = { {0, 0}, {5, 5}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10} };
  ScoreType32 EvalCoefficients::figureAgainstPawnBonus_[10] = { {0, 0}, {25, 15}, {40, 25}, {45, 25}, {45, 25}, {45, 25}, {45, 25}, {45, 25}, {45, 25}, {45, 25} };
  ScoreType32 EvalCoefficients::figuresAgainstRookBonus_[10] = { {0, 0}, {30, 20}, {50, 40}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50} };
  ScoreType32 EvalCoefficients::knightsAgainstRookBonus_[10] = { {0, 0}, {12, 12}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20} };
  ScoreType32 EvalCoefficients::rookAgainstFigureBonus_[10][10] = {
    { { 60,  20}, { 58,  20}, { 57,  20}, { 56,  20}, { 56,  20}, { 55,  20}, { 55,  20}, { 55,  20}, { 55,  20}, {  0,   0} },
    { { 55,  17}, { 54,  17}, { 53,  17}, { 52,  17}, { 51,  17}, { 51,  17}, { 50,  17}, { 50,  17}, { 50,  17}, {  0,   0} },
    { { 51,  15}, { 50,  15}, { 49,  15}, { 48,  15}, { 47,  15}, { 46,  15}, { 46,  15}, { 46,  15}, { 46,  15}, {  0,   0} },
    { { 47,  13}, { 46,  13}, { 45,  13}, { 44,  13}, { 44,  13}, { 43,  13}, { 43,  13}, { 42,  13}, { 42,  13}, {  0,   0} },
    { { 45,  12}, { 43,  12}, { 42,  12}, { 41,  12}, { 41,  12}, { 40,  12}, { 40,  12}, { 40,  12}, { 40,  12}, {  0,   0} },
    { { 42,  11}, { 41,  11}, { 40,  11}, { 39,  11}, { 39,  11}, { 38,  11}, { 38,  11}, { 37,  11}, { 37,  11}, {  0,   0} },
    { { 41,  10}, { 40,  10}, { 39,  10}, { 38,  10}, { 37,  10}, { 36,  10}, { 36,  10}, { 36,  10}, { 36,  10}, {  0,   0} },
    { { 40,  10}, { 39,  10}, { 38,  10}, { 37,  10}, { 36,  10}, { 36,  10}, { 35,  10}, { 35,  10}, { 35,  10}, {  0,   0} },
    { { 40,  10}, { 38,  10}, { 37,  10}, { 36,  10}, { 36,  10}, { 35,  10}, { 35,  10}, { 35,  10}, { 35,  10}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };

  ScoreType32 EvalCoefficients::noKnightsPenalty_ = {5, 5};
  ScoreType32 EvalCoefficients::noBishopsPenalty_ = {8, 8};
  ScoreType32 EvalCoefficients::noRooksPenalty_ = {10, 10};
  ScoreType32 EvalCoefficients::noQueensPenalty_ = {15, 15};

  // passer pawns
  ScoreType32 EvalCoefficients::passerPawn_[8] = { {  0,   0}, {  1,   1}, {  1,   1}, {  8,   9}, { 24,  27}, { 48,  54}, { 80,  90}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawn2_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  4,   4}, { 12,  13}, { 24,  27}, { 40,  45}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnEx_[8] = { {  0,   0}, {  1,   2}, {  1,   2}, {  4,   5}, { 28,  35}, { 48,  60}, {112, 140}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerUnstoppable_[8] = { {  0,   0}, {  2,   3}, {  5,   8}, { 10,  17}, { 25,  42}, { 50,  85}, {100, 170}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnExS_[8][8] = {
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  0,   1}, {  1,   1}, {  1,   1}, {  1,   1}, {  1,   1}, {  1,   2}, {  0,   0} },
    { {  0,   0}, {  0,   1}, {  1,   1}, {  1,   1}, {  1,   1}, {  1,   2}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  2,   3}, {  3,   3}, {  3,   4}, {  4,   5}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 18,  23}, { 23,  29}, { 28,  35}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 36,  45}, { 48,  60}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {112, 140}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };
  ScoreType32 EvalCoefficients::passerPawnPGrds_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  1,   2}, {  5,   7}, { 10,  15}, { 18,  25}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnPGrds2_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   1}, {  2,   3}, {  5,   7}, {  9,  12}, {  0,   0} };
  ScoreType32 EvalCoefficients::kingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   0}, {  0,   1}, {  0,   3}, {  0,   6}, {  0,  11}, {  0,  17}, {  0,   0} };
  ScoreType32 EvalCoefficients::okingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   0}, {  0,   2}, {  0,   5}, {  0,  12}, {  0,  20}, {  0,  32}, {  0,   0} };
  ScoreType32 EvalCoefficients::kingToPasserDistanceBonus2_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   1}, {  0,   3}, {  0,   5}, {  0,   8}, {  0,   0} };
  ScoreType32 EvalCoefficients::okingToPasserDistanceBonus2_[8] = { {  0,   0}, {  0,   0}, {  0,   1}, {  0,   2}, {  0,   6}, {  0,  10}, {  0,  16}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnMyBefore_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  1,   1}, {  4,   5}, {  9,  10}, { 16,  18}, {  0,   0} };
  // end of passer pawns

  int EvalCoefficients::passerPawnSc_[8] = { 0, 1, 2, 4, 6, 8, 12, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 0, 1, 2, 3, 4, 5, 6 };

  // mobility
  ScoreType32 EvalCoefficients::knightMobility_[16] = { {-45, -45}, {-19, -18}, {  6,   8}, {  9,  11}, { 11,  15}, { 14,  17}, { 16,  20}, { 18,  22}, { 20,  25} };
  ScoreType32 EvalCoefficients::bishopMobility_[16] = { {-30, -30}, {-13, -12}, {  4,   6}, {  5,   8}, {  7,   9}, {  8,  11}, { 10,  13}, { 11,  14}, { 12,  16},
                                                        { 13,  17}, { 15,  19}, { 16,  20}, { 17,  21}, { 18,  22}, { 19,  23}, { 20,  25} };
  ScoreType32 EvalCoefficients::rookMobility_[16]   = { {-25, -45}, { -9, -17}, {  7,  10}, { 11,  15}, { 14,  21}, { 18,  26}, { 21,  31}, { 24,  35}, { 27,  40},
                                                        { 30,  44}, { 33,  48}, { 35,  51}, { 38,  55}, { 40,  58}, { 42,  61}, { 45,  65} };
  ScoreType32 EvalCoefficients::queenMobility_[32]  = { {-55, -55}, {-22, -21}, { 10,  12}, { 12,  14}, { 14,  17}, { 17,  20}, { 19,  22}, { 21,  25}, { 23,  27},
                                                        { 25,  30}, { 27,  32}, { 29,  34}, { 31,  36}, { 33,  38}, { 34,  40}, { 36,  42}, { 38,  44}, { 40,  46},
                                                        { 41,  48}, { 43,  50}, { 44,  52}, { 46,  54}, { 47,  55}, { 49,  57}, { 50,  59}, { 52,  60}, { 53,  62},
                                                        { 54,  64}, { 56,  65}, { 57,  67}, { 58,  68}, { 60,  70} };

  ScoreType32 EvalCoefficients::knightPinned_ = { 15, 15 };
  ScoreType32 EvalCoefficients::bishopPinned_ = { 10, 10 };
  ScoreType32 EvalCoefficients::rookPinned_ = { 15, 15 };
  ScoreType32 EvalCoefficients::queenPinned_ = { 20, 20 };

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
    { {0, 0}, {5, 0}, {4, 0}, {3, 0}, {1, 0}, {0, 0}, {-1, 0}, {-2, 0} },
    { {0, 0}, {5, 0}, {4, 0}, {3, 0}, {1, 0}, {0, 0}, {-1, 0}, {-2, 0} },
    { {0, 0}, {5, 0}, {4, 0}, {3, 0}, {1, 0}, {0, 0}, {-1, 0}, {-2, 0} },
    { {0, 0}, {5, 0}, {4, 0}, {3, 0}, {1, 0}, {0, 0}, {-1, 0}, {-2, 0} },
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
        {  2,  -1}, {  2,  -2}, {  2,  -3}, { -8,  -4}, { -8,  -4}, {  2,  -3}, {  2,  -2}, {  2,  -1},
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
        {-23,  -8}, { -8,  -3}, { -8,  -2}, { -8,  -1}, { -8,  -1}, { -8,  -2}, { -8,  -3}, {-23,  -8},
        { -2,  -3}, {  5,   4}, {  3,   3}, {  0,   3}, {  0,   3}, {  3,   3}, {  5,   4}, { -2,  -3},
        {  0,  -2}, {  8,   3}, {  7,   5}, {  5,   5}, {  5,   5}, {  7,   5}, {  8,   3}, {  0,  -2},
        { -4,  -1}, {  7,   3}, { 10,   5}, { 12,   6}, { 12,   6}, { 10,   5}, {  7,   3}, { -4,  -1},
        { -2,  -1}, {  6,   3}, {  9,   5}, { 12,   6}, { 12,   6}, {  9,   5}, {  6,   3}, { -2,  -1},
        {  0,  -2}, {  8,   3}, { 10,   5}, {  8,   5}, {  8,   5}, { 10,   5}, {  8,   3}, {  0,  -2},
        { -2,  -3}, { 10,   4}, {  5,   3}, {  2,   3}, {  2,   3}, {  5,   3}, { 10,   4}, { -2,  -3},
        {-20,  -8}, { -6,  -3}, { -8,  -2}, { -9,  -1}, { -9,  -1}, { -8,  -2}, { -6,  -3}, {-20,  -8}
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
        {-10, -15}, {-12,   2}, {-15,  10}, {-20,  15}, {-20,  15}, {-15,  10}, {-12,   2}, {-10, -15},
        { 25, -20}, { 28, -10}, {  5,   2}, {-14,   5}, {-14,   5}, {  5,   2}, { 28, -10}, { 25, -20},
        { 27, -40}, { 33, -25}, { 20, -15}, {-12, -10}, {-12, -10}, { 20, -15}, { 33, -25}, { 27, -40}
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
