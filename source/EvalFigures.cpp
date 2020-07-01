#include <Evaluator.h>

namespace NEngine
{

bool Evaluator::isPinned(int pos, Figure::Color ocolor, BitMask targets, BitMask attackers, nst::bishop_rook_dirs dir) const
{
  if (!targets)
    return false;
  BitMask tmask{};
  if (dir != nst::rook)
    tmask |= magic_ns::bishop_moves(pos, mask_all_);
  if (dir != nst::bishop)
    tmask |= magic_ns::rook_moves(pos, mask_all_);
  targets &= tmask;
  if ((attackers & tmask) == 0ULL)
    return false;
  auto color = Figure::otherColor(ocolor);
  while (targets)
  {
    int t = clear_lsb(targets);
    auto attacker = board_->isPinned(pos, mask_all_, ocolor, t, dir);
    if (!attacker)
      continue;
    int a = _lsb64(attacker);
    if (board_->getField(a).type() < board_->getField(t).type())
      return true;
    if ((finfo_[color].attack_mask_ & set_mask_bit(t)) == 0ULL)
      return true;
  }
  return false;
}

bool Evaluator::blockedKnight(Figure::Color color, int n) const
{
  if (color == Figure::ColorWhite)
  {
    switch (n)
    {
    case A8:
      if (board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn) ||
        board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case A7:
      if (board_->isFigure(A6, Figure::ColorBlack, Figure::TypePawn) &&
        board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case B8:
      if (board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case H8:
      if (board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn) ||
        board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case H7:
      if (board_->isFigure(H6, Figure::ColorBlack, Figure::TypePawn) &&
        board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case G8:
      if (board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;
    }
  }
  else
  {
    switch (n)
    {
    case A1:
      if (board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn) ||
        board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case A2:
      if (board_->isFigure(A3, Figure::ColorWhite, Figure::TypePawn) &&
        board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case B1:
      if (board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case H1:
      if (board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn) ||
        board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case H2:
      if (board_->isFigure(H3, Figure::ColorWhite, Figure::TypePawn) &&
        board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case G1:
      if (board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;
    }
  }
  return false;
}

bool Evaluator::blockedBishop(Figure::Color color, int n) const
{
  if (color == Figure::ColorWhite)
  {
    switch (n)
    {
    case A7:
      if (board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case A6:
      if (board_->isFigure(B5, Figure::ColorBlack, Figure::TypePawn) &&
        board_->isFigure(C6, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case A8:
      if (board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case B8:
      if (board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn) &&
        (board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn) ||
          board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn))) {
        return true;
      }
      break;

    case H7:
      if (board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case H8:
      if (board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;

    case G8:
      if (board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn) &&
        (board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn) ||
          board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn))) {
        return true;
      }
      break;

    case H6:
      if (board_->isFigure(G5, Figure::ColorBlack, Figure::TypePawn) &&
        board_->isFigure(F6, Figure::ColorBlack, Figure::TypePawn)) {
        return true;
      }
      break;
    }
  }
  else
  {
    switch (n)
    {
    case A2:
      if (board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case A3:
      if (board_->isFigure(B4, Figure::ColorWhite, Figure::TypePawn) &&
        board_->isFigure(C3, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case A1:
      if (board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case B1:
      if (board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn) &&
        (board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn) ||
          board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn))) {
        return true;
      }
      break;

    case H2:
      if (board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case H1:
      if (board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;

    case G1:
      if (board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn) &&
        (board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn) ||
          board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn))) {
        return true;
      }
      break;

    case H3:
      if (board_->isFigure(G4, Figure::ColorWhite, Figure::TypePawn) &&
        board_->isFigure(F3, Figure::ColorWhite, Figure::TypePawn)) {
        return true;
      }
      break;
    }
  }
  return false;
}

Evaluator::FullScore Evaluator::evaluateKnights()
{
  FullScore score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto ocolor = Figure::otherColor(color);
    BitMask mask = board_->fmgr().knight_mask(color);
    for (; mask;)
    {
      const int n = clear_lsb(mask);
      auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & knight_moves;
      finfo_[color].attack_mask_ |= knight_moves;
      finfo_[color].knightAttacks_ |= knight_moves;

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];
    }
    finfo_[color].nb_attacked_ |= finfo_[color].knightAttacks_;
    finfo_[color].nbr_attacked_ |= finfo_[color].knightAttacks_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

Evaluator::FullScore Evaluator::evaluateBishops()
{
  FullScore score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto ocolor = Figure::otherColor(color);
    BitMask mask = board_->fmgr().bishop_mask(color);
    for (; mask;)
    {
      int n = clear_lsb(mask);

      // mobility
      auto bishop_moves = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_);
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & bishop_moves;
      finfo_[color].attack_mask_ |= bishop_moves;
      finfo_[color].bishopAttacks_ |= bishop_moves;
      finfo_[color].bishopDirectAttacks_ |= magic_ns::bishop_moves(n, mask_all_);

      auto mask_all_no_orq = mask_all_ & ~(board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor));
      auto bishop_treat = magic_ns::bishop_moves(n, mask_all_no_orq);
      finfo_[color].bishopTreatAttacks_ |= bishop_treat;

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];
    }
    finfo_[color].nb_attacked_ |= finfo_[color].bishopAttacks_;
    finfo_[color].nbr_attacked_ |= finfo_[color].bishopAttacks_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

Evaluator::FullScore Evaluator::evaluateRook(Figure::Color color)
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  auto mask = fmgr.rook_mask(color);
  auto ocolor = Figure::otherColor(color);
  for(; mask;)
  {
    auto n = clear_lsb(mask);
    // mobility
    auto const& rook_moves = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
    finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & rook_moves;
    finfo_[color].attack_mask_ |= rook_moves;
    finfo_[color].rookAttacks_ |= rook_moves;
    finfo_[color].rookDirectAttacks_ |= magic_ns::rook_moves(n, mask_all_);

    // open column
    auto const& mask_col = pawnMasks().mask_column(n & 7);
    bool no_pw_color = (mask_col & fmgr.pawn_mask(color)) == 0ULL;
    bool no_pw_ocolor = (mask_col & fmgr.pawn_mask(ocolor)) == 0ULL;
    score.opening_ += EvalCoefficients::openRook_[(no_pw_color + no_pw_ocolor) & 3];
    score.endGame_ += EvalCoefficients::openRook_[(no_pw_color + no_pw_ocolor) & 3] >> 1;
    
    // king protection
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
    score.opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeRook][ki_dist];
  }
  finfo_[color].nbr_attacked_ |= finfo_[color].rookAttacks_;
  return score;
}

Evaluator::FullScore Evaluator::evaluateQueens(Figure::Color color)
{
  FullScore score;
  auto const& fmgr = board_->fmgr();
  auto mask = fmgr.queen_mask(color);
  auto ocolor = Figure::otherColor(color);
  for(; mask;)
  {
    auto n = clear_lsb(mask);
    // mobility
    auto queen_moves = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_) | magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
    finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & queen_moves;
    finfo_[color].attack_mask_ |= queen_moves;
    finfo_[color].queenAttacks_ |= queen_moves;
    finfo_[color].queenDirectAttacks_ |= magic_ns::queen_moves(n, mask_all_);

    // king protection
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
    score.opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][ki_dist];
  }
  return score;
}

Evaluator::FullScore Evaluator::evaluateMobilityAndKingPressure(Figure::Color color)
{
  auto const& fmgr = board_->fmgr();
  auto ocolor = Figure::otherColor(color);
  const BitMask attacked_any_but_oking = finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[ocolor].kingAttacks_);
  const BitMask attacked_oking_only = finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].multiattack_mask_;

  const BitMask& o_nb_attacked = finfo_[ocolor].nb_attacked_;
  const BitMask& o_nbr_attacked = finfo_[ocolor].nbr_attacked_;
  const BitMask& cango_mask = finfo_[color].cango_mask_;

  const BitMask not_occupied = ~(fmgr.mask(color) | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_) | finfo_[ocolor].multiattack_mask_);
  const BitMask allowed_moves_q = (cango_mask & not_occupied & ~o_nbr_attacked) | fmgr.queen_mask(ocolor);
  const BitMask allowed_moves_r = (cango_mask & not_occupied & ~o_nb_attacked) | fmgr.queen_mask(ocolor) | fmgr.rook_mask(ocolor);
  const BitMask allowed_moves_nb = (cango_mask & not_occupied) | fmgr.bishop_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);

  const BitMask can_check_q = ~(fmgr.mask(color) | attacked_any_but_oking | (attacked_oking_only & ~finfo_[color].multiattack_mask_));
  const BitMask can_check_r = can_check_q | fmgr.queen_mask(ocolor);
  const BitMask can_check_nb = can_check_r | fmgr.rook_mask(ocolor);

  //const BitMask nb_mask = fmgr.knight_mask(color) | fmgr.bishop_mask(color);
  BitMask q_targets = fmgr.queen_mask(color);
  //BitMask nbr_targets = fmgr.rook_mask(color) | (nb_mask & ~finfo_[color].attack_mask_);
  auto const & obrq_mask = finfo_[ocolor].brq_mask_;
  auto const & obq_mask = finfo_[ocolor].bq_mask_;
  auto const & orq_mask = finfo_[ocolor].rq_mask_;

  auto const & oki_fields = finfo_[ocolor].ki_fields_;
  auto const & oki_fields_protected = finfo_[ocolor].ki_fields_prot_;

#ifdef PROCESS_DANGEROUS_EVAL
  if (needDangerousDetect_ && board_->color() == color) {
    auto pw_under_attack = board_->fmgr().pawn_mask(color) & finfo_[ocolor].attack_mask_ & ~finfo_[color].attack_mask_;
    finfo_[color].pawnsUnderAttack_ = pop_count(pw_under_attack);
  }
#endif // PROCESS_DANGEROUS_EVAL

  finfo_[color].num_attackers_ += (finfo_[color].pawnAttacks_ & oki_fields) != 0ULL;
  finfo_[color].score_king_ += pop_count(finfo_[color].pawnAttacks_ & oki_fields) * EvalCoefficients::pawnKingAttack_;
  finfo_[color].num_attackers_ += (finfo_[color].kingAttacks_ & oki_fields) != 0ULL;

  BitMask knights = fmgr.knight_mask(color);
  for(; knights;)
  {
    int n = clear_lsb(knights);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);

    auto n_attacks = knight_moves & oki_fields;
    auto n_attacks_p = knight_moves & oki_fields_protected;
    finfo_[color].num_attackers_ += ((n_attacks || n_attacks_p) != 0ULL);
    finfo_[color].score_king_ += pop_count(n_attacks) * EvalCoefficients::knightKingAttack_;
    finfo_[color].score_king_ += (pop_count(n_attacks_p) * EvalCoefficients::knightKingAttack_) >> 2;

    if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) ||
      isPinned(n, ocolor, board_->fmgr().queen_mask(color), obrq_mask, nst::none))
    {
      knight_moves = 0ULL;
      if ((finfo_[ocolor].pawnAttacks_ & set_mask_bit(n)) != 0ULL) {
#ifdef PROCESS_DANGEROUS_EVAL
        finfo_[color].forkTreat_ = true;
#endif
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
    }
    auto n_moves = pop_count(knight_moves & finfo_[color].cango_mask_);
    finfo_[color].score_mob_ += EvalCoefficients::knightMobility_[n_moves & 15];
    if (n_moves < 2 && blockedKnight(color, n)) {
      finfo_[color].score_mob_ -= EvalCoefficients::knightBlocked_;
    }

#ifdef PROCESS_DANGEROUS_EVAL
    if (needDangerousDetect_ && board_->color() == color) {
      if ((knight_moves & allowed_moves_nb) == 0ULL)
      {
        auto kn_mask = set_mask_bit(n);
        bool notProtected = (kn_mask & finfo_[color].attack_mask_) == 0ULL;
        bool attacked = (kn_mask & finfo_[ocolor].attack_mask_) != 0ULL;
        bool pawnAttack = (kn_mask & finfo_[ocolor].pawnAttacks_) != 0ULL;
        if ((notProtected && attacked) || pawnAttack)
          finfo_[color].knightsUnderAttack_++;
      }
    }
#endif // PROCESS_DANGEROUS_EVAL

  }

  BitMask bishops = fmgr.bishop_mask(color);
  for(; bishops;)
  {
    int n = clear_lsb(bishops);

    auto const& bishop_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_);
    auto direct_attacks = bishop_attacks & finfo_[color].bishopDirectAttacks_;
    auto xray_attacks = bishop_attacks & ~finfo_[color].bishopDirectAttacks_;
    auto bi_attacks_d = direct_attacks & oki_fields;
    auto bi_attacks_x = xray_attacks & oki_fields;
    auto bi_attacks_p = (direct_attacks| xray_attacks) & oki_fields_protected;
    finfo_[color].num_attackers_ += (bi_attacks_d || bi_attacks_x || bi_attacks_p) !=  0ULL;
    finfo_[color].score_king_ += pop_count(bi_attacks_d) * EvalCoefficients::bishopKingAttack_;
    finfo_[color].score_king_ += (pop_count(bi_attacks_x) * EvalCoefficients::bishopKingAttack_) >> 2;
    finfo_[color].score_king_ += (pop_count(bi_attacks_p) * EvalCoefficients::bishopKingAttack_) >> 2;

    // mobility
    auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
    if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) ||
        isPinned(n, ocolor, q_targets, orq_mask, nst::rook))
    {
      bishop_moves = 0ULL;
      if ((finfo_[ocolor].pawnAttacks_ & set_mask_bit(n)) != 0ULL) {
#ifdef PROCESS_DANGEROUS_EVAL
        finfo_[color].forkTreat_ = true;
#endif
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
    }
    int n_moves = pop_count(bishop_moves & cango_mask);
    finfo_[color].score_mob_ += EvalCoefficients::bishopMobility_[n_moves & 15];
    if (n_moves < 2 && blockedBishop(color, n)) {
      finfo_[color].score_mob_ -= EvalCoefficients::bishopBlocked_;
    }

#ifdef PROCESS_DANGEROUS_EVAL
    if (needDangerousDetect_ && board_->color() == color) {
      if ((bishop_moves & allowed_moves_nb) == 0ULL)
      {
        auto bi_mask = set_mask_bit(n);
        bool notProtected = (bi_mask & finfo_[color].attack_mask_) == 0ULL;
        bool attacked = (bi_mask & finfo_[ocolor].attack_mask_) != 0ULL;
        bool pawnAttack = (bi_mask & finfo_[ocolor].pawnAttacks_) != 0ULL;
        if ((notProtected && attacked) || pawnAttack)
          finfo_[color].bishopsUnderAttack_++;
      }
    }
#endif // PROCESS_DANGEROUS_EVAL
  }

  BitMask rooks = fmgr.rook_mask(color);
  for(; rooks;)
  {
    auto n = clear_lsb(rooks);

    auto const& rook_attacks = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
    auto direct_attacks = rook_attacks & finfo_[color].rookDirectAttacks_;
    auto xray_attacks = rook_attacks & ~finfo_[color].rookDirectAttacks_;
    auto r_attacks_d = direct_attacks & oki_fields;
    auto r_attacks_x = xray_attacks & oki_fields;
    auto r_attacks_p = (direct_attacks| xray_attacks) & oki_fields_protected;
    finfo_[color].num_attackers_ += (r_attacks_d || r_attacks_x || r_attacks_p) != 0ULL;
    finfo_[color].score_king_ += pop_count(r_attacks_d) * EvalCoefficients::rookKingAttack_;
    finfo_[color].score_king_ += (pop_count(r_attacks_x) * EvalCoefficients::rookKingAttack_) >> 2;
    finfo_[color].score_king_ += (pop_count(r_attacks_p) * EvalCoefficients::rookKingAttack_) >> 2;

    // mobility
    auto rook_moves = magic_ns::rook_moves(n, mask_all_);
    if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)) ||
        isPinned(n, ocolor, q_targets, obq_mask, nst::bishop))
    {
      rook_moves = 0ULL;
      if ((finfo_[ocolor].pawnAttacks_ & set_mask_bit(n)) != 0ULL) {
#ifdef PROCESS_DANGEROUS_EVAL
        finfo_[color].forkTreat_ = true;
#endif
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
    }
    auto r_moves_mask = rook_moves & cango_mask;
    int n_moves = pop_count(r_moves_mask);
    finfo_[color].score_mob_ += EvalCoefficients::rookMobility_[n_moves & 15];

    // fake castle possible
    if (n_moves < 4) {
      if (fakeCastle(color, n, r_moves_mask)) {
        finfo_[color].score_opening_ += EvalCoefficients::fakeCastle_;
        finfo_[color].score_mob_ -= EvalCoefficients::rookBlocked_ >> 1;
      }
      else if (blockedRook(color, n, r_moves_mask))
        finfo_[color].score_mob_ -= EvalCoefficients::rookBlocked_;
    }

#ifdef PROCESS_DANGEROUS_EVAL
    if (needDangerousDetect_ && board_->color() == color) {
      if ((rook_moves  & allowed_moves_r) == 0ULL)
      {
        auto r_mask = set_mask_bit(n);
        bool notProtected = (r_mask & finfo_[color].attack_mask_) == 0ULL;
        bool attacked = (r_mask & finfo_[ocolor].attack_mask_) != 0ULL;
        auto rook_attackers = finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].knightAttacks_ | finfo_[ocolor].bishopAttacks_;
        bool lowerAttack = (r_mask & rook_attackers) != 0ULL;
        if ((notProtected && attacked) || lowerAttack)
          finfo_[color].rooksUnderAttack_++;
      }
    }
#endif // PROCESS_DANGEROUS_EVAL
  }

  BitMask queens = fmgr.queen_mask(color);
  for(; queens;)
  {
    auto n = clear_lsb(queens);

    auto const queen_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_) | magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
    auto direct_attacks = queen_attacks & finfo_[color].queenDirectAttacks_;
    auto xray_attacks = queen_attacks & ~finfo_[color].queenDirectAttacks_;
    auto q_attacks_d = direct_attacks & oki_fields;
    auto q_attacks_x = xray_attacks & oki_fields;
    auto q_attacks_p = (direct_attacks| xray_attacks) & oki_fields_protected;
    finfo_[color].num_attackers_ += (q_attacks_d || q_attacks_x || q_attacks_p) != 0ULL;
    finfo_[color].score_king_ += pop_count(q_attacks_d) * EvalCoefficients::queenKingAttack_;
    finfo_[color].score_king_ += (pop_count(q_attacks_x) * EvalCoefficients::queenKingAttack_) >> 2;
    finfo_[color].score_king_ += (pop_count(q_attacks_p) * EvalCoefficients::queenKingAttack_) >> 2;

    // mobility
    auto queen_moves = magic_ns::queen_moves(n, mask_all_);
    if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)))
    {
      auto from_mask = betweenMasks().from(board_->kingPos(color), n);
      queen_moves &= from_mask;
      if ((queen_moves == 0ULL) && ((finfo_[ocolor].pawnAttacks_ & set_mask_bit(n)) != 0ULL)) {
#ifdef PROCESS_DANGEROUS_EVAL
        finfo_[color].forkTreat_ = true;
#endif
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
    }
    auto n_moves = pop_count(queen_moves & cango_mask);
    finfo_[color].score_mob_ += EvalCoefficients::queenMobility_[n_moves & 31];

#ifdef PROCESS_DANGEROUS_EVAL
    if (needDangerousDetect_ && board_->color() == color) {
      if ((queen_moves & allowed_moves_q) == 0ULL)
      {
        auto q_mask = set_mask_bit(n);
        bool notProtected = (q_mask & finfo_[color].attack_mask_) == 0ULL;
        bool attacked = (q_mask & finfo_[ocolor].attack_mask_) != 0ULL;
        auto queen_attackers = finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].knightAttacks_ |
          finfo_[ocolor].bishopAttacks_ | finfo_[ocolor].rookAttacks_;
        bool lowerAttack = (q_mask & queen_attackers) != 0ULL;
        if ((notProtected && attacked) || lowerAttack)
          finfo_[color].queensUnderAttack_++;
      }
    }
#endif // PROCESS_DANGEROUS_EVAL
  }

  auto kn_check = movesTable().caps(Figure::TypeKnight, board_->kingPos(ocolor));
  auto bi_check = magic_ns::bishop_moves(board_->kingPos(ocolor), mask_all_);
  auto r_check = magic_ns::rook_moves(board_->kingPos(ocolor), mask_all_);
  auto q_check = bi_check | r_check;

  kn_check &= finfo_[color].knightAttacks_ & can_check_nb;
  bi_check &= finfo_[color].bishopDirectAttacks_ & can_check_nb;
  r_check &= finfo_[color].rookDirectAttacks_ & can_check_r;
  q_check &= finfo_[color].queenDirectAttacks_ & can_check_q;

#ifdef PROCESS_DANGEROUS_EVAL
  if (needDangerousDetect_ && board_->color() != color) {
    if (kn_check != 0ULL || bi_check != 0ULL || r_check != 0ULL || q_check != 0ULL)
    {
      auto ofgmask = board_->fmgr().pawn_mask(ocolor) | board_->fmgr().knight_mask(ocolor) |
        board_->fmgr().bishop_mask(ocolor) | board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
      BitMask checking_attacked = 0ULL;
      BitMask oking_moves = movesTable().caps(Figure::TypeKing, board_->kingPos(ocolor)) & ~ofgmask & ~finfo_[color].attack_mask_;
      if (kn_check != 0) {
        int n = _lsb64(kn_check);
        checking_attacked |= movesTable().caps(Figure::TypeKnight, n);
        if ((oking_moves & ~checking_attacked) == 0ULL)
          finfo_[ocolor].matThreat_ = true;
      }
      if (!finfo_[ocolor].matThreat_ && bi_check != 0) {
        int n = _lsb64(bi_check);
        checking_attacked |= magic_ns::bishop_moves(n, mask_all_);
        if ((oking_moves & ~checking_attacked) == 0ULL)
          finfo_[ocolor].matThreat_ = true;
      }
      if (!finfo_[ocolor].matThreat_ && r_check != 0) {
        int n = _lsb64(r_check);
        checking_attacked |= magic_ns::rook_moves(n, mask_all_);
        if ((oking_moves & ~checking_attacked) == 0ULL)
          finfo_[ocolor].matThreat_ = true;
      }
      if (!finfo_[ocolor].matThreat_ && q_check != 0) {
        int n = _lsb64(q_check);
        checking_attacked |= magic_ns::queen_moves(n, mask_all_);
        if ((oking_moves & ~checking_attacked) == 0ULL)
          finfo_[ocolor].matThreat_ = true;
      }
    }
  }
#endif // PROCESS_DANGEROUS_EVAL

  FullScore score;
  score.common_ = finfo_[color].score_mob_;
  score.opening_ = finfo_[color].score_opening_;

  int score_king = finfo_[color].score_king_;
  if (fmgr.rooks(color) > 1 || fmgr.queens(color) > 0 || (fmgr.bishops(color) + fmgr.knights(color) + fmgr.rooks(color) > 2))
  {
    int check_score = (kn_check != 0) * EvalCoefficients::knightChecking_ +
      (bi_check != 0) * EvalCoefficients::bishopChecking_ +
      (r_check != 0) * EvalCoefficients::rookChecking_ +
      (q_check != 0) * EvalCoefficients::queenChecking_;

    static const int checkers_coefficients[8] = { 0, 32, 64, 64, 64 };
    static const int attackers_coefficients[8] = { 0, 16, 32, 64, 96, 128, 128, 128 };

    int num_attackers = std::min(finfo_[color].num_attackers_, 7);
    auto attack_coeff = attackers_coefficients[num_attackers];

    int num_checkers = (kn_check != 0ULL) + (bi_check != 0ULL) + (r_check != 0ULL) + (q_check != 0ULL);
    num_checkers = std::min(num_checkers, 4);
    auto check_coeff = checkers_coefficients[num_checkers] + attack_coeff/2;

    score_king = ((score_king * attack_coeff + check_score * check_coeff) >> 5);

    auto king_left = (board_->kingPos(ocolor) & 7) < 4;
    auto general_pressure_mask = ((finfo_[color].attack_mask_ & ~attacked_any_but_oking) | (finfo_[color].multiattack_mask_ & ~finfo_[ocolor].multiattack_mask_)) & ~finfo_[ocolor].pawnAttacks_;
    //auto general_pressure_mask = finfo_[color].attack_mask_ & ~finfo_[ocolor].pawnAttacks_;
    auto attacks_king_side = general_pressure_mask & Figure::quaterBoard_[ocolor][king_left];
    int general_king_attacks_score = pop_count(attacks_king_side) * EvalCoefficients::generalKingPressure_;
    auto attacks_opponent_other = general_pressure_mask & Figure::quaterBoard_[ocolor][!king_left];
    int general_opponent_pressure = pop_count(attacks_opponent_other) * EvalCoefficients::generalOpponentPressure_;
    int general_score = general_king_attacks_score + general_opponent_pressure;

    score.opening_ += general_score;
  }
  score.opening_ += score_king;

  return score;
}

} // NEngine
