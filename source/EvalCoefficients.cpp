/*************************************************************
EvalCoefficients.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <EvalCoefficients.h>
#include <fstream>
#include <set>
#include <algorithm>
#include <boost/algorithm/string/join.hpp>

namespace NEngine
{

void EvalCoefficients::init()
{
  rd = std::make_unique<std::random_device>();
  gen = std::make_unique<std::mt19937>((*rd)());

  // single vars
  vars_.push_back(details::Var{ "pawnEndgameBonus_", pawnEndgameBonus_, &pawnEndgameBonus_ });
  vars_.push_back(details::Var{ "passedPawn_", passedPawn_, &passedPawn_ });
  vars_.push_back(details::Var{ "doubledPawn_", doubledPawn_, &doubledPawn_ });
  vars_.push_back(details::Var{ "isolatedPawn_", isolatedPawn_, &isolatedPawn_ });

  // arrays
  arrs_.push_back(details::Arr{ "centerPawn_", std::vector<int>{ 0, -160, 0, 160, 160, 0, 0, 0 }, centerPawn_, sizeof(centerPawn_)/sizeof(*centerPawn_) });

  for(auto& arr : arrs_)
  {
    for(size_t i = 0; i < arr.initial_.size() && i < arr.size_; ++i)
      arr.parr_[i] = arr.initial_[i];
  }
}

EvalCoefficients::EvalCoefficients()
{
  init();
}

EvalCoefficients::EvalCoefficients(EvalCoefficients const& other)
{
  init();
  this->operator=(other);
}

EvalCoefficients& EvalCoefficients::operator = (EvalCoefficients const& other)
{
  for(size_t i = 0; i < vars_.size(); ++i)
  {
    auto& v = vars_[i];
    auto const& o = other.vars_[i];
    v.initial_ = o.initial_;
    *v.pvar_ = *o.pvar_;
  }
  for(size_t i = 0; i < arrs_.size(); ++i)
  {
    auto& arr = arrs_[i];
    auto const& orr = other.arrs_[i];
    for(size_t j = 0; j < arr.size_ && j < arr.initial_.size(); ++j)
    {
      arr.initial_[j] = orr.initial_[j];
      arr.parr_[j] = orr.parr_[j];
    }
  }
  return *this;
}

void EvalCoefficients::currentToIninital()
{
  for(size_t i = 0; i < vars_.size(); ++i)
  {
    auto& v = vars_[i];
    v.initial_ = *v.pvar_;
  }
  for(size_t i = 0; i < arrs_.size(); ++i)
  {
    auto& arr = arrs_[i];
    for(size_t j = 0; j < arr.size_ && j < arr.initial_.size(); ++j)
    {
      arr.initial_[j] = arr.parr_[j];
    }
  }
}

void EvalCoefficients::save(std::string const& ofname)
{
  std::ofstream ofs(ofname);
  for(auto const& v : vars_)
  {
    ofs << "  int " << v.name_ << "{" << *v.pvar_ << "};" << std::endl;
  }
  ofs << std::endl;
  for(auto const& a : arrs_)
  {
    std::vector<std::string> values;
    std::transform(a.parr_, a.parr_ + a.size_, std::back_inserter(values), [](int v)
    {
      return std::to_string(v);
    });
    ofs << "  arrs_.push_back(details::Arr{ \"" << a.name_
      << "\", std::vector<int>{ " << boost::algorithm::join(values, ", ")
      << " }, " << a.name_ << ", sizeof(" << a.name_ << ")/sizeof(*" << a.name_ << ") });" << std::endl;
  }
}

void EvalCoefficients::random(std::set<std::string> const& exclude,
                              std::vector<details::Which> const& which,
                              double percent)
{
  auto normalize_if = [&percent, &which](std::string const& name, double& r) -> bool
  {
    if(which.empty())
      return true;
    auto it = std::find_if(which.begin(), which.end(), [&name](details::Which const& w) { return w.first == name; });
    if(it == which.end())
      return false;
    r *= it->second / percent;
    return true;
  };
  std::uniform_real_distribution<> dis(-percent, percent);
  for(auto const& v : vars_)
  {
    if(exclude.count(v.name_) > 0)
      continue;
    auto r = dis(*gen);
    if(!normalize_if(v.name_, r))
      continue;
    *v.pvar_ = v.initial_ * (1.0 + r);
  }
  for(auto const& a : arrs_)
  {
    if(exclude.count(a.name_) > 0)
      continue;
    double f = 1.0;
    if(!normalize_if(a.name_, f))
      continue;
    for(size_t i = 0; i < a.initial_.size() && i < a.size_; ++i)
    {
      auto r = dis(*gen) * f;
      a.parr_[i] = a.initial_[i] * (1.0 + r);
    }
  }
}

}
