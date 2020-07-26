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
  static int rookBehindBonus_;
  static int protectedPawnPressure_;
  static int unprotectedPawnPressure_;
  static int protectedPawnBishopTreat_;
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
  static int doubledPawn_[2];
  static int isolatedPawn_[2];
  static int backwardPawn_[2];
  static int unguardedPawn_[2];
  static int unprotectedPawn_[2];
  static int hasneighborPawn_[2];
  
  static int opponentPawnPressure_[8];

  // rook on open column
  static int openRook_[2][2];

  // material diff
  static int doubleBishopBonus_[2][2];
  static int twoKnightsBonus_[2][2];
  static int oneBishopBonus_[2][2];
  static int twoBishopsBonus_[2][2];
  static int twoRooksBonus_[2][2];
  static int figureAgainstPawnBonus_[2][2];
  static int rookAgainstFigureBonus_[2][2];
  static int figuresAgainstRookBonus_[2][2];
  static int rookAgainstPawnBonus_[2][2];

  static int pawnShieldA_[2];
  static int pawnShieldB_[2];
  static int pawnShieldC_[2];

  // arrays
  static int passerPawn_[8];
  static int passerPawnSc_[8]; // special cases only
  static int farKingPawn_[8];
  static int cangoPawn_[8];
  static int canpromotePawn_[8];
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

  // position evaluation. 0 - opening, 1 - endgame; color,type,pos
  static int positionEvaluations_[2][8][64];
};

}
