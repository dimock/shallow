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

ScoreType32 Evaluator::evaluateKnights()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto ocolor = Figure::otherColor(color);
    BitMask mask = board_->fmgr().knight_mask(color);
    for (; mask;)
    {
      const int n = clear_lsb(mask);
      auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
      finfo_[color].knightAttacks_ |= knight_moves;

#ifdef GENERATE_MAT_MOVES_IN_EVAL
      finfo_[ocolor].kingMoves_ &= ~knight_moves;
#endif // GENERATE_MAT_MOVES_IN_EVAL

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];

      if (knight_moves & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        auto n_attacks = pop_count(knight_moves & finfo_[ocolor].kingAttacks_);
        finfo_[color].score_king_ += EvalCoefficients::knightKingAttack_ * n_attacks + EvalCoefficients::basicAttack_;
      }

      bool qpinned = false;
      if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color))) {
        knight_moves = 0ULL;
      }
      else
      {
#ifdef GENERATE_MAT_MOVES_IN_EVAL
        finfo_[color].knights_ |= set_mask_bit(n);
#endif // GENERATE_MAT_MOVES_IN_EVAL

        if (isPinned(n, color, ocolor, finfo_[color].rq_mask_, fmgr.bishop_mask(ocolor), nst::none)) {
          qpinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::knightPinned_;
        }
        if (board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor))) {
          finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
          finfo_[color].discoveredCheck_ = true;

#ifdef GENERATE_MAT_MOVES_IN_EVAL
          auto nmsk = knight_moves & inv_mask_all_;
          if (!finfo_[ocolor].kingMoves_ && nmsk) {
            finfo_[color].matTreat_ = true;
            if (color == board_->color()) {
              nmsk &= ~(finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].kingAttacks_);
              if (nmsk & !(knight_moves & finfo_[ocolor].rq_mask_)) {
                int to = _lsb64(nmsk);
                move_[color] = SMove{ n, to };
              }
            }
          }
          else if (color == board_->color() && !(knight_moves & finfo_[ocolor].rq_mask_)) {
            while (nmsk) {
              int to = clear_lsb(nmsk);
              auto to_attacks = movesTable().caps(Figure::TypeKnight, to);
              if (to_attacks & fmgr.queen_mask(ocolor)) {
                move_[color] = SMove{ n, to };
                break;
              }
              else if (to_attacks & finfo_[ocolor].rq_mask_) {
                move_[color] = SMove{ n, to };
                // continue searching for queen attack
              }
            }
          }
#endif // GENERATE_MAT_MOVES_IN_EVAL
        }
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
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
    }
    finfo_[color].nb_attacked_ |= finfo_[color].knightMoves_;
    finfo_[color].nbr_attacked_ |= finfo_[color].knightMoves_;
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

ScoreType32 Evaluator::evaluateBishops()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto ocolor = Figure::otherColor(color);
    BitMask mask = fmgr.bishop_mask(color);
    for (; mask;)
    {
      int n = clear_lsb(mask);

      auto const& bishop_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_);
      auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
      finfo_[color].bishopAttacks_ |= bishop_attacks;
      
#ifdef GENERATE_MAT_MOVES_IN_EVAL
      finfo_[ocolor].kingMoves_ &= ~bishop_attacks;
#endif // GENERATE_MAT_MOVES_IN_EVAL

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];

      if(bishop_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        auto n_attacks = pop_count(bishop_attacks & finfo_[ocolor].kingAttacks_);
        finfo_[color].score_king_ += EvalCoefficients::bishopKingAttack_ * n_attacks + EvalCoefficients::basicAttack_;
      }
      
      bool q_pinned = false;
      if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color))) {
        bishop_moves = 0ULL;
      }
      else
      {
#ifdef GENERATE_MAT_MOVES_IN_EVAL
        finfo_[color].bishops_ |= set_mask_bit(n);
#endif // GENERATE_MAT_MOVES_IN_EVAL

        if (isPinned(n, color, ocolor, fmgr.queen_mask(color), fmgr.rook_mask(ocolor), nst::rook)) {
          q_pinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::bishopPinned_;
        }
        if (!finfo_[color].discoveredCheck_ && board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor))) {
          finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
          finfo_[color].discoveredCheck_ = true;
          
#ifdef GENERATE_MAT_MOVES_IN_EVAL
          auto bimsk = bishop_moves & inv_mask_all_;
          if (!finfo_[ocolor].kingMoves_) {
            finfo_[color].matTreat_ = true;
            if (color == board_->color()) {
              bimsk &= ~(finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].kingAttacks_);
              if (bimsk && !(bishop_moves & finfo_[ocolor].rq_mask_)) {
                int to = _lsb64(bimsk);
                move_[color] = SMove{ n, to };
              }
            }
          }
          else if (color == board_->color() && !(bishop_moves & finfo_[ocolor].rq_mask_)) {
            while (bimsk) {
              int to = clear_lsb(bimsk);
              auto to_attacks = magic_ns::bishop_moves(to, mask_all_);
              if (to_attacks & fmgr.queen_mask(ocolor)) {
                move_[color] = SMove{ n, to };
                break;
              }
              else if (to_attacks & finfo_[ocolor].rq_mask_) {
                move_[color] = SMove{ n, to };
                // continue searching for queen attack
              }
            }
          }
#endif // GENERATE_MAT_MOVES_IN_EVAL
        }
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

ScoreType32 Evaluator::evaluateRook()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
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
      finfo_[color].rookAttacks_ |= rook_attacks;

#ifdef GENERATE_MAT_MOVES_IN_EVAL
      finfo_[ocolor].kingMoves_ &= ~rook_attacks;
#endif // GENERATE_MAT_MOVES_IN_EVAL

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

      if (rook_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        auto n_attacks = pop_count(rook_attacks & finfo_[ocolor].kingAttacks_);
        finfo_[color].score_king_ += EvalCoefficients::rookKingAttack_ * n_attacks + EvalCoefficients::basicAttack_;
      }

      bool q_pinned = false;
      if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color))) {
        rook_moves = 0ULL;
      }
      else
      {
#ifdef GENERATE_MAT_MOVES_IN_EVAL
        finfo_[color].rooks_ |= set_mask_bit(n);
#endif // GENERATE_MAT_MOVES_IN_EVAL

        if (isPinned(n, color, ocolor, finfo_[color].rq_mask_, fmgr.bishop_mask(ocolor), nst::bishop)) {
          q_pinned = true;
          finfo_[color].score_mob_ -= EvalCoefficients::rookPinned_;
        }
        if (!finfo_[color].discoveredCheck_ && board_->discoveredCheck(n, mask_all_, color, board_->kingPos(ocolor))) {
          finfo_[color].score_mob_ += EvalCoefficients::discoveredCheckBonus_;
          finfo_[color].discoveredCheck_ = true;

#ifdef GENERATE_MAT_MOVES_IN_EVAL
          auto rmsk = rook_moves & inv_mask_all_;
          if (!finfo_[ocolor].kingMoves_) {
            finfo_[color].matTreat_ = true;
            if (color == board_->color()) {
              rmsk &= ~(finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].nb_attacked_ | finfo_[ocolor].kingAttacks_);
              if (rmsk && !(rook_moves & fmgr.queen_mask(ocolor))) {
                int to = _lsb64(rmsk);
                move_[color] = SMove{ n, to };
              }
            }
          }
          else if(color == board_->color() && !(rook_moves & fmgr.queen_mask(ocolor))) {
            while (rmsk) {
              int to = clear_lsb(rmsk);
              auto to_attacks = magic_ns::rook_moves(to, mask_all_);
              if (to_attacks & fmgr.queen_mask(ocolor)) {
                move_[color] = SMove{ n, to };
                break;
              }
            }
          }
#endif // GENERATE_MAT_MOVES_IN_EVAL
        }
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
          ((finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].nb_attacked_) & set_mask_bit(n)) != 0ULL)
      {
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

ScoreType32 Evaluator::evaluateQueens()
{
  auto const& fmgr = board_->fmgr();
  ScoreType32 score[2];
  for (auto color : { Figure::ColorBlack, Figure::ColorWhite })
  {
    auto mask = fmgr.queen_mask(color);
    auto ocolor = Figure::otherColor(color);
    for (; mask;)
    {
      auto n = clear_lsb(mask);
      // mobility
      auto qr_attacks = magic_ns::rook_moves(n, finfo_[color].mask_xray_r_);
      auto const queen_attacks = magic_ns::bishop_moves(n, finfo_[color].mask_xray_b_) | qr_attacks;
      auto queen_moves = magic_ns::queen_moves(n, mask_all_);
      finfo_[color].queenAttacks_ |= queen_attacks;

#ifdef GENERATE_MAT_MOVES_IN_EVAL
      finfo_[ocolor].kingMoves_ &= ~queen_attacks;
#endif // GENERATE_MAT_MOVES_IN_EVAL

      // king protection
      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color] += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][ki_dist];

      auto qx_attacks = queen_moves | qr_attacks;
      if (qx_attacks & finfo_[ocolor].ki_fields_)
      {
        finfo_[color].num_attackers_++;
        auto n_attacks = pop_count(qx_attacks & finfo_[ocolor].kingAttacks_);
        finfo_[color].score_king_ += EvalCoefficients::queenKingAttack_ * n_attacks + EvalCoefficients::basicAttack_;
      }

      // mobility
      if (board_->discoveredCheck(n, mask_all_, ocolor, board_->kingPos(color))) {
        auto from_mask = betweenMasks().from(board_->kingPos(color), n);
        queen_moves &= from_mask;
      }
#ifdef GENERATE_MAT_MOVES_IN_EVAL
      else {
        finfo_[color].queens_ |= set_mask_bit(n);
      }
#endif // GENERATE_MAT_MOVES_IN_EVAL

      if (queen_moves) {
        finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & queen_attacks;
        finfo_[color].attack_mask_ |= queen_attacks;
        finfo_[color].queenMoves_ |= queen_moves;
      }

      auto q_moves_mask = queen_moves & ((finfo_[color].cango_mask_ & ~finfo_[ocolor].nbr_attacked_) | fmgr.queen_mask(ocolor));
      auto n_moves = pop_count(q_moves_mask);
      finfo_[color].score_mob_ += EvalCoefficients::queenMobility_[n_moves & 31];

      if (!(q_moves_mask & ~fmgr.mask(color) & ~finfo_[ocolor].rookTreatAttacks_ & ~finfo_[ocolor].bishopTreatAttacks_) &&
        (((finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].nbr_attacked_) & set_mask_bit(n)) != 0ULL))
      {
        finfo_[color].score_mob_ -= EvalCoefficients::immobileAttackBonus_;
      }
    }
  }
  return score[Figure::ColorWhite] - score[Figure::ColorBlack];
}

//Evaluator::FullScore Evaluator::evaluateKingPressure(Figure::Color color)
//{
//  auto const& fmgr = board_->fmgr();
//  auto ocolor = Figure::otherColor(color);
//  auto const oki_pos = board_->kingPos(ocolor);
//  const BitMask attacked_any_but_oking = finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[ocolor].kingAttacks_);
//  const BitMask attacked_oking_only = finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].multiattack_mask_;
//
//  auto onbrp_attacked = finfo_[ocolor].nbr_attacked_ | finfo_[ocolor].pawnAttacks_;
//  auto onbp_attacked  = finfo_[ocolor].nb_attacked_ | finfo_[ocolor].pawnAttacks_;
//  const BitMask can_check_q = ~(fmgr.mask(color) | attacked_any_but_oking | (attacked_oking_only & ~finfo_[color].multiattack_mask_));
//  const BitMask can_check_r = ~(fmgr.mask(color) | onbrp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_)) |
//    fmgr.queen_mask(ocolor);
//  const BitMask can_check_nb = ~(fmgr.mask(color) | onbp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_)) |
//    fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);
//
//  auto const & oki_fields = finfo_[ocolor].ki_fields_;
//  auto const& near_oking = oki_fields & finfo_[ocolor].kingAttacks_;
//  auto const near_oking_pw = near_oking | (oki_fields & fmgr.pawn_mask(ocolor));
//  finfo_[color].num_attackers_ += (finfo_[color].pawnAttacks_ & near_oking) != 0ULL;
//  finfo_[color].num_attackers_ += (finfo_[color].kingAttacks_ & near_oking) != 0ULL;
//  finfo_[color].score_king_ += pop_count(finfo_[color].pawnAttacks_ & near_oking_pw) * EvalCoefficients::pawnKingAttack_;
//
//  auto kn_check = movesTable().caps(Figure::TypeKnight, oki_pos) & finfo_[color].knightMoves_;
//  auto bi_check = magic_ns::bishop_moves(oki_pos, mask_all_);
//  auto r_check = magic_ns::rook_moves(oki_pos, mask_all_);
//  auto q_check = (bi_check | r_check) & finfo_[color].queenMoves_;
//  bi_check &= finfo_[color].bishopMoves_;
//  r_check &= finfo_[color].rookMoves_;
//
//  auto check_possible_mask = (finfo_[color].multiattack_mask_ | ~finfo_[ocolor].kingAttacks_) & ~finfo_[ocolor].pawnAttacks_ & ~fmgr.pawn_mask(color);
//  bool qcheck_possible = q_check & check_possible_mask;
//  bool rcheck_possible = r_check & check_possible_mask;
//  bool bicheck_possible = bi_check & check_possible_mask;
//  bool kncheck_possible = kn_check & check_possible_mask;
//  bool could_be_check = qcheck_possible || rcheck_possible || bicheck_possible || kncheck_possible;
//
//  kn_check &= can_check_nb;
//  bi_check &= can_check_nb;
//  r_check &= can_check_r;
//  q_check &= can_check_q;
//  int num_checkers = (kn_check != 0ULL) + (bi_check != 0ULL) + (r_check != 0ULL) + (q_check != 0ULL) + finfo_[color].discoveredCheck_;
//
//  if (num_checkers) {
//    could_be_check = false;
//  }
//  FullScore score;
//  score.opening_ = finfo_[color].score_opening_;
//  int score_king = finfo_[color].score_king_;
//  int check_coeff = 0;
//
//  auto mask_all_npw = mask_all_ & ~fmgr.pawn_mask(color);
//  auto bi_check_npw = magic_ns::bishop_moves(oki_pos, mask_all_npw) & (fmgr.bishop_mask(color) | fmgr.queen_mask(color));
//  auto r_check_npw = magic_ns::rook_moves(oki_pos, mask_all_npw) & (fmgr.rook_mask(color) | fmgr.queen_mask(color)) & pawnMasks().mask_row(oki_pos);
//  auto check_npw = bi_check_npw | r_check_npw;
//  while (check_npw) {
//    auto n = clear_lsb(check_npw);
//    auto const& btw_mask = betweenMasks().between(n, oki_pos);
//    auto mpawn = btw_mask & fmgr.pawn_mask(color);
//    if (!mpawn || !one_bit_set(mpawn))
//      continue;
//    int from = _lsb64(mpawn);
//    BitMask mpattack{};
//    if (color) {
//      mpawn <<= 8;
//      mpattack = (((mpawn << 9) & Figure::pawnCutoffMasks_[0]) | ((mpawn << 7) & Figure::pawnCutoffMasks_[1])) & 0xffffffffffffff00;
//    }
//    else {
//      mpawn >>= 8;
//      mpattack = (((mpawn >> 7) & Figure::pawnCutoffMasks_[0]) | ((mpawn >> 9) & Figure::pawnCutoffMasks_[1])) & 0x00ffffffffffffff;
//    }
//    if (mpawn & mask_all_)
//      continue;
//    finfo_[color].discoveredCheck_ = true;
//    score.common_ += EvalCoefficients::discoveredCheckBonus_;
//    if (mpattack & finfo_[ocolor].nbrq_mask_ & (finfo_[color].attack_mask_ | ~finfo_[ocolor].kingAttacks_)) {
//      score.common_ += EvalCoefficients::discoveredCheckBonus_ << 2;
//#ifdef GENERATE_MAT_MOVES_IN_EVAL
//      if (color == board_->color()) {
//        if (mpawn && !board_->discoveredCheck(from, mask_all_, ocolor, board_->kingPos(color))) {
//          int to = _lsb64(mpawn);
//          move_[color] = SMove{ from, to };
//        }
//      }
//    }
//    else if (mpawn && !finfo_[ocolor].kingMoves_ && !board_->discoveredCheck(from, mask_all_, ocolor, board_->kingPos(color))) {
//      if (color == board_->color()) {
//        int to = _lsb64(mpawn);
//        move_[color] = SMove{ from, to };
//      }
//      finfo_[color].matTreat_ = true;
//#endif // GENERATE_MAT_MOVES_IN_EVAL
//    }
//    break;
//  }
//
//  if (fmgr.rooks(color) > 1 || fmgr.queens(color) > 0 || (fmgr.bishops(color) + fmgr.knights(color) + fmgr.rooks(color) > 2))
//  {
//    int check_score = (kn_check != 0) * EvalCoefficients::knightChecking_ +
//                      (bi_check != 0) * EvalCoefficients::bishopChecking_ +
//                      (r_check != 0) * EvalCoefficients::rookChecking_ +
//                      (q_check != 0) * EvalCoefficients::queenChecking_ +
//                      finfo_[color].discoveredCheck_* EvalCoefficients::discoveredCheckBonus_;
//
//    if (could_be_check) {
//      check_score = EvalCoefficients::possibleCheckBonus_;
//    }
//    if (num_checkers)
//    {
//      int n_checked_fields = pop_count(q_check | r_check | bi_check | kn_check);
//      check_coeff += n_checked_fields * EvalCoefficients::checkedFieldBonus_;
//      auto const& ofgmask = board_->fmgr().mask(ocolor);
//      while (!finfo_[color].matTreat_ && kn_check) {
//        int n = clear_lsb(kn_check);
//        auto oking_mv = (finfo_[color].knightMoves_ | ~finfo_[color].attack_mask_) & ~finfo_[color].multiattack_mask_ &
//          ~movesTable().caps(Figure::TypeKnight, n) & finfo_[ocolor].kingAttacks_ & ~ofgmask;
//        if (!oking_mv) {
//          check_score += EvalCoefficients::knightChecking_;
//          finfo_[color].matTreat_ = true;
//#ifdef GENERATE_MAT_MOVES_IN_EVAL
//          if (color == board_->color() && (set_mask_bit(n) & inv_mask_all_)) {
//            if (auto nmsk = movesTable().caps(Figure::TypeKnight, n) & finfo_[color].knights_) {
//              int from = _lsb64(nmsk);
//              move_[color] = SMove{ from, n };
//            }
//          }
//#endif // GENERATE_MAT_MOVES_IN_EVAL
//        }
//      }
//      if (!finfo_[color].matTreat_)
//      {
//        auto const o_protected_mask = finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].multiattack_mask_;
//        auto const o_qnbr_protect_mask = ((finfo_[ocolor].pawns_fwd_ | finfo_[ocolor].nbr_attacked_ | finfo_[ocolor].queenMoves_) & o_protected_mask) |
//          (finfo_[ocolor].pawns_fwd_ & finfo_[color].attack_mask_);
//        auto x_mask = mask_all_ & ~fmgr.king_mask(ocolor);
//        while (!finfo_[color].matTreat_ && bi_check) {
//          int n = clear_lsb(bi_check);
//          auto oking_mv = (finfo_[color].bishopMoves_ | ~finfo_[color].attack_mask_) & ~finfo_[color].multiattack_mask_ &
//            ~magic_ns::bishop_moves(n, x_mask) & finfo_[ocolor].kingAttacks_ & ~ofgmask;
//          if (oking_mv != 0ULL)
//            continue;
//          auto const& btw_mask = betweenMasks().between(n, oki_pos);
//          if (!(btw_mask & o_qnbr_protect_mask)) {
//            check_score += EvalCoefficients::matTreatBonus_;
//            finfo_[color].matTreat_ = true;
//#ifdef GENERATE_MAT_MOVES_IN_EVAL
//            if (color == board_->color() && (set_mask_bit(n) & inv_mask_all_)) {
//              if (auto bimsk = magic_ns::bishop_moves(n, mask_all_) & finfo_[color].bishops_) {
//                int from = _lsb64(bimsk);
//                move_[color] = SMove{ from, n };
//              }
//            }
//#endif // GENERATE_MAT_MOVES_IN_EVAL
//          }
//        }
//        while (!finfo_[color].matTreat_ && r_check) {
//          int n = clear_lsb(r_check);
//          auto oking_mv = (finfo_[color].rookMoves_ | ~finfo_[color].attack_mask_) & ~finfo_[color].multiattack_mask_ &
//            ~magic_ns::rook_moves(n, x_mask) & finfo_[ocolor].kingAttacks_ & ~ofgmask;
//          if (oking_mv != 0ULL)
//            continue;
//          auto const& btw_mask = betweenMasks().between(n, oki_pos);
//          if (!(btw_mask & o_qnbr_protect_mask)) {
//            check_score += EvalCoefficients::matTreatBonus_;
//            finfo_[color].matTreat_ = true;
//#ifdef GENERATE_MAT_MOVES_IN_EVAL
//            if (color == board_->color() && (set_mask_bit(n) & inv_mask_all_)) {
//              if (auto rmsk = magic_ns::rook_moves(n, mask_all_) & finfo_[color].rooks_) {
//                int from = _lsb64(rmsk);
//                move_[color] = SMove{ from, n };
//              }
//            }
//#endif // GENERATE_MAT_MOVES_IN_EVAL
//          }
//        }
//        if (!finfo_[color].matTreat_)
//        {
//          while (!finfo_[color].matTreat_ && q_check) {
//            int n = clear_lsb(q_check);
//            auto oking_mv = (finfo_[color].queenMoves_ | ~finfo_[color].attack_mask_) & ~finfo_[color].multiattack_mask_ &
//              ~magic_ns::queen_moves(n, x_mask) & finfo_[ocolor].kingAttacks_ & ~ofgmask;
//            if (oking_mv != 0ULL)
//              continue;
//            auto const& btw_mask = betweenMasks().between(n, oki_pos);
//            if (!(btw_mask & o_qnbr_protect_mask)) {
//              check_score += EvalCoefficients::matTreatBonus_;
//              finfo_[color].matTreat_ = true;
//#ifdef GENERATE_MAT_MOVES_IN_EVAL
//              if (color == board_->color() && (set_mask_bit(n) & inv_mask_all_)) {
//                if (auto qmsk = magic_ns::queen_moves(n, mask_all_) & finfo_[color].queens_) {
//                  int from = _lsb64(qmsk);
//                  move_[color] = SMove{ from, n };
//                }
//              }
//#endif // GENERATE_MAT_MOVES_IN_EVAL
//            }
//          }
//        }
//      }
//    }
//
//    static const int checkers_coefficients[8] = { 0, 32, 64, 64, 64 };
//    static const int attackers_coefficients[8] = { 0, 0, 12, 32, 36, 40, 42, 44 };
//
//    int num_attackers = std::min(finfo_[color].num_attackers_, 7);
//    auto attack_coeff = attackers_coefficients[num_attackers];
//
//    auto near_oking = (finfo_[ocolor].kingAttacks_ & ~attacked_any_but_oking) & finfo_[color].attack_mask_;
//    if (near_oking)
//      attack_coeff += pop_count(near_oking) * EvalCoefficients::attackedNearKing_;
//    
//    auto protected_oking = (finfo_[ocolor].kingAttacks_ & attacked_any_but_oking) & finfo_[color].attack_mask_;
//    auto remaining_oking = oki_fields & ~finfo_[ocolor].kingAttacks_ & finfo_[color].attack_mask_;
//    remaining_oking |= protected_oking;
//    if (remaining_oking)
//      attack_coeff += (pop_count(remaining_oking) * EvalCoefficients::attackedNearKing_) >> 3;
//
//    num_checkers = std::min(num_checkers, 4);
//    check_coeff += checkers_coefficients[num_checkers] + ((attack_coeff + checkers_coefficients[could_be_check]) >> 1);
//    if (num_attackers == 0)
//      check_coeff >>= 3;
//
//    score_king = ((score_king * attack_coeff + check_score * check_coeff) >> 5);
//
//    auto king_left = (oki_pos & 7) < 4;
//    auto general_pressure_mask = ((finfo_[color].attack_mask_ & ~attacked_any_but_oking) |
//      (finfo_[color].multiattack_mask_ & ~finfo_[ocolor].multiattack_mask_)) & ~finfo_[ocolor].pawnAttacks_;
//    auto attacks_king_side = general_pressure_mask & Figure::quaterBoard_[ocolor][king_left];
//    int general_king_attacks_score = pop_count(attacks_king_side) * EvalCoefficients::generalKingPressure_;
//    auto attacks_opponent_other = general_pressure_mask & Figure::quaterBoard_[ocolor][!king_left];
//    auto attacks_king_other = finfo_[color].attack_mask_ & Figure::quaterBoard_[ocolor][king_left] & ~attacks_king_side;
//    int general_opponent_pressure = pop_count(attacks_opponent_other | attacks_king_other) * EvalCoefficients::generalOpponentPressure_;
//    int general_score = general_king_attacks_score + general_opponent_pressure;
//
//    score_king += general_score;
//  }
//
//  score.opening_ += score_king;
//  return score;
//}

} // NEngine
