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

Evaluator::FullScore Evaluator::evaluatePsq(Figure::Color color)
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  auto const ocolor = Figure::otherColor(color);
  // knights
  BitMask knmask = fmgr.knight_mask(color);
  for(; knmask;)
  {
    int n = clear_lsb(knmask);
    int p = color ? Figure::mirrorIndex_[n] : n;
    score.opening_ += evalCoeffs().knightPsq_[p];
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
    finfo_[color].attack_mask_ |= knight_moves;
    finfo_[color].knightAttacks_ |= knight_moves;
    finfo_[color].knightMasks_.push_back(knight_moves);
  }
  // bishops
  BitMask bimask = fmgr.bishop_mask(color);
  for(; bimask;)
  {
    int n = clear_lsb(bimask);
    int p = color ? Figure::mirrorIndex_[n] : n;
    score.opening_ += evalCoeffs().bishopPsq_[p];
    auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
    finfo_[color].attack_mask_ |= bishop_moves;
    finfo_[color].bishopAttacks_ |= bishop_moves;
    finfo_[color].bishopMasks_.push_back(bishop_moves);
  }
  // rooks
  BitMask rmask = fmgr.rook_mask(color);
  for(; rmask;)
  {
    int n = clear_lsb(rmask);
    int p = color ? Figure::mirrorIndex_[n] : n;
    score.opening_ += evalCoeffs().rookPsq_[p];
    auto rook_moves = magic_ns::rook_moves(n, mask_all_);
    finfo_[color].attack_mask_ |= rook_moves;
    finfo_[color].rookAttacks_ |= rook_moves;
    finfo_[color].rookMasks_.push_back(rook_moves);

    // open column
    auto const& mask_col = pawnMasks().mask_column(Index(n).x());
    bool no_pw_color = (mask_col & fmgr.pawn_mask(color)) == 0ULL;
    bool no_pw_ocolor = (mask_col & fmgr.pawn_mask(ocolor)) == 0ULL;
    score.common_ += no_pw_color  * evalCoeffs().semiopenRook_;
    score.common_ += no_pw_ocolor * evalCoeffs().semiopenRook_;
  }
  // queens
  BitMask qmask = fmgr.queen_mask(color);
  for(; qmask;)
  {
    int n = clear_lsb(qmask);
    int p = color ? Figure::mirrorIndex_[n] : n;
    score.opening_ += evalCoeffs().queenPsq_[p];
    auto queen_moves = magic_ns::queen_moves(n, mask_all_);
    finfo_[color].attack_mask_ |= queen_moves;
    finfo_[color].queenAttacks_ |= queen_moves;
    finfo_[color].queenMasks_.push_back(queen_moves);
  }
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
