#include <Evaluator.h>

namespace NEngine
{

Evaluator::FullScore Evaluator::evaluateKnights()
{
  FullScore score_w, score_b;
  BitMask knight_w = board_->fmgr().knight_mask(Figure::ColorWhite);
  finfo_[Figure::ColorWhite].cango_mask_ = ~finfo_[Figure::ColorBlack].pawnAttacks_ & inv_mask_all_;
  for(; knight_w;)
  {
    int n = clear_lsb(knight_w);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
    finfo_[Figure::ColorWhite].attack_mask_ |= knight_moves;
    finfo_[Figure::ColorWhite].knightAttacks_ |= knight_moves;

    // mobility
    int num_moves = pop_count(finfo_[Figure::ColorWhite].cango_mask_ & knight_moves);
    score_w.common_ += evalCoeffs().knightMobility_[num_moves & 15];

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
  finfo_[Figure::ColorBlack].cango_mask_ = ~finfo_[Figure::ColorWhite].pawnAttacks_ & inv_mask_all_;
  for(; knight_b;)
  {
    int n = clear_lsb(knight_b);
    auto knight_moves = movesTable().caps(Figure::TypeKnight, n);
    finfo_[Figure::ColorBlack].attack_mask_ |= knight_moves;
    finfo_[Figure::ColorBlack].knightAttacks_ |= knight_moves;

    // mobility
    int num_moves = pop_count(finfo_[Figure::ColorBlack].cango_mask_ & knight_moves);
    score_b.common_ += evalCoeffs().knightMobility_[num_moves & 15];

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
    finfo_[Figure::ColorWhite].attack_mask_ |= bishop_moves;
    finfo_[Figure::ColorWhite].bishopAttacks_ |= bishop_moves;
    int num_moves = pop_count(finfo_[Figure::ColorWhite].cango_mask_ & bishop_moves);
    score_w.common_ += evalCoeffs().bishopMobility_[num_moves & 15];

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
    finfo_[Figure::ColorBlack].attack_mask_ |= bishop_moves;
    finfo_[Figure::ColorBlack].bishopAttacks_ |= bishop_moves;
    int num_moves = pop_count(finfo_[Figure::ColorBlack].cango_mask_ & bishop_moves);
    score_b.common_ += evalCoeffs().bishopMobility_[num_moves & 15];

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
  finfo_[color].cango_mask_ &= ~(finfo_[ocolor].knightAttacks_ | finfo_[ocolor].bishopAttacks_);
  for(; mask;)
  {
    auto n = clear_lsb(mask);
    // mobility
    auto const& rook_moves = magic_ns::rook_moves(n, mask_all_);
    finfo_[color].attack_mask_ |= rook_moves;
    finfo_[color].rookAttacks_ |= rook_moves;
    int num_moves = pop_count(finfo_[color].cango_mask_ & rook_moves);
    score += evalCoeffs().rookMobility_[num_moves & 15];

    // open column
    auto const& mask_col = pawnMasks().mask_column(n & 7);
    bool no_pw_color = (mask_col & fmgr.pawn_mask(color)) == 0ULL;
    bool no_pw_ocolor = (mask_col & fmgr.pawn_mask(ocolor)) == 0ULL;
    if(no_pw_color && no_pw_ocolor)
      score += evalCoeffs().openRook_;
    else if(no_pw_color || no_pw_ocolor)
      score += evalCoeffs().semiopenRook_;
    
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
  finfo_[color].cango_mask_ &= ~finfo_[ocolor].rookAttacks_;
  for(; mask;)
  {
    auto n = clear_lsb(mask);
    // mobility
    auto queen_moves = magic_ns::queen_moves(n, mask_all_);
    finfo_[color].attack_mask_ |= queen_moves;
    finfo_[color].queenAttacks_ |= queen_moves;
    int num_moves = pop_count(finfo_[color].cango_mask_ & queen_moves);
    score += evalCoeffs().queenMobility_[num_moves & 31];

    // king pressure
    auto ki_dist = distanceCounter().getDistance(n, board_->kingPos(ocolor));
    score += Figure::kingDistanceBonus_[Figure::TypeQueen][ki_dist];
  }
  return score;
}

} // NEngine
