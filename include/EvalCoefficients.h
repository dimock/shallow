/*************************************************************
EvalCoefficients.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <xcommon.h>

namespace NEngine
{

struct EvalCoefficients
{
  // single vars
  static int additionalMatBonus_;

  // pawns
  static ScoreType32 protectedPawnPressure_;
  static ScoreType32 unprotectedPawnPressure_;
  static ScoreType32 semiprotectedPawnPressure_;
  static ScoreType32 unprotectedPawnBishopTreat_;

  // forks & other treats
  static ScoreType bishopsAttackBonus_;
  static ScoreType knightForkBonus_;
  static ScoreType doublePawnAttack_;
  static ScoreType queenUnderRookAttackBonus_;
  static ScoreType generalAttackBonus_;
  static ScoreType generalAttackBonus4_;

  // immobility
  static ScoreType32 immobileAttackBonus_;

  // check & mat treat
  static ScoreType32 discoveredCheckBonus_;

  // king
  static ScoreType32 fakeCastle_;

  // blocked figure
  static ScoreType32 knightBlocked_;
  static ScoreType32 bishopBlocked_;
  static ScoreType32 rookBlocked_;

  // king attacks
  static int pawnKingAttack_;
  static int knightKingAttack_;
  static int bishopKingAttack_;
  static int rookKingAttack_;
  static int queenKingAttack_;
  static int basicAttack_;

  static int generalKingPressure_;
  static int generalOpponentPressure_;

    // for special cases
  static int kingToPawnDistanceMulti_;
  static int knightToPawnDistanceMulti_;
  static int kingToKingDistanceMulti_;
  static int figureToKingDistanceMulti_;

  // king threat
  static int knightChecking_;
  static int bishopChecking_;
  static int rookChecking_;
  static int queenChecking_;
  
  static int attackedNearKingCoeff_;
  static int kingCheckersCoefficients[8];
  static int kingAttackersCoefficients[8];
  static int kingPossibleMovesCoefficients[10];

  static ScoreType32 doubledPawn_;
  static ScoreType32 isolatedPawn_;
  static ScoreType32 backwardPawn_;
  static ScoreType32 blockedPawn_;
  static ScoreType32 unprotectedPawn_;
  static ScoreType32 hasneighborPawn_;
  
  static int opponentPawnPressure_[8];

  // rook on open column
  static ScoreType32 openRook_[2];

  // material diff
  static ScoreType32 doubleBishopBonus_[10];
  static ScoreType32 doubleKnightBonus_[10];
  static ScoreType32 twoKnightsBonus_[10];
  static ScoreType32 twoBishopsBonus_[10];
  static ScoreType32 twoRooksBonus_[10];
  static ScoreType32 figureAgainstPawnBonus_[10];
  static ScoreType32 figuresAgainstRookBonus_[10];
  static ScoreType32 knightsAgainstRookBonus_[10];
  static ScoreType32 rookAgainstPawnBonus_[10];
  static ScoreType32 rookAgainstFigureBonus_[10][10];

  static ScoreType32 noKnightsPenalty_;
  static ScoreType32 noBishopsPenalty_;
  static ScoreType32 noRooksPenalty_;
  static ScoreType32 noQueensPenalty_;

  static int pawnShieldA_[4];
  static int pawnShieldB_[4];
  static int pawnShieldC_[4];
  static int pawnShieldAbove_[4];

  // arrays
  static ScoreType32 passerPawn_[8];
  static ScoreType32 passerPawn2_[8];
  static ScoreType32 passerPawn4_[8];
  static ScoreType32 passerPawnEx_[2][8];
  static ScoreType32 passerPawnBasic_[2][8];
  static ScoreType32 kingToPasserDistanceBonus_[8];
  static ScoreType32 okingToPasserDistanceBonus_[8];
  static ScoreType32 okingFarFromPasserBonus_[8];
  static int passerPawnSc_[8]; // special cases only
  // distance between forwards
  static int closeToPromotion_[8];
  static int kingToPawnBonus_[8];

  // mobility
  static ScoreType32 knightMobility_[16];
  static ScoreType32 bishopMobility_[16];
  static ScoreType32 rookMobility_[16];
  static ScoreType32 queenMobility_[32];

  static ScoreType32 knightPinned_;
  static ScoreType32 bishopPinned_;
  static ScoreType32 rookPinned_;
  static ScoreType32 queenPinned_;

  static int bishopKnightMat_[64];

  // basic king pressure
  static ScoreType32 kingDistanceBonus_[8][8];

  // position evaluation. 0 - black color, 1 - white color; color,type,pos
  static ScoreType32 positionEvaluations_[2][8][64];

  static void initialize();
};

}
