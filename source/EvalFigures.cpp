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
    score_w.common_ += Figure::kingDistanceBonus_[Figure::TypeKnight][ki_dist];

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
    score_b.common_ += Figure::kingDistanceBonus_[Figure::TypeKnight][ki_dist];

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
    score_w.common_ += Figure::kingDistanceBonus_[Figure::TypeBishop][ki_dist];

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
    score_b.common_ += Figure::kingDistanceBonus_[Figure::TypeBishop][ki_dist];

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
    score += Figure::kingDistanceBonus_[Figure::TypeRook][ki_dist];
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
    score += Figure::kingDistanceBonus_[Figure::TypeQueen][ki_dist];
  }
  return score;
}

int Evaluator::evaluateMobility(Figure::Color color)
{
  int score = 0;
  auto const& fmgr = board_->fmgr();
  auto ocolor = Figure::otherColor(color);
  BitMask cango_mask = ~finfo_[ocolor].pawnAttacks_ & ~fmgr.mask(color)
    & (finfo_[color].multiattack_mask_ | ~finfo_[ocolor].attack_mask_)
    & ~finfo_[ocolor].multiattack_mask_;
  BitMask include_mask = fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor) | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);
  BitMask knights = fmgr.knight_mask(color);
  for(; knights;)
  {
    int n = clear_lsb(knights);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n) & (cango_mask | include_mask);
    int num_moves = pop_count(knight_moves);
    score += evalCoeffs().knightMobility_[num_moves & 15];
  }
  BitMask bishops = fmgr.bishop_mask(color);
  for(; bishops;)
  {
    int n = clear_lsb(bishops);
    auto bishop_moves = magic_ns::bishop_moves(n, mask_all_) & (cango_mask | include_mask);
    int num_moves = pop_count(bishop_moves);
    score += evalCoeffs().bishopMobility_[num_moves & 15];
  }
  BitMask rooks = fmgr.rook_mask(color);
  cango_mask &= ~(finfo_[ocolor].knightAttacks_ | finfo_[ocolor].bishopAttacks_);
  include_mask ^= fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor);
  for(; rooks;)
  {
    auto n = clear_lsb(rooks);
    auto rook_moves = magic_ns::rook_moves(n, mask_all_) & (cango_mask | include_mask);
    int num_moves = pop_count(cango_mask & rook_moves);
    score += evalCoeffs().rookMobility_[num_moves & 15];
  }
  BitMask queens = fmgr.queen_mask(color);
  cango_mask &= ~finfo_[ocolor].rookAttacks_;
  include_mask ^= fmgr.rook_mask(ocolor);
  for(; queens;)
  {
    auto n = clear_lsb(queens);
    auto queen_moves = magic_ns::queen_moves(n, mask_all_) & (cango_mask | include_mask);
    int num_moves = pop_count(queen_moves);
    score += evalCoeffs().queenMobility_[num_moves & 31];
  }
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
        score.common_ -= Figure::kingDistanceBonus_[type][ki_dist];
      }
    }
    {
      auto mask = fmgr.type_mask((Figure::Type)type, Figure::ColorWhite);
      for(; mask;)
      {
        auto p = clear_lsb(mask);
        auto ki_dist = distanceCounter().getDistance(p, board_->kingPos(Figure::ColorBlack));
        score.common_ += Figure::kingDistanceBonus_[type][ki_dist];
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

        score.opening_ -= Figure::positionEvaluations_[0][type][p];
        score.endGame_ -= Figure::positionEvaluations_[1][type][p];
      }
    }
    {
      auto mask = fmgr.type_mask((Figure::Type)type, Figure::ColorWhite);
      for(; mask;)
      {
        auto n = clear_lsb(mask);
        auto p = Figure::mirrorIndex_[n];

        score.opening_ += Figure::positionEvaluations_[0][type][p];
        score.endGame_ += Figure::positionEvaluations_[1][type][p];
      }
    }
  }
  return score;
}

} // NEngine
