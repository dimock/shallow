#include <Evaluator.h>

namespace NEngine
{

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
      int n = clear_lsb(mask);
      auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & knight_moves;
      finfo_[color].attack_mask_ |= knight_moves;
      finfo_[color].knightAttacks_ |= knight_moves;

      // king protection
#ifdef EVAL_KP_BASIC
      //auto oki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
      //score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][oki_dist];

      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];
#endif
    }
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
      auto mask_all_no_bq = mask_all_ & ~(board_->fmgr().bishop_mask(color) | board_->fmgr().queen_mask(color));
      auto bishop_moves = magic_ns::bishop_moves(n, mask_all_no_bq);
      finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & bishop_moves;
      finfo_[color].attack_mask_ |= bishop_moves;
      finfo_[color].bishopAttacks_ |= bishop_moves;
      finfo_[color].bishopDirectAttacks_ |= magic_ns::bishop_moves(n, mask_all_);

      // king protection
#ifdef EVAL_KP_BASIC
      //auto oki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
      //score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][oki_dist];

      auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
      score[color].opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];
#endif
    }
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
    auto mask_all_no_rq = mask_all_ & ~(fmgr.rook_mask(color) | fmgr.queen_mask(color));
    auto const& rook_moves = magic_ns::rook_moves(n, mask_all_no_rq);
    finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & rook_moves;
    finfo_[color].attack_mask_ |= rook_moves;
    finfo_[color].rookAttacks_ |= rook_moves;
    finfo_[color].rookDirectAttacks_ |= magic_ns::rook_moves(n, mask_all_);

    // open column
#ifdef EVAL_OPEN_R
    auto const& mask_col = pawnMasks().mask_column(n & 7);
    bool no_pw_color = (mask_col & fmgr.pawn_mask(color)) == 0ULL;
    bool no_pw_ocolor = (mask_col & fmgr.pawn_mask(ocolor)) == 0ULL;
    score.opening_ += EvalCoefficients::openRook_[(no_pw_color+no_pw_ocolor) & 3];
    score.endGame_ += EvalCoefficients::openRook_[(no_pw_color + no_pw_ocolor) & 3] >> 2;
#endif
    
    // king protection
#ifdef EVAL_KP_BASIC
    //auto oki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
    //score.opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeRook][oki_dist];

    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
    score.opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeRook][ki_dist];
#endif
  }
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
    auto mask_all_no_bq = mask_all_ & ~(fmgr.bishop_mask(color) | fmgr.queen_mask(color));
    auto mask_all_no_rq = mask_all_ & ~(fmgr.rook_mask(color) | fmgr.queen_mask(color));
    auto queen_moves = magic_ns::bishop_moves(n, mask_all_no_bq) | magic_ns::rook_moves(n, mask_all_no_rq);
    finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & queen_moves;
    finfo_[color].attack_mask_ |= queen_moves;
    finfo_[color].queenAttacks_ |= queen_moves;
    finfo_[color].queenDirectAttacks_ |= magic_ns::queen_moves(n, mask_all_);

    // king protection
#ifdef EVAL_KP_BASIC
    //auto oki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
    //score.opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][oki_dist];

    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(color));
    score.opening_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][ki_dist];
#endif
  }
  return score;
}

Evaluator::FullScore Evaluator::evaluateMobilityAndKingPressure(Figure::Color color)
{
#ifdef EVAL_MOB
  int score_mob = 0;
#endif

#ifdef EVAL_KING_PR
  int score_king = 0;
  int attackers_count[Figure::TypesNum] = {};
#endif

  auto const& fmgr = board_->fmgr();
  auto ocolor = Figure::otherColor(color);
  const BitMask attacked_any_but_oking = finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[ocolor].kingAttacks_);
  const BitMask attacked_oking_only = finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].multiattack_mask_;

  const BitMask mask_xray_b = mask_all_ & ~(fmgr.bishop_mask(color) | fmgr.queen_mask(color));
  const BitMask mask_xray_r = mask_all_ & ~(fmgr.rook_mask(color) | fmgr.queen_mask(color));

  const BitMask cango_mask = ~(finfo_[ocolor].pawnAttacks_ | fmgr.pawn_mask(color) | fmgr.king_mask(color) | fmgr.king_mask(ocolor));
  const BitMask can_check_mask = ~(fmgr.mask(color) | attacked_any_but_oking | (attacked_oking_only & ~finfo_[color].multiattack_mask_));

  const BitMask allowed_not_o_attacked = ~(fmgr.mask(color) | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_));
  const BitMask allowed_moves_q = (cango_mask & allowed_not_o_attacked) | fmgr.queen_mask(ocolor);
  const BitMask allowed_moves_r = allowed_moves_q | fmgr.rook_mask(ocolor);
  const BitMask allowed_moves_nb = allowed_moves_r | fmgr.bishop_mask(ocolor) | fmgr.knight_mask(ocolor);

  //auto ctype = getCastleType(ocolor);
  //auto castle_mask = ctype >= 0 ? king_attack_mask_[ocolor][ctype] : 0ULL;

  auto oki_fields = movesTable().caps(Figure::TypeKing, board_->kingPos(ocolor));
  if (ocolor)
    oki_fields = oki_fields | (oki_fields << 8);
  else
    oki_fields = oki_fields | (oki_fields >> 8);
  oki_fields &= ~(fmgr.king_mask(ocolor) | finfo_[ocolor].pawnAttacks_);
  const auto oki_fields_unprotected = oki_fields & ~attacked_any_but_oking;

#ifdef PROCESS_DANGEROUS_EVAL
  auto pw_under_attack = board_->fmgr().pawn_mask(color) & finfo_[ocolor].attack_mask_ & ~finfo_[color].attack_mask_;
  finfo_[color].pawnsUnderAttack_ = pop_count(pw_under_attack);
  if (finfo_[color].pawnsUnderAttack_ > 0)
  {
    finfo_[color].pawnsUnderAttack_ = finfo_[color].pawnsUnderAttack_;
  }
#endif // PROCESS_DANGEROUS_EVAL

#ifdef EVAL_KING_PR
  attackers_count[Figure::TypePawn] = (finfo_[color].pawnAttacks_ & oki_fields) != 0ULL;
  score_king += pop_count(finfo_[color].pawnAttacks_ & oki_fields) * EvalCoefficients::pawnKingAttack_;
  attackers_count[Figure::TypeKing] = (finfo_[color].kingAttacks_ & oki_fields) != 0ULL;
#endif // EVAL_KING_PR

  BitMask knights = fmgr.knight_mask(color);
  for(; knights;)
  {
    int n = clear_lsb(knights);
    auto const& knight_moves = movesTable().caps(Figure::TypeKnight, n);

#ifdef EVAL_KING_PR
    int n_attack = (knight_moves & oki_fields) != 0ULL;
    attackers_count[Figure::TypeKnight] += n_attack;
    score_king += n_attack * EvalCoefficients::knightKingAttack_;
#endif // EVAL_KING_PR

#ifdef EVAL_MOB
    auto n_moves = pop_count(knight_moves & cango_mask);
    score_mob += EvalCoefficients::knightMobility_[n_moves & 15];

    if (n_moves < 2 && blockedKnight(color, n)) {
      score_mob -= EvalCoefficients::knightBlocked_;
    }

#ifdef PROCESS_DANGEROUS_EVAL
    if ((knight_moves & allowed_moves_nb) == 0ULL)
    {
      auto kn_mask = set_mask_bit(n);
      bool notProtected = (kn_mask & finfo_[color].attack_mask_) == 0ULL;
      bool attacked = (kn_mask & finfo_[ocolor].attack_mask_) != 0ULL;
      bool pawnAttack = (kn_mask & finfo_[ocolor].pawnAttacks_) != 0ULL;
      if((notProtected && attacked) || pawnAttack)
        finfo_[color].knightsUnderAttack_++;
    }
#endif // PROCESS_DANGEROUS_EVAL
#endif // EVAL_MOB

  }

  BitMask bishops = fmgr.bishop_mask(color);
  for(; bishops;)
  {
    int n = clear_lsb(bishops);

#ifdef EVAL_KING_PR
    auto const& bishop_attacks = magic_ns::bishop_moves(n, mask_xray_b);
    int bi_attack = (bishop_attacks & oki_fields) != 0ULL;
    attackers_count[Figure::TypeBishop] += bi_attack;
    score_king += bi_attack * EvalCoefficients::bishopKingAttack_;
#endif // EVAL_KING_PR

    // mobility
#ifdef EVAL_MOB
    auto const& bishop_moves = magic_ns::bishop_moves(n, mask_all_);
    int n_moves = pop_count(bishop_moves & cango_mask);
    score_mob += EvalCoefficients::bishopMobility_[n_moves & 15];

    if (n_moves < 2 && blockedBishop(color, n)) {
      score_mob -= EvalCoefficients::bishopBlocked_;
    }

#ifdef PROCESS_DANGEROUS_EVAL
    if ((bishop_moves & allowed_moves_nb) == 0ULL)
    {
      auto bi_mask = set_mask_bit(n);
      bool notProtected = (bi_mask & finfo_[color].attack_mask_) == 0ULL;
      bool attacked = (bi_mask & finfo_[ocolor].attack_mask_) != 0ULL;
      bool pawnAttack = (bi_mask & finfo_[ocolor].pawnAttacks_) != 0ULL;
      if ((notProtected && attacked) || pawnAttack)
        finfo_[color].bishopsUnderAttack_++;
    }
#endif // PROCESS_DANGEROUS_EVAL

#endif // EVAL_MOB
  }

  BitMask rooks = fmgr.rook_mask(color);
  for(; rooks;)
  {
    auto n = clear_lsb(rooks);

#ifdef EVAL_KING_PR
    auto const& rook_attacks = magic_ns::rook_moves(n, mask_xray_r);
    int r_attack = (rook_attacks & oki_fields) != 0ULL;
    attackers_count[Figure::TypeRook] += r_attack;
    score_king += r_attack * EvalCoefficients::rookKingAttack_;
#endif // EVAL_KING_PR

    // mobility
#ifdef EVAL_MOB
    auto const& rook_moves = magic_ns::rook_moves(n, mask_all_);
    auto r_moves_mask = rook_moves & cango_mask;
    int n_moves = pop_count(r_moves_mask);
    score_mob += EvalCoefficients::rookMobility_[n_moves & 15];

    // fake castle possible
    if (n_moves < 4) {
      if (fakeCastle(color, n, r_moves_mask))
        score_mob += EvalCoefficients::fakeCastle_;
      else if (blockedRook(color, n, r_moves_mask))
        score_mob -= EvalCoefficients::rookBlocked_;
    }

#ifdef PROCESS_DANGEROUS_EVAL
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
#endif // PROCESS_DANGEROUS_EVAL

#endif // EVAL_MOB
  }

  BitMask queens = fmgr.queen_mask(color);
  for(; queens;)
  {
    auto n = clear_lsb(queens);

#ifdef EVAL_KING_PR
    auto const queen_attacks = magic_ns::bishop_moves(n, mask_xray_b) | magic_ns::rook_moves(n, mask_xray_r);
    int q_attack = (queen_attacks & oki_fields) != 0ULL;
    attackers_count[Figure::TypeQueen] += q_attack;
    score_king += q_attack * EvalCoefficients::queenKingAttack_;
#endif // EVAL_KING_PR

    // mobility
#ifdef EVAL_MOB
    auto const& queen_moves = magic_ns::queen_moves(n, mask_all_);
    auto n_moves = pop_count(queen_moves & cango_mask);
    score_mob += EvalCoefficients::queenMobility_[n_moves & 31];

#ifdef PROCESS_DANGEROUS_EVAL
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
#endif // PROCESS_DANGEROUS_EVAL

#endif // EVAL_MOB
  }

  auto kn_check = movesTable().caps(Figure::TypeKnight, board_->kingPos(ocolor));
  auto bi_check = magic_ns::bishop_moves(board_->kingPos(ocolor), mask_all_);
  auto r_check = magic_ns::rook_moves(board_->kingPos(ocolor), mask_all_);
  auto q_check = bi_check | r_check;

  kn_check &= finfo_[color].knightAttacks_ & can_check_mask;
  bi_check &= finfo_[color].bishopDirectAttacks_ & can_check_mask;
  r_check &= finfo_[color].rookDirectAttacks_ & can_check_mask;
  q_check &= finfo_[color].queenDirectAttacks_ & can_check_mask;

#ifdef PROCESS_DANGEROUS_EVAL
  auto ofgmask = board_->fmgr().pawn_mask(ocolor) | board_->fmgr().knight_mask(ocolor) |
    board_->fmgr().bishop_mask(ocolor) | board_->fmgr().rook_mask(ocolor) | board_->fmgr().queen_mask(ocolor);
  BitMask oking_moves = movesTable().caps(Figure::TypeKing, board_->kingPos(ocolor)) & ~ofgmask & ~finfo_[color].attack_mask_;
  finfo_[ocolor].matThreat_ = (kn_check != 0ULL || bi_check != 0ULL || r_check != 0ULL || q_check != 0ULL) &&
    (oking_moves == 0ULL);
#endif // PROCESS_DANGEROUS_EVAL


#ifdef EVAL_KING_PR

  if (fmgr.rooks(color) > 1 || fmgr.queens(color) > 0 || (fmgr.bishops(color) + fmgr.knights(color) + fmgr.rooks(color) > 2))
  {
    auto king_attacks = oki_fields & finfo_[color].attack_mask_;
    auto attacks_unprotected = oki_fields_unprotected & finfo_[color].attack_mask_;
    auto attacks_unprotected_multi = oki_fields_unprotected & finfo_[color].multiattack_mask_;

    int check_score = (kn_check != 0) * EvalCoefficients::knightChecking_ +
      (bi_check != 0) * EvalCoefficients::bishopChecking_ +
      (r_check != 0) * EvalCoefficients::rookChecking_ +
      (q_check != 0) * EvalCoefficients::queenChecking_;

    //int attackersWeight = fmgr.knights(color)*Figure::figureWeight_[Figure::TypeKnight] +
    //  fmgr.bishops(color)*Figure::figureWeight_[Figure::TypeBishop] +
    //  fmgr.rooks(color)*Figure::figureWeight_[Figure::TypeRook] +
    //  fmgr.queens(color)*Figure::figureWeight_[Figure::TypeQueen];

    int unprotected_multi_score = pop_count(attacks_unprotected_multi) * EvalCoefficients::unprotectedKingMultiattacksCoeff_;
    int unprotected_score = pop_count(attacks_unprotected) * EvalCoefficients::unprotectedKingAttacksCoeff_;
    int all_attacks_score = pop_count(king_attacks) * EvalCoefficients::allKingAttacksCoeff_;
    int attacks_total_score = unprotected_score + unprotected_multi_score + all_attacks_score;

    score_king += attacks_total_score;

    static const int checkers_coefficients[8] = { 0, 32, 48, 64, 64 };
    static const int attackers_coefficients[8] = { 0, 16, 32, 48, 64, 64, 64, 64 };

    int num_attackers = attackers_count[Figure::TypePawn] + attackers_count[Figure::TypeKnight] + attackers_count[Figure::TypeBishop] +
      attackers_count[Figure::TypeRook] + attackers_count[Figure::TypeQueen] + attackers_count[Figure::TypeKing];
    num_attackers = std::min(num_attackers, 7);
    auto attack_coeff = attackers_coefficients[num_attackers];

    int num_checkers = (kn_check != 0ULL) + (bi_check != 0ULL) + (r_check != 0ULL) + (q_check != 0ULL);
    num_checkers = std::min(num_checkers, 4);
    auto check_coeff = checkers_coefficients[num_checkers] + attack_coeff;

    score_king = (score_king * attack_coeff + check_score * check_coeff) >> 5;

    auto king_left = (board_->kingPos(ocolor) & 7) < 4;
    auto attacks_king_side = finfo_[color].attack_mask_ & Figure::quaterBoard_[ocolor][king_left];
    int general_king_attacks_score = pop_count(attacks_king_side) * EvalCoefficients::generalKingPressure_;
    auto attacks_opponent_other = finfo_[color].attack_mask_ & Figure::quaterBoard_[ocolor][!king_left];
    int general_opponent_pressure = pop_count(attacks_opponent_other) * EvalCoefficients::generalOpponentPressure_;
    int general_score = general_king_attacks_score + general_opponent_pressure;

    score_king += general_score;
  }
#endif // EVAL_KING_PR

  FullScore score;
#ifdef EVAL_MOB
  score.common_ = score_mob;
#endif
#ifdef EVAL_KING_PR
  score.opening_ = score_king;
#endif // EVAL_KING_PR

  return score;
}

} // NEngine
