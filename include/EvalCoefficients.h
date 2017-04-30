/*************************************************************
EvalCoefficients.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <string>
#include <vector>
#include <set>
#include <memory>
#include <random>

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
  int passedPawn_{ 10 };
  int doubledPawn_{ -12 };
  int isolatedPawn_{ -20 };
  int backwardPawn_{ -15 };
  int unsupportedPawn_{ -15 };
  int unprotectedPawn_{ -10 };

  // material diff
  int doubleBishop_{ 30 };

  // king
  int castleImpossible_{ -25 };
  int fakeCastle_{ -25 };
  int castleBonus_{ 12 };
  int roamingKing_{ -12 };
  int pawnShieldA_{ 13 };
  int pawnShieldB_{ 12 };
  int pawnShieldC_{ 6 };
  int pawnPenaltyA_{ -13 };
  int pawnPenaltyB_{ -12 };
  int pawnPenaltyC_{ -6 };
  int opponentPawnA_{ -20 };
  int opponentPawnB_{ -10 };
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

  // mobility
  int knightMobility_[16] = {};
  int bishopMobility_[16] = {};
  int rookMobility_[16]   = {};
  int queenMobility_[32]  = {};

  // PSQ-tables
  int pawnPsq_[64]   = {};
  int knightPsq_[64] = {};
  int bishopPsq_[64] = {};
  int rookPsq_[64]   = {};
  int queenPsq_[64]  = {};
  int kingPsqEg_[64] = {};

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