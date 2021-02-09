#include <Evaluator.h>

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

ScoreType32 Evaluator::evaluateKnights()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    finfo_[color].score_mob_ = ScoreType32{};
    finfo_[color].discoveredCheck_ = false;
    finfo_[color].knightMoves_ = BitMask{};
#ifdef DO_KING_EVAL
    finfo_[color].num_attackers_ = 0;
    finfo_[color].score_king_ = 0;
#endif

    auto ocolor = Figure::otherColor(color);
    BitMask mask = board_->fmgr().knight_mask(color);
    for (; mask;)
    {
      const int n = clear_lsb(mask);
      auto knight_moves = movesTable().caps(Figure::TypeKnight, n);

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];
      
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(n, ocolor), "discovered check not detected");
      if (discoveredCheck(n, ocolor)) {
        finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
        finfo_[color].discoveredCheck_ = true;
      }
      
      bool qpinned = false;
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        knight_moves = 0ULL;
      }
#ifdef MOBILITY_EXTENDED
      else
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
        auto n_attacks = pop_count(knight_moves & finfo_[ocolor].kingAttacks_);
        finfo_[color].score_king_ += EvalCoefficients::knightKingAttack_ * n_attacks + EvalCoefficients::basicAttack_ * (!n_attacks);
      }
#endif
      
      if (knight_moves) {
        finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & knight_moves;
        finfo_[color].attack_mask_ |= knight_moves;
        finfo_[color].knightMoves_ |= knight_moves;
      }
      
      auto n_moves_mask = knight_moves & (finfo_[color].cango_mask_ | finfo_[ocolor].nbrq_mask_);
      auto n_moves = pop_count(n_moves_mask);

      finfo_[color].score_mob_ += EvalCoefficients::knightMobility_[n_moves & 15];

      if ((n_moves < 2 || !(n_moves_mask & ~finfo_[ocolor].attack_mask_)) && blockedKnight(color, n)) {
        finfo_[color].score_mob_ -= EvalCoefficients::knightBlocked_;
      }

#ifdef MOBILITY_EXTENDED
      if ((qpinned || !(n_moves_mask & ~fmgr.mask(color))) && (finfo_[ocolor].pawnAttacks_ & set_mask_bit(n))) {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
#endif // king protection, discovered checks etc...
    }
    finfo_[color].nb_attacked_ = finfo_[color].knightMoves_;
    finfo_[color].nbr_attacked_ = finfo_[color].knightMoves_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

ScoreType32 Evaluator::evaluateBishops()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    finfo_[color].bishopMoves_ = BitMask{};
    finfo_[color].bishopTreatAttacks_ = BitMask{};

    auto ocolor = Figure::otherColor(color);
    BitMask mask = fmgr.bishop_mask(color);
    for (; mask;)
    {
      int n = clear_lsb(mask);

      auto const& bishop_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_);
      auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];

      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(n, ocolor), "discovered check not detected");
      if (!finfo_[color].discoveredCheck_ && discoveredCheck(n, ocolor)) {
        finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
        finfo_[color].discoveredCheck_ = true;
      }
      bool q_pinned = false;
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        bishop_moves = 0ULL;
      }
#ifdef MOBILITY_EXTENDED
      else
      {
        if (isPinned(n, color, ocolor, fmgr.queen_mask(color), fmgr.rook_mask(ocolor), nst::rook)) {
          q_pinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::bishopPinned_;
        }
      }
#endif // king protection, discovered checks etc...

#ifdef DO_KING_EVAL
      if (bishop_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        auto n_attacks = pop_count(bishop_attacks & finfo_[ocolor].kingAttacks_);
        finfo_[color].score_king_ += EvalCoefficients::bishopKingAttack_ * n_attacks + EvalCoefficients::basicAttack_ * (!n_attacks);
      }
#endif

      // mobility
      if (bishop_moves) {
        finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & bishop_attacks;
        finfo_[color].attack_mask_ |= bishop_attacks;
        auto mask_all_no_orq = mask_all_ & ~(board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor));
        auto bishop_treat = magic_ns::bishop_moves(n, mask_all_no_orq);
        finfo_[color].bishopTreatAttacks_ |= bishop_treat;
        finfo_[color].bishopMoves_ |= bishop_moves;
      }

      auto b_moves_mask = bishop_moves & (finfo_[color].cango_mask_ | finfo_[ocolor].nbrq_mask_);
      int n_moves = pop_count(b_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::bishopMobility_[n_moves & 15];

      if ((n_moves < 2 || !(b_moves_mask & ~finfo_[ocolor].attack_mask_)) && blockedBishop(color, n)) {
        finfo_[color].score_mob_ -= EvalCoefficients::bishopBlocked_;
      }

#ifdef MOBILITY_EXTENDED
      if ((q_pinned || !(b_moves_mask & ~fmgr.mask(color))) && (finfo_[ocolor].pawnAttacks_ & set_mask_bit(n)) != 0ULL) {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
#endif // king protection, discovered checks etc...
    }
    finfo_[color].nb_attacked_ |= finfo_[color].bishopMoves_;
    finfo_[color].nbr_attacked_ |= finfo_[color].bishopMoves_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

ScoreType32 Evaluator::evaluateRook()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    finfo_[color].rookMoves_ = BitMask{};
    finfo_[color].rookTreatAttacks_ = BitMask{};

    auto mask = fmgr.rook_mask(color);
    auto ocolor = Figure::otherColor(color);
    for (; mask;)
    {
      auto n = clear_lsb(mask);
      // mobility
      auto const& rook_attacks = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
      auto rook_moves = magic_ns::rook_moves(n, mask_all_);

      // open column
      auto const& mask_col = pawnMasks().mask_column(n & 7);
      if(!(mask_col & fmgr.pawn_mask(color)))
      {
        bool pw_ocolor_n = (mask_col & fmgr.pawn_mask(ocolor)) != 0ULL;
        score[color] += EvalCoefficients::openRook_[pw_ocolor_n];
      }

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeRook][ki_dist];

      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(n, ocolor), "discovered check not detected");
      if (!finfo_[color].discoveredCheck_ && discoveredCheck(n, ocolor)) {
        finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
        finfo_[color].discoveredCheck_ = true;
      }

      bool q_pinned = false;
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        rook_moves = 0ULL;
      }
#ifdef MOBILITY_EXTENDED
      else
      {
        if (isPinned(n, color, ocolor, finfo_[color].rq_mask_, fmgr.bishop_mask(ocolor), nst::bishop)) {
          q_pinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::rookPinned_;
        }
      }
#endif // king protection, discovered checks etc...

#ifdef DO_KING_EVAL
      if (rook_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        auto n_attacks = pop_count(rook_attacks & finfo_[ocolor].kingAttacks_);
        finfo_[color].score_king_ += EvalCoefficients::rookKingAttack_ * n_attacks + EvalCoefficients::basicAttack_ * (!n_attacks);
      }
#endif

      if (rook_moves) {
        finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & rook_attacks;
        finfo_[color].attack_mask_ |= rook_attacks;
        auto mask_all_no_oq = mask_all_ & ~board_->fmgr().queen_mask(ocolor);
        auto rook_treat = magic_ns::rook_moves(n, mask_all_no_oq);
        finfo_[color].rookTreatAttacks_ |= rook_treat;
        finfo_[color].rookMoves_ |= rook_moves;
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
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
#endif // king protection, discovered checks etc...
    }
    finfo_[color].nbr_attacked_ |= finfo_[color].rookMoves_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

ScoreType32 Evaluator::evaluateQueens()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    finfo_[color].queenMoves_ = BitMask{};

    auto mask = fmgr.queen_mask(color);
    auto ocolor = Figure::otherColor(color);
    for (; mask;)
    {
      auto n = clear_lsb(mask);
      // mobility
      auto qr_attacks = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
      auto const queen_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_) | qr_attacks;
      auto queen_moves = magic_ns::queen_moves(n, mask_all_);

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][ki_dist];

#ifdef DO_KING_EVAL
      auto qx_attacks = queen_moves | qr_attacks;
      if (qx_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        auto n_attacks = pop_count(qx_attacks & finfo_[ocolor].kingAttacks_);
        finfo_[color].score_king_ += EvalCoefficients::queenKingAttack_ * n_attacks + EvalCoefficients::basicAttack_ * (!n_attacks);
      }
#endif

      // pinned
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        auto from_mask = betweenMasks().from(board_->kingPos(color), n);
        queen_moves &= from_mask;
      }

      // mobility
      if (queen_moves) {
        finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & queen_attacks;
        finfo_[color].attack_mask_ |= queen_attacks;
        finfo_[color].queenMoves_ |= queen_moves;
      }

      auto q_moves_mask = queen_moves & ((finfo_[color].cango_mask_ & ~finfo_[ocolor].nbr_attacked_) | fmgr.queen_mask(ocolor));
      auto n_moves = pop_count(q_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::queenMobility_[n_moves & 31];

#ifdef MOBILITY_EXTENDED
      if (!(q_moves_mask & ~fmgr.mask(color) & ~finfo_[ocolor].rookTreatAttacks_ & ~finfo_[ocolor].bishopTreatAttacks_) &&
        (((finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].nbr_attacked_) & set_mask_bit(n)) != 0ULL))
      {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
#endif // king protection, discovered checks etc...
    }
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

#ifdef DO_KING_EVAL
ScoreType32 Evaluator::evaluateKingPressure(Figure::Color color)
{
  const auto& fmgr = board_->fmgr();
  const auto ocolor = Figure::otherColor(color);
  const auto  ki_pos = board_->kingPos(color);
  const auto oki_pos = board_->kingPos(ocolor);

  const auto& oki_fields = finfo_[ocolor].ki_fields_;
  const auto& near_oking = oki_fields & finfo_[ocolor].kingAttacks_;
  const auto near_oking_pw = near_oking | (oki_fields & fmgr.pawn_mask(ocolor));
  const auto attacked_any_but_oking = finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[ocolor].kingAttacks_);
  const auto attacked_oking_only = finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].multiattack_mask_;
  
  const auto onbrp_attacked = finfo_[ocolor].nbr_attacked_ | finfo_[ocolor].pawnAttacks_;
  const auto onbp_attacked  = finfo_[ocolor].nb_attacked_ | finfo_[ocolor].pawnAttacks_;
  const auto can_check_q = ~(fmgr.mask(color) | attacked_any_but_oking | (attacked_oking_only & ~finfo_[color].multiattack_mask_));
  const auto can_check_r = ~(fmgr.mask(color) | onbrp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_)) |
    fmgr.queen_mask(ocolor);
  const auto can_check_nb = ~(fmgr.mask(color) | onbp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_)) |
    fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);

  finfo_[color].num_attackers_ += (finfo_[color].pawnAttacks_ & near_oking) != 0ULL;
  finfo_[color].num_attackers_ += (finfo_[color].kingAttacks_ & near_oking) != 0ULL;
  finfo_[color].score_king_ += EvalCoefficients::pawnKingAttack_ * pop_count(finfo_[color].pawnAttacks_ & near_oking_pw);

  auto kn_check = movesTable().caps(Figure::TypeKnight, oki_pos) & finfo_[color].knightMoves_;
  auto bi_check = magic_ns::bishop_moves(oki_pos, mask_all_);
  auto r_check = magic_ns::rook_moves(oki_pos, mask_all_);
  auto q_check = (bi_check | r_check) & finfo_[color].queenMoves_;
  bi_check &= finfo_[color].bishopMoves_;
  r_check &= finfo_[color].rookMoves_;

  if(!finfo_[color].discoveredCheck_) {
    X_ASSERT_R(board_->discoveredCheck(ki_pos, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(ki_pos, ocolor), "discovered check not detected");
    X_ASSERT(board_->discoveredCheck(ki_pos, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(ki_pos, ocolor), "discovered check not detected");
    finfo_[color].discoveredCheck_ = discoveredCheck(ki_pos, ocolor);
  }

  kn_check &= can_check_nb;
  bi_check &= can_check_nb;
  r_check &= can_check_r;
  q_check &= can_check_q;
  int num_checkers = (kn_check != 0ULL) + (bi_check != 0ULL) + (r_check != 0ULL) + (q_check != 0ULL) + finfo_[color].discoveredCheck_;
  
  auto check_score = EvalCoefficients::knightChecking_ * (kn_check != 0) +
                     EvalCoefficients::bishopChecking_ * (bi_check != 0) +
                     EvalCoefficients::rookChecking_ * (r_check != 0) +
                     EvalCoefficients::queenChecking_ * (q_check != 0);
  
  int num_attackers = std::min(finfo_[color].num_attackers_, 7);
  auto attack_coeff = EvalCoefficients::kingAttackersCoefficients[num_attackers];

  const auto near_oking_att = (finfo_[ocolor].kingAttacks_ & ~attacked_any_but_oking) & finfo_[color].attack_mask_;
  if (near_oking_att) {
    auto near_king_coeff = EvalCoefficients::attackedNearKingCoeff_ * pop_count(near_oking_att);
    attack_coeff += near_king_coeff;
  }
      
  auto protected_oking = (finfo_[ocolor].kingAttacks_ & attacked_any_but_oking) & finfo_[color].attack_mask_;
  auto remaining_oking = oki_fields & ~finfo_[ocolor].kingAttacks_ & finfo_[color].attack_mask_;
  remaining_oking |= protected_oking;
  if (remaining_oking) {
    auto remaining_coeff = EvalCoefficients::attackedNearKingCoeff_ * pop_count(remaining_oking);
    attack_coeff += remaining_coeff >> 3;
  }

  num_checkers = std::min(num_checkers, 4);
  auto check_coeff = EvalCoefficients::kingCheckersCoefficients[num_checkers] + attack_coeff + (attack_coeff >> 2);

  auto king_moves = finfo_[ocolor].kingAttacks_ & ~(finfo_[color].attack_mask_ | mask_all_);
  int num_king_moves = pop_count(king_moves);
  check_coeff += EvalCoefficients::kingPossibleMovesCoefficients[num_king_moves];
  
  if (num_attackers == 0)
    check_coeff >>= 3;
  
  auto score = finfo_[color].score_king_ * attack_coeff + check_score * check_coeff;
  score >>= 5;
  return { score, 0 };
}
#endif
} // NEngine
