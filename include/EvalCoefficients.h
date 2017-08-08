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

  struct Arr2
  {
    std::string name_;
    std::vector<std::vector<int>> initial_;
    int* parr_;
    size_t size1_, size2_;
  };

  struct Arr3
  {
    std::string name_;
    std::vector<std::vector<std::vector<int>>> initial_;
    int* parr_;
    size_t size1_, size2_, size3_;
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
  int isolatedPawn_{ -12 };
  int backwardPawn_{ -12 };
  int unsupportedPawn_{ -12 };
  int unprotectedPawn_{ -8 };
  int rookBehindBonus_{ 5 };
  int protectedPawnPressure_{ 3 };
  int unprotectedPawnPressure_{ 7 };
  int protectedPawnBishopTreat_{ 2 };
  int unprotectedPawnBishopTreat_{ 5 };
  int kingPressure_{ 3 };
  int generalPressure_{ 1 };
  //int unstoppablePasser_{ 50 };

  // forks
  int forkBonus_{ 35 };
  int doublePawnAttack_{ 50 };

  // king
  int castleImpossible_{ -28 };
  int fakeCastle_{ -30 };
  int castleBonus_{ 10 };
  int pawnPenaltyA_{ -14 };
  int pawnPenaltyB_{ -14 };
  int pawnPenaltyC_{ -4 };
  int noPawnPenaltyA_{ -8 };
  int noPawnPenaltyB_{ -8 };
  int noPawnPenaltyC_{ -1 };
  int opponentPawnNearA_{ -15 };
  int opponentPawnNearB_{ -12 };
  int opponentPawnNearC_{ -10 };
  int opponentPawnFarA_{ -8 };
  int opponentPawnFarB_{ -6 };
  int opponentPawnFarC_{ -5 };

  // blocked figure
  int knightBlocked_{ 80 };
  int bishopBlocked_{ 80 };
  int rookBlocked_{ 70 };
  int queenBlocked_{ 80 };

  // king attacks
  int pawnKingAttack_{ 2 };
  int knightKingAttack_{ 3 };
  int bishopKingAttack_{ 3 };
  int rookKingAttack_{ 6 };
  int queenKingAttack_{ 12 };

  // fields near king attacks
  int pawnAttackBonus_{ 2 };
  int knightAttackBonus_{ 5 };
  int bishopAttackBonus_{ 5 };
  int rookAttackBonus_{ 6 };
  int queenAttackBonus_{ 8 };

  // for special cases
  int kingToPawnDistanceMulti_{ 3 };
  int knightToPawnDistanceMulti_{ 1 };
  int kingToKingDistanceMulti_{ 2 };
  int figureToKingDistanceMulti_{ 2 };

  // king threat
  int knightChecking_{ 3 };
  int bishopChecking_{ 3 };
  int rookChecking_{ 6 };
  int queenChecking_{ 12 };

  // arrays
  // rook on open column
  int openRook_[4] = {};

  // material diff
  int bishopBonus_[4] = {};
  int figureAgainstPawnBonus_[2] = {};
  int rookAgainstFigureBonus_[2] = {};
  int figuresAgainstRookBonus_[2] = {};

  int pawnShieldA_[2] = {};
  int pawnShieldB_[2] = {};
  int pawnShieldC_[2] = {};

  // number of possible king moves
  int kingMovesMultiplier_[8] = {};

  // arrays
  int passerPawn_[8]  = {};
  int semipasserPawn_[8] = {};
  int protectedPasser_[8] = {};
  int farKingPawn_[8] = {};
  int cangoPawn_[8] = {};
  int canpromotePawn_[8] = {};
  // distance between forwards
  int closeToPromotion_[8] = {};
  int kingToPawnBonus_[8] = {};
  //int forwardPasser_[8] = {};

  // mobility
  int knightMobility_[16] = {};
  int bishopMobility_[16] = {};
  int rookMobility_[16]   = {};
  int queenMobility_[32]  = {};

  int bishopKnightMat_[64] = {};

  // basic king pressure
  int kingDistanceBonus_[8][8] = {};

  // position evaluation. 0 - opening, 1 - endgame; color,type,pos
  int positionEvaluations_[2][8][64] = {};

  void save(std::string const& ofname) const;
  void random(std::set<std::string> const& exclude,
              std::vector<details::Which> const& which,
              double percent,
              int min_val);
  void currentToIninital();

private:
  void init();

  std::unique_ptr<std::random_device> rd;
  std::unique_ptr<std::mt19937> gen;
  std::vector<details::Var> vars_;
  std::vector<details::Arr> arrs_;
  std::vector<details::Arr2> arrs2_;
  std::vector<details::Arr3> arrs3_;
};

}
