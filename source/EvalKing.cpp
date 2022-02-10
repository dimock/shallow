#include "Evaluator.h"

namespace NEngine
{

inline bool checkQTreat(BitMask q_mask, BitMask ki_mask, BitMask mask_all, BitMask bi_check, BitMask r_check)
{
  mask_all &= ~ki_mask;
  while (q_mask) {
    auto n = clear_lsb(q_mask);
    if (bi_check) {
      auto bi_moves = magic_ns::bishop_moves(n, mask_all);
      if (bi_moves & bi_check) {
        return true;
      }
    }
    if (r_check) {
      auto r_moves = magic_ns::rook_moves(n, mask_all);
      if (r_moves & r_check) {
        return true;
      }
    }
  }
  return false;
}

int Evaluator::getCastleType(Figure::Color color) const
{
  auto const ki_mask = board_->fmgr().king_mask(color);

  // short
  bool cking = (castle_mask_[color][0] & ki_mask) != 0;

  // long
  bool cqueen = (castle_mask_[color][1] & ki_mask) != 0;

  // -1 == no castle
  return (!cking && !cqueen) * (-1) + cqueen;
}

int Evaluator::getCastleType2(Figure::Color color) const
{
  auto const ki_mask = board_->fmgr().king_mask(color);

  // short
  bool cking = (castle_mask2_[color][0] & ki_mask) != 0;

  // long
  bool cqueen = (castle_mask2_[color][1] & ki_mask) != 0;

  // -1 == no castle
  return (!cking && !cqueen) * (-1) + cqueen;
}

template <Figure::Color color>
int evaluateKingSafety2(FiguresManager const& fmgr, BitMask const& pawnAttacks, Index const kingPos);

template <>
int evaluateKingSafety2<Figure::ColorWhite>(FiguresManager const& fmgr, BitMask const& pawnAttacks, Index const kingPos)
{
  int score = 0;
  int oscore = 0;
  int kx = kingPos.x();
  int ky = kingPos.y();
  if (ky == 7) {
    return 0;
  }
  int ky1 = ky + 1;
  int xc = kx;
  if (xc == 0) {
    xc = 1;
  }
  if (xc == 7) {
    xc = 6;
  }
  int x0 = std::max(0, xc - 1);
  int x1 = std::min(7, xc + 1);
  auto pawns_mask = fmgr.pawn_mask(Figure::ColorWhite);
  auto opawns_mask = fmgr.pawn_mask(Figure::ColorBlack);
  for (int x = x0; x <= x1; ++x) {
    int p = Index(x, ky);
    int ap = Index(x, ky1);
    auto fwdmsk = pawnMasks().mask_forward_plus(Figure::ColorWhite, p);
    auto pwmsk = fwdmsk & pawns_mask;
    auto opmsk = fwdmsk & opawns_mask;
    auto afmsk = pawnMasks().mask_forward(Figure::ColorWhite, ap);
    auto atmsk = afmsk & pawnAttacks;
    int py = 0, opy = 0, ay = 7;
    int ay1 = 0, opy1 = 0;
    bool canAttack = false;
    int odist = 7;
    if (pwmsk) {
      py = Index(_lsb64(pwmsk)).y();
    }
    if (opmsk) {
      opy = Index(_lsb64(opmsk)).y();
      opy1 = opy;
    }
    if (atmsk) {
      ay = Index(_msb64(atmsk)).y();
      ay1 = ay - 1;
    }
    if (ay <= opy) {
      int a = Index(x, ay1);
      int o = Index(x, opy1);
      canAttack = (betweenMasks().between(o, a) & pawns_mask) == 0ULL;
      odist = opy - ay;
    }
    score += EvalCoefficients::pawnsShields_[x][py];
    oscore += (EvalCoefficients::opawnsShieldAttack_[canAttack][odist] * EvalCoefficients::opawnsAttackCoeffs_[opy]) >> 5;
    oscore += EvalCoefficients::opawnsNearKing_[opy];
  }
  auto kifwdmsk = set_mask_bit(Index(kx, ky1)) & opawns_mask & ~pawnAttacks;
  if (kifwdmsk) {
    int oy = Index(_lsb64(kifwdmsk)).y();
    score += EvalCoefficients::opawnAboveKing_[oy];
  }
  score -= oscore;
  return score;
}

template <>
int evaluateKingSafety2<Figure::ColorBlack>(FiguresManager const& fmgr, BitMask const& pawnAttacks, Index const kingPos)
{
  int score = 0;
  int oscore = 0;
  int kx = kingPos.x();
  int ky = kingPos.y();
  if (ky == 0) {
    return 0;
  }
  int ky1 = ky - 1;
  int xc = kx;
  if (xc == 0) {
    xc = 1;
  }
  if (xc == 7) {
    xc = 6;
  }
  int x0 = std::max(0, xc - 1);
  int x1 = std::min(7, xc + 1);
  auto pawns_mask = fmgr.pawn_mask(Figure::ColorBlack);
  auto opawns_mask = fmgr.pawn_mask(Figure::ColorWhite);
  for (int x = x0; x <= x1; ++x) {
    int p = Index(x, ky);
    int ap = Index(x, ky1);
    auto fwdmsk = pawnMasks().mask_forward_plus(Figure::ColorBlack, p);
    auto pwmsk = fwdmsk & pawns_mask;
    auto opmsk = fwdmsk & opawns_mask;
    auto afmsk = pawnMasks().mask_forward(Figure::ColorBlack, ap);
    auto atmsk = afmsk & pawnAttacks;
    int py = 0, opy = 0, ay = 7;
    int ay1 = 0, opy1 = 0;
    bool canAttack = false;
    int odist = 7;
    if (pwmsk) {
      py = 7 - Index(_msb64(pwmsk)).y();
    }
    if (opmsk) {
      opy1 = Index(_msb64(opmsk)).y();
      opy = 7 - opy1;
    }
    if (atmsk) {
      ay1 = Index(_lsb64(atmsk)).y();
      ay = 7 - ay1;
      ay1 += 1;
    }
    if (ay <= opy) {
      int a = Index(x, ay1);
      int o = Index(x, opy1);
      canAttack = (betweenMasks().between(o, a) & pawns_mask) == 0ULL;
      odist = opy - ay;
    }
    score += EvalCoefficients::pawnsShields_[x][py];
    oscore += (EvalCoefficients::opawnsShieldAttack_[canAttack][odist] * EvalCoefficients::opawnsAttackCoeffs_[opy]) >> 5;
    oscore += EvalCoefficients::opawnsNearKing_[opy];
  }
  auto kifwdmsk = set_mask_bit(Index(kx, ky1)) & opawns_mask & ~pawnAttacks;
  if (kifwdmsk) {
    int oy = 7 - Index(_msb64(kifwdmsk)).y();
    score += EvalCoefficients::opawnAboveKing_[oy];
  }
  score -= oscore;
  return score;
}

int Evaluator::evaluateKingSafetyW() const
{
  Index kingPos(board_->kingPos(Figure::ColorWhite));
  auto score = evaluateKingSafety2<Figure::ColorWhite>(board_->fmgr(), finfo_[Figure::ColorWhite].pawnAttacks_, kingPos);
  if (board_->castling(Figure::ColorWhite, 0)) {
    Index kingPosK{ 6, promo_y_[Figure::ColorBlack] };
    int scoreK = evaluateKingSafety2<Figure::ColorWhite>(board_->fmgr(), finfo_[Figure::ColorWhite].pawnAttacks_, kingPosK);
    score = std::max(score, scoreK);
  }
  if (board_->castling(Figure::ColorWhite, 1)) {
    Index kingPosQ{ 1, promo_y_[Figure::ColorBlack] };
    int scoreQ = evaluateKingSafety2<Figure::ColorWhite>(board_->fmgr(), finfo_[Figure::ColorWhite].pawnAttacks_, kingPosQ);
    score = std::max(score, scoreQ);
  }
  return score;
}

int Evaluator::evaluateKingSafetyB() const
{
  Index kingPos(board_->kingPos(Figure::ColorBlack));
  auto score = evaluateKingSafety2<Figure::ColorBlack>(board_->fmgr(), finfo_[Figure::ColorBlack].pawnAttacks_, kingPos);
  if (board_->castling(Figure::ColorBlack, 0)) {
    Index kingPosK{ 6, promo_y_[Figure::ColorWhite] };
    int scoreK = evaluateKingSafety2<Figure::ColorBlack>(board_->fmgr(), finfo_[Figure::ColorBlack].pawnAttacks_, kingPosK);
    score = std::max(score, scoreK);
  }
  if (board_->castling(Figure::ColorBlack, 1)) {
    Index kingPosQ{ 1, promo_y_[Figure::ColorWhite] };
    int scoreQ = evaluateKingSafety2<Figure::ColorBlack>(board_->fmgr(), finfo_[Figure::ColorBlack].pawnAttacks_, kingPosQ);
    score = std::max(score, scoreQ);
  }
  return score;
}

int Evaluator::evaluateKingSafety(Figure::Color color) const
{
  Index kingPos(board_->kingPos(color));
  int ky = kingPos.y();
  if ((ky < 2 && color) || (ky > 5 && !color))
    ky = promo_y_[Figure::otherColor(color)];
  int ctype = getCastleType(color);
  int score = 0;
  if (ctype == 0) // king side
  {
    Index kingPos6{ 6, ky };
    score = evaluateKingSafety(color, kingPos6) + evaluateKingsPawn(color, kingPos) - opponentPawnsPressure(color, kingPos6);
  }
  else if (ctype == 1) // queen side
  {
    Index kingPos1{ 1, ky };
    score = evaluateKingSafety(color, kingPos1) + evaluateKingsPawn(color, kingPos) - opponentPawnsPressure(color, kingPos1);
  }
  else
  {
    score = evaluateKingSafety(color, kingPos) + evaluateKingsPawn(color, kingPos) - opponentPawnsPressure(color, kingPos);
    if (board_->castling(color, 0)) {
      Index kingPosK{ 6, promo_y_[Figure::otherColor(color)] };
      int scoreK = evaluateKingSafety(color, kingPosK) + evaluateKingsPawn(color, kingPosK) - opponentPawnsPressure(color, kingPosK);
      score = std::max(score, scoreK);
    }
    if (board_->castling(color, 1)) {
      Index kingPosQ{ 1, promo_y_[Figure::otherColor(color)] };
      int scoreQ = evaluateKingSafety(color, kingPosQ) + evaluateKingsPawn(color, kingPosQ) - opponentPawnsPressure(color, kingPosQ);
      score = std::max(score, scoreQ);
    }
  }
  return score;
}

int Evaluator::evaluateKingSafety(Figure::Color color, Index const kingPos) const
{
  static const int delta_y[2] = { -8, 8 };
  const FiguresManager & fmgr = board_->fmgr();
  auto const pmask = fmgr.pawn_mask(color);
  Figure::Color ocolor = Figure::otherColor(color);
  auto const opmask = fmgr.pawn_mask(ocolor);

  auto king_cy = colored_y_[color][kingPos.y()];
  if (king_cy > 5)
    return 0;

  int above1 = (kingPos + delta_y[color]) & 63;
  int above2 = (above1 + delta_y[color]) & 63;
  BitMask mabove1{ set_mask_bit(above1) };
  BitMask mabove2{ set_mask_bit(above2) };
  BitMask mabove3{};
  if (king_cy < 5)
    mabove3 = set_mask_bit((above2 + delta_y[color]) & 63);

  auto mleft1 = (mabove1 >> 1) & Figure::pawnCutoffMasks_[1];
  auto mleft2 = (mabove2 >> 1) & Figure::pawnCutoffMasks_[1];
  auto mleft3 = (mabove3 >> 1) & Figure::pawnCutoffMasks_[1];

  auto mright1 = (mabove1 << 1) & Figure::pawnCutoffMasks_[0];
  auto mright2 = (mabove2 << 1) & Figure::pawnCutoffMasks_[0];
  auto mright3 = (mabove3 << 1) & Figure::pawnCutoffMasks_[0];

  int score = 0;

  if (mabove1 & pmask)
    score += EvalCoefficients::pawnShieldB_[0];
  else if (mabove2 & pmask)
    score += EvalCoefficients::pawnShieldB_[1];
  else if (mabove3 & pmask)
    score += EvalCoefficients::pawnShieldB_[2];

  if (kingPos.x() > 3) // right side
  {
    if (mleft1 & pmask)
      score += EvalCoefficients::pawnShieldC_[0];
    else if (mleft2 & pmask)
      score += EvalCoefficients::pawnShieldC_[1];
    else if (mleft3 & pmask)
      score += EvalCoefficients::pawnShieldC_[2];

    if (mright1 & pmask)
      score += EvalCoefficients::pawnShieldA_[0];
    else if (mright2 & pmask)
      score += EvalCoefficients::pawnShieldA_[1];
    else if (mright3 & pmask)
      score += EvalCoefficients::pawnShieldA_[2];
  }
  else // left side
  {
    if (mleft1 & pmask)
      score += EvalCoefficients::pawnShieldA_[0];
    else if (mleft2 & pmask)
      score += EvalCoefficients::pawnShieldA_[1];
    else if (mleft3 & pmask)
      score += EvalCoefficients::pawnShieldA_[2];

    if (mright1 & pmask)
      score += EvalCoefficients::pawnShieldC_[0];
    else if (mright2 & pmask)
      score += EvalCoefficients::pawnShieldC_[1];
    else if (mright3 & pmask)
      score += EvalCoefficients::pawnShieldC_[2];
  }
  return score;
}

int Evaluator::opponentPawnsPressure(Figure::Color color, Index const kingPos) const
{
  const FiguresManager & fmgr = board_->fmgr();
  auto const pmask = fmgr.pawn_mask(color);
  Figure::Color ocolor = Figure::otherColor(color);
  auto const opmask = fmgr.pawn_mask(ocolor);

  int xk = kingPos.x();
  int opponent_penalty = 0;
  int x_arr[3] = { -1,-1,-1 };
  {
    int j = 0;
    if (xk > 0)
      x_arr[j++] = xk - 1;
    x_arr[j++] = xk;
    if (xk < 7)
      x_arr[j++] = xk + 1;
  }
  if (color)
  {
    for (auto x : x_arr)
    {
      if (x < 0)
        break;
      if (auto m = (opmask & pawnMasks().mask_column(x)))
      {
        int y = std::max(0, Index(_lsb64(m)).y());
        auto penalty = EvalCoefficients::opponentPawnPressure_[y];
        opponent_penalty += penalty;
      }
    }
  }
  else
  {
    for (auto x : x_arr)
    {
      if (x < 0)
        break;
      if (auto m = (opmask & pawnMasks().mask_column(x)))
      {
        int y = std::max(0, 7 - Index(_msb64(m)).y());
        auto penalty = EvalCoefficients::opponentPawnPressure_[y];
        opponent_penalty += penalty;
      }
    }
  }
  return opponent_penalty;
}

int Evaluator::evaluateKingsPawn(Figure::Color color, Index const kingPos) const
{
  static const int delta_y[2] = { -8, 8 };
  const FiguresManager & fmgr = board_->fmgr();
  auto const pmask = fmgr.pawn_mask(color);
  Figure::Color ocolor = Figure::otherColor(color);
  auto const opmask = fmgr.pawn_mask(ocolor);

  auto king_cy = colored_y_[color][kingPos.y()];
  if (king_cy > 5)
    return 0;

  int above1 = (kingPos + delta_y[color]) & 63;
  int above2 = (above1 + delta_y[color]) & 63;
  BitMask mabove1{ set_mask_bit(above1) };
  BitMask mabove2{ set_mask_bit(above2) };
  BitMask mabove3{};
  if (king_cy < 5)
    mabove3 = set_mask_bit((above2 + delta_y[color]) & 63);

  int score = 0;
  if (mabove1 & (pmask | opmask))
    score += EvalCoefficients::pawnShieldAbove_[0];
  else if (mabove2 & pmask)
    score += EvalCoefficients::pawnShieldAbove_[1];
  else if (mabove3 & pmask)
    score += EvalCoefficients::pawnShieldAbove_[2];
  return score;
}

ScoreType32 Evaluator::evaluateKingPressure(Figure::Color color, int const kscore_o)
{
  const auto& fmgr = board_->fmgr();
  const auto ocolor = Figure::otherColor(color);
  const auto  ki_pos = board_->kingPos(color);
  const auto oki_pos = board_->kingPos(ocolor);
  const auto fmask_no_check = ~(fmgr.mask(color) | finfo_[ocolor].pawnAttacks_);

  const auto oki_fields = finfo_[ocolor].ki_fields_;
  const auto near_oking = oki_fields & finfo_[ocolor].kingAttacks_;
  const auto near_oking_pw = near_oking | (oki_fields & fmgr.pawn_mask(ocolor));
  const auto attacked_any_but_oking = finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[ocolor].kingAttacks_);
  const auto attacked_oking_only = finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].multiattack_mask_;

  const auto onbrp_attacked = finfo_[ocolor].nbr_attacked_ | finfo_[ocolor].pawnAttacks_;
  const auto onbp_attacked = finfo_[ocolor].nb_attacked_ | finfo_[ocolor].pawnAttacks_;
  const auto can_check_q = ~(attacked_any_but_oking | (attacked_oking_only & ~finfo_[color].multiattack_mask_));
  const auto can_check_r = ~(onbrp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_));
  const auto can_check_nb = ~(onbp_attacked | finfo_[ocolor].multiattack_mask_ | (finfo_[ocolor].attack_mask_ & ~finfo_[color].multiattack_mask_));

  finfo_[color].num_attackers_ += (finfo_[color].pawnAttacks_ & near_oking) != 0ULL;
  finfo_[color].num_attackers_ += (finfo_[color].kingAttacks_ & near_oking) != 0ULL;
  finfo_[color].score_king_ += EvalCoefficients::pawnKingAttack_ * (!!(finfo_[color].pawnAttacks_ & near_oking_pw));

  auto kn_check = movesTable().caps(Figure::TypeKnight, oki_pos) & finfo_[color].knightMoves_ & fmask_no_check;
  auto bi_check = finfo_[ocolor].bishopMovesKipos_;
  auto r_check = finfo_[ocolor].rookMovesKipos_;
  auto q_check = (bi_check | r_check) & finfo_[color].queenMoves_ & fmask_no_check;
  bi_check &= finfo_[color].bishopMoves_ & fmask_no_check;
  r_check &= finfo_[color].rookMoves_ & fmask_no_check;

  if (!finfo_[color].discoveredCheck_) {
    X_ASSERT_R(board_->discoveredCheck(ki_pos, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(ki_pos, ocolor), "discovered check not detected");
    X_ASSERT(board_->discoveredCheck(ki_pos, mask_all_, color, board_->kingPos(ocolor)) != discoveredCheck(ki_pos, ocolor), "discovered check not detected");
    finfo_[color].discoveredCheck_ = discoveredCheck(ki_pos, ocolor);
  }

  if (!finfo_[color].discoveredCheck_) {
    auto mask_all_npw = mask_all_ & ~fmgr.pawn_mask(color);
    auto bi_check_npw = magic_ns::bishop_moves(oki_pos, mask_all_npw) & (fmgr.bishop_mask(color) | fmgr.queen_mask(color));
    auto r_check_npw = magic_ns::rook_moves(oki_pos, mask_all_npw) & finfo_[color].rq_mask_ & pawnMasks().mask_row(oki_pos);
    auto check_npw = bi_check_npw | r_check_npw;
    while (check_npw && !finfo_[color].discoveredCheck_) {
      auto n = clear_lsb(check_npw);
      auto const btw_mask = betweenMasks().between(n, oki_pos);
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
      if (finfo_[color].discoveredCheck_) {
        finfo_[color].discoveredMoves_ |= mpawn;
      }
    }
  }

  // promotion check
  auto pfwd = finfo_[color].pawns_fwd_ & Figure::pawnPromoteMasks_[color] & (~finfo_[ocolor].attack_mask_ | finfo_[color].pawnAttacks_);
  auto cfwd = finfo_[color].pawnAttacks_ & Figure::pawnPromoteMasks_[color] & fmgr.mask(ocolor);
  auto p_check = ((pfwd | cfwd) & (finfo_[ocolor].bishopMovesKipos_ | finfo_[ocolor].rookMovesKipos_));

  bool canCheck = !!(kn_check | bi_check | r_check | p_check);
  kn_check &= can_check_nb;
  bi_check &= can_check_nb;
  r_check &= can_check_r;
  q_check &= can_check_q;
  int num_checkers = (!!p_check) + (!!kn_check) + (!!bi_check) + (!!r_check) + (!!q_check) + finfo_[color].discoveredCheck_;

  int check_score = EvalCoefficients::knightChecking_ * (!!kn_check) +
    EvalCoefficients::bishopChecking_ * (!!bi_check) +
    EvalCoefficients::rookChecking_ * (!!r_check) +
    EvalCoefficients::queenChecking_ * (!!q_check) +
    EvalCoefficients::discoveredChecking_ * finfo_[color].discoveredCheck_ +
    EvalCoefficients::promotionChecking_ * (!!p_check);

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
    check_coeff += attack_coeff >> 2;
  }

  auto near_oking_att = finfo_[ocolor].kingAttacks_ & finfo_[color].attack_mask_ & ~fmgr.pawn_mask(color);
  if (near_oking_att) {
    auto strong_msk = near_oking_att & ~attacked_any_but_oking & finfo_[color].multiattack_mask_;
    auto weak_msk = near_oking_att & ~strong_msk & (~finfo_[ocolor].multiattack_mask_ | finfo_[color].multiattack_mask_);
    int strongN = pop_count(strong_msk);
    int weakN = pop_count(weak_msk);
    near_oking_att = strong_msk | weak_msk;
    auto near_king_attacks = EvalCoefficients::attackedNearKingStrong_ * strongN;
    near_king_attacks += EvalCoefficients::attackedNearKingWeak_ * weakN;
    auto near_king_checks = EvalCoefficients::checkNearKingStrong_ * strongN;
    near_king_checks += EvalCoefficients::checkNearKingWeak_ * weakN;
    attack_coeff += near_king_attacks;
    check_coeff += near_king_checks;
  }

  const auto near_oking_rem = ((finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].pawnAttacks_) |
                               (oki_fields & ~finfo_[ocolor].kingAttacks_ & ~finfo_[ocolor].attack_mask_)) &
      (~near_oking_att & ~fmgr.pawn_mask(color) & finfo_[color].attack_mask_);
  if (near_oking_rem) {
    int remsN = pop_count(near_oking_rem & ~finfo_[ocolor].attack_mask_);
    int otherN = pop_count(near_oking_rem & finfo_[ocolor].attack_mask_);
    auto rem_king_attacks = EvalCoefficients::attackedNearKingRem_ * remsN;
    auto rem_king_checks = EvalCoefficients::checkNearKingRem_ * remsN;
    rem_king_attacks += EvalCoefficients::attackedNearKingOther_ * otherN;
    rem_king_checks += EvalCoefficients::checkNearKingOther_ * otherN;
    attack_coeff += rem_king_attacks;
    check_coeff += rem_king_checks;
  }

  auto oking_pw_attacked = (finfo_[color].pawnAttacks_ & near_oking_pw & ~finfo_[ocolor].pawnAttacks_);
  if (oking_pw_attacked) {
    int pawnsN = pop_count(oking_pw_attacked);
    auto pw_king_attacks = EvalCoefficients::attackedNearKingPawns_ * pawnsN;
    auto pw_king_checks = EvalCoefficients::checkNearKingPawns_ * pawnsN;
    attack_coeff += pw_king_attacks;
    check_coeff += pw_king_checks;
  }
  if (num_attackers == 0) {
    check_coeff >>= 3;
  }

  check_coeff -= kscore_o >> 4;
  attack_coeff -= kscore_o >> 4;

  check_coeff = std::max(0, check_coeff);
  attack_coeff = std::max(0, attack_coeff);

  // mat is possible
  if((p_check | q_check | r_check) && isMatTreat(color, ocolor, attacked_any_but_oking, p_check|q_check, r_check)) {
    const bool myMove = (board_->color() == color);
    check_coeff += EvalCoefficients::possibleMatTreat_ * (1 + (int)myMove);
  }

  auto score = finfo_[color].score_king_ * attack_coeff + check_score * check_coeff;
  score >>= 5;

  int oki_x = (oki_pos & 7);
  int king_side = (oki_x > 4) ? 0 : ((oki_x < 3) ? 1 : 2); // 0 = right, 1 = left, 2 = center

  auto general_pressure_mask =
    ((finfo_[color].attack_mask_ & ~finfo_[ocolor].pawnAttacks_) | finfo_[color].multiattack_mask_ | finfo_[color].pawnAttacks_) &
    ~(near_oking_att | near_oking_rem);

  auto attacks_king_side = general_pressure_mask & Figure::quaterBoard_[ocolor][king_side];
  int general_score = pop_count(attacks_king_side) * EvalCoefficients::generalKingPressure_;

  if (bi_check || r_check) {
    bool b = checkQTreat(fmgr.queen_mask(ocolor), fmgr.king_mask(ocolor), mask_all_, bi_check, r_check);
    score += EvalCoefficients::queenCheckTreatBonus_ * b;
  }

  score += general_score;

  return { score, 0 };
}

} // NEngine
