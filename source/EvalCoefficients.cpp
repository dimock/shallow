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
  vars_.push_back(details::Var{ "protectedPawnBishopTreat_", protectedPawnBishopTreat_, &protectedPawnBishopTreat_ });
  vars_.push_back(details::Var{ "unprotectedPawnBishopTreat_", unprotectedPawnBishopTreat_, &unprotectedPawnBishopTreat_ });
  vars_.push_back(details::Var{ "generalPressure_", generalPressure_, &generalPressure_ });
  vars_.push_back(details::Var{ "kingPressure_", kingPressure_, &kingPressure_ });
  //vars_.push_back(details::Var{ "unstoppablePasser_", unstoppablePasser_, &unstoppablePasser_ });

  // forks
  vars_.push_back(details::Var{ "forkBonus_", forkBonus_, &forkBonus_ });
  vars_.push_back(details::Var{ "doublePawnAttack_", doublePawnAttack_, &doublePawnAttack_ });

  // king
  vars_.push_back(details::Var{ "castleImpossible_", castleImpossible_, &castleImpossible_ });
  vars_.push_back(details::Var{ "fakeCastle_", fakeCastle_, &fakeCastle_ });
  vars_.push_back(details::Var{ "castleBonus_", castleBonus_, &castleBonus_ });
  vars_.push_back(details::Var{ "pawnPenaltyA_", pawnPenaltyA_, &pawnPenaltyA_ });
  vars_.push_back(details::Var{ "pawnPenaltyB_", pawnPenaltyB_, &pawnPenaltyB_ });
  vars_.push_back(details::Var{ "pawnPenaltyC_", pawnPenaltyC_, &pawnPenaltyC_ });
  vars_.push_back(details::Var{ "noPawnPenaltyA_", noPawnPenaltyA_, &noPawnPenaltyA_ });
  vars_.push_back(details::Var{ "noPawnPenaltyB_", noPawnPenaltyB_, &noPawnPenaltyB_ });
  vars_.push_back(details::Var{ "noPawnPenaltyC_", noPawnPenaltyC_, &noPawnPenaltyC_ });

  // blocked figure
  vars_.push_back(details::Var{ "bishopBlocked_", bishopBlocked_, &bishopBlocked_ });
  vars_.push_back(details::Var{ "knightBlocked_", knightBlocked_, &knightBlocked_ });

  // king attacks
  vars_.push_back(details::Var{ "pawnKingAttack_", pawnKingAttack_, &pawnKingAttack_ });
  vars_.push_back(details::Var{ "knightKingAttack_", knightKingAttack_, &knightKingAttack_ });
  vars_.push_back(details::Var{ "bishopKingAttack_", bishopKingAttack_, &bishopKingAttack_ });
  vars_.push_back(details::Var{ "rookKingAttack_", rookKingAttack_, &rookKingAttack_ });
  vars_.push_back(details::Var{ "queenKingAttack_", queenKingAttack_, &queenKingAttack_ });

  // x-ray attacks
  vars_.push_back(details::Var{ "rookKingAttackXray_", rookKingAttackXray_, &rookKingAttackXray_ });
  vars_.push_back(details::Var{ "queenKingAttackXray_", queenKingAttackXray_, &queenKingAttackXray_ });


  // king threat through check
#ifdef EVAL_KING_CHECK
  vars_.push_back(details::Var{ "knightChecking_", knightChecking_, &knightChecking_ });
  vars_.push_back(details::Var{ "bishopChecking_", bishopChecking_, &bishopChecking_ });
  vars_.push_back(details::Var{ "rookChecking_", rookChecking_, &rookChecking_ });
  vars_.push_back(details::Var{ "queenChecking_", queenChecking_, &queenChecking_ });
#endif

  // special cases
  vars_.push_back(details::Var{ "kingToPawnDistanceMulti_", kingToPawnDistanceMulti_, &kingToPawnDistanceMulti_ });
  vars_.push_back(details::Var{ "knightToPawnDistanceMulti_", knightToPawnDistanceMulti_, &knightToPawnDistanceMulti_ });
  vars_.push_back(details::Var{ "kingToKingDistanceMulti_", kingToKingDistanceMulti_, &kingToKingDistanceMulti_ });
  vars_.push_back(details::Var{ "figureToKingDistanceMulti_", figureToKingDistanceMulti_, &figureToKingDistanceMulti_ });

  // pinned figures
  vars_.push_back(details::Var{ "pinnedPawn_", pinnedPawn_, &pinnedPawn_ });
  vars_.push_back(details::Var{ "pinnedKnight_", pinnedKnight_, &pinnedKnight_ });
  vars_.push_back(details::Var{ "pinnedBishop_", pinnedBishop_, &pinnedBishop_ });
  vars_.push_back(details::Var{ "pinnedRook_", pinnedRook_, &pinnedRook_ });
  vars_.push_back(details::Var{ "pinnedQueen_", pinnedQueen_, &pinnedQueen_ });

  // Arrays  
  arrs_.push_back(details::Arr{ "opponentPawnPressure_", std::vector<int>{20, 20, 15, 8, 5, 2, 1, 0, 0}, opponentPawnPressure_,
                  sizeof(opponentPawnPressure_)/sizeof(*opponentPawnPressure_) });

  arrs_.push_back(details::Arr{ "pawnShieldA_", std::vector<int>{14, 10}, pawnShieldA_, sizeof(pawnShieldA_)/sizeof(*pawnShieldA_) });
  arrs_.push_back(details::Arr{ "pawnShieldB_", std::vector<int>{14, 10}, pawnShieldB_, sizeof(pawnShieldB_)/sizeof(*pawnShieldB_) });
  arrs_.push_back(details::Arr{ "pawnShieldC_", std::vector<int>{ 6, 4}, pawnShieldC_, sizeof(pawnShieldC_)/sizeof(*pawnShieldC_) });

  // rook on open column
  arrs_.push_back(details::Arr{ "openRook_", std::vector<int>{ 0, 10, 30, 30}, openRook_,
                  sizeof(openRook_)/sizeof(*openRook_) });

  // material diff
  arrs_.push_back(details::Arr{ "bishopBonus_", std::vector<int>{ 0, 10, 50, 50}, bishopBonus_,
                  sizeof(bishopBonus_)/sizeof(*bishopBonus_) });

  arrs_.push_back(details::Arr{ "figureAgainstPawnBonus_", std::vector<int>{ 25, 60}, figureAgainstPawnBonus_,
                  sizeof(figureAgainstPawnBonus_)/sizeof(*figureAgainstPawnBonus_) });
  arrs_.push_back(details::Arr{ "rookAgainstFigureBonus_", std::vector<int>{25, 60}, rookAgainstFigureBonus_,
                  sizeof(rookAgainstFigureBonus_)/sizeof(*rookAgainstFigureBonus_) });
  arrs_.push_back(details::Arr{ "figuresAgainstRookBonus_", std::vector<int>{10, 40}, figuresAgainstRookBonus_,
                  sizeof(figuresAgainstRookBonus_)/sizeof(*figuresAgainstRookBonus_) });
  arrs_.push_back(details::Arr{ "rookAgainstPawnBonus_", std::vector<int>{ 15, 40}, rookAgainstPawnBonus_,
                  sizeof(rookAgainstPawnBonus_)/sizeof(*rookAgainstPawnBonus_) });

  // arrays
  arrs_.push_back(details::Arr{ "passerPawn_", std::vector<int>{ 0, 10, 20, 30, 40, 60, 90, 0 }, passerPawn_,
                  sizeof(passerPawn_)/sizeof(*passerPawn_) });
  arrs_.push_back(details::Arr{ "multipasserPawn_", std::vector<int>{ 0, 5, 10, 20, 30, 40, 50, 0  }, multipasserPawn_,
                  sizeof(multipasserPawn_)/sizeof(*multipasserPawn_) });
  arrs_.push_back(details::Arr{ "passerPawnSc_", std::vector<int>{ 0, 3, 6, 9, 12, 15, 18, 0 }, passerPawnSc_,
                  sizeof(passerPawnSc_)/sizeof(*passerPawnSc_) });
  arrs_.push_back(details::Arr{ "semipasserPawn_", std::vector<int>{ 0, 3, 6, 10, 14, 18, 0, 0 }, semipasserPawn_,
                  sizeof(semipasserPawn_)/sizeof(*semipasserPawn_) });
  arrs_.push_back(details::Arr{ "protectedPasser_", std::vector<int>{ 0, 9, 12, 15, 17, 20, 26, 0 }, protectedPasser_,
                  sizeof(protectedPasser_)/sizeof(*protectedPasser_) });
  arrs_.push_back(details::Arr{ "farKingPawn_", std::vector<int>{ 0, 10, 12, 16, 20, 30, 40, 0 }, farKingPawn_,
                  sizeof(farKingPawn_)/sizeof(*farKingPawn_) });
  arrs_.push_back(details::Arr{ "cangoPawn_", std::vector<int>{ 0, 6, 7, 8, 10, 12, 16, 0 }, cangoPawn_,
                  sizeof(cangoPawn_)/sizeof(*cangoPawn_) });
  arrs_.push_back(details::Arr{ "canpromotePawn_", std::vector<int>{ 0, 3, 6, 8, 10, 12, 15, 0 }, canpromotePawn_,
                  sizeof(canpromotePawn_)/sizeof(*canpromotePawn_) });
  arrs_.push_back(details::Arr{ "closeToPromotion_", std::vector<int>{ 0, 2, 4, 5, 6, 7, 8, 9 }, closeToPromotion_,
                  sizeof(closeToPromotion_)/sizeof(*closeToPromotion_) });
  arrs_.push_back(details::Arr{ "kingToPawnBonus_", std::vector<int>{ 0, 6, 5, 4, 3, 2, 1, 0 }, kingToPawnBonus_,
                  sizeof(kingToPawnBonus_)/sizeof(*kingToPawnBonus_) });

  // mobility
  arrs_.push_back(details::Arr{ "knightMobility_", std::vector<int>{-40, -10, 0, 3, 5, 7, 9, 11}, knightMobility_,
                  sizeof(knightMobility_)/sizeof(*knightMobility_) });
  arrs_.push_back(details::Arr{ "bishopMobility_", std::vector<int>{-40, -10, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
                  bishopMobility_, sizeof(bishopMobility_)/sizeof(*bishopMobility_) });
  arrs_.push_back(details::Arr{ "rookMobility_", std::vector<int>{-35, -8, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13},
                  rookMobility_, sizeof(rookMobility_)/sizeof(*rookMobility_) });
  arrs_.push_back(details::Arr{ "queenMobility_",
                    std::vector<int>{-45, -35, -7,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12,
                  13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28},
                  queenMobility_, sizeof(queenMobility_)/sizeof(*queenMobility_) });

  // king position eval for BN-mat
  arrs_.push_back(details::Arr{ "bishopKnightMat_", std::vector<int>
  {
    20, 16, 8, -1, -2, -8, -16, -20,
      16, 16, 8, -1, -3, -8, -16, -16,
      8, 8, 6, -2, -4, -6, -8, -8,
      -1, -1, -2, -8, -8, -4, -3, -2,
      -2, -3, -4, -8, -8, -2, -1, -1,
      -8, -8, -6, -4, -2, 6, 8, 8,
      -16, -16, -8, -3, -1, 8, 16, 16,
      -20, -16, -8, -2, -1, 8, 16, 20
  },
  bishopKnightMat_,
  sizeof(bishopKnightMat_)/sizeof(*bishopKnightMat_)
  });

  arrs2_.push_back(details::Arr2{ "kingDistanceBonus_", std::vector<std::vector<int>>
  {
    {},
    {},
    { 10, 8, 8, 7, 4, 1, 0, 0 },
    { 10, 8, 8, 7, 4, 2, 1, 0 },
    { 10, 10, 9, 6, 4, 2, 1, 0 },
    { 15, 15, 15, 8, 6, 3, 1, 0 },
    {},
    {}
  },
  reinterpret_cast<int*>(kingDistanceBonus_),
  sizeof(kingDistanceBonus_)/sizeof(*kingDistanceBonus_),
  sizeof(kingDistanceBonus_[0])/sizeof(*kingDistanceBonus_[0])
  });

  arrs3_.push_back(details::Arr3{ "positionEvaluations_", std::vector<std::vector<std::vector<int>>>
  {
    // begin
    {
      // empty
      {},

        // pawn
      {
        0, 0, 0, 0, 0, 0, 0, 0,
        5, 5, 5, 5, 5, 5, 5, 5,
        0, 0, 3, 8, 8, 3, 0, 0,
        0, 0, 2, 7, 7, 2, 0, 0,
        0, 0, 1, 8, 8, 1, 0, 0,
        3, 0, 0, 0, 0, 0, 0, 3,
        3, 4, 4, -10, -10, 4, 4, 3,
        0, 0, 0, 0, 0, 0, 0, 0
      },

      // knight
      {
        -8, -8, -8, -8, -8, -8, -8, -8,
        -8, -8, 0, 0, 0, 0, -8, -8,
        -5, 0, 3, 4, 4, 3, 0, -5,
        0, 5, 5, 5, 5, 5, 5, 0,
        -7, 0, 4, 5, 5, 4, 0, -7,
        -8, 2, 4, 4, 4, 4, 2, -8,
        -8, -8, 0, 2, 2, 0, -8, -8,
        -8, -12, -5, -5, -5, -5, -12, -8
      },

      // bishop
      {
        -8, -4, -4, -4, -4, -4, -4, -8,
        -2, 0, 0, 0, 0, 0, 0, -2,
        2, 0, 2, 6, 6, 2, 0, 2,
        -2, 2, 2, 6, 6, 2, 2, -2,
        -2, 0, 6, 6, 6, 6, 0, -2,
        0, 6, 6, 6, 6, 6, 6, 0,
        -2, 4, 0, 0, 0, 0, 4, -2,
        -5, -4, -12, -4, -4, -12, -4, -5
      },

      // rook
      {
        15, 15, 15, 15, 15, 15, 15, 15,
        15, 15, 15, 15, 15, 15, 15, 15,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        -4, 0, 0, 0, 0, 0, 0, -4,
        -8, 0, 0, 0, 0, 0, 0, -8,
        -5, -10, 0, 5, 5, 0, -10, -5
      },

      // queen
      {
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        -2, 0, 2, 2, 2, 2, 0, -2,
        -3, 0, 2, 3, 3, 2, 0, -3,
        -2, 0, 2, 3, 3, 2, 0, -2,
        -3, 0, 2, 2, 2, 2, 0, -3,
        -4, 0, 0, 1, 1, 0, 0, -4,
        -5, -5, -5, -5, -5, -5, -5, -5
      },


      // king
      {
        -30, -30, -30, -30, -30, -30, -30, -30,
        -26, -26, -26, -26, -26, -26, -26, -26,
        -24, -24, -24, -24, -24, -24, -24, -24,
        -22, -22, -22, -22, -22, -22, -22, -22,
        -16, -18, -20, -22, -22, -20, -18, -16,
        -10, -14, -14, -14, -14, -14, -14, -10,
        0, 0, -10, -10, -10, -6, 0, 0,
        4, 12, 0, -8, -8, 0, 16, 5
      },

      {}
    },

    // end
    {
      // empty
      {},

      // pawn
      {
        0, 0, 0, 0, 0, 0, 0, 0,
        10, 10, 10, 10, 10, 10, 10, 10,
        8, 8, 8, 8, 8, 8, 8, 8,
        6, 6, 6, 6, 6, 6, 6, 6,
        4, 4, 4, 4, 4, 4, 4, 4,
        2, 2, 2, 2, 2, 2, 2, 2,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0
      },

      // knight
      {},

      // bishop
      {},

      // rook
      {},

      // queen
      {},

      // king
      {
        -12, -10, -8, -8, -8, -8, -10, -12,
        -10, -4, 0, 0, 0, 0, -4, -10,
        -8, 0, 6, 6, 6, 6, 0, -8,
        -6, 0, 6, 8, 8, 6, 0, -6,
        -6, 0, 6, 8, 8, 6, 0, -6,
        -8, 0, 6, 6, 6, 6, 0, -8,
        -10, -4, 0, 0, 0, 0, -4, -10,
        -12, -10, -8, -8, -8, -8, -10, -12
      },

      {}
    }
  },
  reinterpret_cast<int*>(positionEvaluations_),
  sizeof(positionEvaluations_)/sizeof(*positionEvaluations_),
  sizeof(positionEvaluations_[0])/sizeof(*positionEvaluations_[0]),
  sizeof(positionEvaluations_[0][0])/sizeof(*positionEvaluations_[0][0]) });

  for(auto& arr : arrs_)
  {
    for(size_t i = 0; i < arr.initial_.size() && i < arr.size_; ++i)
      arr.parr_[i] = arr.initial_[i];
  }

  for(auto& arr : arrs2_)
  {
    for(size_t i = 0; i < arr.initial_.size() && i < arr.size1_; ++i)
    {
      for(size_t j = 0; j < arr.initial_[i].size() && j < arr.size2_; ++j)
        arr.parr_[i*arr.size2_+ j] = arr.initial_[i][j];
    }
  }

  for(auto& arr : arrs3_)
  {
    for(size_t i = 0; i < arr.initial_.size() && i < arr.size1_; ++i)
    {
      for(size_t j = 0; j < arr.initial_[i].size() && j < arr.size2_; ++j)
      {
        for(size_t k = 0; k < arr.initial_[i][j].size() && k < arr.size3_; ++k)
          arr.parr_[i*arr.size2_*arr.size3_ + j*arr.size3_ + k] = arr.initial_[i][j][k];
      }
    }
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
  for(size_t i = 0; i < arrs2_.size(); ++i)
  {
    auto& arr = arrs2_[i];
    auto const& orr = other.arrs2_[i];
    for(size_t j = 0; j < arr.size1_ && j < arr.initial_.size(); ++j)
    {
      for(size_t k = 0; k < arr.size2_ && k < arr.initial_[j].size(); ++k)
      {
        arr.initial_[j][k] = orr.initial_[j][k];
        arr.parr_[j*arr.size2_ + k] = orr.parr_[j*arr.size2_ + k];
      }
    }
  }  
  for(size_t i = 0; i < arrs3_.size(); ++i)
  {
    auto& arr = arrs3_[i];
    auto const& orr = other.arrs3_[i];
    for(size_t j = 0; j < arr.size1_ && j < arr.initial_.size(); ++j)
    {
      for(size_t k = 0; k < arr.size2_ && k < arr.initial_[j].size(); ++k)
      {
        for(size_t m = 0; m < arr.size3_ && m < arr.initial_[j][k].size(); ++m)
        {
          arr.initial_[j][k][m] = orr.initial_[j][k][m];
          arr.parr_[j*arr.size2_*arr.size3_ + k*arr.size3_ + m] = orr.parr_[j*arr.size2_*arr.size3_ + k*arr.size3_ + m];
        }
      }
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
  for(size_t i = 0; i < arrs2_.size(); ++i)
  {
    auto& arr = arrs2_[i];
    for(size_t j = 0; j < arr.size1_ && j < arr.initial_.size(); ++j)
    {
      for(size_t k = 0; k < arr.size2_ && k < arr.initial_[j].size(); ++k)
        arr.initial_[j][k] = arr.parr_[j*arr.size2_ + k];
    }
  }
  for(size_t i = 0; i < arrs3_.size(); ++i)
  {
    auto& arr = arrs3_[i];
    for(size_t j = 0; j < arr.size1_ && j < arr.initial_.size(); ++j)
    {
      for(size_t k = 0; k < arr.size2_ && k < arr.initial_[j].size(); ++k)
      {
        for(size_t m = 0; m < arr.size3_ && m < arr.initial_[j][k].size(); ++m)
          arr.initial_[j][k][m] = arr.parr_[j*arr.size2_*arr.size3_ + k*arr.size3_ + m];
      }
    }
  }
}

void EvalCoefficients::save(std::string const& ofname) const
{
  std::ofstream ofs(ofname);
  for(auto const& v : vars_)
  {
    ofs << "  int " << v.name_ << "{" << *v.pvar_ << "};" << std::endl;
  }
  ofs << std::endl;
  for(auto const& a : arrs_)
  {
    std::string values;
    for(size_t i = 0; i < a.size_; ++i)
    {
      auto v = std::to_string(a.parr_[i]);
      if(i > 0 && (i & 7) == 0)
        values += "\n      ";
      values += v;
      if(i < a.size_-1)
        values += ", ";
    }
    ofs << "    arrs_.push_back(details::Arr{ \"" << a.name_
      << "\", std::vector<int>{\n      " << values
      << "\n    },\n    " << a.name_ << ",\n    sizeof(" << a.name_ << ")/sizeof(*" << a.name_ << ") });"
      << std::endl << std::endl;
  }
  ofs << std::endl;
  for(auto const& a : arrs2_)
  {
    std::string values;
    for(size_t i = 0; i < a.size1_; ++i)
    {
      values += "      {";
      bool exists = false;
      for(size_t j = 0; !exists && j < a.size2_; ++j)
      {
        if(a.parr_[i*a.size2_ + j] != 0)
          exists = true;
      }
      if(exists)
      {
        values += " ";
        for(size_t j = 0; j < a.size2_; ++j)
        {
          auto v = std::to_string(a.parr_[i*a.size2_ + j]);
          values += v;
          if(j < a.size2_)
            values += ", ";
        }
        values += " ";
      }
      values += "}";
      if(i < a.size1_-1)
        values += ",";
      values += '\n';
    }
    ofs << "    arrs2_.push_back(details::Arr2{ \"" << a.name_
      << "\", std::vector<std::vector<int>>{\n" << values
      << "    },\n    reinterpret_cast<int*>(" << a.name_ << "),\n    sizeof(" << a.name_ << ")/sizeof(*" << a.name_ << "),"
      << "\n    sizeof(" << a.name_ << "[0])/sizeof(*" << a.name_ << "[0]) });"
      << std::endl << std::endl;
  }
  ofs << std::endl;
  for(auto const& a : arrs3_)
  {
    std::string values;
    for(size_t i = 0; i < a.size1_; ++i)
    {
      values += "      {";
      for(size_t j = 0; j < a.size2_; ++j)
      {
        bool exists = false;
        for(size_t k = 0; !exists && k < a.size3_; ++k)
        {
          if(a.parr_[i*a.size2_*a.size3_ + j*a.size3_ + k] != 0)
            exists = true;
        }
        values += "\n        {";
        if(exists)
        {
          for(size_t k = 0; k < a.size3_; ++k)
          {
            auto v = std::to_string(a.parr_[i*a.size2_*a.size3_ + j*a.size3_ + k]);
            if((k & 7) == 0)
              values += "\n          ";
            values += v;
            if(k < a.size3_-1)
              values += ", ";
          }
          values += "\n        ";
        }
        values += "}";
        if(j < a.size2_-1)
          values += ",";
      }
      values += "\n      }";
      if(i < a.size1_-1)
        values += ",";
      values += '\n';
    }
    ofs << "    arrs3_.push_back(details::Arr3{ \"" << a.name_
      << "\", std::vector<std::vector<std::vector<int>>>{\n" << values
      << "    },\n    reinterpret_cast<int*>(" << a.name_ << "),\n    sizeof(" << a.name_ << ")/sizeof(*" << a.name_ << "),"
      << "\n    sizeof(" << a.name_ << "[0])/sizeof(*" << a.name_ << "[0]),"
      << "\n    sizeof(" << a.name_ << "[0][0])/sizeof(*" << a.name_ << "[0][0]) });" << std::endl;
  }
}

void EvalCoefficients::random(std::set<std::string> const& exclude,
                              std::vector<details::Which> const& which,
                              double percent,
                              int min_val)
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
  for(auto const& var : vars_)
  {
    if(exclude.count(var.name_) > 0)
      continue;
    auto r = dis(*gen);
    if(!normalize_if(var.name_, r))
      continue;
    double v = var.initial_;
    if(std::abs(v) < min_val)
      v = min_val * (v < 0 ? -1 : 1);
    v *= (1.0 + r);
    *var.pvar_ = v;
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
      double v = a.initial_[i];
      if(std::abs(v) < min_val)
        v = min_val * (v < 0 ? -1 : 1);
      v *= (1.0 + r);
      a.parr_[i] = v;
    }
  }
  for(auto const& a : arrs2_)
  {
    if(exclude.count(a.name_) > 0)
      continue;
    double f = 1.0;
    if(!normalize_if(a.name_, f))
      continue;
    for(size_t i = 0; i < a.initial_.size() && i < a.size1_; ++i)
    {
      for(size_t j = 0; j < a.initial_[i].size() && j < a.size2_; ++j)
      {
        auto r = dis(*gen) * f;
        double v = a.initial_[i][j];
        if(std::abs(v) < min_val)
          v = min_val * (v < 0 ? -1 : 1);
        v *= (1.0 + r);
        a.parr_[i*a.size2_ + j] = v;
      }
    }
  }
  for(auto const& a : arrs3_)
  {
    if(exclude.count(a.name_) > 0)
      continue;
    double f = 1.0;
    if(!normalize_if(a.name_, f))
      continue;
    for(size_t i = 0; i < a.initial_.size() && i < a.size1_; ++i)
    {
      for(size_t j = 0; j < a.initial_[i].size() && j < a.size2_; ++j)
      {
        for(size_t k = 0; k < a.initial_[i][j].size() && k < a.size3_; ++k)
        {
          auto r = dis(*gen) * f;
          auto v = a.initial_[i][j][k];
          if(std::abs(v) < min_val)
            v = min_val * (v < 0 ? -1 : 1);
          v *= (1.0 + r);
          a.parr_[i*a.size2_*a.size3_ + j*a.size3_ + k] = v;
        }
      }
    }
  }
}

}
