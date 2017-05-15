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
    // pawns
    vars_.push_back(details::Var{ "pawnEndgameBonus_", pawnEndgameBonus_, &pawnEndgameBonus_ });
    vars_.push_back(details::Var{ "doubledPawn_", doubledPawn_, &doubledPawn_ });
    vars_.push_back(details::Var{ "isolatedPawn_", isolatedPawn_, &isolatedPawn_ });
    vars_.push_back(details::Var{ "backwardPawn_", backwardPawn_, &backwardPawn_ });
    vars_.push_back(details::Var{ "unsupportedPawn_", unsupportedPawn_, &unsupportedPawn_ });
    vars_.push_back(details::Var{ "unprotectedPawn_", unprotectedPawn_, &unprotectedPawn_ });
    vars_.push_back(details::Var{ "rookBehindBonus_", rookBehindBonus_, &rookBehindBonus_ });
    vars_.push_back(details::Var{ "protectedPawnPressure_", protectedPawnPressure_, &protectedPawnPressure_ });
    vars_.push_back(details::Var{ "unprotectedPawnPressure_", unprotectedPawnPressure_, &unprotectedPawnPressure_ });

    // rook on open column
    vars_.push_back(details::Var{ "openRook_", openRook_, &openRook_ });

    // material diff
    vars_.push_back(details::Var{ "bishopBonus_", bishopBonus_, &bishopBonus_ });

    arrs_.push_back(details::Arr{ "figureAgainstPawnBonus_", std::vector<int>{ 25, 90}, figureAgainstPawnBonus_,
                    sizeof(figureAgainstPawnBonus_)/sizeof(*figureAgainstPawnBonus_) });
    arrs_.push_back(details::Arr{ "rookAgainstFigureBonus_", std::vector<int>{35, 80}, rookAgainstFigureBonus_,
                    sizeof(rookAgainstFigureBonus_)/sizeof(*rookAgainstFigureBonus_) });

    // king
    vars_.push_back(details::Var{ "castleImpossible_", castleImpossible_, &castleImpossible_ });
    vars_.push_back(details::Var{ "fakeCastle_", fakeCastle_, &fakeCastle_ });
    vars_.push_back(details::Var{ "castleBonus_", castleBonus_, &castleBonus_ });
    vars_.push_back(details::Var{ "roamingKing_", roamingKing_, &roamingKing_ });
    vars_.push_back(details::Var{ "pawnShieldA_", pawnShieldA_, &pawnShieldA_ });
    vars_.push_back(details::Var{ "pawnShieldB_", pawnShieldB_, &pawnShieldB_ });
    vars_.push_back(details::Var{ "pawnShieldC_", pawnShieldC_, &pawnShieldC_ });
    vars_.push_back(details::Var{ "pawnPenaltyA_", pawnPenaltyA_, &pawnPenaltyA_ });
    vars_.push_back(details::Var{ "pawnPenaltyB_", pawnPenaltyB_, &pawnPenaltyB_ });
    vars_.push_back(details::Var{ "pawnPenaltyC_", pawnPenaltyC_, &pawnPenaltyC_ });
    vars_.push_back(details::Var{ "opponentPawnA_", opponentPawnA_, &opponentPawnA_ });
    vars_.push_back(details::Var{ "opponentPawnB_", opponentPawnB_, &opponentPawnB_ });
    vars_.push_back(details::Var{ "opponentPawnC_", opponentPawnC_, &opponentPawnC_ });

    // blocked figure
    vars_.push_back(details::Var{ "bishopBlocked_", bishopBlocked_, &bishopBlocked_ });
    vars_.push_back(details::Var{ "knightBlocked_", knightBlocked_, &knightBlocked_ });
    vars_.push_back(details::Var{ "rookBlocked_", rookBlocked_, &rookBlocked_ });
    vars_.push_back(details::Var{ "queenBlocked_", queenBlocked_, &queenBlocked_ });

    // king attacks
    vars_.push_back(details::Var{ "pawnKingAttack_", pawnKingAttack_, &pawnKingAttack_ });
    vars_.push_back(details::Var{ "knightKingAttack_", knightKingAttack_, &knightKingAttack_ });
    vars_.push_back(details::Var{ "bishopKingAttack_", bishopKingAttack_, &bishopKingAttack_ });
    vars_.push_back(details::Var{ "rookKingAttack_", rookKingAttack_, &rookKingAttack_ });
    vars_.push_back(details::Var{ "queenKingAttack_", queenKingAttack_, &queenKingAttack_ });

    // arrays
    arrs_.push_back(details::Arr{ "centerPawn_", std::vector<int>{ 0, -10, 0, 10, 8, 0, 0, 0 }, centerPawn_, sizeof(centerPawn_)/sizeof(*centerPawn_) });
    arrs_.push_back(details::Arr{ "forwardPawn_", std::vector<int>{ 0, 0, 0, 3, 6, 10, 12, 0 }, forwardPawn_, sizeof(forwardPawn_)/sizeof(*forwardPawn_) });
    arrs_.push_back(details::Arr{ "passerPawn_", std::vector<int>{ 0, 5, 10, 20, 40, 60, 80, 0 }, passerPawn_, sizeof(passerPawn_)/sizeof(*passerPawn_) });
    arrs_.push_back(details::Arr{ "passerGroup_", std::vector<int>{ 0, 3, 5, 7, 9, 11, 13, 0  }, passerGroup_, sizeof(passerGroup_)/sizeof(*passerGroup_) });
    arrs_.push_back(details::Arr{ "unstoppablePawn_", std::vector<int>{ 0, 10, 20, 30, 40, 50, 60, 0 }, unstoppablePawn_,
                    sizeof(unstoppablePawn_)/sizeof(*unstoppablePawn_) });    
    arrs_.push_back(details::Arr{ "oKingToPasserBonus_", std::vector<int>{ 0, 0, 2, 4, 6, 8, 10, 12 }, oKingToPasserBonus_,
                    sizeof(oKingToPasserBonus_)/sizeof(*oKingToPasserBonus_) });
    arrs_.push_back(details::Arr{ "myKingToPasserBonus_", std::vector<int>{ 6, 6, 5, 4, 3, 2, 1, 0 }, myKingToPasserBonus_,
                    sizeof(myKingToPasserBonus_)/sizeof(*myKingToPasserBonus_) });

    // mobility
    arrs_.push_back(details::Arr{ "knightMobility_", std::vector<int>{-30, -15, 0, 3, 5, 7, 9, 11}, knightMobility_,
                    sizeof(knightMobility_)/sizeof(*knightMobility_) });
    arrs_.push_back(details::Arr{ "bishopMobility_", std::vector<int>{-20, -10, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4},
                    bishopMobility_, sizeof(bishopMobility_)/sizeof(*bishopMobility_) });
    arrs_.push_back(details::Arr{ "rookMobility_", std::vector<int>{-25, -15, -5, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4},
                    rookMobility_, sizeof(rookMobility_)/sizeof(*rookMobility_) });
    arrs_.push_back(details::Arr{ "queenMobility_",
                    std::vector<int>{-45, -35, -15, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 10,
                    10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12},
                    queenMobility_, sizeof(queenMobility_)/sizeof(*queenMobility_) });

    // PSQ-tables
    // pawn
    arrs_.push_back(details::Arr{ "pawnPsq_", std::vector<int>
      {
        0, 0, 0, 0, 0, 0, 0, 0,
        5, 5, 5, 5, 5, 5, 5, 5,
        0, 0, 3, 8, 8, 3, 0, 0,
        0, 0, 2, 7, 7, 2, 0, 0,
        0, 0, 1, 8, 8, 1, 0, 0,
        2, 0, 0, 0, 0, 0, 0, 2,
        2, 4, 4, -10, -10, 4, 4, 2,
        0, 0, 0, 0, 0, 0, 0, 0
      },
      pawnPsq_,
      sizeof(pawnPsq_)/sizeof(*pawnPsq_)
    });

    // knight
    arrs_.push_back(details::Arr{ "knightPsq_", std::vector<int>
      {
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8,  0,  0,  0,  0, -8, -8,
        -5,  0,  3,  4,  4,  3,  0, -5,
         0,  5,  5,  5,  5,  5,  5,  0,
        -7,  0,  4,  5,  5,  4,  0, -7,
        -8,  2,  4,  4,  4,  4,  2, -8,
        -8, -8,  0,  2,  2,  0, -8, -8,
        -8, -12,-5, -5, -5,  5, -12,-8
      },
      knightPsq_,
      sizeof(knightPsq_)/sizeof(*knightPsq_)
    });
  
    // bishop
    arrs_.push_back(details::Arr{ "bishopPsq_", std::vector<int>
      {
        -8,  -4,  -4,  -4,  -4,  -4,  -4,  -8,
        -2,   0,   0,   0,   0,   0,   0,  -2,
         2,   0,   2,   6,   6,   2,   0,   2,
        -2,   2,   2,   6,   6,   2,   2,  -2,
        -2,   0,   6,   6,   6,   6,   0,  -2,
         0,   6,   6,   6,   6,   6,   6,   0,
        -2,   2,   0,   0,   0,   0,   2,  -2,
        -5,  -4, -12,  -4,  -4, -12,  -4,  -5
      },
      bishopPsq_,
      sizeof(bishopPsq_)/sizeof(*bishopPsq_)
    });

    // rook
    arrs_.push_back(details::Arr{ "rookPsq_", std::vector<int>
      {
        10,  10,  10,  10,  10,  10,  10,  10,
        15,  15,  15,  15,  15,  15,  15,  15,
        -2,   0,   0,   0,   0,   0,   0,  -2,
        -2,   0,   0,   0,   0,   0,   0,  -2,
        -2,   0,   0,   0,   0,   0,   0,  -2,
        -2,   0,   0,   0,   0,   0,   0,  -2,
        -2,   0,   0,   0,   0,   0,   0,  -2,
        -5,  -5,   0,   3,   3,   0,  -5,  -5
      },
      rookPsq_,
      sizeof(rookPsq_)/sizeof(*rookPsq_)
    });
  
    // queen
    arrs_.push_back(details::Arr{ "queenPsq_", std::vector<int>
      {
         0,   0,   0,   0,   0,   0,   0,   0,
         0,   0,   0,   0,   0,   0,   0,   0,
        -2,   0,   2,   2,   2,   2,   0,  -2,
        -2,   0,   2,   3,   3,   2,   0,  -2,
         0,   0,   2,   3,   3,   2,   0,  -2,
        -4,   0,   2,   2,   2,   2,   0,  -4,
        -4,   0,   0,   1,   1,   0,   0,  -4,
        -5,  -5,  -5,  -5,  -5,  -5,  -5,  -5
      },
      queenPsq_,
      sizeof(queenPsq_)/sizeof(*queenPsq_)
    });
    
    // king
    arrs_.push_back(details::Arr{ "kingPsqEg_", std::vector<int>
      {
        -14, -12, -10, -10, -10, -10, -12, -14,
        -12,  -8,   0,   0,   0,   0,  -8, -12,
        -10,  -4,   6,   8,   8,   6,  -4, -10,
        -10,  -4,   8,  10,  10,   8,  -4, -10,
        -10,  -4,   8,  10,  10,   8,  -4, -10,
        -10,  -4,   6,   8,   8,   6,  -4, -10,
        -12,  -8,   0,   0,   0,   0,  -8, -12,
        -14, -12, -10, -10, -10, -10, -12, -14
      },
      kingPsqEg_,
      sizeof(kingPsqEg_)/sizeof(*kingPsqEg_)
    });

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
