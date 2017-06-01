/*************************************************************
EvalCoefficients.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <xcommon.h>

namespace NEngine
{

namespace details
{
  struct Var
  {
    std::string name_;
    int initial_;
    int* pvar_;
  };

  struct Arr
  {
    std::string name_;
    std::vector<int> initial_;
    int* parr_;
    size_t size_;
  };

  using Which = std::pair<std::string, double>;
}

struct EvalCoefficients
{
  EvalCoefficients();
  EvalCoefficients(EvalCoefficients const&);
  EvalCoefficients& operator = (EvalCoefficients const&);

  // single vars
  // pawns
  int pawnEndgameBonus_{ 15 };
  int doubledPawn_{ -12 };
  int isolatedPawn_{ -17 };
  int backwardPawn_{ -15 };
  int unsupportedPawn_{ -15 };
  int unprotectedPawn_{ -10 };
  int rookBehindBonus_{ 7 };
  int protectedPawnPressure_{ 3 };
  int unprotectedPawnPressure_{ 8 };
  int protectedPawnBishopTreat_{ 3 };
  int unprotectedPawnBishopTreat_{ 8 };

  // rook on open column
  int semiopenRook_{ 10 };

  // material diff
  int bishopBonus_{ 10 };
  int figureAgainstPawnBonus_[2]  = {};
  int rookAgainstFigureBonus_[2]  = {};
  int figuresAgainstRookBonus_[2] = {};

  // forks
  int attackedByWeakBonus_{ 10 };
  int forkBonus_{ 60 };

  // king
  int castleImpossible_{ -25 };
  int fakeCastle_{ -25 };
  int castleBonus_{ 12 };
  int roamingKing_{ -12 };
  int pawnShieldA_{ 13 };
  int pawnShieldB_{ 12 };
  int pawnShieldC_{ 6 };
  int pawnPenaltyA_{ -20 };
  int pawnPenaltyB_{ -15 };
  int pawnPenaltyC_{ -6 };
  int opponentPawnA_{ -20 };
  int opponentPawnB_{ -15 };
  int opponentPawnC_{ -10 };

  // blocked figure
  int knightBlocked_{ 60 };
  int bishopBlocked_{ 60 };
  int rookBlocked_{ 70 };
  int queenBlocked_{ 80 };

  // king attacks
  int pawnKingAttack_{ 5 };
  int knightKingAttack_{ 4 };
  int bishopKingAttack_{ 4 };
  int rookKingAttack_{ 5 };
  int queenKingAttack_{ 8 };

  // arrays
  int centerPawn_[8]  = {};
  int forwardPawn_[8] = {};
  int passerPawn_[8]  = {};
  int passerGroup_[8] = {};
  int farKingPawn_[8] = {};
  int cangoPawn_[8] = {};
  int canpromotePawn_[8] = {};
  // distance between forwards
  int closeToPromotion_[8] = {};
  int oKingToPawnBonus_[8] = {};
  int myKingToPawnBonus_[8] = {};
  int oKingToPasserBonus_[8] = {};
  int myKingToPasserBonus_[8] = {};

  // mobility
  int knightMobility_[16] = {};
  int bishopMobility_[16] = {};
  int rookMobility_[16]   = {};
  int queenMobility_[32]  = {};

  // distance to opponent's king
  int kingDistanceKnight_[8] = {};
  int kingDistanceBishop_[8] = {};
  int kingDistanceRook_[8] = {};
  int kingDistanceQueen_[8] = {};

  // fields near king attacks
  int pawnAttackBonus_ = {2};
  int knightAttackBonus_ = {5};
  int bishopAttackBonus_ = {5};
  int rookAttackBonus_ = {6};
  int queenAttackBonus_ = {8};

  // PSQ-tables
  int pawnPsq_[64]   = {};
  int knightPsq_[64] = {};
  int bishopPsq_[64] = {};
  int rookPsq_[64]   = {};
  int queenPsq_[64]  = {};
  int kingPsqEg_[64] = {};

  // for special cases
  int kingToPawnDistanceMulti_{ 3 };
  int knightToPawnDistanceMulti_{ 1 };
  int kingToKingDistanceMulti_{ 2 };
  int figureToKingDistanceMulti_{ 2 };

  int bishopKnightMat_[64] = {};

  void save(std::string const& ofname);
  void random(std::set<std::string> const& exclude,
              std::vector<details::Which> const& which,
              double percent);
  void currentToIninital();

private:
  void init();

  std::unique_ptr<std::random_device> rd;
  std::unique_ptr<std::mt19937> gen;
  std::vector<details::Var> vars_;
  std::vector<details::Arr> arrs_;
};

}
