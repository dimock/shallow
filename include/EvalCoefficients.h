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

  static int const shift_divider = 4;

  // single vars
  // pawns
  int pawnEndgameBonus_{ 15 << shift_divider };
  int passedPawn_{ 15 << shift_divider };
  int doubledPawn_{ -15 << shift_divider };
  int isolatedPawn_{ -15 << shift_divider };
  int backwardPawn_{ -15 << shift_divider };

  // king
  int castleImpossible_{ -25 };
  int fakeCastle_{ -25 };
  int castleBonus_{ 20 };
  int roamingKing_{ -15 };
  int pawnShieldA_{ 15 };
  int pawnShieldB_{ 15 };
  int pawnShieldC_{ 10 };
  int pawnPenaltyA_{ -15 };
  int pawnPenaltyB_{ -15 };
  int pawnPenaltyC_{ -10 };

  // blocked figure
  int knightBlocked_{ -30 };
  int bishopBlocked_{ -20 };
  int rookBlocked_{ -25 };
  int queenBlocked_{ -45 };

  // mobility
  int knightMobility_{ 2 };
  int bishopMobility_{ 1 };
  int rookMobility_{ 1 };
  int queenMobility_{ 1 };

  // attacked fields
  int knightAttacks_{ 1 };
  int bishopAttacks_{ 1 };
  int rookAttacks_{ 1 };
  int queenAttacks_{ 1 };

  // king attacks
  int pawnKingAttack_{ 5 };
  int knightKingAttack_{ 4 };
  int bishopKingAttack_{ 4 };
  int rookKingAttack_{ 5 };
  int queenKingAttack_{ 8 };

  // arrays
  int centerPawn_[8] = {};

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