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

void Evaluator::prepareAttacksMasks()
{
  auto const& fmgr = board_->fmgr();
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto ocolor = Figure::otherColor(color);
    finfo_[color].knightMoves_ = BitMask{};
    finfo_[color].knightSafeMoves_ = BitMask{};
    finfo_[color].attackedByKnightRq_ = BitMask{};
    finfo_[color].behindPawnAttacks_ = BitMask{};

    BitMask nmask = board_->fmgr().knight_mask(color);
    for (; nmask;)
    {
      const int n = clear_lsb(nmask);
      auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & knight_moves;
      finfo_[color].attack_mask_ |= knight_moves;
      attacks_masks_[n] = knight_moves;

      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        knight_moves = 0ULL;
      }
      
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

      auto const& bishop_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_);
      auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
      auto bishop_moves_x = magic_ns::bishop_moves(n, mask_all_ & ~fmgr.queen_mask(ocolor));

      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        auto const& from_mask = betweenMasks().from(board_->kingPos(color), n);
        bishop_moves &= from_mask;
        bishop_moves_x &= from_mask;
      }

      moves_masks_[n] = bishop_moves;
      attacks_masks_[n] = bishop_attacks;
      finfo_[color].nbr_attacked_ |= bishop_moves_x;

      // mobility
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & bishop_attacks;
      finfo_[color].attack_mask_ |= bishop_attacks;
      finfo_[color].bishopMoves_ |= bishop_moves;
    }
    finfo_[color].nb_attacked_ |= finfo_[color].bishopMoves_;

    finfo_[color].rookMoves_ = BitMask{};
    auto rmask = fmgr.rook_mask(color);
    for (; rmask;)
    {
      auto n = clear_lsb(rmask);
      finfo_[color].attackedByKnightRq_ |= movesTable().caps(Figure::TypeKnight, n);

      // mobility
      auto const& rook_attacks = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
      auto rook_moves = magic_ns::rook_moves(n, mask_all_);
      auto rook_moves_x = magic_ns::rook_moves(n, mask_all_ & ~fmgr.queen_mask(ocolor));
      auto rook_moves_p = magic_ns::rook_moves(n, mask_all_ & ~fmgr.pawn_mask(color)) & pawnMasks().mask_forward(color, n);
    
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        auto const& from_mask = betweenMasks().from(board_->kingPos(color), n);
        rook_moves &= from_mask;
        rook_moves_x &= from_mask;
        rook_moves_p &= from_mask;
      }

      moves_masks_[n] = rook_moves;
      attacks_masks_[n] = rook_attacks;
      finfo_[color].nbr_attacked_ |= rook_moves_x;
    
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & rook_attacks;
      finfo_[color].attack_mask_ |= rook_attacks;
      finfo_[color].rookMoves_ |= rook_moves;
      finfo_[color].behindPawnAttacks_ |= rook_moves_p;
    }
    
    finfo_[color].queenMoves_ = BitMask{};
    auto qmask = fmgr.queen_mask(color);
    for (; qmask;)
    {
      auto n = clear_lsb(qmask);
      finfo_[color].attackedByKnightRq_ |= movesTable().caps(Figure::TypeKnight, n);

      // mobility
      auto qr_attacks = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
      auto queen_moves_p = magic_ns::rook_moves(n, mask_all_ & ~fmgr.pawn_mask(color)) & pawnMasks().mask_forward(color, n);
      auto const queen_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_) | qr_attacks;
      auto queen_moves = magic_ns::queen_moves(n, mask_all_);

      // pinned
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) != discoveredCheck(n, color), "discovered check not detected");
      if (discoveredCheck(n, color)) {
        auto const& from_mask = betweenMasks().from(board_->kingPos(color), n);
        queen_moves &= from_mask;
        queen_moves_p &= from_mask;
      }

      moves_masks_[n] = queen_moves;
      attacks_masks_[n] = queen_attacks;
      qr_attacks_masks_[n] = qr_attacks;

      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & queen_attacks;
      finfo_[color].attack_mask_ |= queen_attacks;
      finfo_[color].queenMoves_ |= queen_moves;
      finfo_[color].behindPawnAttacks_ |= queen_moves_p;
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
      auto const& knight_moves = moves_masks_[n];
      auto const& knight_attacks = attacks_masks_[n];

      // outpost
      const auto nbit = set_mask_bit(n);
      const bool boutpost = ((nbit | (knight_moves & ~fmgr.mask(color))) & outpost_mask) != 0ULL;
      score[color] += EvalCoefficients::knightOutpost_ * boutpost;

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];
      
      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(n, ocolor), "discovered check not detected");
      if (discoveredCheck(n, ocolor)) {
        finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
        finfo_[color].discoveredCheck_ = true;
      }
      
#ifdef DO_KING_EVAL
      if (knight_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;

#ifdef KING_EVAL_ATTACKED_MULTI_FIELD
        auto n_attacks = pop_count(knight_attacks & finfo_[ocolor].ki_fields_no_pw_);
        finfo_[color].score_king_ += EvalCoefficients::knightKingAttack_ * n_attacks + EvalCoefficients::basicAttack_ * (!n_attacks);
#else
        bool b_attacks = (knight_attacks & finfo_[ocolor].ki_fields_no_pw_) != 0ULL;
        finfo_[color].score_king_ += EvalCoefficients::knightKingAttack_ * b_attacks + EvalCoefficients::basicAttack_ * (!b_attacks);
#endif
      }
#endif
      
      auto n_moves_mask = knight_moves & (finfo_[color].cango_mask_ | finfo_[ocolor].nbrq_mask_);
      auto n_moves = pop_count(n_moves_mask);

      finfo_[color].score_mob_ += EvalCoefficients::knightMobility_[n_moves & 15];

      if ((n_moves < 2 || !(n_moves_mask & ~finfo_[ocolor].attack_mask_)) && blockedKnight(color, n)) {
        finfo_[color].score_mob_ -= EvalCoefficients::knightBlocked_;
      }

      auto kn_safe_moves = n_moves_mask;
#ifdef MOBILITY_EXTENDED
      bool qpinned = false;
      if (kn_safe_moves)
      {
        if (isPinned(n, color, ocolor, finfo_[color].rq_mask_, fmgr.bishop_mask(ocolor), nst::none) ||
            isPinned(n, color, ocolor, finfo_[color].rq_mask_ & ~finfo_[color].attack_mask_,
                                       fmgr.rook_mask(ocolor) & finfo_[ocolor].attack_mask_, nst::none)) {
          qpinned = true;
          kn_safe_moves = 0ULL;
          finfo_[color].score_mob_ -= EvalCoefficients::knightPinned_;
        }
      }
      if ((qpinned || !(n_moves_mask & ~fmgr.mask(color))) && (finfo_[ocolor].pawnAttacks_ & set_mask_bit(n))) {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_[0];
      }
#endif // king protection, discovered checks etc...

      finfo_[color].knightSafeMoves_ |= kn_safe_moves;
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
    for (; mask;)
    {
      int n = clear_lsb(mask);

      auto const& bishop_attacks = attacks_masks_[n];
      auto const& bishop_moves = moves_masks_[n];

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];

      X_ASSERT_R(board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(n, ocolor), "discovered check not detected");
      if (!finfo_[color].discoveredCheck_ && discoveredCheck(n, ocolor)) {
        finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
        finfo_[color].discoveredCheck_ = true;
      }

#ifdef MOBILITY_EXTENDED
      bool q_pinned = false;
      if(bishop_moves)
      {
        if (isPinned(n, color, ocolor, fmgr.queen_mask(color), fmgr.rook_mask(ocolor), nst::rook) ||
            isPinned(n, color, ocolor, fmgr.queen_mask(color) & ~finfo_[color].attack_mask_,
                                       fmgr.queen_mask(ocolor) & finfo_[ocolor].attack_mask_, nst::rook)) {
          q_pinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::bishopPinned_;
        }
      }
#endif // king protection, discovered checks etc...

#ifdef DO_KING_EVAL
      if (bishop_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;

#ifdef KING_EVAL_ATTACKED_MULTI_FIELD
        auto n_attacks = pop_count(bishop_attacks & finfo_[ocolor].ki_fields_no_pw_);
        finfo_[color].score_king_ += EvalCoefficients::bishopKingAttack_ * n_attacks + EvalCoefficients::basicAttack_ * (!n_attacks);
#else
        bool b_attacks = (bishop_attacks & (finfo_[ocolor].ki_fields_no_pw_)) != 0ULL;
        finfo_[color].score_king_ += EvalCoefficients::bishopKingAttack_ * b_attacks + EvalCoefficients::basicAttack_ * (!b_attacks);
#endif
      }
#endif

      // treat attacks
      if (bishop_moves && !q_pinned) {
        auto mask_all_no_orq = mask_all_ & ~(board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor));
        auto bishop_treat = magic_ns::bishop_moves(n, mask_all_no_orq);
        finfo_[color].bishopTreatAttacks_ |= bishop_treat;
      }

      // mobility
      auto b_moves_mask = bishop_moves & (finfo_[color].cango_mask_ | finfo_[ocolor].nbrq_mask_);
      int n_moves = pop_count(b_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::bishopMobility_[n_moves & 15];

      if ((n_moves < 2 || !(b_moves_mask & ~finfo_[ocolor].attack_mask_)) && blockedBishop(color, n)) {
        finfo_[color].score_mob_ -= EvalCoefficients::bishopBlocked_;
      }

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
    const auto r_cango_mask = (finfo_[color].cango_mask_ & ~finfo_[ocolor].nb_attacked_) | finfo_[ocolor].rq_mask_;
    for (; mask;)
    {
      auto n = clear_lsb(mask);

      auto const& rook_attacks = attacks_masks_[n];
      auto const& rook_moves = moves_masks_[n];

      // open column
      auto const& mask_fwd = pawnMasks().mask_forward(color, n);
      if(!(mask_fwd & fmgr.pawn_mask(color)))
      {
        bool pw_ocolor_n = (mask_fwd & fmgr.pawn_mask(ocolor)) != 0ULL;
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
      if (rook_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;

#ifdef KING_EVAL_ATTACKED_MULTI_FIELD
        auto n_attacks = pop_count(rook_attacks & finfo_[ocolor].ki_fields_no_pw_);
        finfo_[color].score_king_ += EvalCoefficients::rookKingAttack_ * n_attacks + EvalCoefficients::basicAttack_ * (!n_attacks);
#else
        bool b_attacks = (rook_attacks & finfo_[ocolor].ki_fields_no_pw_) != 0ULL;
        finfo_[color].score_king_ += EvalCoefficients::rookKingAttack_ * b_attacks + EvalCoefficients::basicAttack_ * (!b_attacks);
#endif
      }
#endif

      if (rook_moves) {
        auto mask_all_no_oq = mask_all_ & ~board_->fmgr().queen_mask(ocolor);
        auto rook_treat = magic_ns::rook_moves(n, mask_all_no_oq);
        finfo_[color].rookTreatAttacks_ |= rook_treat;
      }
      
      auto r_moves_mask = rook_moves & r_cango_mask;
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
    const auto q_cango_mask = (finfo_[color].cango_mask_ & ~finfo_[ocolor].nbr_attacked_) | fmgr.queen_mask(ocolor);
    for (; mask;)
    {
      auto n = clear_lsb(mask);
      auto const& queen_moves = moves_masks_[n];

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][ki_dist];

#ifdef DO_KING_EVAL
      auto qx_attacks = queen_moves | qr_attacks_masks_[n];
      bool qkattack = qx_attacks & finfo_[ocolor].ki_fields_;
      finfo_[color].qkingAttack_ |= qkattack;
      if (qkattack)
      {
        finfo_[color].num_attackers_++;

#ifdef KING_EVAL_ATTACKED_MULTI_FIELD
        auto n_attacks = pop_count(queen_moves & finfo_[ocolor].ki_fields_no_pw_);
        finfo_[color].score_king_ += EvalCoefficients::queenKingAttack_ * n_attacks + EvalCoefficients::basicAttack_ * (!n_attacks);
#else
        bool b_attacks = (queen_moves & finfo_[ocolor].ki_fields_no_pw_) != 0ULL;
        finfo_[color].score_king_ += EvalCoefficients::queenKingAttack_ * b_attacks + EvalCoefficients::basicAttack_ * (!b_attacks);
#endif
      }
#endif

      auto q_moves_mask = queen_moves & q_cango_mask;
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

#ifdef DO_KING_EVAL
ScoreType32 Evaluator::evaluateKingPressure(Figure::Color color)
{
  const auto& fmgr = board_->fmgr();
  const auto ocolor = Figure::otherColor(color);
  const auto  ki_pos = board_->kingPos(color);
  const auto oki_pos = board_->kingPos(ocolor);
  const auto fmask_no_check = ~fmgr.mask(color);

  const auto& oki_fields = finfo_[ocolor].ki_fields_;
  const auto& near_oking = oki_fields & finfo_[ocolor].kingAttacks_;
  const auto near_oking_pw = near_oking | (oki_fields & fmgr.pawn_mask(ocolor));
  const auto attacked_any_but_oking = finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[ocolor].kingAttacks_);
  const auto attacked_oking_only = finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].multiattack_mask_;
  
  const auto onbrp_attacked = finfo_[ocolor].nbr_attacked_ | finfo_[ocolor].pawnAttacks_;
  const auto onbp_attacked  = finfo_[ocolor].nb_attacked_ | finfo_[ocolor].pawnAttacks_;
  const auto can_check_q = ~(attacked_any_but_oking | (attacked_oking_only & ~finfo_[color].multiattack_mask_));
  const auto can_check_r = ~(onbrp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_));
  const auto can_check_nb = ~(onbp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_));

  finfo_[color].num_attackers_ += (finfo_[color].pawnAttacks_ & near_oking) != 0ULL;
  finfo_[color].num_attackers_ += (finfo_[color].kingAttacks_ & near_oking) != 0ULL;

#ifdef KING_EVAL_ATTACKED_MULTI_FIELD
  finfo_[color].score_king_ += EvalCoefficients::pawnKingAttack_ * pop_count(finfo_[color].pawnAttacks_ & near_oking_pw);
#else
  finfo_[color].score_king_ += EvalCoefficients::pawnKingAttack_ * ((finfo_[color].pawnAttacks_ & near_oking_pw) != 0ULL);
#endif

  auto kn_check = movesTable().caps(Figure::TypeKnight, oki_pos) & finfo_[color].knightMoves_ & fmask_no_check;
  auto bi_check = magic_ns::bishop_moves(oki_pos, mask_all_);
  auto r_check = magic_ns::rook_moves(oki_pos, mask_all_);
  auto q_check = (bi_check | r_check) & finfo_[color].queenMoves_ & fmask_no_check;
  bi_check &= finfo_[color].bishopMoves_ & fmask_no_check;
  r_check &= finfo_[color].rookMoves_ & fmask_no_check;

  if(!finfo_[color].discoveredCheck_) {
    X_ASSERT_R(board_->discoveredCheck(ki_pos, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(ki_pos, ocolor), "discovered check not detected");
    X_ASSERT(board_->discoveredCheck(ki_pos, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(ki_pos, ocolor), "discovered check not detected");
    finfo_[color].discoveredCheck_ = discoveredCheck(ki_pos, ocolor);
  }

  if (!finfo_[color].discoveredCheck_) {
    auto mask_all_npw = mask_all_ & ~fmgr.pawn_mask(color);
    auto bi_check_npw = magic_ns::bishop_moves(oki_pos, mask_all_npw) & (fmgr.bishop_mask(color) | fmgr.queen_mask(color));
    auto r_check_npw = magic_ns::rook_moves(oki_pos, mask_all_npw) & (fmgr.rook_mask(color) | fmgr.queen_mask(color)) & pawnMasks().mask_row(oki_pos);
    auto check_npw = bi_check_npw | r_check_npw;
    while (check_npw && !finfo_[color].discoveredCheck_) {
      auto n = clear_lsb(check_npw);
      auto const& btw_mask = betweenMasks().between(n, oki_pos);
      auto mpawn = btw_mask & fmgr.pawn_mask(color);
      if (!mpawn || !one_bit_set(mpawn))
        continue;
      int from = _lsb64(mpawn);
      if (color) {
        mpawn <<= 8;
      }
      else {
        mpawn >>= 8;
      }
      finfo_[color].discoveredCheck_ = ((mpawn & mask_all_) == 0ULL);
    }
  }

  bool canCheck = ((kn_check | bi_check | r_check) != 0ULL);
  kn_check &= can_check_nb;
  bi_check &= can_check_nb;
  r_check &= can_check_r;
  q_check &= can_check_q;
  int num_checkers = (kn_check != 0ULL) + (bi_check != 0ULL) + (r_check != 0ULL) + (q_check != 0ULL) + finfo_[color].discoveredCheck_;
  
  auto check_score = EvalCoefficients::knightChecking_ * (kn_check != 0) +
                     EvalCoefficients::bishopChecking_ * (bi_check != 0) +
                     EvalCoefficients::rookChecking_ * (r_check != 0) +
                     EvalCoefficients::queenChecking_ * (q_check != 0) +
                     EvalCoefficients::discoveredChecking_ * finfo_[color].discoveredCheck_;
  
  int num_attackers = std::min(finfo_[color].num_attackers_, 7);
  auto attack_coeff = EvalCoefficients::kingAttackersCoefficients_[num_attackers];

  auto check_coeff = 0;
  if (!num_checkers && canCheck) {
    check_score = EvalCoefficients::weakChecking_;
    check_coeff = EvalCoefficients::kingWeakCheckersCoefficients_;
    check_coeff += attack_coeff >> 2;
  }
  else {
    num_checkers = std::min(num_checkers, 4);
    check_coeff = EvalCoefficients::kingCheckersCoefficients_[num_checkers];
    check_coeff += attack_coeff >> 1;
  }

  const auto near_oking_att = (finfo_[ocolor].kingAttacks_ & ~attacked_any_but_oking) & finfo_[color].attack_mask_ & ~fmgr.pawn_mask(color);
  if (near_oking_att) {
    auto near_king_coeff = EvalCoefficients::attackedNearKingCoeff_ * pop_count(near_oking_att & finfo_[color].multiattack_mask_);
    near_king_coeff += (EvalCoefficients::attackedNearKingCoeff_ * pop_count(near_oking_att & ~finfo_[color].multiattack_mask_)) >> 1;
    attack_coeff += near_king_coeff;
    check_coeff += near_king_coeff >> 1;
  }

  const auto near_oking_rem = finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].pawnAttacks_ & finfo_[color].attack_mask_ & ~near_oking_att & ~fmgr.pawn_mask(color);
  if (near_oking_rem) {
    auto rem_king_coeff = (EvalCoefficients::attackedNearKingCoeff_ * pop_count(near_oking_rem)) >> 2;
    attack_coeff += rem_king_coeff;
    check_coeff += rem_king_coeff >> 1;
  }

  auto around_oking = oki_fields & ~finfo_[ocolor].kingAttacks_ & finfo_[color].attack_mask_ & ~finfo_[ocolor].attack_mask_;
  if (around_oking) {
    auto remaining_coeff = (EvalCoefficients::attackedNearKingCoeff_ * pop_count(around_oking)) >> 2;
    attack_coeff += remaining_coeff;
    const int queen_is_close = (around_oking & ~fmgr.mask(color) & finfo_[color].queenMoves_) != 0ULL;
    const int is_near_king = (near_oking_att & ~finfo_[color].queenMoves_ & ~fmgr.mask(color)) != 0ULL;
    attack_coeff += (EvalCoefficients::attackedNearKingCoeff_ * (queen_is_close + is_near_king)) >> 1;
    check_coeff += remaining_coeff >> 1;
  }
 
  if (num_attackers == 0) {
    check_coeff >>= 3;
  }
  else if ((num_attackers == 1) && (!q_check || (q_check && finfo_[color].qkingAttack_))) {
    check_coeff >>= 1;
  }
  // mat is possible
  if (num_checkers) {
    int mat_treat_coef = 0;
    auto oking_possible_moves = finfo_[ocolor].kingAttacks_ &
      ~(finfo_[color].multiattack_mask_ | mask_all_ | (finfo_[color].attack_mask_ & ~finfo_[color].queenMoves_));
    auto mat_fields_mask = (mask_all_ | (finfo_[ocolor].multiattack_mask_ & ~(finfo_[color].attack_mask_ & ~finfo_[color].queenMoves_))) & ~fmgr.king_mask(ocolor);
    q_check &= ~attacked_any_but_oking;
    while (q_check) {
      auto n = clear_lsb(q_check);
      const auto& qmat_attacks = magic_ns::queen_moves(n, mat_fields_mask);
      const auto attacked_ok_field = qmat_attacks & fmgr.king_mask(ocolor);
      if (attacked_ok_field && !(oking_possible_moves & ~qmat_attacks)) {
        mat_treat_coef = 1;
        break;
      }
    }
    if (mat_treat_coef) {
      oking_possible_moves = finfo_[ocolor].kingAttacks_ &
        ~(finfo_[color].multiattack_mask_ | mask_all_ | (finfo_[color].attack_mask_ & ~finfo_[color].rookMoves_));
      mat_fields_mask = (mask_all_ | (finfo_[ocolor].multiattack_mask_ & ~finfo_[color].attack_mask_)) & ~fmgr.king_mask(ocolor);
      r_check &= ~attacked_any_but_oking;
      while (r_check) {
        auto n = clear_lsb(r_check);
        const auto& rmat_attacks = magic_ns::rook_moves(n, mat_fields_mask);
        if ((rmat_attacks & fmgr.king_mask(ocolor)) && !(oking_possible_moves & ~rmat_attacks)) {
          mat_treat_coef = 1;
          break;
        }
      }
    }
    const bool my_move = (board_->color() == color);
    check_coeff += (EvalCoefficients::possibleMatTreat_ * mat_treat_coef) >> 1;
    check_coeff +=  EvalCoefficients::possibleMatTreat_ * my_move * mat_treat_coef;
    check_coeff += EvalCoefficients::checkMyMoveBonus_ * my_move * (!mat_treat_coef);
  }

  auto score = finfo_[color].score_king_ * attack_coeff + check_score * check_coeff;
  score >>= 5;

  int oki_x = (oki_pos & 7);
  int king_side = (oki_x > 4) ? 0 : ((oki_x < 3) ? 1 : 2); // 0 = right, 1 = left, 2 = center
  auto general_pressure_mask =
    ((finfo_[color].attack_mask_ & ~finfo_[ocolor].attack_mask_) | (finfo_[color].multiattack_mask_ & ~finfo_[ocolor].multiattack_mask_) |
      finfo_[color].pawnAttacks_) & ~finfo_[ocolor].pawnAttacks_ & ~(near_oking_att | around_oking | finfo_[ocolor].kingAttacks_);

  auto attacks_king_side = general_pressure_mask & Figure::quaterBoard_[ocolor][king_side];
  int general_score = pop_count(attacks_king_side) * EvalCoefficients::generalKingPressure_;
  score += general_score;

  return { score, 0 };
}
#endif
} // NEngine
