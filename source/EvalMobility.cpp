/*************************************************************
Evaluator.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <Evaluator.h>
#include <Board.h>
#include <HashTable.h>
#include <xindex.h>
#include <magicbb.h>

namespace NEngine
{

Evaluator::FullScore Evaluator::evaluatePsqBruteforce() const
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  for(int type = Figure::TypePawn; type <= Figure::TypeKing; ++type)
  {
    {
      auto mask = fmgr.type_mask((Figure::Type)type, Figure::ColorBlack);
      for(; mask;)
      {
        auto p = clear_lsb(mask);
        score.opening_ -= Figure::positionEvaluations_[0][type][p];
        score.endGame_ -= Figure::positionEvaluations_[1][type][p];
      }
    }
    {
      auto mask = fmgr.type_mask((Figure::Type)type, Figure::ColorWhite);
      for(; mask;)
      {
        auto p = Figure::mirrorIndex_[clear_lsb(mask)];
        score.opening_ += Figure::positionEvaluations_[0][type][p];
        score.endGame_ += Figure::positionEvaluations_[1][type][p];
      }
    }
  }
  return score;
}

Evaluator::FullScore Evaluator::evaluateKnights(Figure::Color color)
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  auto const ocolor = Figure::otherColor(color);
  // knights
  BitMask knmask = fmgr.knight_mask(color);
  BitMask knight_from_king = movesTable().caps(Figure::TypeKnight, finfo_[ocolor].king_pos_) & inv_mask_all_;
  for(; knmask;)
  {
    int n = clear_lsb(knmask);
    int p = color ? Figure::mirrorIndex_[n] : n;
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
    finfo_[color].attack_mask_ |= knight_moves;
    finfo_[color].knightAttacks_ |= knight_moves;
    finfo_[color].knightMasks_.push_back(knight_moves);
    auto kn_dist = distanceCounter().getDistance(n, finfo_[ocolor].king_pos_);
    bool could_check = (knight_from_king & knight_moves) != 0;
    bool could_attack = (finfo_[ocolor].kingAttacks_ & knight_moves) != 0ULL;
    finfo_[color].knightPressure_ += evalCoeffs().kingDistanceKnight_[kn_dist];
    finfo_[color].knightPressure_ += evalCoeffs().knightAttackBonus_ * (could_attack || could_check);
  }
  score.common_ += finfo_[color].knightPressure_;
  return score;
}

Evaluator::FullScore Evaluator::evaluateFigures(Figure::Color color)
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  auto const ocolor = Figure::otherColor(color);
  BitMask mask_no_queens = mask_all_ & ~(fmgr.queen_mask(color));
  // bishops
  BitMask bimask = fmgr.bishop_mask(color);
  BitMask mask_all_no_bishops = mask_no_queens & ~bimask;
  BitMask const& bishop_from_king = magic_ns::bishop_moves(finfo_[ocolor].king_pos_, mask_all_) & inv_mask_all_;
  BitMask const& rook_from_king   = magic_ns::rook_moves(finfo_[ocolor].king_pos_, mask_all_) & inv_mask_all_;
  for(; bimask;)
  {
    int n = clear_lsb(bimask);
    int p = color ? Figure::mirrorIndex_[n] : n;
    auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
    finfo_[color].attack_mask_   |= bishop_moves;
    finfo_[color].bishopAttacks_ |= bishop_moves;
    finfo_[color].bishopMasks_.push_back(bishop_moves);
    // opponent's king attacks with x-ray
    auto x_bishop_moves = magic_ns::bishop_moves(n, mask_all_no_bishops);
    bool could_attack   = (finfo_[ocolor].kingAttacks_ & x_bishop_moves) != 0ULL;
    auto bi_dist        = distanceCounter().getDistance(n, finfo_[ocolor].king_pos_);
    bool could_check    = (bishop_from_king & bishop_moves) != 0;
    finfo_[color].bishopPressure_ += evalCoeffs().kingDistanceBishop_[bi_dist];
    finfo_[color].bishopPressure_ += evalCoeffs().bishopAttackBonus_ * (could_attack || could_check);
  }
  score.common_ += finfo_[color].bishopPressure_;
  // rooks
  BitMask rmask = fmgr.rook_mask(color);
  BitMask mask_all_no_rooks = mask_no_queens & ~rmask;
  for(; rmask;)
  {
    int n = clear_lsb(rmask);
    int p = color ? Figure::mirrorIndex_[n] : n;
    auto const& rook_moves = magic_ns::rook_moves(n, mask_all_);
    finfo_[color].attack_mask_ |= rook_moves;
    finfo_[color].rookAttacks_ |= rook_moves;
    finfo_[color].rookMasks_.push_back(rook_moves);
    auto r_dist = distanceCounter().getDistance(n, finfo_[ocolor].king_pos_);
    // opponent's king attacks with x-ray
    auto const& x_rook_moves = magic_ns::rook_moves(n, mask_all_no_rooks);
    bool could_attack = (finfo_[ocolor].kingAttacks_ & x_rook_moves) != 0ULL;
    bool could_check  = (rook_from_king & x_rook_moves) != 0ULL;
    finfo_[color].rookPressure_ += evalCoeffs().kingDistanceRook_[r_dist];
    finfo_[color].rookPressure_ += evalCoeffs().rookAttackBonus_ * (could_attack || could_check);

    // open column
    auto const& mask_col = pawnMasks().mask_column(Index(n).x());
    bool no_pw_color  = (mask_col & fmgr.pawn_mask(color)) == 0ULL;
    bool no_pw_ocolor = (mask_col & fmgr.pawn_mask(ocolor)) == 0ULL;
    score.common_ += no_pw_color  * evalCoeffs().semiopenRook_;
    score.common_ += no_pw_ocolor * evalCoeffs().semiopenRook_;
  }
  score.common_ += finfo_[color].rookPressure_;
  // queens
  BitMask qmask = fmgr.queen_mask(color);
  for(; qmask;)
  {
    int n = clear_lsb(qmask);
    int p = color ? Figure::mirrorIndex_[n] : n;
    auto queen_moves = magic_ns::queen_moves(n, mask_all_);
    finfo_[color].attack_mask_ |= queen_moves;
    finfo_[color].queenAttacks_ |= queen_moves;
    finfo_[color].queenMasks_.push_back(queen_moves);
    auto q_dist = distanceCounter().getDistance(n, finfo_[ocolor].king_pos_);
    // opponent's king attacks with x-ray
    auto const& x_rook_moves   = magic_ns::rook_moves(n, mask_all_no_rooks);
    auto const& x_bishop_moves = magic_ns::bishop_moves(n, mask_all_no_bishops);
    bool could_attack   = ((finfo_[ocolor].kingAttacks_ & x_bishop_moves) != 0ULL)
      || ((finfo_[ocolor].kingAttacks_ & x_rook_moves) != 0ULL);
    bool could_check    = ((rook_from_king & x_rook_moves) | (bishop_from_king & x_bishop_moves)) != 0ULL;
    finfo_[color].queenPressure_ += evalCoeffs().kingDistanceQueen_[q_dist];
    finfo_[color].queenPressure_ += evalCoeffs().queenAttackBonus_ * (could_attack || could_check);
  }
  score.common_ += finfo_[color].queenPressure_;
  return score;
}

Evaluator::FullScore Evaluator::evaluateMobility(Figure::Color color)
{
  FullScore score{};
  auto ocolor = Figure::otherColor(color);
  auto const& fmgr = board_->fmgr();
  auto const color_mask_inv = ~fmgr.mask(color);
  auto const o_attack_inv = ~finfo_[ocolor].attack_mask_;

  BitMask nb_guard = finfo_[color].pawnAttacks_;
  BitMask r_guard = nb_guard | finfo_[color].knightAttacks_ | finfo_[color].bishopAttacks_;
  BitMask q_guard = r_guard | finfo_[color].rookAttacks_ | finfo_[color].kingAttacks_;
  BitMask nb_attack = finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].knightAttacks_ | finfo_[ocolor].bishopAttacks_;
  BitMask r_attack = nb_attack | finfo_[ocolor].rookAttacks_;
  BitMask q_attack = r_attack | finfo_[ocolor].queenAttacks_;
  BitMask q_allow = o_attack_inv | fmgr.queen_mask(ocolor);
  BitMask r_allow = q_allow | fmgr.rook_mask(ocolor);
  BitMask nb_allow = r_allow | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor);

  BitMask q_blockers = ((q_guard & ~q_attack) | q_allow) & color_mask_inv;
  for(auto queen_moves : finfo_[color].queenMasks_)
  {
    queen_moves &= q_blockers;
    finfo_[color].queenMobility_ += evalCoeffs().queenMobility_[pop_count(queen_moves) & 31];
  }

  BitMask r_blockers = ((r_guard & ~r_attack) | r_allow) & color_mask_inv;
  for(auto rook_moves : finfo_[color].rookMasks_)
  {
    rook_moves &= r_blockers;
    finfo_[color].rookMobility_ += evalCoeffs().rookMobility_[pop_count(rook_moves) & 15];
  }

  BitMask nb_blockers = ((nb_guard & ~nb_attack) | nb_allow) & color_mask_inv;
  for(auto knight_moves : finfo_[color].knightMasks_)
  {
    knight_moves &= nb_blockers;
    finfo_[color].knightMobility_ += evalCoeffs().knightMobility_[pop_count(knight_moves) & 15];
  }
  for(auto bishop_moves : finfo_[color].bishopMasks_)
  {
    bishop_moves &= nb_blockers;
    finfo_[color].bishopMobility_ += evalCoeffs().bishopMobility_[pop_count(bishop_moves) & 15];
  }

  score.common_ += finfo_[color].knightMobility_;
  score.common_ += finfo_[color].bishopMobility_;
  score.common_ += finfo_[color].rookMobility_;
  score.common_ += finfo_[color].queenMobility_;
  return score;
}

} //NEngine
