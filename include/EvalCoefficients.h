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
  int passedPawn_{ 10 << shift_divider };
  int doubledPawn_{ -10 << shift_divider };
  int isolatedPawn_{ -10 << shift_divider };
  int backwardPawn_{ -10 << shift_divider };

  // king
  int castleImpossible_{ -20 };
  int fakeCastle_{ -20 };
  int castleBonus_{ 10 };
  int roamingKing_{ -10 };
  int pawnShieldA_{ 10 };
  int pawnShieldB_{ 10 };
  int pawnShieldC_{ 5 };
  int pawnPenaltyA_{ -10 };
  int pawnPenaltyB_{ -10 };
  int pawnPenaltyC_{ -5 };

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