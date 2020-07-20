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
    if(board_->isPinned(pos, mask_all_, ocolor, t, dir) & attackers)
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
  auto const& fmgr = board_->fmgr();
  FullScore score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto ocolor = Figure::otherColor(color);
    BitMask mask = board_->fmgr().knight_mask(color);
    for (; mask;)
    {
      const int n = clear_lsb(mask);
      auto knight_moves = movesTable().caps(Figure::TypeKnight, n);

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];

      auto n_attacks = knight_moves & finfo_[ocolor].ki_fields_;
      auto n_attacks_p = knight_moves & finfo_[ocolor].ki_fields_prot_;
      finfo_[color].num_attackers_ += ((n_attacks | n_attacks_p) != 0ULL);
      finfo_[color].score_king_ +=  pop_count(n_attacks) * EvalCoefficients::knightKingAttack_;
      finfo_[color].score_king_ += (pop_count(n_attacks_p) * EvalCoefficients::knightKingAttack_) >> 2;

      bool qpinned = false;
      if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color))) {
        knight_moves = 0ULL;
      }
      else if (isPinned(n, color, ocolor, board_->fmgr().queen_mask(color), finfo_[ocolor].brq_mask_, nst::none)) {
        qpinned = true;
        finfo_[color].score_mob_ += EvalCoefficients::knightMobility_[0] >> 1;
      }
      
      if (knight_moves) {
        finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & knight_moves;
        finfo_[color].attack_mask_ |= knight_moves;
        finfo_[color].knightMoves_ |= knight_moves;
      }
      
      auto n_moves_mask = knight_moves & (finfo_[color].cango_mask_ | finfo_[ocolor].nbrq_mask_);
      auto n_moves = pop_count(n_moves_mask);

      finfo_[color].score_mob_ += EvalCoefficients::knightMobility_[n_moves & 15];
      if (n_moves < 2 && blockedKnight(color, n)) {
        finfo_[color].score_mob_ -= EvalCoefficients::knightBlocked_;
      }
      if ((qpinned || !(n_moves_mask & ~fmgr.mask(color))) && (finfo_[ocolor].pawnAttacks_ & set_mask_bit(n))) {
#ifdef PROCESS_DANGEROUS_EVAL
        finfo_[color].forkTreat_ = true;
#endif
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
    }
    finfo_[color].nb_attacked_ |= finfo_[color].knightMoves_;
    finfo_[color].nbr_attacked_ |= finfo_[color].knightMoves_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

Evaluator::FullScore Evaluator::evaluateBishops()
{
  auto const& fmgr = board_->fmgr();
  FullScore score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto ocolor = Figure::otherColor(color);
    BitMask mask = fmgr.bishop_mask(color);
    for (; mask;)
    {
      int n = clear_lsb(mask);

      auto const& bishop_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_);
      auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];

      auto xray_attacks = bishop_attacks & ~bishop_moves;
      auto bi_attacks_d = bishop_moves & finfo_[ocolor].ki_fields_;
      auto bi_attacks_x = xray_attacks & finfo_[ocolor].ki_fields_;
      auto bi_attacks_p = (bishop_moves | xray_attacks) & finfo_[ocolor].ki_fields_prot_;
      finfo_[color].num_attackers_ += (bi_attacks_d | bi_attacks_x | bi_attacks_p) != 0ULL;
      finfo_[color].score_king_ +=  pop_count(bi_attacks_d) * EvalCoefficients::bishopKingAttack_;
      finfo_[color].score_king_ += (pop_count(bi_attacks_x) * EvalCoefficients::bishopKingAttack_) >> 2;
      finfo_[color].score_king_ += (pop_count(bi_attacks_p) * EvalCoefficients::bishopKingAttack_) >> 2;
      
      bool q_pinned = false;
      if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color))) {
        bishop_moves = 0ULL;
      }
      else if(isPinned(n, color, ocolor, board_->fmgr().queen_mask(color), board_->fmgr().rook_mask(ocolor), nst::rook)) {
        q_pinned = true;
        finfo_[color].score_mob_ += EvalCoefficients::bishopMobility_[0] >> 1;
      }

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

      if ((q_pinned || !(b_moves_mask & ~fmgr.mask(color))) && (finfo_[ocolor].pawnAttacks_ & set_mask_bit(n)) != 0ULL) {
#ifdef PROCESS_DANGEROUS_EVAL
        finfo_[color].forkTreat_ = true;
#endif
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }

      if (n_moves < 2 && blockedBishop(color, n)) {
        finfo_[color].score_mob_ -= EvalCoefficients::bishopBlocked_;
      }
    }
    finfo_[color].nb_attacked_ |= finfo_[color].bishopMoves_;
    finfo_[color].nbr_attacked_ |= finfo_[color].bishopMoves_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

Evaluator::FullScore Evaluator::evaluateRook()
{
  auto const& fmgr = board_->fmgr();
  FullScore score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
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
        score[color].opening_ += EvalCoefficients::openRook_[0][pw_ocolor_n];
        score[color].endGame_ += EvalCoefficients::openRook_[1][pw_ocolor_n];
      }

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeRook][ki_dist];

      auto xray_attacks = rook_attacks & ~rook_moves;
      auto r_attacks_d = rook_moves & finfo_[ocolor].ki_fields_;
      auto r_attacks_x = xray_attacks & finfo_[ocolor].ki_fields_;
      auto r_attacks_p = (rook_moves | xray_attacks) & finfo_[ocolor].ki_fields_prot_;
      finfo_[color].num_attackers_ += (r_attacks_d | r_attacks_x | r_attacks_p) != 0ULL;
      finfo_[color].score_king_ +=  pop_count(r_attacks_d) * EvalCoefficients::rookKingAttack_;
      finfo_[color].score_king_ += (pop_count(r_attacks_x) * EvalCoefficients::rookKingAttack_) >> 2;
      finfo_[color].score_king_ += (pop_count(r_attacks_p) * EvalCoefficients::rookKingAttack_) >> 2;

      bool q_pinned = false;
      if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color))) {
        rook_moves = 0ULL;
      }
      else if (isPinned(n, color, ocolor, fmgr.queen_mask(color), fmgr.bishop_mask(ocolor), nst::bishop)) {
        q_pinned = true;
        finfo_[color].score_mob_ += EvalCoefficients::rookMobility_[0] >> 1;
      }

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

      if ((q_pinned || !(r_moves_mask & ~fmgr.mask(color) & ~finfo_[ocolor].bishopTreatAttacks_)) &&
          ((finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].nb_attacked_) & set_mask_bit(n)) != 0ULL) {
#ifdef PROCESS_DANGEROUS_EVAL
        finfo_[color].forkTreat_ = true;
#endif
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }

      // fake castle possible
      if (n_moves < 4) {
        if (fakeCastle(color, n, r_moves_mask)) {
          finfo_[color].score_opening_ += EvalCoefficients::fakeCastle_;
        }
        else if (blockedRook(color, n, r_moves_mask))
          finfo_[color].score_mob_ -= EvalCoefficients::rookBlocked_;
      }
    }
    finfo_[color].nbr_attacked_ |= finfo_[color].rookMoves_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

Evaluator::FullScore Evaluator::evaluateQueens()
{
  auto const& fmgr = board_->fmgr();
  FullScore score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto mask = fmgr.queen_mask(color);
    auto ocolor = Figure::otherColor(color);
    for (; mask;)
    {
      auto n = clear_lsb(mask);
      // mobility
      auto const queen_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_) | magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
      auto queen_moves = magic_ns::queen_moves(n, mask_all_);

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][ki_dist];

      auto xray_attacks = queen_attacks & ~queen_moves;
      auto q_attacks_d = queen_moves & finfo_[ocolor].ki_fields_;
      auto q_attacks_x = xray_attacks & finfo_[ocolor].ki_fields_;
      auto q_attacks_p = (queen_moves | xray_attacks) & finfo_[ocolor].ki_fields_prot_;
      finfo_[color].num_attackers_ += (q_attacks_d | q_attacks_x | q_attacks_p) != 0ULL;
      finfo_[color].score_king_ +=  pop_count(q_attacks_d) * EvalCoefficients::queenKingAttack_;
      finfo_[color].score_king_ += (pop_count(q_attacks_x) * EvalCoefficients::queenKingAttack_) >> 2;
      finfo_[color].score_king_ += (pop_count(q_attacks_p) * EvalCoefficients::queenKingAttack_) >> 2;

      // mobility
      if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color)))
      {
        auto from_mask = betweenMasks().from(board_->kingPos(color), n);
        queen_moves &= from_mask;
      }

      if (queen_moves) {
        finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & queen_attacks;
        finfo_[color].attack_mask_ |= queen_attacks;
        finfo_[color].queenMoves_ |= queen_moves;
      }

      auto q_moves_mask = queen_moves & ((finfo_[color].cango_mask_ & ~finfo_[ocolor].nbr_attacked_) | fmgr.queen_mask(ocolor));
      auto n_moves = pop_count(q_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::queenMobility_[n_moves & 31];

      if (!(q_moves_mask & ~fmgr.mask(color) & ~finfo_[ocolor].rookTreatAttacks_ & ~finfo_[ocolor].bishopTreatAttacks_) &&
        (((finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].nbr_attacked_) & set_mask_bit(n)) != 0ULL)) {
#ifdef PROCESS_DANGEROUS_EVAL
        finfo_[color].forkTreat_ = true;
#endif
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
    }
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

Evaluator::FullScore Evaluator::evaluateKingPressure(Figure::Color color)
{
  auto const& fmgr = board_->fmgr();
  auto ocolor = Figure::otherColor(color);
  const BitMask attacked_any_but_oking = finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[ocolor].kingAttacks_);
  const BitMask attacked_oking_only = finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].multiattack_mask_;

  auto onbrp_attacked = finfo_[ocolor].nbr_attacked_ | finfo_[ocolor].pawnAttacks_;
  auto onbp_attacked  = finfo_[ocolor].nb_attacked_ | finfo_[ocolor].pawnAttacks_;
  const BitMask can_check_q = ~(fmgr.mask(color) | attacked_any_but_oking | (attacked_oking_only & ~finfo_[color].multiattack_mask_));
  const BitMask can_check_r = ~(fmgr.mask(color) | onbrp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_)) |
    fmgr.queen_mask(ocolor);
  const BitMask can_check_nb = ~(fmgr.mask(color) | onbp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_)) |
    fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);

  auto const & oki_fields = finfo_[ocolor].ki_fields_;

#ifdef PROCESS_DANGEROUS_EVAL
  if (needDangerousDetect_ && board_->color() == color) {
    auto pw_under_attack = board_->fmgr().pawn_mask(color) & finfo_[ocolor].attack_mask_ & ~finfo_[color].attack_mask_;
    finfo_[color].attackedPawnsCount_ = pop_count(pw_under_attack);
  }
#endif // PROCESS_DANGEROUS_EVAL

  auto const& near_oking = oki_fields & finfo_[ocolor].kingAttacks_;
  finfo_[color].num_attackers_ += (finfo_[color].pawnAttacks_ & near_oking) != 0ULL;
  finfo_[color].score_king_ += pop_count(finfo_[color].pawnAttacks_ & oki_fields) * EvalCoefficients::pawnKingAttack_;
  finfo_[color].num_attackers_ += (finfo_[color].kingAttacks_ & near_oking) != 0ULL;

  auto kn_check = movesTable().caps(Figure::TypeKnight, board_->kingPos(ocolor)) & finfo_[color].knightMoves_;
  auto bi_check = magic_ns::bishop_moves(board_->kingPos(ocolor), mask_all_);
  auto r_check = magic_ns::rook_moves(board_->kingPos(ocolor), mask_all_);
  auto q_check = (bi_check | r_check) & finfo_[color].queenMoves_;
  bi_check &= finfo_[color].bishopMoves_;
  r_check &= finfo_[color].rookMoves_;

  auto check_possible_mask = (finfo_[color].multiattack_mask_ | ~finfo_[ocolor].kingAttacks_) & ~finfo_[ocolor].pawnAttacks_ & ~fmgr.pawn_mask(color);
  bool qcheck_possible = q_check & check_possible_mask;
  bool rcheck_possible = r_check & check_possible_mask;
  bool bicheck_possible = bi_check & check_possible_mask;
  bool kncheck_possible = kn_check & check_possible_mask;
  bool could_be_check = qcheck_possible || rcheck_possible || bicheck_possible || kncheck_possible;

  kn_check &= can_check_nb;
  bi_check &= can_check_nb;
  r_check &= can_check_r;
  q_check &= can_check_q;
  int num_checkers = (kn_check != 0ULL) + (bi_check != 0ULL) + (r_check != 0ULL) + (q_check != 0ULL);

  if (num_checkers)
    could_be_check = false;

#ifdef PROCESS_DANGEROUS_EVAL
  if (needDangerousDetect_ && board_->color() != color) {
    if (num_checkers)
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
  score.opening_ = finfo_[color].score_opening_;

  int score_king = finfo_[color].score_king_;
  if (fmgr.rooks(color) > 1 || fmgr.queens(color) > 0 || (fmgr.bishops(color) + fmgr.knights(color) + fmgr.rooks(color) > 2))
  {
    int check_score = (kn_check != 0) * EvalCoefficients::knightChecking_ +
                      (bi_check != 0) * EvalCoefficients::bishopChecking_ +
                      (r_check != 0) * EvalCoefficients::rookChecking_ +
                      (q_check != 0) * EvalCoefficients::queenChecking_;

    if (could_be_check) {
      check_score = ( kncheck_possible * EvalCoefficients::knightChecking_ +
                      bicheck_possible * EvalCoefficients::bishopChecking_ +
                      rcheck_possible * EvalCoefficients::rookChecking_ +
                      qcheck_possible * EvalCoefficients::queenChecking_ ) >> 2;
    }

    static const int checkers_coefficients[8] = { 0, 32, 64, 64, 64 };
    static const int attackers_coefficients[8] = { 0, 16, 32, 64, 96, 128, 128, 128 };

    int num_attackers = std::min(finfo_[color].num_attackers_, 7);
    auto attack_coeff = attackers_coefficients[num_attackers];

    auto near_oking = (finfo_[ocolor].kingAttacks_ & ~attacked_any_but_oking) & finfo_[color].attack_mask_;
    if (near_oking)
      attack_coeff += pop_count(near_oking) * EvalCoefficients::attackedNearKing_;
    
    auto protected_oking = (finfo_[ocolor].kingAttacks_ & attacked_any_but_oking) & finfo_[color].attack_mask_;
    auto remaining_oking = (oki_fields & ~finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].attack_mask_) & finfo_[color].attack_mask_;
    remaining_oking |= protected_oking;
    if (remaining_oking)
      attack_coeff += (pop_count(remaining_oking) * EvalCoefficients::attackedNearKing_) >> 1;

    num_checkers = std::min(num_checkers, 4);
    auto check_coeff = checkers_coefficients[num_checkers] + attack_coeff/2 + checkers_coefficients[could_be_check];
    if (num_attackers == 0)
      check_coeff >>= 3;

    score_king = ((score_king * attack_coeff + check_score * check_coeff) >> 5);

    auto king_left = (board_->kingPos(ocolor) & 7) < 4;
    auto general_pressure_mask = ((finfo_[color].attack_mask_ & ~attacked_any_but_oking) | (finfo_[color].multiattack_mask_ & ~finfo_[ocolor].multiattack_mask_)) & ~finfo_[ocolor].pawnAttacks_;
    auto attacks_king_side = general_pressure_mask & Figure::quaterBoard_[ocolor][king_left];
    int general_king_attacks_score = pop_count(attacks_king_side) * EvalCoefficients::generalKingPressure_;
    auto attacks_opponent_other = general_pressure_mask & Figure::quaterBoard_[ocolor][!king_left];
    int general_opponent_pressure = pop_count(attacks_opponent_other) * EvalCoefficients::generalOpponentPressure_;
    int general_score = general_king_attacks_score + general_opponent_pressure;

    score_king += general_score;
  }

  score.opening_ += score_king;
  return score;
}

} // NEngine
