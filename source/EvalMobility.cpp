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

int Evaluator::evaluateFigures()
{
  int score = 0;

  calculateKnightsMoves();
  calculateBishopsMoves();
  calculateRooksMoves();
  calculateQueenMoves();

  calculateKnightsMobility();
  calculateBishopsMobility();
  calculateRooksMobility();
  calculateQueenMobility();

  score -= finfo_[0].knightMobility_;
  score += finfo_[1].knightMobility_;

  score -= finfo_[0].bishopMobility_;
  score += finfo_[1].bishopMobility_;

  score -= finfo_[0].rookMobility_;
  score += finfo_[1].rookMobility_;

  score -= finfo_[0].queenMobility_;
  score += finfo_[1].queenMobility_;

  //score -= finfo_[0].knightAttackBonus_;
  //score += finfo_[1].knightAttackBonus_;

  //score -= finfo_[0].bishopAttackBonus_;
  //score += finfo_[1].bishopAttackBonus_;

  //score -= finfo_[0].rookAttackBonus_;
  //score += finfo_[1].rookAttackBonus_;

  //score -= finfo_[0].queenAttackBonus_;
  //score += finfo_[1].queenAttackBonus_;

  return score;
}

int Evaluator::evaluateFields()
{
  auto evaluate_king_attack = [this](Figure::Color color, Figure::Color ocolor)
  {
    auto kimask = movesTable().caps(Figure::TypeKing, finfo_[ocolor].king_pos_);
    if(!(finfo_[color].attack_mask_ & kimask))
      return 0;
    int score = 0;
    score += pop_count(finfo_[color].pawnAttacks_ & kimask) * coeffs_->pawnKingAttack_;
    score += pop_count(finfo_[color].knightAttacks_  & kimask) * coeffs_->knightKingAttack_;
    score += pop_count(finfo_[color].bishopAttacks_  & kimask) * coeffs_->bishopKingAttack_;
    score += pop_count(finfo_[color].rookAttacks_    & kimask) * coeffs_->rookKingAttack_;
    score += pop_count(finfo_[color].queenAttacks_   & kimask) * coeffs_->queenKingAttack_;
    return score;
  };
  auto score_w = evaluate_king_attack(Figure::ColorWhite, Figure::ColorBlack);
  auto score_b = evaluate_king_attack(Figure::ColorBlack, Figure::ColorWhite);
  int score = score_w - score_b;
  return score;
}

void Evaluator::calculateKnightsMoves()
{
  auto calculate_moves = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    BitMask knmask = board_->fmgr().knight_mask(color);
    BitMask blockers = ~(finfo_[ocolor].pawnAttacks_ | board_->fmgr().mask(color));
    for(; knmask;)
    {
      int n = clear_lsb(knmask);
      auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
      finfo_[color].knightAttacks_ |= knight_moves;
      finfo_[color].allButBishopAttacks_ |= knight_moves;
      finfo_[color].allButRookAttacks_ |= knight_moves;
      finfo_[color].allButQueenAttacks_ |= knight_moves;
      knight_moves &= blockers;
      finfo_[color].knightMasks_.push_back(knight_moves);
    }
    finfo_[color].attack_mask_ |= finfo_[color].knightAttacks_;
  };
  calculate_moves(Figure::ColorWhite);
  calculate_moves(Figure::ColorBlack);
}

void Evaluator::calculateBishopsMoves()
{
  auto calculate_moves = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    BitMask bimask = board_->fmgr().bishop_mask(color);
    BitMask blockers = ~(finfo_[ocolor].pawnAttacks_ | board_->fmgr().mask(color));
    for(; bimask;)
    {
      int n = clear_lsb(bimask);
      auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
      finfo_[color].bishopAttacks_ |= bishop_moves;
      finfo_[color].allButKnightAttacks_ |= bishop_moves;
      finfo_[color].allButRookAttacks_ |= bishop_moves;
      finfo_[color].allButQueenAttacks_ |= bishop_moves;
      bishop_moves &= blockers;
      finfo_[color].bishopMasks_.push_back(bishop_moves);
    }
    finfo_[color].attack_mask_ |= finfo_[color].bishopAttacks_;
  };
  calculate_moves(Figure::ColorWhite);
  calculate_moves(Figure::ColorBlack);
}

void Evaluator::calculateRooksMoves()
{
  auto calculate_moves = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    BitMask romask = board_->fmgr().rook_mask(color);
    BitMask blockers = ~(finfo_[ocolor].pawnAttacks_
                          | finfo_[ocolor].knightAttacks_
                          | finfo_[ocolor].bishopAttacks_
                          | board_->fmgr().mask(color));
    for(; romask;)
    {
      int n = clear_lsb(romask);
      auto rook_moves = magic_ns::rook_moves(n, mask_all_);
      finfo_[color].rookAttacks_ |= rook_moves;
      finfo_[color].allButKnightAttacks_ |= rook_moves;
      finfo_[color].allButBishopAttacks_ |= rook_moves;
      finfo_[color].allButQueenAttacks_ |= rook_moves;
      rook_moves &= blockers;
      finfo_[color].rookMasks_.push_back(rook_moves);
    }
    finfo_[color].attack_mask_ |= finfo_[color].rookAttacks_;
  };
  calculate_moves(Figure::ColorWhite);
  calculate_moves(Figure::ColorBlack);
}

void Evaluator::calculateQueenMoves()
{
  auto calculate_moves = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    BitMask qumask = board_->fmgr().queen_mask(color);
    BitMask blockers = ~(finfo_[ocolor].pawnAttacks_
                          | finfo_[ocolor].knightAttacks_
                          | finfo_[ocolor].bishopAttacks_
                          | finfo_[ocolor].rookAttacks_
                          | board_->fmgr().mask(color));
    for(; qumask;)
    {
      int n = clear_lsb(qumask);
      auto queen_moves = magic_ns::queen_moves(n, mask_all_);
      finfo_[color].queenAttacks_ |= queen_moves;
      finfo_[color].allButKnightAttacks_ |= queen_moves;
      finfo_[color].allButBishopAttacks_ |= queen_moves;
      finfo_[color].allButRookAttacks_ |= queen_moves;
      queen_moves &= blockers;
      finfo_[color].queenMasks_.push_back(queen_moves);
    }
    finfo_[color].attack_mask_ |= finfo_[color].queenAttacks_;
  };
  calculate_moves(Figure::ColorWhite);
  calculate_moves(Figure::ColorBlack);
}

void Evaluator::calculateKnightsMobility()
{
  auto calculate_mobility = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    BitMask blockers = ~finfo_[ocolor].attack_mask_
      | finfo_[color].allButKnightAttacks_
      | board_->fmgr().knight_mask(ocolor)
      | board_->fmgr().bishop_mask(ocolor)
      | board_->fmgr().rook_mask(ocolor)
      | board_->fmgr().queen_mask(ocolor);
    int score = 0;
    for(auto knight_moves : finfo_[color].knightMasks_)
    {
      knight_moves &= blockers;
      score += coeffs_->knightMobility_[pop_count(knight_moves) & 15];
    }
    return score;
  };
  finfo_[Figure::ColorWhite].knightMobility_ = calculate_mobility(Figure::ColorWhite);
  finfo_[Figure::ColorBlack].knightMobility_ = calculate_mobility(Figure::ColorBlack);
}

void Evaluator::calculateBishopsMobility()
{
  auto calculate_mobility = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    BitMask blockers = ~finfo_[ocolor].attack_mask_
      | finfo_[color].allButBishopAttacks_
      | board_->fmgr().knight_mask(ocolor)
      | board_->fmgr().bishop_mask(ocolor)
      | board_->fmgr().rook_mask(ocolor)
      | board_->fmgr().queen_mask(ocolor);
    int score = 0;
    for(auto bishop_moves : finfo_[color].bishopMasks_)
    {
      bishop_moves &= blockers;
      score += coeffs_->bishopMobility_[pop_count(bishop_moves) & 15];
    }
    return score;
  };
  finfo_[Figure::ColorWhite].bishopMobility_ = calculate_mobility(Figure::ColorWhite);
  finfo_[Figure::ColorBlack].bishopMobility_ = calculate_mobility(Figure::ColorBlack);
}

void Evaluator::calculateRooksMobility()
{
  auto calculate_mobility = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    BitMask blockers = ~finfo_[ocolor].attack_mask_
      | finfo_[color].allButRookAttacks_
      | board_->fmgr().rook_mask(ocolor)
      | board_->fmgr().queen_mask(ocolor);
    int score = 0;
    for(auto rook_moves : finfo_[color].rookMasks_)
    {
      rook_moves &= blockers;
      score += coeffs_->rookMobility_[pop_count(rook_moves) & 15];
    }
    return score;
  };
  finfo_[Figure::ColorWhite].rookMobility_ = calculate_mobility(Figure::ColorWhite);
  finfo_[Figure::ColorBlack].rookMobility_ = calculate_mobility(Figure::ColorBlack);
}

void Evaluator::calculateQueenMobility()
{
  auto calculate_mobility = [this](Figure::Color color)
  {
    auto ocolor = Figure::otherColor(color);
    BitMask blockers = ~finfo_[ocolor].attack_mask_
      | finfo_[color].allButQueenAttacks_
      | board_->fmgr().queen_mask(ocolor);
    int score = 0;
    for(auto queen_moves : finfo_[color].queenMasks_)
    {
      queen_moves &= blockers;
      score += coeffs_->queenMobility_[pop_count(queen_moves) & 31];
    }
    return score;
  };
  finfo_[Figure::ColorWhite].queenMobility_ = calculate_mobility(Figure::ColorWhite);
  finfo_[Figure::ColorBlack].queenMobility_ = calculate_mobility(Figure::ColorBlack);
}

} //NEngine
