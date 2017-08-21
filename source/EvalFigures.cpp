#include <Evaluator.h>

namespace NEngine
{

Evaluator::FullScore Evaluator::evaluateKnights()
{
  FullScore score_w, score_b;
  BitMask knight_w = board_->fmgr().knight_mask(Figure::ColorWhite);
  for(; knight_w;)
  {
    int n = clear_lsb(knight_w);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
    finfo_[Figure::ColorWhite].multiattack_mask_ |= finfo_[Figure::ColorWhite].attack_mask_ & knight_moves;
    finfo_[Figure::ColorWhite].attack_mask_ |= knight_moves;
    finfo_[Figure::ColorWhite].knightAttacks_ |= knight_moves;

    // king pressure
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(Figure::ColorBlack));
    score_w.common_ += evalCoeffs().kingDistanceBonus_[Figure::TypeKnight][ki_dist];

    switch(n)
    {
    case A8:
      if(board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn))
         score_w.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case A7:
      if(board_->isFigure(A6, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
         score_w.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case B8:
      if(board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
        score_w.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case H8:
      if(board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn))
         score_w.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case H7:
      if(board_->isFigure(H6, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
         score_w.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case G8:
      if(board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
        score_w.opening_ -= evalCoeffs().knightBlocked_;
      break;
    }
  }

  BitMask knight_b = board_->fmgr().knight_mask(Figure::ColorBlack);
  for(; knight_b;)
  {
    int n = clear_lsb(knight_b);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
    finfo_[Figure::ColorBlack].multiattack_mask_ |= finfo_[Figure::ColorBlack].attack_mask_ & knight_moves;
    finfo_[Figure::ColorBlack].attack_mask_ |= knight_moves;
    finfo_[Figure::ColorBlack].knightAttacks_ |= knight_moves;

    // king pressure
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(Figure::ColorWhite));
    score_b.common_ += evalCoeffs().kingDistanceBonus_[Figure::TypeKnight][ki_dist];

    switch(n)
    {
    case A1:
      if(board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn))
         score_b.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case A2:
      if(board_->isFigure(A3, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
         score_b.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case B1:
      if(board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
        score_b.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case H1:
      if(board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn))
         score_b.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case H2:
      if(board_->isFigure(H3, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
         score_b.opening_ -= evalCoeffs().knightBlocked_;
      break;

    case G1:
      if(board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
        score_b.opening_ -= evalCoeffs().knightBlocked_;
      break;
    }
  }
  score_w -= score_b;
  return score_w;
}

Evaluator::FullScore Evaluator::evaluateBishops()
{
  FullScore score_b, score_w;
  BitMask bimask_w = board_->fmgr().bishop_mask(Figure::ColorWhite);
  for(; bimask_w;)
  {
    int n = clear_lsb(bimask_w);

    // mobility
    auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
    finfo_[Figure::ColorWhite].multiattack_mask_ |= finfo_[Figure::ColorWhite].attack_mask_ & bishop_moves;
    finfo_[Figure::ColorWhite].attack_mask_ |= bishop_moves;
    finfo_[Figure::ColorWhite].bishopAttacks_ |= bishop_moves;

    // king pressure
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(Figure::ColorBlack));
    score_w.common_ += evalCoeffs().kingDistanceBonus_[Figure::TypeBishop][ki_dist];

    switch(n)
    {
    case A7:
      if(board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn))
        score_w.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case A6:
      if(board_->isFigure(B5, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(C6, Figure::ColorBlack, Figure::TypePawn))
         score_w.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case A8:
      if(board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
        score_w.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case B8:
      if(board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn) &&
         (board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn)))
         score_w.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case H7:
      if(board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn))
        score_w.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case H8:
      if(board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
        score_w.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case G8:
      if(board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn) &&
         (board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn)))
         score_w.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case H6:
      if(board_->isFigure(G5, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(F6, Figure::ColorBlack, Figure::TypePawn))
         score_w.opening_ -= evalCoeffs().bishopBlocked_;
      break;
    }
  }

  BitMask bimask_b = board_->fmgr().bishop_mask(Figure::ColorBlack);
  for(; bimask_b;)
  {
    int n = clear_lsb(bimask_b);

    // mobility
    auto bishop_moves = magic_ns::bishop_moves(n, mask_all_);
    finfo_[Figure::ColorBlack].multiattack_mask_ |= finfo_[Figure::ColorBlack].attack_mask_ & bishop_moves;
    finfo_[Figure::ColorBlack].attack_mask_ |= bishop_moves;
    finfo_[Figure::ColorBlack].bishopAttacks_ |= bishop_moves;

    // king pressure
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(Figure::ColorWhite));
    score_b.common_ += evalCoeffs().kingDistanceBonus_[Figure::TypeBishop][ki_dist];

    switch(n)
    {
    case A2:
      if(board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn))
        score_b.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case A3:
      if(board_->isFigure(B4, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(C3, Figure::ColorWhite, Figure::TypePawn))
         score_b.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case A1:
      if(board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
        score_b.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case B1:
      if(board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn) &&
         (board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn)))
         score_b.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case H2:
      if(board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn))
        score_b.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case H1:
      if(board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
        score_b.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case G1:
      if(board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn) &&
         (board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn)))
         score_b.opening_ -= evalCoeffs().bishopBlocked_;
      break;

    case H3:
      if(board_->isFigure(G4, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(F3, Figure::ColorWhite, Figure::TypePawn))
         score_b.opening_ -= evalCoeffs().bishopBlocked_;
      break;
    }
  }
  score_w -= score_b;
  return score_w;
}

int Evaluator::evaluateRook(Figure::Color color)
{
  int score{ 0 };
  auto const& fmgr = board_->fmgr();
  auto mask = fmgr.rook_mask(color);
  auto ocolor = Figure::otherColor(color);
  for(; mask;)
  {
    auto n = clear_lsb(mask);
    // mobility
    auto const& rook_moves = magic_ns::rook_moves(n, mask_all_);
    finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & rook_moves;
    finfo_[color].attack_mask_ |= rook_moves;
    finfo_[color].rookAttacks_ |= rook_moves;

    // open column
    auto const& mask_col = pawnMasks().mask_column(n & 7);
    bool no_pw_color = (mask_col & fmgr.pawn_mask(color)) == 0ULL;
    bool no_pw_ocolor = (mask_col & fmgr.pawn_mask(ocolor)) == 0ULL;
    score += evalCoeffs().openRook_[(no_pw_color+no_pw_ocolor) & 3];
    
    // king pressure
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
    score += evalCoeffs().kingDistanceBonus_[Figure::TypeRook][ki_dist];
  }
  return score;
}

int Evaluator::evaluateQueens(Figure::Color color)
{
  int score{ 0 };
  auto const& fmgr = board_->fmgr();
  auto mask = fmgr.queen_mask(color);
  auto ocolor = Figure::otherColor(color);
  for(; mask;)
  {
    auto n = clear_lsb(mask);
    // mobility
    auto queen_moves = magic_ns::queen_moves(n, mask_all_);
    finfo_[color].multiattack_mask_ |= finfo_[color].attack_mask_ & queen_moves;
    finfo_[color].attack_mask_ |= queen_moves;
    finfo_[color].queenAttacks_ |= queen_moves;

    // king pressure
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
    score += evalCoeffs().kingDistanceBonus_[Figure::TypeQueen][ki_dist];
  }
  return score;
}

int Evaluator::evaluateMobilityAndKingPressure(Figure::Color color)
{
  int score_mob = 0;
  int score_king = 0;
  int score_king_pw = 0;
  auto const& fmgr = board_->fmgr();
  auto ocolor = Figure::otherColor(color);

  // basic king pressure
  auto ki_fields = movesTable().caps(Figure::TypeKing, board_->kingPos(ocolor));
  auto ctype = getCastleType(ocolor);
  if(ctype >= 0)
  {
    ki_fields |= king_attack_mask_[ocolor][ctype];
  }
  // attacked by opponents pawns
  auto ki_fields_pw = ki_fields;

  auto exclude_mask = ~finfo_[ocolor].pawnAttacks_;
  ki_fields &= exclude_mask;
  exclude_mask &= ~(ki_fields_pw | fmgr.mask(color));
  ki_fields_pw ^= ki_fields;


  // number of king attackers
  int num_bi = 0;
  int num_r = 0;
  int num_q = 0;

  int num_attackers = (finfo_[color].kingAttacks_ & ki_fields) != 0ULL;

  // number of attackers to fileds controller by opponent pawns
  int num_attackers_pw = 0;

  // pawns pressure
  if(auto pw_att = (finfo_[color].pawnAttacks_ & ki_fields))
  {
    int pw_num = pop_count(pw_att);
    score_king += pw_num * evalCoeffs().pawnKingAttack_;
    num_attackers++;
  }
  if(auto pw_att = (finfo_[color].pawnAttacks_ & ki_fields_pw))
  {
    int pw_num = pop_count(pw_att);
    score_king_pw += pw_num * evalCoeffs().pawnKingAttack_;
    num_attackers_pw++;
  }

  // knights pressure
  if(auto kn_att = (finfo_[color].knightAttacks_ & ki_fields))
  {
    int kn_num = pop_count(kn_att);
    score_king += kn_num * evalCoeffs().knightKingAttack_;
    num_attackers++;
  }
  if(auto kn_att = (finfo_[color].knightAttacks_ & ki_fields_pw))
  {
    int kn_num = pop_count(kn_att);
    score_king_pw += (kn_num * evalCoeffs().knightKingAttack_) >> 1;
    num_attackers_pw++;
  }

  BitMask cango_mask = ~finfo_[ocolor].pawnAttacks_ & ~fmgr.mask(color)
    & (finfo_[color].multiattack_mask_ | ~finfo_[ocolor].attack_mask_)
    & ~finfo_[ocolor].multiattack_mask_;
  BitMask include_mask = fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor)
    | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);
  BitMask knights = fmgr.knight_mask(color);
  for(; knights;)
  {
    int n = clear_lsb(knights);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n) & (cango_mask | include_mask);
    int num_moves = pop_count(knight_moves);
    score_mob += evalCoeffs().knightMobility_[num_moves & 15];
  }
  // knight check
  auto kn_check = movesTable().caps(Figure::TypeKnight, board_->kingPos(ocolor));

  int num_kn_checks = pop_count(kn_check & finfo_[color].knightAttacks_ & (cango_mask | include_mask));
  auto kn_check_mask_pot = kn_check & finfo_[color].knightAttacks_ & (exclude_mask & ~(cango_mask | include_mask));
  int num_kn_checks_pot = pop_count(kn_check_mask_pot);
  //kn_check &= finfo_[color].knightAttacks_ & exclude_mask;// (cango_mask | include_mask);

  auto bi_check = magic_ns::bishop_moves(board_->kingPos(ocolor), mask_all_);
  auto r_check = magic_ns::rook_moves(board_->kingPos(ocolor), mask_all_);
  auto q_check = bi_check | r_check;

  // bishop check
  int num_bi_checks = pop_count(bi_check & finfo_[color].bishopAttacks_ & (cango_mask | include_mask));
  auto bi_check_mask_pot = bi_check & finfo_[color].bishopAttacks_ & (exclude_mask & ~(cango_mask | include_mask));
  int num_bi_checks_pot = pop_count(bi_check_mask_pot);
  //bi_check &= finfo_[color].bishopAttacks_ & (cango_mask | include_mask);

  // x-ray bishop attack
  auto bq_mask = fmgr.queen_mask(color) | fmgr.bishop_mask(color);
  auto all_bq = mask_all_ ^ bq_mask;
  X_ASSERT(all_bq != (mask_all_ & ~bq_mask), "invalid all but BQ mask");

  BitMask bishops = fmgr.bishop_mask(color);
  for(; bishops;)
  {
    int n = clear_lsb(bishops);
    // mobility
    auto bishop_moves = magic_ns::bishop_moves(n, mask_all_) & (cango_mask | include_mask);
    int num_moves = pop_count(bishop_moves);
    score_mob += evalCoeffs().bishopMobility_[num_moves & 15];
    
    // king pressure
    if(!(movesTable().caps(Figure::TypeBishop, n) & ki_fields))
      continue;
    auto const& xray_moves = magic_ns::bishop_moves(n, all_bq);
    if(auto bi_att = (xray_moves & ki_fields))
    {
      int bi_num = pop_count(bi_att);
      score_king += bi_num * evalCoeffs().bishopKingAttack_;
      num_attackers++;
      num_bi++;
    }
    if(auto bi_att = (xray_moves & ki_fields_pw))
    {
      int bi_num = pop_count(bi_att);
      score_king_pw += (bi_num * evalCoeffs().bishopKingAttack_) >> 1;
      num_attackers_pw++;
    }
  }

  // x-ray rook attackes
  auto rq_mask = fmgr.queen_mask(color) | fmgr.rook_mask(color);
  auto all_rq = mask_all_ ^ rq_mask;
  X_ASSERT(all_rq != (mask_all_ & ~rq_mask), "invalid all but RQ mask");

  BitMask rooks = fmgr.rook_mask(color);
  cango_mask &= ~(finfo_[ocolor].knightAttacks_ | finfo_[ocolor].bishopAttacks_);
  include_mask ^= fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor);
  // rook check
  int num_r_checks = pop_count(r_check & finfo_[color].rookAttacks_ & (cango_mask | include_mask));
  auto r_check_mask_pot = r_check & finfo_[color].rookAttacks_ & (exclude_mask &  ~(cango_mask | include_mask));
  int num_r_checks_pot = pop_count(r_check_mask_pot);
  //r_check &= finfo_[color].rookAttacks_ & exclude_mask;// (cango_mask | include_mask);

  for(; rooks;)
  {
    auto n = clear_lsb(rooks);
    // mobility
    auto rook_moves = magic_ns::rook_moves(n, mask_all_) & (cango_mask | include_mask);
    int num_moves = pop_count(cango_mask & rook_moves);
    score_mob += evalCoeffs().rookMobility_[num_moves & 15];

    // king pressure
    if(!(movesTable().caps(Figure::TypeRook, n) & ki_fields))
      continue;
    auto const& xray_moves = magic_ns::rook_moves(n, all_rq);
    if(auto r_att = (xray_moves & ki_fields))
    {
      int r_num = pop_count(r_att);
      score_king += r_num * evalCoeffs().rookKingAttack_;
      num_attackers++;
      num_r++;
    }
    if(auto r_att = (xray_moves & ki_fields_pw))
    {
      int r_num = pop_count(r_att);
      score_king_pw += (r_num * evalCoeffs().rookKingAttack_) >> 1;
      num_attackers_pw++;
    }
  }

  // x-ray queen attacks
  auto brq_mask = bq_mask | rq_mask;
  auto all_brq = mask_all_ ^ brq_mask;
  X_ASSERT(all_brq != (mask_all_ & ~brq_mask), "invalid all but BRQ mask");

  BitMask queens = fmgr.queen_mask(color);
  cango_mask &= ~finfo_[ocolor].rookAttacks_;
  include_mask ^= fmgr.rook_mask(ocolor);

  // queen check
  int num_q_checks = pop_count(q_check & finfo_[color].queenAttacks_ & (cango_mask | include_mask));
  auto q_check_mask_pot = q_check & finfo_[color].queenAttacks_ & (exclude_mask & ~(cango_mask | include_mask));
  int num_q_checks_pot = pop_count(q_check_mask_pot);
  //q_check &= finfo_[color].queenAttacks_ & exclude_mask;// (cango_mask | include_mask);

  for(; queens;)
  {
    auto n = clear_lsb(queens);
    // mobility
    auto queen_moves = magic_ns::queen_moves(n, mask_all_) & (cango_mask | include_mask);
    int num_moves = pop_count(queen_moves);
    score_mob += evalCoeffs().queenMobility_[num_moves & 31];

    // king pressure
    if(!(movesTable().caps(Figure::TypeQueen, n) & ki_fields))
      continue;
    auto const& xray_moves = magic_ns::queen_moves(n, all_brq);
    if(auto q_att = (xray_moves & ki_fields))
    {
      int q_num = pop_count(q_att);
      score_king += q_num * evalCoeffs().queenKingAttack_;
      num_attackers++;
      num_q++;
    }
    if(auto q_att = (xray_moves & ki_fields_pw))
    {
      int q_num = pop_count(q_att);
      score_king_pw += (q_num * evalCoeffs().queenKingAttack_) >> 1;
      num_attackers_pw++;
    }
  }

  // basic attacks
  static const int number_of_attackers[8] = { 0, 0, 32, 48, 64, 64, 64, 64 };
  int num_total = std::min(num_attackers + (num_attackers_pw>>1), 7);
  int coeff = number_of_attackers[num_total];
  if(num_total > 1)
  {
    if(num_q > 0)
      coeff += 24;
    else if(num_bi > 1)
      coeff += 16;
    else if(num_r > 0)
      coeff += 10;
  }
  score_king = ((score_king + (score_king_pw>>1)) * coeff) >> 5;

  //int check_score = pop_count(kn_check) * evalCoeffs().knightChecking_
  //  + pop_count(bi_check) * evalCoeffs().bishopChecking_
  //  + pop_count(r_check) * evalCoeffs().rookChecking_
  //  + pop_count(q_check) * evalCoeffs().queenChecking_;

  int check_score = num_kn_checks * evalCoeffs().knightChecking_
    + num_bi_checks * evalCoeffs().bishopChecking_
    + num_r_checks * evalCoeffs().rookChecking_
    + num_q_checks * evalCoeffs().queenChecking_;

  check_score += (num_kn_checks_pot * evalCoeffs().knightChecking_
    + num_bi_checks_pot * evalCoeffs().bishopChecking_
    + num_r_checks_pot * evalCoeffs().rookChecking_
    + num_q_checks_pot * evalCoeffs().queenChecking_) >> 1;

  //if(num_total > 1)
  //  check_score *= 2;

  auto score = score_mob + score_king + check_score;
  return score;
}

// for tests
Evaluator::FullScore Evaluator::evaluateKpressureBasic() const
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  for(int type = Figure::TypeKnight; type <= Figure::TypeKing; ++type)
  {
    {
      auto mask = fmgr.type_mask((Figure::Type)type, Figure::ColorBlack);
      for(; mask;)
      {
        auto p = clear_lsb(mask);
        auto ki_dist = distanceCounter().getDistance(p, board_->kingPos(Figure::ColorWhite));
        score.common_ -= evalCoeffs().kingDistanceBonus_[type][ki_dist];
      }
    }
    {
      auto mask = fmgr.type_mask((Figure::Type)type, Figure::ColorWhite);
      for(; mask;)
      {
        auto p = clear_lsb(mask);
        auto ki_dist = distanceCounter().getDistance(p, board_->kingPos(Figure::ColorBlack));
        score.common_ += evalCoeffs().kingDistanceBonus_[type][ki_dist];
      }
    }
  }
  return score;
}

Evaluator::FullScore Evaluator::evaluatePsqBruteforce() const
{
  FullScore score{};
  auto const& fmgr = board_->fmgr();
  for(int type = Figure::TypePawn; type <= Figure::TypeKing; ++type)
  {
    {
      auto mask = fmgr.type_mask((Figure::Type)type, Figure::ColorBlack);
      for(; mask;)
      {
        auto p = clear_lsb(mask);

        score.opening_ -= evalCoeffs().positionEvaluations_[0][type][p];
        score.endGame_ -= evalCoeffs().positionEvaluations_[1][type][p];
      }
    }
    {
      auto mask = fmgr.type_mask((Figure::Type)type, Figure::ColorWhite);
      for(; mask;)
      {
        auto n = clear_lsb(mask);
        auto p = Figure::mirrorIndex_[n];

        score.opening_ += evalCoeffs().positionEvaluations_[0][type][p];
        score.endGame_ += evalCoeffs().positionEvaluations_[1][type][p];
      }
    }
  }
  return score;
}

} // NEngine
