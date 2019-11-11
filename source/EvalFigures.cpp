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
#ifdef EVAL_KP_BASIC
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(Figure::ColorBlack));
    score_w.common_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];
#endif

#ifdef EVAL_BLOCK_KN
    switch(n)
    {
    case A8:
      if(board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn))
         score_w.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case A7:
      if(board_->isFigure(A6, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
         score_w.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case B8:
      if(board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
        score_w.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case H8:
      if(board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn))
         score_w.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case H7:
      if(board_->isFigure(H6, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
         score_w.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case G8:
      if(board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
        score_w.common_ -= EvalCoefficients::knightBlocked_;
      break;
    }
#endif
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
#ifdef EVAL_KP_BASIC
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(Figure::ColorWhite));
    score_b.common_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeKnight][ki_dist];
#endif

#ifdef EVAL_BLOCK_KN
    switch(n)
    {
    case A1:
      if(board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn))
         score_b.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case A2:
      if(board_->isFigure(A3, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
         score_b.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case B1:
      if(board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
        score_b.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case H1:
      if(board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn))
         score_b.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case H2:
      if(board_->isFigure(H3, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
         score_b.common_ -= EvalCoefficients::knightBlocked_;
      break;

    case G1:
      if(board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
        score_b.opening_ -= EvalCoefficients::knightBlocked_;
      break;
    }
#endif
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
#ifdef EVAL_KP_BASIC
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(Figure::ColorBlack));
    score_w.common_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];
#endif

#ifdef EVAL_BLOCK_BI
    switch(n)
    {
    case A7:
      if(board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn))
        score_w.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case A6:
      if(board_->isFigure(B5, Figure::ColorBlack, Figure::TypePawn) &&
         board_->isFigure(C6, Figure::ColorBlack, Figure::TypePawn))
         score_w.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case A8:
      if(board_->isFigure(B7, Figure::ColorBlack, Figure::TypePawn))
        score_w.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case B8:
      if(board_->isFigure(C7, Figure::ColorBlack, Figure::TypePawn) &&
         (board_->isFigure(B6, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(A7, Figure::ColorBlack, Figure::TypePawn)))
         score_w.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case H7:
      if(board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn))
        score_w.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case H6:
      if (board_->isFigure(G5, Figure::ColorBlack, Figure::TypePawn) &&
        board_->isFigure(F6, Figure::ColorBlack, Figure::TypePawn))
        score_w.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case H8:
      if(board_->isFigure(G7, Figure::ColorBlack, Figure::TypePawn))
        score_w.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case G8:
      if(board_->isFigure(F7, Figure::ColorBlack, Figure::TypePawn) &&
         (board_->isFigure(G6, Figure::ColorBlack, Figure::TypePawn) ||
         board_->isFigure(H7, Figure::ColorBlack, Figure::TypePawn)))
         score_w.common_ -= EvalCoefficients::bishopBlocked_;
      break;
    }
#endif
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
#ifdef EVAL_KP_BASIC
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(Figure::ColorWhite));
    score_b.common_ += EvalCoefficients::kingDistanceBonus_[Figure::TypeBishop][ki_dist];
#endif

#ifdef EVAL_BLOCK_BI
    switch(n)
    {
    case A2:
      if(board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn))
        score_b.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case A3:
      if(board_->isFigure(B4, Figure::ColorWhite, Figure::TypePawn) &&
         board_->isFigure(C3, Figure::ColorWhite, Figure::TypePawn))
         score_b.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case A1:
      if(board_->isFigure(B2, Figure::ColorWhite, Figure::TypePawn))
        score_b.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case B1:
      if(board_->isFigure(C2, Figure::ColorWhite, Figure::TypePawn) &&
         (board_->isFigure(B3, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(A2, Figure::ColorWhite, Figure::TypePawn)))
         score_b.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case H2:
      if(board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn))
        score_b.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case H3:
      if (board_->isFigure(G4, Figure::ColorWhite, Figure::TypePawn) &&
        board_->isFigure(F3, Figure::ColorWhite, Figure::TypePawn))
        score_b.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case H1:
      if(board_->isFigure(G2, Figure::ColorWhite, Figure::TypePawn))
        score_b.common_ -= EvalCoefficients::bishopBlocked_;
      break;

    case G1:
      if(board_->isFigure(F2, Figure::ColorWhite, Figure::TypePawn) &&
         (board_->isFigure(G3, Figure::ColorWhite, Figure::TypePawn) ||
         board_->isFigure(H2, Figure::ColorWhite, Figure::TypePawn)))
         score_b.common_ -= EvalCoefficients::bishopBlocked_;
      break;
    }
#endif
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
#ifdef EVAL_OPEN_R
    auto const& mask_col = pawnMasks().mask_column(n & 7);
    bool no_pw_color = (mask_col & fmgr.pawn_mask(color)) == 0ULL;
    bool no_pw_ocolor = (mask_col & fmgr.pawn_mask(ocolor)) == 0ULL;
    score += EvalCoefficients::openRook_[(no_pw_color+no_pw_ocolor) & 3];
#endif
    
    // king pressure
#ifdef EVAL_KP_BASIC
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
    score += EvalCoefficients::kingDistanceBonus_[Figure::TypeRook][ki_dist];
#endif
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
#ifdef EVAL_KP_BASIC
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
    score += EvalCoefficients::kingDistanceBonus_[Figure::TypeQueen][ki_dist];
#endif
  }
  return score;
}

int Evaluator::evaluateMobilityAndKingPressure(Figure::Color color)
{
#ifdef EVAL_MOB
  int score_mob = 0;
#endif

#ifdef EVAL_KING_PR
  int score_king = 0;
#endif

#ifdef EVAL_PIN
  int pinned_score = 0;
#endif

  auto const& fmgr = board_->fmgr();
  auto ocolor = Figure::otherColor(color);

  // basic king pressure
  auto ki_fields = movesTable().caps(Figure::TypeKing, board_->kingPos(ocolor));
  auto ctype = getCastleType(ocolor);
  if(ctype >= 0)
  {
    ki_fields |= king_attack_mask_[ocolor][ctype];
  }
  ki_fields &= ~finfo_[ocolor].pawnAttacks_;

  // number of king attackers
  int num_pawns = 0;
  int num_knights = 0;
  int num_bishops = 0;
  int num_rooks = 0;
  int num_queens = 0;
  bool has_king = (finfo_[color].kingAttacks_ & ki_fields) != 0ULL;

  // pawns pressure
#ifdef EVAL_KING_PR
  if(auto pw_att = (finfo_[color].pawnAttacks_ & ki_fields))
  {
    int pw_num = pop_count(pw_att);
    score_king += pw_num * EvalCoefficients::pawnKingAttack_;
    num_pawns++;
  }
#endif

  // knights pressure
#ifdef EVAL_KING_PR
  if(auto kn_att = (finfo_[color].knightAttacks_ & ki_fields))
  {
    int kn_num = pop_count(kn_att);
    score_king += kn_num * EvalCoefficients::knightKingAttack_;
    num_knights++;
  }
#endif

  BitMask cango_mask = ~finfo_[ocolor].pawnAttacks_ & ~fmgr.mask(color)
    & (finfo_[color].multiattack_mask_ | ~finfo_[ocolor].attack_mask_)
    & ~finfo_[ocolor].multiattack_mask_;
  BitMask include_mask = fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor)
    | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);

#ifdef EVAL_MOB
  BitMask knights = fmgr.knight_mask(color);
  for(; knights;)
  {
    int n = clear_lsb(knights);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n) & (cango_mask | include_mask);
    int num_moves = pop_count(knight_moves);
    score_mob += EvalCoefficients::knightMobility_[num_moves & 15];
  }
#endif
  // knight check
#ifdef EVAL_KING_CHECK
  auto kn_check = movesTable().caps(Figure::TypeKnight, board_->kingPos(ocolor));
  kn_check &= finfo_[color].knightAttacks_ & (cango_mask | include_mask);
  auto bi_check = magic_ns::bishop_moves(board_->kingPos(ocolor), mask_all_);
#endif

  // pinned figures
#ifdef EVAL_PIN
  if(auto opawns = (fmgr.pawn_mask(ocolor) & bi_check & ~finfo_[ocolor].pawnAttacks_))
  {
    while(opawns)
    {
      int ppos = clear_lsb(opawns);
      if(auto from_pawn = (betweenMasks().from(board_->kingPos(ocolor), ppos)
        & (fmgr.bishop_mask(color)|fmgr.queen_mask(color)))
        & ~finfo_[color].pawnAttacks_)
      {
        while(from_pawn)
        {
          int apos = clear_lsb(from_pawn);
          if(board_->is_nothing_between(ppos, apos, inv_mask_all_))
          {
            pinned_score += EvalCoefficients::pinnedPawn_;
            break;
          }
        }
      }
    }
  }
  if(auto oknights = (fmgr.knight_mask(ocolor) & bi_check))
  {
    while(oknights)
    {
      int npos = clear_lsb(oknights);
      if(auto from_knight = (betweenMasks().from(board_->kingPos(ocolor), npos)
        & (fmgr.bishop_mask(color)|fmgr.queen_mask(color))))
      {
        while(from_knight)
        {
          int apos = clear_lsb(from_knight);
          if(board_->is_nothing_between(npos, apos, inv_mask_all_))
          {
            if(set_mask_bit(npos) & finfo_[ocolor].attack_mask_)
              pinned_score += EvalCoefficients::pinnedKnight_ >> 1;
            else
              pinned_score += EvalCoefficients::pinnedKnight_;
            break;
          }
        }
      }
    }
  }
  if(auto orooks = (fmgr.rook_mask(ocolor) & bi_check))
  {
    while(orooks)
    {
      int rpos = clear_lsb(orooks);
      if(auto from_rook = (betweenMasks().from(board_->kingPos(ocolor), rpos)
        & (fmgr.bishop_mask(color)|fmgr.queen_mask(color))))
      {
        while(from_rook)
        {
          int apos = clear_lsb(from_rook);
          if(board_->is_nothing_between(rpos, apos, inv_mask_all_))
          {
            if(board_->getField(apos).type() == Figure::TypeBishop
               || ((set_mask_bit(rpos) & finfo_[ocolor].attack_mask_) == 0ULL))
              pinned_score += EvalCoefficients::pinnedRook_;
            else
              pinned_score += EvalCoefficients::pinnedRook_ >> 1;
            break;
          }
        }
      }
    }
  }
  if(auto oqueens = (fmgr.queen_mask(ocolor) & bi_check))
  {
    while(oqueens)
    {
      int qpos = clear_lsb(oqueens);
      if(auto from_queen = (betweenMasks().from(board_->kingPos(ocolor), qpos)
        & (fmgr.bishop_mask(color)) & finfo_[color].attack_mask_))
      {
        while(from_queen)
        {
          int apos = clear_lsb(from_queen);
          if(board_->is_nothing_between(qpos, apos, inv_mask_all_))
          {
            pinned_score += EvalCoefficients::pinnedQueen_;
            break;
          }
        }
      }
    }
  }
#endif

  auto r_check = magic_ns::rook_moves(board_->kingPos(ocolor), mask_all_);
  
  // pinned figures
#ifdef EVAL_PIN
  if(auto opawns = (fmgr.pawn_mask(ocolor) & r_check & ~finfo_[ocolor].pawnAttacks_))
  {
    while(opawns)
    {
      int ppos = clear_lsb(opawns);
      if(auto from_pawn = (betweenMasks().from(board_->kingPos(ocolor), ppos)
        & (fmgr.rook_mask(color)|fmgr.queen_mask(color))))
      {
        while(from_pawn)
        {
          int apos = clear_lsb(from_pawn);
          if(board_->is_nothing_between(ppos, apos, inv_mask_all_))
          {
            pinned_score += EvalCoefficients::pinnedPawn_;
            break;
          }
        }
      }
    }
  }
  if(auto oknights = (fmgr.knight_mask(ocolor) & r_check))
  {
    while(oknights)
    {
      int npos = clear_lsb(oknights);
      if(auto from_knight = (betweenMasks().from(board_->kingPos(ocolor), npos)
        & (fmgr.rook_mask(color)|fmgr.queen_mask(color))))
      {
        while(from_knight)
        {
          int apos = clear_lsb(from_knight);
          if(board_->is_nothing_between(npos, apos, inv_mask_all_))
          {
            if(set_mask_bit(npos) & finfo_[ocolor].attack_mask_)
              pinned_score += EvalCoefficients::pinnedKnight_ >> 1;
            else
              pinned_score += EvalCoefficients::pinnedKnight_;
            break;
          }
        }
      }
    }
  }
  if(auto obishops = (fmgr.bishop_mask(ocolor) & r_check))
  {
    while(obishops)
    {
      int bpos = clear_lsb(obishops);
      if(auto from_bishop = (betweenMasks().from(board_->kingPos(ocolor), bpos)
        & (fmgr.rook_mask(color)|fmgr.queen_mask(color))))
      {
        while(from_bishop)
        {
          int apos = clear_lsb(from_bishop);
          if(board_->is_nothing_between(bpos, apos, inv_mask_all_))
          {
            if(set_mask_bit(bpos) & finfo_[ocolor].attack_mask_)
              pinned_score += EvalCoefficients::pinnedBishop_ >> 1;
            else
              pinned_score += EvalCoefficients::pinnedBishop_;
            break;
          }
        }
      }
    }
  }
  if(auto oqueen = (fmgr.queen_mask(ocolor) & r_check))
  {
    while(oqueen)
    {
      int qpos = clear_lsb(oqueen);
      if(auto from_queen = (betweenMasks().from(board_->kingPos(ocolor), qpos)
        & (fmgr.rook_mask(color)) & finfo_[color].attack_mask_))
      {
        while(from_queen)
        {
          int apos = clear_lsb(from_queen);
          if(board_->is_nothing_between(qpos, apos, inv_mask_all_))
          {
            pinned_score += EvalCoefficients::pinnedQueen_;
            break;
          }
        }
      }
    }
  }
#endif

#ifdef EVAL_KING_CHECK
  auto q_check = bi_check | r_check;
  // bishop check
  bi_check &= finfo_[color].bishopAttacks_ & (cango_mask | include_mask);
#endif

  // x-ray bishop attack
  auto bq_mask = fmgr.queen_mask(color) | fmgr.bishop_mask(color);
  auto all_bq = mask_all_ ^ bq_mask;
  X_ASSERT(all_bq != (mask_all_ & ~bq_mask), "invalid all but BQ mask");

  BitMask bishops = fmgr.bishop_mask(color);
  for(; bishops;)
  {
    int n = clear_lsb(bishops);
    // mobility
#ifdef EVAL_MOB
    auto bishop_moves = magic_ns::bishop_moves(n, mask_all_) & (cango_mask | include_mask);
    int num_moves = pop_count(bishop_moves);
    score_mob += EvalCoefficients::bishopMobility_[num_moves & 15];
#endif
    // king pressure
#ifdef EVAL_KING_PR
    if(!(movesTable().caps(Figure::TypeBishop, n) & ki_fields))
      continue;
    auto const& xray_moves = magic_ns::bishop_moves(n, all_bq);
    if(auto bi_att = (xray_moves & ki_fields))
    {
      int bi_num = pop_count(bi_att);
      score_king += bi_num * EvalCoefficients::bishopKingAttack_;
      num_bishops++;
    }
#endif
  }

  // x-ray rook attackes
  auto rq_mask = fmgr.queen_mask(color) | fmgr.rook_mask(color);
  auto all_rq = mask_all_ ^ rq_mask;
  X_ASSERT(all_rq != (mask_all_ & ~rq_mask), "invalid all but RQ mask");

  BitMask rooks = fmgr.rook_mask(color);
  cango_mask &= ~(finfo_[ocolor].knightAttacks_ | finfo_[ocolor].bishopAttacks_);
  include_mask ^= fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor);

  // rook check
#ifdef EVAL_KING_CHECK
  r_check &= finfo_[color].rookAttacks_ & (cango_mask | include_mask);
#endif

  for(; rooks;)
  {
    auto n = clear_lsb(rooks);
    int r_num = 0;
    auto rook_moves = magic_ns::rook_moves(n, mask_all_);
#ifdef EVAL_KING_PR
    if(auto r_att = (rook_moves & ki_fields))
    {
      r_num = pop_count(r_att);
      score_king += r_num * EvalCoefficients::rookKingAttack_;
      num_rooks++;
    }
#endif
    rook_moves &= cango_mask | include_mask;
    // mobility
#ifdef EVAL_MOB
    int num_moves = pop_count(cango_mask & rook_moves);
    score_mob += EvalCoefficients::rookMobility_[num_moves & 15];
#endif

    // king pressure
#ifdef EVAL_KING_PR
    if(r_num || !(movesTable().caps(Figure::TypeRook, n) & ki_fields))
      continue;
    auto const& xray_moves = magic_ns::rook_moves(n, all_rq);
    if(auto r_att = (xray_moves & ki_fields))
    {
      r_num = pop_count(r_att);
      score_king += r_num * EvalCoefficients::rookKingAttackXray_;
      num_rooks++;
    }
#endif
  }

  // x-ray queen attacks
  auto brq_mask = bq_mask | rq_mask;
  auto all_brq = mask_all_ ^ brq_mask;
  X_ASSERT(all_brq != (mask_all_ & ~brq_mask), "invalid all but BRQ mask");

  BitMask queens = fmgr.queen_mask(color);
  cango_mask &= ~finfo_[ocolor].rookAttacks_;
  include_mask ^= fmgr.rook_mask(ocolor);

  // queen check
#ifdef EVAL_KING_CHECK
  q_check &= finfo_[color].queenAttacks_ & (cango_mask | include_mask);
#endif

  for(; queens;)
  {
    auto n = clear_lsb(queens);
    // mobility
    int q_num = 0;
    auto queen_moves = magic_ns::queen_moves(n, mask_all_);
#ifdef EVAL_KING_PR
    if(auto q_att = (queen_moves & ki_fields))
    {
      q_num = pop_count(q_att);
      score_king += q_num * EvalCoefficients::queenKingAttack_;
      num_queens++;
    }
#endif
    queen_moves &= cango_mask | include_mask;
#ifdef EVAL_MOB
    int num_moves = pop_count(queen_moves);
    score_mob += EvalCoefficients::queenMobility_[num_moves & 31];
#endif

    // king pressure
#ifdef EVAL_KING_PR
    if(q_num || !(movesTable().caps(Figure::TypeQueen, n) & ki_fields))
      continue;
    auto const& xray_moves = magic_ns::queen_moves(n, all_brq);
    if(auto q_att = (xray_moves & ki_fields))
    {
      q_num = pop_count(q_att);
      score_king += q_num * EvalCoefficients::queenKingAttackXray_;
      num_queens++;
    }
#endif
  }



  // basic attacks
#ifdef EVAL_KING_PR

#ifdef EVAL_KING_CHECK
  int check_score = pop_count(kn_check) * EvalCoefficients::knightChecking_
    + pop_count(bi_check) * EvalCoefficients::bishopChecking_
    + pop_count(r_check) * EvalCoefficients::rookChecking_
    + pop_count(q_check) * EvalCoefficients::queenChecking_;

  if(!num_knights && kn_check)
    num_knights = 1;
  if(!num_bishops && bi_check)
    num_bishops = 1;
  if(!num_rooks && r_check)
    num_rooks = 1;
  if(!num_queens && q_check)
    num_queens = 1;
#endif

  static const int number_of_attackers[8] = { 0, 0, 32, 48, 64, 64, 64, 64 };
  int num_total = std::min(num_pawns + num_knights + num_bishops + num_rooks + num_queens + has_king, 7);
  int coeff = number_of_attackers[num_total];
  //if(coeff > 0)
  //{
  //  if(num_bishops > 1)
  //    coeff += 16;
  //  if(num_rooks > 0)
  //    coeff += 10;
  //  if(num_queens > 0)
  //    coeff += 24;
  //}
  score_king = (score_king * coeff) >> 5;

#ifdef EVAL_KING_CHECK
  score_king += check_score;
#endif

#endif //EVAL_KING_PR

  int score = 0
#ifdef EVAL_MOB
    + score_mob
#endif
#ifdef EVAL_KING_PR
    + score_king
#endif
    ;

#ifdef EVAL_PIN
  score += pinned_score;
#endif

  return score;
}

} // NEngine
