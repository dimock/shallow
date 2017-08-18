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

  ki_fields &= ~finfo_[ocolor].pawnAttacks_ | finfo_[color].pawnAttacks_;
  ki_fields_pw ^= ki_fields;

  // number of king attackers
  int num_pawns = 0;
  int num_knights = 0;
  int num_bishops = 0;
  int num_rooks = 0;
  int num_queens = 0;
  bool has_king = (finfo_[color].kingAttacks_ & ki_fields) != 0ULL;

  // pawns pressure
  if(auto pw_att = (finfo_[color].pawnAttacks_ & ki_fields))
  {
    int pw_num = pop_count(pw_att);
    score_king += pw_num * evalCoeffs().pawnKingAttack_;
    num_pawns++;
  }

  // knights pressure
  bool has_kn = false;
  if(auto kn_att = (finfo_[color].knightAttacks_ & ki_fields))
  {
    int kn_num = pop_count(kn_att);
    score_king += kn_num * evalCoeffs().knightKingAttack_;
    has_kn = true;
  }
  if(auto kn_att = (finfo_[color].knightAttacks_ & ki_fields_pw))
  {
    int kn_num = pop_count(kn_att);
    score_king += (kn_num * evalCoeffs().knightKingAttack_) >> 1;
    has_kn = true;
  }
  num_knights += has_kn;

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
  kn_check &= finfo_[color].knightAttacks_ & (cango_mask | include_mask);

  auto bi_check = magic_ns::bishop_moves(board_->kingPos(ocolor), mask_all_);
  auto r_check = magic_ns::rook_moves(board_->kingPos(ocolor), mask_all_);
  auto q_check = bi_check | r_check;

  // bishop check
  bi_check &= finfo_[color].bishopAttacks_ & (cango_mask | include_mask);

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
    bool has_bi = false;
    if(auto bi_att = (xray_moves & ki_fields))
    {
      int bi_num = pop_count(bi_att);
      score_king += bi_num * evalCoeffs().bishopKingAttack_;
      has_bi = true;
    }
    if(auto bi_att = (xray_moves & ki_fields_pw))
    {
      int bi_num = pop_count(bi_att);
      score_king += (bi_num * evalCoeffs().bishopKingAttack_) >> 1;
      has_bi = true;
    }
    num_bishops += has_bi;
  }

  // x-ray rook attackes
  auto rq_mask = fmgr.queen_mask(color) | fmgr.rook_mask(color);
  auto all_rq = mask_all_ ^ rq_mask;
  X_ASSERT(all_rq != (mask_all_ & ~rq_mask), "invalid all but RQ mask");

  BitMask rooks = fmgr.rook_mask(color);
  cango_mask &= ~(finfo_[ocolor].knightAttacks_ | finfo_[ocolor].bishopAttacks_);
  include_mask ^= fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor);
  // rook check
  r_check &= finfo_[color].rookAttacks_ & (cango_mask | include_mask);

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
    bool has_r = false;
    if(auto r_att = (xray_moves & ki_fields))
    {
      int r_num = pop_count(r_att);
      score_king += r_num * evalCoeffs().rookKingAttack_;
      has_r = true;
    }
    if(auto r_att = (xray_moves & ki_fields_pw))
    {
      int r_num = pop_count(r_att);
      score_king += (r_num * evalCoeffs().rookKingAttack_) >> 1;
      has_r = true;
    }
    num_rooks += has_r;;
  }

  // x-ray queen attacks
  auto brq_mask = bq_mask | rq_mask;
  auto all_brq = mask_all_ ^ brq_mask;
  X_ASSERT(all_brq != (mask_all_ & ~brq_mask), "invalid all but BRQ mask");

  BitMask queens = fmgr.queen_mask(color);
  cango_mask &= ~finfo_[ocolor].rookAttacks_;
  include_mask ^= fmgr.rook_mask(ocolor);

  // queen check
  q_check &= finfo_[color].queenAttacks_ & (cango_mask | include_mask);

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
    bool has_q = false;
    if(auto q_att = (xray_moves & ki_fields))
    {
      int q_num = pop_count(q_att);
      score_king += q_num * evalCoeffs().queenKingAttack_;
      has_q = true;
    }
    if(auto q_att = (xray_moves & ki_fields_pw))
    {
      int q_num = pop_count(q_att);
      score_king += (q_num * evalCoeffs().queenKingAttack_) >> 1;
      has_q = true;
    }
    num_queens += has_q;
  }

  // basic attacks
  static const int number_of_attackers[8] = { 0, 0, 32, 48, 64, 64, 64, 64 };
  int num_total = std::min(num_pawns + num_knights + num_bishops + num_rooks + num_queens + has_king, 7);
  int coeff = number_of_attackers[num_total];
  if(coeff > 0)
  {
    if(num_queens > 0)
      coeff += 24;
    else if(num_bishops > 1)
      coeff += 16;
    else if(num_rooks > 0)
      coeff += 10;
  }
  score_king = (score_king * coeff) >> 5;

  int check_score = pop_count(kn_check) * evalCoeffs().knightChecking_
    + pop_count(bi_check) * evalCoeffs().bishopChecking_
    + pop_count(r_check) * evalCoeffs().rookChecking_
    + pop_count(q_check) * evalCoeffs().queenChecking_;

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
