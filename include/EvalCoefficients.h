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
  // pawns
  static int pawnEndgameBonus_;
  static int doubledPawn_;
  static int isolatedPawn_;
  static int backwardPawn_;
  static int unsupportedPawn_;
  static int unprotectedPawn_;
  static int rookBehindBonus_;
  static int protectedPawnPressure_;
  static int unprotectedPawnPressure_;
  static int protectedPawnBishopTreat_;
  static int unprotectedPawnBishopTreat_;
  static int kingPressure_;
  static int generalPressure_;

  // forks
  static int forkBonus_;
  static int doublePawnAttack_;

  // king
  static int castleImpossible_;
  static int fakeCastle_;
  static int castleBonus_;
  static int pawnPenaltyA_;
  static int pawnPenaltyB_;
  static int pawnPenaltyC_;
  static int noPawnPenaltyA_;
  static int noPawnPenaltyB_;
  static int noPawnPenaltyC_;

  // blocked figure
  static int knightBlocked_;
  static int bishopBlocked_;

  // king attacks
  static int pawnKingAttack_;
  static int knightKingAttack_;
  static int bishopKingAttack_;
  static int rookKingAttack_;
  static int queenKingAttack_;

  // x-ray attacks
  static int rookKingAttackXray_;
  static int queenKingAttackXray_;

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

  // could be captured with check
  static int couldCaptureWithCheck_;

  // pinned figures
  static int pinnedPawn_;
  static int pinnedKnight_;
  static int pinnedBishop_;
  static int pinnedRook_;
  static int pinnedQueen_;

  // arrays
  static int opponentPawnPressure_[8];

  // rook on open column
  static int openRook_[4];

  // material diff
  static int bishopBonus_[4];
  static int figureAgainstPawnBonus_[2];
  static int rookAgainstFigureBonus_[2];
  static int figuresAgainstRookBonus_[2];
  static int rookAgainstPawnBonus_[2];

  static int pawnShieldA_[2];
  static int pawnShieldB_[2];
  static int pawnShieldC_[2];

  // arrays
  static int passerPawn_[8];
  static int multipasserPawn_[8];
  static int passerPawnSc_[8]; // special cases only
  static int semipasserPawn_[8];
  static int passerCandidatePawn_[8];
  static int protectedPasser_[8];
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
