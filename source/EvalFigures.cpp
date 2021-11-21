#include "Evaluator.h"

namespace NEngine
{

bool Evaluator::isPinned(int pos, Figure::Color color, Figure::Color ocolor, BitMask targets, BitMask attackers, nst::bishop_rook_dirs dir) const
{
  if (!targets)
    return false;
  BitMask tmask{};
  if (dir != nst::rook)
    tmask |= magic_ns::bishop_moves(pos, mask_all_);
  if (dir != nst::bishop)
    tmask |= magic_ns::rook_moves(pos, mask_all_);
  targets &= tmask;
  attackers &= tmask;
  if (!attackers)
    return false;
  while (targets)
  {
    int t = clear_lsb(targets);
    if (board_->isPinned(pos, mask_all_, ocolor, t, dir) & attackers)
      return true;
  }
  return false;
}

bool Evaluator::blockedKnight(Figure::Color color, int n) const
{
  auto const& fmgr = board_->fmgr();
  return (pawnMasks().blocked_knight(color, n) & fmgr.pawn_mask(Figure::otherColor(color))) != 0ULL;
}

bool Evaluator::blockedBishop(Figure::Color color, int n) const
{
  auto const& fmgr = board_->fmgr();
  return (pawnMasks().blocked_bishop(color, n) & fmgr.pawn_mask(Figure::otherColor(color))) != 0ULL;
}

void Evaluator::prepareAttacksMasks()
{
  auto const& fmgr = board_->fmgr();
  BitMask mask_all_not_pw[2] = {
    mask_all_ & ~fmgr.pawn_mask(Figure::ColorBlack), mask_all_ & ~fmgr.pawn_mask(Figure::ColorWhite)
  };
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto ocolor = Figure::otherColor(color);
    finfo_[color].knightMoves_ = BitMask{};
    finfo_[color].attackedByKnightRq_ = BitMask{};
    finfo_[color].behindPawnAttacks_ = BitMask{};
    finfo_[color].behindOPawnAttacks_ = BitMask{};
    finfo_[color].pinnedFigures_ = BitMask{};
    finfo_[color].checks_mask_ = finfo_[color].attack_mask_;

    BitMask nmask = board_->fmgr().knight_mask(color);
    for (; nmask;)
    {
      const int n = clear_lsb(nmask);
      auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
      finfo_[color].checks_mask_ |= knight_moves;
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        knight_moves = 0ULL;
        finfo_[color].pinnedFigures_ |= set_mask_bit(n);
      }
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & knight_moves;
      finfo_[color].attack_mask_ |= knight_moves;      
      moves_masks_[n] = knight_moves;
      finfo_[color].knightMoves_ |= knight_moves;
    }
    finfo_[color].nb_attacked_ = finfo_[color].knightMoves_;
    finfo_[color].nbr_attacked_ = finfo_[color].knightMoves_;
    finfo_[color].bishopMoves_ = BitMask{};
    BitMask bimask = fmgr.bishop_mask(color);
    for (; bimask;)
    {
      int n = clear_lsb(bimask);
      auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
      finfo_[color].checks_mask_ |= bishop_moves;
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        auto const from_mask = betweenMasks().from(board_->kingPos(color), n);
        bishop_moves &= from_mask;
        finfo_[color].pinnedFigures_ |= set_mask_bit(n);
      }
      moves_masks_[n] = bishop_moves;
      finfo_[color].nbr_attacked_ |= bishop_moves;
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & bishop_moves;
      finfo_[color].attack_mask_ |= bishop_moves;
      finfo_[color].bishopMoves_ |= bishop_moves;
    }
    finfo_[color].nb_attacked_ |= finfo_[color].bishopMoves_;
    finfo_[color].rookMoves_ = BitMask{};
    auto rmask = fmgr.rook_mask(color);
    for (; rmask;)
    {
      auto n = clear_lsb(rmask);
      finfo_[color].attackedByKnightRq_ |= movesTable().caps(Figure::TypeKnight, n);
      auto rook_moves = magic_ns::rook_moves(n, mask_all_);
      finfo_[color].checks_mask_ |= rook_moves;
      auto nrook_moves = ~rook_moves;
      auto rook_moves_p = magic_ns::rook_moves(n, mask_all_not_pw[color]) & pawnMasks().mask_forward(color, n) & nrook_moves;
      auto rook_moves_op = magic_ns::rook_moves(n, mask_all_not_pw[ocolor]) & pawnMasks().mask_forward(ocolor, n) & nrook_moves;
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        auto const from_mask = betweenMasks().from(board_->kingPos(color), n);
        rook_moves &= from_mask;
        rook_moves_p &= from_mask;
        rook_moves_op &= from_mask;
        finfo_[color].pinnedFigures_ |= set_mask_bit(n);
      }
      moves_masks_[n] = rook_moves;
      finfo_[color].nbr_attacked_ |= rook_moves;
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & rook_moves;
      finfo_[color].attack_mask_ |= rook_moves;
      finfo_[color].rookMoves_ |= rook_moves;
      finfo_[color].behindPawnAttacks_ |= rook_moves_p;
      finfo_[color].behindOPawnAttacks_ |= rook_moves_op;
    }
    finfo_[color].rq_attacked_ = finfo_[color].r_attacked_ = finfo_[color].rookMoves_;
    finfo_[color].queenMoves_ = BitMask{};
    auto qmask = fmgr.queen_mask(color);
    for (; qmask;)
    {
      auto n = clear_lsb(qmask);
      finfo_[color].attackedByKnightRq_ |= movesTable().caps(Figure::TypeKnight, n);
      auto qr_attacks = magic_ns::rook_moves(n, mask_all_);
      auto nqr_attacks = ~qr_attacks;
      auto queen_moves_p = magic_ns::rook_moves(n, mask_all_not_pw[color]) & pawnMasks().mask_forward(color, n) & nqr_attacks;
      auto queen_moves_op = magic_ns::rook_moves(n, mask_all_not_pw[ocolor]) & pawnMasks().mask_forward(ocolor, n) & nqr_attacks;
      auto queen_moves = magic_ns::queen_moves(n, mask_all_);
      finfo_[color].checks_mask_ |= queen_moves;
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        auto const from_mask = betweenMasks().from(board_->kingPos(color), n);
        queen_moves &= from_mask;
        queen_moves_p &= from_mask;
        qr_attacks &= from_mask;
        finfo_[color].pinnedFigures_ |= set_mask_bit(n);
      }
      finfo_[color].rq_attacked_ |= queen_moves;
      finfo_[color].r_attacked_ |= qr_attacks;
      moves_masks_[n] = queen_moves;
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & queen_moves;
      finfo_[color].attack_mask_ |= queen_moves;
      finfo_[color].queenMoves_ |= queen_moves;
      finfo_[color].behindPawnAttacks_ |= queen_moves_p;
      finfo_[color].behindOPawnAttacks_ |= queen_moves_op;
    }
  }
  finfo_[Figure::ColorBlack].cango_mask_ &= ~(finfo_[Figure::ColorWhite].attack_mask_ & ~finfo_[Figure::ColorBlack].multiattack_mask_);
  finfo_[Figure::ColorWhite].cango_mask_ &= ~(finfo_[Figure::ColorBlack].attack_mask_ & ~finfo_[Figure::ColorWhite].multiattack_mask_);
}

ScoreType32 Evaluator::evaluateKnights()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    finfo_[color].score_mob_ = ScoreType32{};
    finfo_[color].discoveredCheck_ = false;

#ifdef DO_KING_EVAL
    finfo_[color].num_attackers_ = 0;
    finfo_[color].score_king_ = 0;
#endif

    auto ocolor = Figure::otherColor(color);
    BitMask mask = board_->fmgr().knight_mask(color);
    BitMask outpost_mask = finfo_[color].pawnAttacks_ & ~finfo_[ocolor].pawnPossibleAttacks_ & Figure::outpostMask_[color];
    for (; mask;)
    {
      const int n = clear_lsb(mask);
      auto const knight_moves = moves_masks_[n];
      // outpost
      const auto nbit = set_mask_bit(n);
      const bool boutpost = ((nbit | (knight_moves & ~fmgr.mask(color))) & outpost_mask) != 0ULL;
      score[color] += EvalCoefficients::knightOutpost_ * boutpost;
      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];      
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(n, ocolor), "discovered check not detected");
      if (knight_moves && discoveredCheck(n, ocolor)) {
        finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
        finfo_[color].discoveredCheck_ = true;
      }
      
#ifdef MOBILITY_EXTENDED
      bool qpinned = false;
      if(knight_moves)
      {
        if (isPinned(n, color, ocolor, finfo_[color].rq_mask_, fmgr.bishop_mask(ocolor), nst::none)) {
          qpinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::knightPinned_;
        }
      }
#endif // king protection, discovered checks etc...

#ifdef DO_KING_EVAL
      if (knight_moves & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        bool b_attacks = (knight_moves & finfo_[ocolor].ki_fields_no_pw_) != 0ULL;
        finfo_[color].score_king_ += EvalCoefficients::knightKingAttack_ * b_attacks + EvalCoefficients::basicAttack_ * (!b_attacks);
      }
#endif
      
      auto n_moves_mask = knight_moves & (finfo_[color].cango_mask_ | finfo_[ocolor].nbrq_mask_);
      auto n_moves = pop_count(n_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::knightMobility_[n_moves & 15];

#ifdef MOBILITY_EXTENDED
      if ((qpinned || !(n_moves_mask & ~fmgr.mask(color))) && (finfo_[ocolor].pawnAttacks_ & set_mask_bit(n))) {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_[0];
      }
#endif // king protection, discovered checks etc...
    }
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

ScoreType32 Evaluator::evaluateBishops()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    finfo_[color].bishopTreatAttacks_ = BitMask{};
    auto ocolor = Figure::otherColor(color);
    BitMask mask = fmgr.bishop_mask(color);
    auto const pwmask = fmgr.pawn_mask(color);

    // bishop on the same color square as its pawns
    if (mask) {
      auto bi_mask_w = mask &  FiguresCounter::s_whiteMask_;
      auto bi_mask_b = mask & ~FiguresCounter::s_whiteMask_;
      if (bi_mask_w)
        score[color] += EvalCoefficients::pawnsOnBishopSquares_ * pop_count(pwmask &  FiguresCounter::s_whiteMask_);
      if (bi_mask_b)
        score[color] += EvalCoefficients::pawnsOnBishopSquares_ * pop_count(pwmask & ~FiguresCounter::s_whiteMask_);
    }

    for (; mask;)
    {
      int n = clear_lsb(mask);
      auto const bishop_moves = moves_masks_[n];
      BitMask bishop_moves_x{};
      if (!(finfo_[color].pinnedFigures_ & set_mask_bit(n))) {
        bishop_moves_x = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_ | finfo_[color].pinnedFigures_) & ~bishop_moves;
        finfo_[color].multiattack_mask_ |= bishop_moves_x;
      }
      bishop_moves_x |= bishop_moves;

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];

      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(n, ocolor), "discovered check not detected");
      if (bishop_moves && !finfo_[color].discoveredCheck_ && discoveredCheck(n, ocolor)) {
        finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
        finfo_[color].discoveredCheck_ = true;
      }

#ifdef MOBILITY_EXTENDED
      bool q_pinned = false;
      if(bishop_moves)
      {
        if (isPinned(n, color, ocolor, fmgr.queen_mask(color), fmgr.rook_mask(ocolor), nst::rook)) {
          q_pinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::bishopPinned_;
        }
      }
#endif // king protection, discovered checks etc...

#ifdef DO_KING_EVAL
      if (bishop_moves_x & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        bool b_attacks = (bishop_moves_x & (finfo_[ocolor].ki_fields_no_pw_)) != 0ULL;
        finfo_[color].score_king_ += EvalCoefficients::bishopKingAttack_ * b_attacks + EvalCoefficients::basicAttack_ * (!b_attacks);
      }
#endif

      // treat attacks
      if (bishop_moves) {
        auto mask_all_no_orq = mask_all_ & ~(board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor));
        auto bishop_treat = magic_ns::bishop_moves(n, mask_all_no_orq);
        finfo_[color].bishopTreatAttacks_ |= bishop_treat;
      }

      // mobility
      auto b_moves_mask = bishop_moves & (finfo_[color].cango_mask_ | finfo_[ocolor].nbrq_mask_);
      int n_moves = pop_count(b_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::bishopMobility_[n_moves & 15];

#ifdef MOBILITY_EXTENDED
      if ((q_pinned || !(b_moves_mask & ~fmgr.mask(color))) && (finfo_[ocolor].pawnAttacks_ & set_mask_bit(n)) != 0ULL) {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_[1];
      }
#endif // king protection, discovered checks etc...
    }
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

ScoreType32 Evaluator::evaluateRook()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    finfo_[color].rookTreatAttacks_ = BitMask{};

    auto mask = fmgr.rook_mask(color);
    auto ocolor = Figure::otherColor(color);
    for (; mask;)
    {
      auto n = clear_lsb(mask);
      auto const rook_moves = moves_masks_[n];
      BitMask rook_moves_x{};
      if (!(finfo_[color].pinnedFigures_ & set_mask_bit(n))) {
        rook_moves_x = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_ | finfo_[color].pinnedFigures_) & ~rook_moves;
        finfo_[color].multiattack_mask_ |= rook_moves_x;
      }
      rook_moves_x |= rook_moves;

      // open column
      auto const mask_fwd = pawnMasks().mask_forward(color, n);
      if(!(mask_fwd & fmgr.pawn_mask(color)))
      {
        bool pw_ocolor_n = (mask_fwd & fmgr.pawn_mask(ocolor)) != 0ULL;
        score[color] += EvalCoefficients::openRook_[pw_ocolor_n];
      }

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeRook][ki_dist];

      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(n, ocolor), "discovered check not detected");
      if (rook_moves && !finfo_[color].discoveredCheck_ && discoveredCheck(n, ocolor)) {
        finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
        finfo_[color].discoveredCheck_ = true;
      }

#ifdef MOBILITY_EXTENDED
      bool q_pinned = false;
      if(rook_moves)
      {
        if (isPinned(n, color, ocolor, finfo_[color].rq_mask_, fmgr.bishop_mask(ocolor), nst::bishop)) {
          q_pinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::rookPinned_;
        }
      }
#endif // king protection, discovered checks etc...

#ifdef DO_KING_EVAL
      if (rook_moves_x & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        bool b_attacks = (rook_moves_x & finfo_[ocolor].ki_fields_no_pw_) != 0ULL;
        finfo_[color].score_king_ += EvalCoefficients::rookKingAttack_ * b_attacks + EvalCoefficients::basicAttack_ * (!b_attacks);
      }
#endif

      if (rook_moves) {
        auto mask_all_no_oq = mask_all_ & ~board_->fmgr().queen_mask(ocolor);
        auto rook_treat = magic_ns::rook_moves(n, mask_all_no_oq);
        finfo_[color].rookTreatAttacks_ |= rook_treat;
      }
      
      auto r_moves_mask = rook_moves & ((finfo_[color].cango_mask_ & ~finfo_[ocolor].nb_attacked_) | finfo_[ocolor].rq_mask_);
      int n_moves = pop_count(r_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::rookMobility_[n_moves & 15];

      // fake castle possible
      if (n_moves < 3) {
        if (fakeCastle(color, n, r_moves_mask)) {
          finfo_[color].score_mob_ += EvalCoefficients::fakeCastle_;
        }
        else if (blockedRook(color, n, r_moves_mask))
          finfo_[color].score_mob_ -= EvalCoefficients::rookBlocked_;
      }

#ifdef MOBILITY_EXTENDED
      if ((q_pinned || !(r_moves_mask & ~fmgr.mask(color) & ~finfo_[ocolor].bishopTreatAttacks_)) &&
          ((finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].nb_attacked_) & set_mask_bit(n)) != 0ULL)
      {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_[2];
      }
#endif // king protection, discovered checks etc...
    }
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

ScoreType32 Evaluator::evaluateQueens()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    finfo_[color].qkingAttack_ = false;
    auto mask = fmgr.queen_mask(color);
    auto ocolor = Figure::otherColor(color);
    for (; mask;)
    {
      auto n = clear_lsb(mask);
      auto const queen_moves = moves_masks_[n];
      BitMask queen_moves_xr{};
      if (!(finfo_[color].pinnedFigures_ & set_mask_bit(n))) {
        queen_moves_xr = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_ | finfo_[color].pinnedFigures_);
        auto queen_moves_x = (queen_moves_xr | magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_ | finfo_[color].pinnedFigures_))
          & ~queen_moves;
        finfo_[color].multiattack_mask_ |= queen_moves_x;
      }
      queen_moves_xr |= queen_moves;

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][ki_dist];

#ifdef DO_KING_EVAL
      bool qkattack = queen_moves_xr & finfo_[ocolor].ki_fields_;
      finfo_[color].qkingAttack_ |= qkattack;
      if (qkattack)
      {
        finfo_[color].num_attackers_++;
        bool b_attacks = (queen_moves_xr & finfo_[ocolor].ki_fields_no_pw_) != 0ULL;
        finfo_[color].score_king_ += EvalCoefficients::queenKingAttack_ * b_attacks + EvalCoefficients::basicAttack_ * (!b_attacks);
      }
#endif

      auto q_moves_mask = queen_moves & ((finfo_[color].cango_mask_ & ~finfo_[ocolor].nbr_attacked_) | fmgr.queen_mask(ocolor));
      auto n_moves = pop_count(q_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::queenMobility_[n_moves & 31];

#ifdef MOBILITY_EXTENDED
      if (!(q_moves_mask & ~fmgr.mask(color) & ~finfo_[ocolor].rookTreatAttacks_ & ~finfo_[ocolor].bishopTreatAttacks_) &&
        (((finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].nbr_attacked_) & set_mask_bit(n)) != 0ULL))
      {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_[3];
      }
#endif // king protection, discovered checks etc...
    }
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

} // NEngine
