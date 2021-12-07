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
  ScoreType32 EvalCoefficients::pawnPressureStrong_{ 17, 22 };
  ScoreType32 EvalCoefficients::pawnPressureWeak_{ 4, 7 };
  ScoreType32 EvalCoefficients::pawnsOnBishopSquares_{ -2, -2 };

  // outpost
  ScoreType32 EvalCoefficients::knightOutpost_{ 10, 3 };
  ScoreType32 EvalCoefficients::bishopOutpost_{ 10, 3 };

  // forks
  ScoreType EvalCoefficients::bishopsAttackBonus_{ 32 };
  ScoreType EvalCoefficients::knightAttack_{ 32 };
  ScoreType EvalCoefficients::pawnAttack_{ 50 };
  ScoreType EvalCoefficients::possibleKnightAttack_{ 12 };
  ScoreType EvalCoefficients::possiblePawnAttack_{ 10 };
  ScoreType EvalCoefficients::rookQueenAttackedBonus_{ 30 };
  ScoreType EvalCoefficients::queenUnderRookAttackBonus_{ 20 };
  ScoreType EvalCoefficients::multiattackedBonus_{ 20 };
  ScoreType EvalCoefficients::attackedByKingBonus_{ 20 };

  // immobility
  ScoreType EvalCoefficients::immobileAttackBonus_ = 30;
  ScoreType EvalCoefficients::pinnedFigureBonus_ = 20;

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

  int EvalCoefficients::generalKingPressure_{ 4 };

  // king threat
  int EvalCoefficients::knightChecking_{ 60 };
  int EvalCoefficients::bishopChecking_{ 30 };
  int EvalCoefficients::rookChecking_{ 85 };
  int EvalCoefficients::queenChecking_{ 85 };
  int EvalCoefficients::weakChecking_{ 9 };
  int EvalCoefficients::discoveredChecking_{ 40 };
  
  int EvalCoefficients::checkMyMoveBonus_{ 10 };
  int EvalCoefficients::possibleMatTreat_{ 50 };
  int EvalCoefficients::attackedNearKingStrong_{ 40 };
  int EvalCoefficients::attackedNearKingWeak_{ 15 };
  int EvalCoefficients::attackedNearKingOther_{ 4 };
  int EvalCoefficients::attackedNearKingPawns_{ 3 };
  int EvalCoefficients::checkNearKingStrong_{ 30 };
  int EvalCoefficients::checkNearKingWeak_{ 15 };
  int EvalCoefficients::checkNearKingOther_{ 4 };
  int EvalCoefficients::checkNearKingPawns_{ 3 };

  int EvalCoefficients::kingWeakCheckersCoefficients_{ 16 };
  int EvalCoefficients::kingCheckersCoefficients_[8]  = { 0, 32, 64, 64, 64, 64, 64, 64 };
  int EvalCoefficients::kingAttackersCoefficients_[8] = { 0,  0, 16, 29, 48, 56, 60, 62 };
  int EvalCoefficients::kingPossibleMovesCoefficients_[10] = { 16,  8, 4, 2, 1, 0, 0, 0, 0, 0 };

  // for special cases
  int EvalCoefficients::kingToPawnDistanceMulti_{ 3 };
  int EvalCoefficients::knightToPawnDistanceMulti_{ 1 };
  int EvalCoefficients::kingToKingDistanceMulti_{ 3 };
  int EvalCoefficients::figureToKingDistanceMulti_{ 3 };

  // arrays
  ScoreType32 EvalCoefficients::doubledPawn_ = {-11, -9};
  ScoreType32 EvalCoefficients::disconnectedPawn_ = {-4, -4};
  ScoreType32 EvalCoefficients::isolatedPawn_[2] = { {-12, -10}, {-16, -13} };
  ScoreType32 EvalCoefficients::backwardPawn_ = {-12, -10};
  ScoreType32 EvalCoefficients::unprotectedPawn_ = {-5, -5};
  ScoreType32 EvalCoefficients::hasneighborPawn_ = {5, 5};
  ScoreType32 EvalCoefficients::attackingPawn_[8] = { { 0, 0 }, { 1, 1 }, { 2, 2 }, { 3, 3 }, { 8, 10 }, { 14, 18 }, { 0, 0 }, { 0, 0 } };

  int EvalCoefficients::opponentPawnPressure_[8] = { 20, 20, 15, 10, 7, 1, 0, 0 };

  int EvalCoefficients::pawnShieldA_[4] = { 30, 26, 5, 0 };
  int EvalCoefficients::pawnShieldB_[4] = { 30, 26, 5, 0 };
  int EvalCoefficients::pawnShieldC_[4] = { 20, 16, 3, 0 };
  int EvalCoefficients::pawnShieldAbove_[4] = { 12, 5, 1, 0 };

  // new shield
  int EvalCoefficients::pawnsShields_[8][8] = {
    {0, 32, 25, 15, 2, 1, 0, 0},
    {0, 35, 30, 3, 2, 1, 0, 0},
    {0, 32, 10, 3, 2, 1, 0, 0},
    {0, 12, 6, 2, 1, 0, 0, 0},
    {0, 12, 6, 2, 1, 0, 0, 0},
    {0, 32, 10, 3, 2, 1, 0, 0},
    {0, 35, 30, 3, 2, 1, 0, 0},
    {0, 32, 25, 15, 2, 1, 0, 0}
  };
  int EvalCoefficients::opawnsShieldAttack_[2][8] = {
    {0, 3, 1, 0, 0, 0, 0, 0},
    {20, 10, 6, 5, 3, 2, 1, 0}
  };
  int EvalCoefficients::opawnsNearKing_[8] = { 0, 10, 7, 5, 3, 1, 0, 0 };
  int EvalCoefficients::opawnsAttackCoeffs_[8] = { 0, 32, 32, 32, 16, 8, 4, 0 };
  int EvalCoefficients::opawnAboveKing_[8] = { 0, 40, 25, 10, 0, 0, 0, 0 };
  // rook on open column
  ScoreType32 EvalCoefficients::openRook_[2] = { {20, 8}, {10, 4} };

  // material diff
  // opening, endgame
  ScoreType32 EvalCoefficients::doubleBishopBonus_[10] = { {0, 0}, {10, 10}, {12, 12}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15}, {15, 15} };
  ScoreType32 EvalCoefficients::doubleKnightBonus_[10] = { {0, 0}, {3, 3}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5} };
  ScoreType32 EvalCoefficients::twoKnightsBonus_[10] = { {0, 0}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5}, {5, 5} };
  ScoreType32 EvalCoefficients::twoBishopsBonus_[10] = { {5, 5}, {25, 25}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35}, {35, 35} };
  ScoreType32 EvalCoefficients::twoRooksBonus_[10] = { {0, 0}, {5, 5}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10}, {10, 10} };
  ScoreType32 EvalCoefficients::figureAgainstPawnBonus_[10] = { {0, 0}, {25, 10}, {35, 20}, {40, 20}, {40, 20}, {40, 20}, {40, 20}, {40, 20}, {40, 20}, {40, 20} };
  ScoreType32 EvalCoefficients::figuresAgainstRookBonus_[10] = { {0, 0}, {30, 20}, {50, 40}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50}, {60, 50} };
  ScoreType32 EvalCoefficients::knightsAgainstRookBonus_[10] = { {0, 0}, {12, 12}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20}, {25, 20} };
  ScoreType32 EvalCoefficients::rookAgainstFigureBonus_[10][10] = {
    { { 70,  20}, { 67,  20}, { 65,  20}, { 63,  20}, { 62,  20}, { 61,  20}, { 60,  20}, { 60,  20}, { 60,  20}, {  0,   0} },
    { { 62,  15}, { 60,  15}, { 58,  15}, { 56,  15}, { 54,  15}, { 53,  15}, { 52,  15}, { 51,  15}, { 51,  15}, {  0,   0} },
    { { 56,  11}, { 54,  11}, { 51,  11}, { 49,  11}, { 47,  11}, { 46,  11}, { 45,  11}, { 44,  11}, { 44,  11}, {  0,   0} },
    { { 51,   7}, { 48,   7}, { 46,   7}, { 43,   7}, { 41,   7}, { 40,   7}, { 39,   7}, { 38,   7}, { 38,   7}, {  0,   0} },
    { { 47,   5}, { 44,   5}, { 41,   5}, { 39,   5}, { 37,   5}, { 35,   5}, { 34,   5}, { 33,   5}, { 33,   5}, {  0,   0} },
    { { 44,   2}, { 40,   2}, { 37,   2}, { 35,   2}, { 33,   2}, { 31,   2}, { 30,   2}, { 30,   2}, { 29,   2}, {  0,   0} },
    { { 41,   1}, { 38,   1}, { 35,   1}, { 32,   1}, { 30,   1}, { 29,   1}, { 28,   1}, { 27,   1}, { 27,   1}, {  0,   0} },
    { { 40,   0}, { 36,   0}, { 33,   0}, { 31,   0}, { 29,   0}, { 27,   0}, { 26,   0}, { 25,   0}, { 25,   0}, {  0,   0} },
    { { 40,   0}, { 36,   0}, { 33,   0}, { 30,   0}, { 28,   0}, { 27,   0}, { 25,   0}, { 25,   0}, { 25,   0}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };

  ScoreType32 EvalCoefficients::noKnightsPenalty_ = {5, 5};
  ScoreType32 EvalCoefficients::noBishopsPenalty_ = {8, 8};
  ScoreType32 EvalCoefficients::noRooksPenalty_ = {10, 10};
  ScoreType32 EvalCoefficients::noQueensPenalty_ = {15, 15};

  // passer pawns
  ScoreType32 EvalCoefficients::passerPawn_[8] = { {  0,   0}, {  1,   1}, {  1,   1}, {  7,   8}, { 22,  24}, { 45,  48}, { 75,  80}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawn2_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  3,   4}, { 11,  12}, { 22,  24}, { 37,  40}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnEx_[8] = { {  0,   0}, {  1,   2}, {  1,   2}, {  9,  10}, { 27,  30}, { 63,  70}, {126, 140}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnExS_[8][8] = {
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  1,   1}, {  1,   1}, {  1,   1}, {  1,   1}, {  1,   1}, {  1,   2}, {  0,   0} },
    { {  0,   0}, {  1,   1}, {  1,   1}, {  1,   1}, {  1,   1}, {  1,   2}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  5,   6}, {  6,   7}, {  7,   8}, {  9,  10}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 18,  20}, { 22,  25}, { 27,  30}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, { 47,  52}, { 63,  70}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {126, 140}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} },
    { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0} }
  };
  ScoreType32 EvalCoefficients::passerPawnNbs_[8] = { {  0,   0}, {  0,   1}, {  0,   1}, {  3,   6}, {  9,  18}, { 18,  36}, { 30,  60}, {  0,   0} };
  ScoreType32 EvalCoefficients::passerPawnNbs2_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  1,   3}, {  4,   9}, {  9,  18}, { 15,  30}, {  0,   0} };
  ScoreType32 EvalCoefficients::kingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   1}, {  0,   4}, {  0,   9}, {  0,  15}, {  0,   0} };
  ScoreType32 EvalCoefficients::okingToPasserDistanceBonus_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   2}, {  0,   9}, {  0,  18}, {  0,  30}, {  0,   0} };
  ScoreType32 EvalCoefficients::kingToPasserDistanceBonus2_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   2}, {  0,   4}, {  0,   7}, {  0,   0} };
  ScoreType32 EvalCoefficients::okingToPasserDistanceBonus2_[8] = { {  0,   0}, {  0,   0}, {  0,   0}, {  0,   0}, {  0,   4}, {  0,   9}, {  0,  15}, {  0,   0} };
  // end of passer pawns

  int EvalCoefficients::passerPawnSc_[8] = { 0, 3, 6, 9, 12, 15, 18, 0 };
  int EvalCoefficients::closeToPromotion_[8] = { 0, 2, 4, 6, 8, 10, 12, 16 };
  int EvalCoefficients::kingToPawnBonus_[8] = { 0, 6, 5, 4, 3, 2, 1, 0 };

  // mobility
  ScoreType32 EvalCoefficients::knightMobility_[16] = { {-45, -45}, {-22, -22}, {  0,   0}, {  4,   5}, {  8,  10}, { 11,  14}, { 14,  18}, { 17,  21}, { 20,  25} };
  ScoreType32 EvalCoefficients::bishopMobility_[16] = { {-40, -40}, {-20, -20}, {  0,   0}, {  2,   2}, {  4,   5}, {  5,   7}, {  7,   9}, {  9,  11}, { 10,  13},
                                                        { 12,  15}, { 13,  17}, { 15,  18}, { 16,  20}, { 17,  22}, { 18,  23}, { 20,  25} };
  ScoreType32 EvalCoefficients::rookMobility_[16]   = { {-25, -45}, {-12, -22}, {  0,   0}, {  4,   6}, {  9,  13}, { 13,  19}, { 17,  25}, { 21,  30}, { 24,  35},
                                                        { 27,  40}, { 31,  44}, { 34,  49}, { 37,  53}, { 39,  57}, { 42,  61}, { 45,  65} };
  ScoreType32 EvalCoefficients::queenMobility_[32]  = { {-55, -55}, {-27, -27}, {  0,   0}, {  2,   3}, {  5,   6}, {  8,   9}, { 11,  13}, { 13,  16}, { 16,  18},
                                                        { 18,  21}, { 21,  24}, { 23,  27}, { 25,  29}, { 27,  32}, { 29,  34}, { 32,  37}, { 34,  39}, { 36,  42},
                                                        { 38,  44}, { 39,  46}, { 41,  48}, { 43,  50}, { 45,  52}, { 47,  55}, { 48,  57}, { 50,  58}, { 52,  60},
                                                        { 53,  62}, { 55,  64}, { 56,  66}, { 58,  68}, { 60,  70} };

  ScoreType32 EvalCoefficients::knightPinned_ = { 30, 30 };
  ScoreType32 EvalCoefficients::bishopPinned_ = { 20, 20 };
  ScoreType32 EvalCoefficients::rookPinned_ = { 20, 20 };
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
    { {0, 0}, {7, 0}, {6, 0}, {5, 0}, {3, 0}, {1, 0}, {0, 0}, {0, 0} },
    { {0, 0}, {7, 0}, {6, 0}, {5, 0}, {3, 0}, {1, 0}, {0, 0}, {0, 0} },
    { {0, 0}, {7, 0}, {6, 0}, {5, 0}, {3, 0}, {1, 0}, {0, 0}, {0, 0} },
    { {0, 0}, {7, 0}, {6, 0}, {5, 0}, {3, 0}, {1, 0}, {0, 0}, {0, 0} },
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
