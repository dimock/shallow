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
  static int queenRookVsPwPenalty_;

  // pawns
  static int pawnEndgameBonus_;
  static ScoreType32 rookBehindBonus_;
  static int protectedPawnPressure_;
  static int unprotectedPawnPressure_;
  static int semiprotectedPawnPressure_;
  static int unprotectedPawnBishopTreat_;
  static int kingToPasserDistanceBonus_;

  // forks & other treats
  static int bishopsAttackBonus_;
  static int knightForkBonus_;
  static int doublePawnAttack_;
  static int queenUnderRookAttackBonus_;
  static int generalAttackBonus_;
  static int immobileAttackBonus_;
  static int discoveredCheckBonus_;
  static int possibleCheckBonus_;
  static int matTreatBonus_;

  // king
  static int fakeCastle_;
  static int kingIsUnsafe_;
  static int pawnPenaltyA_;
  static int pawnPenaltyB_;
  static int pawnPenaltyC_;

  // blocked figure
  static int knightBlocked_;
  static int bishopBlocked_;
  static int rookBlocked_;

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
  static int checkedFieldBonus_;

  static int attackedNearKing_;

  // arrays
  static ScoreType32 doubledPawn_;
  static ScoreType32 isolatedPawn_;
  static ScoreType32 backwardPawn_;
  static ScoreType32 unguardedPawn_;
  static ScoreType32 unprotectedPawn_;
  static ScoreType32 hasneighborPawn_;
  
  static int opponentPawnPressure_[8];

  // rook on open column
  static int openRook_[2][2];

  // material diff
  static ScoreType32 doubleBishopBonus_[10];
  static ScoreType32 twoKnightsBonus_[10];
  static ScoreType32 oneBishopBonus_[10];
  static ScoreType32 twoBishopsBonus_[10];
  static ScoreType32 twoRooksBonus_[10];
  static ScoreType32 figureAgainstPawnBonus_[10];
  static ScoreType32 rookAgainstFigureBonus_[10];
  static ScoreType32 figuresAgainstRookBonus_[10];
  static ScoreType32 rookAgainstPawnBonus_[10];

  static int pawnShieldA_[2];
  static int pawnShieldB_[2];
  static int pawnShieldC_[2];

  // arrays
  static ScoreType32 passerPawn_[8];
  static ScoreType32 passerPawn2_[8];
  static ScoreType32 passerPawn4_[8];
  static ScoreType32 passerPawn38_[8];
  static ScoreType32 passerPawnEx_[8];
  static int passerPawnSc_[8]; // special cases only
  static int farKingPawn_[8];
  static int cangoPawn_[8];
  static ScoreType32 canpromotePawn_[8];
  // distance between forwards
  static int closeToPromotion_[8];
  static int kingToPawnBonus_[8];

  // mobility
  static int knightMobility_[16];
  static int bishopMobility_[16];
  static int rookMobility_[16];
  static int queenMobility_[32];

  static int bishopKnightMat_[64];

  // basic king pressure
  static int kingDistanceBonus_[8][8];

  // position evaluation. 0 - black color, 1 - white color; color,type,pos
  static ScoreType32 positionEvaluations_[2][8][64];

  static void initialize();
};

}
