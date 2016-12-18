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
    int  size_;
  };

  using Which = std::pair<std::string, double>;
}

struct EvalCoefficients
{
  EvalCoefficients();

  // single vars
  int doublePawn_{ 10 };

  // arrays
  int passedPawn_[8] = {};

  void save(std::string const& ofname);
  void random(std::set<std::string> const& exclude,
              std::vector<details::Which> const& which,
              double percent);

private:
  std::unique_ptr<std::random_device> rd;
  std::unique_ptr<std::mt19937> gen;
  std::vector<details::Var> vars_;
  std::vector<details::Arr> arrs_;
};

}