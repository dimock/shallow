#include <SpecialCases.h>
#include <Board.h>
#include <xindex.h>
#include <kpk.h>

namespace NEngine
{
namespace
{
  struct Triplet
  {
    Figure::Type  type_;
    Figure::Color color_;
    char          count_;
  };

  inline int calc_shift(Figure::Type t, Figure::Color c)
  {
    return (c << 5) + ((t - 1) << 2);
  }
  
  SpecialCasesDetector::Scase format(std::initializer_list<Triplet>&& figures)
  {
    SpecialCasesDetector::Scase sc{};
    for(auto const& f : figures)
    {
      sc |= ((uint64)f.count_) << calc_shift(f.type_, f.color_);
    }
    return sc;
  }

  const int pawn_colored_y_[2][8] = {
    { 7, 6, 5, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7 } };

  const int delta_y_[2] = { -8, 8 };

  ScoreType evalKings1Pawn(Board const& board, Figure::Color pawnColor)
  {
    Figure::Color ocolor = Figure::otherColor(pawnColor);
    Index p(_lsb64(board.fmgr().pawn_mask(pawnColor)));
    int kw = board.kingPos(pawnColor);
    int kl = board.kingPos(ocolor);
    Index pp(p.x(), pawnColor*7);
    int dist_w = distanceCounter().getDistance(kw, pp);
    int dist_l = distanceCounter().getDistance(kl, pp);
    int y = pawn_colored_y_[pawnColor][p.y()];
    ScoreType score = (7-dist_w + dist_l) * EvalCoefficients::kingToPawnDistanceMulti_ + EvalCoefficients::passerPawnSc_[y];
    auto kn_mask = board.fmgr().knight_mask(ocolor);
    for(; kn_mask;)
    {
      int n = clear_lsb(kn_mask);
      score += distanceCounter().getDistance(n, pp) * EvalCoefficients::knightToPawnDistanceMulti_;
    }
    if(!pawnColor)
      score = -score;
    return score;
  }

  bool kpkPassed(Board const& board, Figure::Color pawnColor, int pawnPos)
  {
    auto loserColor = Figure::otherColor(pawnColor);
    auto moveColor = board.color();
    int kw = board.kingPos(pawnColor);
    int kl = board.kingPos(loserColor);
    if (pawnColor == Figure::ColorBlack)
    {
      pawnPos = Figure::mirrorIndex_[pawnPos];
      kw = Figure::mirrorIndex_[kw];
      kl = Figure::mirrorIndex_[kl];
      moveColor = Figure::otherColor(moveColor);
    }
    return (kpk_[kw][kl][moveColor] & (1ULL << pawnPos)) != 0ULL;
  }

  bool kpkPassed(Board const& board, Figure::Color pawnColor)
  {
    int p = _lsb64(board.fmgr().pawn_mask(pawnColor));
    return kpkPassed(board, pawnColor, p);
  }

  //SBoard<8> sboard;
  //sboard.initEmpty(moveColor);
  //sboard.addFigure(xpawnColor, Figure::TypePawn, p);
  //sboard.addFigure(xpawnColor, Figure::TypeKing, kw);
  //sboard.addFigure(Figure::otherColor(xpawnColor), Figure::TypeKing, kl);
  //sboard.invalidate();
  //std::string sfen = toFEN(sboard);

  ScoreType pawnOnly(Board const& board, Figure::Color pawnColor)
  {
    ScoreType score{ +10 };
    if(kpkPassed(board, pawnColor))
    {
      Index index(_lsb64(board.fmgr().pawn_mask(pawnColor)));
      auto xpawnColor = (pawnColor == Figure::ColorWhite) ? pawnColor : Figure::otherColor(pawnColor);
      score = 2*Figure::figureWeight_[Figure::TypePawn] + pawn_colored_y_[xpawnColor][index.y()] * 20;
    }
    if(pawnColor == Figure::ColorBlack)
      score = -score;
    return score;
  }
  
  bool couldIntercept(Board const& board, Figure::Color pawnColor, Index pawnPos)
  {
    auto const& fmgr = board.fmgr();
    int promo_pos = pawnPos.x() | (pawnColor*56);
    int stepsMax = std::abs(pawnColor*7 - pawnPos.y());
    auto ocolor = Figure::otherColor(pawnColor);
    BitMask mask_all = fmgr.king_mask(ocolor) | fmgr.king_mask(pawnColor);
    BitMask inv_mask_all = ~mask_all;
    BitMask attack_mask = movesTable().caps(Figure::TypeKing, board.kingPos(pawnColor));
    auto rmask = fmgr.rook_mask(pawnColor);
    while (rmask) {
      int n = clear_lsb(rmask);
      auto rook_moves = magic_ns::rook_moves(n, mask_all | fmgr.pawn_mask(pawnColor));
      attack_mask |= rook_moves;
    }
    auto qmask = fmgr.queen_mask(pawnColor);
    while (qmask) {
      int n = clear_lsb(qmask);
      auto queen_moves = magic_ns::queen_moves(n, mask_all | fmgr.pawn_mask(pawnColor));
      attack_mask |= queen_moves;
    }
    return NEngine::couldIntercept(board, inv_mask_all, attack_mask, pawnColor, pawnPos, promo_pos, stepsMax);
  }

  ScoreType pawnAndHeavy(Board const& board, Figure::Color pawnColor)
  {
    ScoreType score{ 0 };
    if (board.fmgr().rooks(pawnColor))
      score += Figure::figureWeight_[Figure::TypePawn];
    else if (board.fmgr().queens(pawnColor))
      score += (Figure::figureWeight_[Figure::TypePawn]*3)/2;
    auto ocolor = Figure::otherColor(pawnColor);
    Index index(_lsb64(board.fmgr().pawn_mask(pawnColor)));
    Index kingP(board.kingPos(pawnColor));
    Index kingO(board.kingPos(ocolor));
    int pcy = pawn_colored_y_[pawnColor][index.y()];
    score += EvalCoefficients::passerPawn_[pcy];
    int pp = index.x() | (pawnColor * 56);
    int o_dist_promo = distanceCounter().getDistance(kingO, pp) - (board.color() == ocolor);
    int dist_promo = distanceCounter().getDistance(kingP, pp);
    score += (o_dist_promo - dist_promo) * 3;
    if (std::abs(kingO.x() - index.x()) < 2 && pawn_colored_y_[pawnColor][kingO.y()] >= pcy) {
      score /= 4;
    }
    else if (couldIntercept(board, pawnColor, index)) {
      score = (score * 3) / 4;
    }
    else {
      score += EvalCoefficients::passerPawn_[pcy] / 2;
    }
    if (index.x() == 0 || index.x() == 7) {
      score = (score * 3) / 4;
    }
    if(pawnColor == Figure::ColorBlack)
      score = -score;
    return score;
  }

} // namespace {}

SpecialCasesDetector::SpecialCasesDetector()
{
  initUsual();
  initWinnerLoser();
  initMatCases();
}


void SpecialCasesDetector::initMatCases()
{
  auto eval_kings = [](Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    auto kw = board.kingPos(winnerColor);
    auto kl = board.kingPos(loserColor);
    auto score = (7 - distanceCounter().getDistance(kw, kl)) * EvalCoefficients::kingToKingDistanceMulti_;
    score -= EvalCoefficients::positionEvaluations_[1][Figure::TypeKing][kl];
    score += board.fmgr().weight(winnerColor) + 20;
    if(!winnerColor)
      score = -score;
    return score;
  };

  matCases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorWhite);
  };

  matCases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeQueen, Figure::ColorWhite, 1 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorWhite);
  };

  matCases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 2 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorWhite);
  };

  matCases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorWhite);
  };

  matCases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] =
    [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorWhite);
  };

  matCases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
  { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
  { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorWhite);
  };

  matCases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
  { Figure::TypeBishop, Figure::ColorBlack, 2 } })] = [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
  { Figure::TypeBishop, Figure::ColorWhite, 2 } })] = [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorWhite);
  };

  matCases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
  { Figure::TypeBishop, Figure::ColorBlack, 2 } })] = [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
  { Figure::TypeBishop, Figure::ColorWhite, 2 } })] = [eval_kings](Board const& board)
  {
    return eval_kings(board, Figure::ColorWhite);
  };

  auto bishopKnightMat = [](Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    int kw = board.kingPos(winnerColor);
    int kl = board.kingPos(loserColor);
    int bp = _lsb64(board.fmgr().bishop_mask(winnerColor));
    int kn = _lsb64(board.fmgr().knight_mask(winnerColor));
    int dist = distanceCounter().getDistance(kw, kl);
    ScoreType score = board.fmgr().weight(winnerColor) - dist;
    int kp = kl;
    if(FiguresCounter::s_whiteColors_[bp])
    {
      kp = Figure::mirrorIndex_[kp];
    }
    score += EvalCoefficients::bishopKnightMat_[kp];
    int ndist = distanceCounter().getDistance(kn, kl);
    int bdist = distanceCounter().getDistance(bp, kl);
    score -= ndist;
    score -= bdist >> 1;
    return score;
  };
  matCases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
  { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [bishopKnightMat](Board const& board)
  {
    return -bishopKnightMat(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
  { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [bishopKnightMat](Board const& board)
  {
    return +bishopKnightMat(board, Figure::ColorWhite);
  };

  auto bishop_and_pawn = [](Board const& board, Figure::Color winnerColor) -> ScoreType
  {
    auto score = evalKings1Pawn(board, winnerColor);
    Figure::Color ocolor = Figure::otherColor(winnerColor);
    Index p(_lsb64(board.fmgr().pawn_mask(winnerColor)));
    Index pp(p.x(), winnerColor*7);
    auto b = _lsb64(board.fmgr().bishop_mask(winnerColor));
    if(FiguresCounter::s_whiteColors_[b] != FiguresCounter::s_whiteColors_[pp]
       && (p.x() == 0 || p.x() == 7))
    {
      int kw = board.kingPos(winnerColor);
      int kl = board.kingPos(ocolor);
      int pr_dist = distanceCounter().getDistance(p, pp);
      int l_pr_dist = distanceCounter().getDistance(kl, pp);
      int w_pr_dist = distanceCounter().getDistance(kw, pp);
      if((pr_dist > l_pr_dist && l_pr_dist <= w_pr_dist) || (l_pr_dist < 2 && pr_dist > 0))
        return score;
    }
    auto fscore = Figure::figureWeight_[Figure::TypePawn] + Figure::figureWeight_[Figure::TypeBishop];
    return score + (winnerColor ? fscore : -fscore);
  };

  // bishop + pawn
  matCases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
  { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [bishop_and_pawn](Board const& board)
  {
    return bishop_and_pawn(board, Figure::ColorWhite);
  };
  matCases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
  { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [bishop_and_pawn](Board const& board)
  {
    return bishop_and_pawn(board, Figure::ColorBlack);
  };

  // only 1 pawn
  matCases_[format({ { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return pawnOnly(board, Figure::ColorBlack);
  };
  matCases_[format({ { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return pawnOnly(board, Figure::ColorWhite);
  };
}

void SpecialCasesDetector::initUsual()
{
  auto eval_king = [](Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    auto kw = board.kingPos(winnerColor);
    auto kl = board.kingPos(loserColor);
    auto score = (7 - distanceCounter().getDistance(kw, kl)) * EvalCoefficients::kingToKingDistanceMulti_;
    score -= EvalCoefficients::positionEvaluations_[1][Figure::TypeKing][kl];
    if(auto n_mask = board.fmgr().knight_mask(winnerColor))
    {
      auto np = _lsb64(n_mask);
      score += (7 - distanceCounter().getDistance(kl, np)) * EvalCoefficients::figureToKingDistanceMulti_;
    }
    if(auto b_mask = board.fmgr().bishop_mask(winnerColor))
    {
      auto bp = _lsb64(b_mask);
      score += (7 - distanceCounter().getDistance(kl, bp)) * EvalCoefficients::figureToKingDistanceMulti_;
    }
    if(auto n_mask = board.fmgr().knight_mask(loserColor))
    {
      auto np = _lsb64(n_mask);
      score += distanceCounter().getDistance(kl, np) * EvalCoefficients::figureToKingDistanceMulti_;
    }
    if(auto b_mask = board.fmgr().bishop_mask(loserColor))
    {
      auto bp = _lsb64(b_mask);
      score += distanceCounter().getDistance(kl, bp) * EvalCoefficients::figureToKingDistanceMulti_;
    }
    if(!winnerColor)
      score = -score;
    return score;
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return -15 + eval_king(board, Figure::ColorBlack);
  };
  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return -15 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return +15 + eval_king(board, Figure::ColorWhite);
  };
  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return +15 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return +20 + eval_king(board, Figure::ColorWhite);
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return +20 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return -20 + eval_king(board, Figure::ColorBlack);
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return -20 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 2 } })] = [eval_king](Board const& board)
  {
    return -15 + eval_king(board, Figure::ColorBlack);
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 2 } })] = [eval_king](Board const& board)
  {
    return +40 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
  { Figure::TypeKnight, Figure::ColorBlack, 2 } })] = [eval_king](Board const& board)
  {
    return +15 + eval_king(board, Figure::ColorWhite);
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
  { Figure::TypeBishop, Figure::ColorBlack, 2 } })] = [eval_king](Board const& board)
  {
    return -40 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return +25 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return -25 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return -40 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return -50 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return +40 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return +50 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return -35 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 2 },
  { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return +35 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
  { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return -15 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return +15 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [eval_king](Board const& board)
  {
    return -15 + eval_king(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [eval_king](Board const& board)
  {
    return +15 + eval_king(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return 50 + evalKings1Pawn(board, Figure::ColorBlack);
  }; 

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return -50 + evalKings1Pawn(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return 50 + evalKings1Pawn(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return -50 + evalKings1Pawn(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return -10 + evalKings1Pawn(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return -10 + evalKings1Pawn(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return +10 + evalKings1Pawn(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return +10 + evalKings1Pawn(board, Figure::ColorWhite);
  };

  auto bishops_vs_knight = [](Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    auto kw = board.kingPos(winnerColor);
    auto kl = board.kingPos(loserColor);
    int npos = _lsb64(board.fmgr().knight_mask(loserColor));
    auto score = 70 + (7 - distanceCounter().getDistance(kw, kl)) * EvalCoefficients::kingToKingDistanceMulti_;
    score -= EvalCoefficients::positionEvaluations_[1][Figure::TypeKing][kl];
    score += distanceCounter().getDistance(npos, kl) * EvalCoefficients::figureToKingDistanceMulti_;
    if(winnerColor == Figure::ColorBlack)
      score = -score;
    return score;
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [bishops_vs_knight](Board const& board)
  {
    return bishops_vs_knight(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 2 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [bishops_vs_knight](Board const& board)
  {
    return bishops_vs_knight(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return pawnAndHeavy(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return pawnAndHeavy(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeQueen, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return pawnAndHeavy(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeQueen, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeQueen, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return pawnAndHeavy(board, Figure::ColorWhite);
  };
}

void SpecialCasesDetector::initWinnerLoser()
{
  auto knighGoToPawn = [](Board const& board, Figure::Color pawnColor, BitMask const& blocked_additional) -> bool
  {
    int deltay = delta_y_[pawnColor];
    auto knightColor = Figure::otherColor(pawnColor);
    auto p = _lsb64(board.fmgr().pawn_mask(pawnColor));
    auto n = _lsb64(board.fmgr().knight_mask(knightColor));
    int kp = board.kingPos(pawnColor);
    int kn = board.kingPos(knightColor);
    BitMask current = set_mask_bit(n);
    BitMask visited{current};
    int p1 = p + deltay * (board.color() == pawnColor);
    BitMask blocked = ~(movesTable().pawnCaps(pawnColor, p1)
                        | board.fmgr().king_mask(pawnColor) | board.fmgr().king_mask(knightColor)
                        | (movesTable().caps(Figure::TypeKing, kp) & ~movesTable().caps(Figure::TypeKing, kn))
                        | blocked_additional);
    BitMask target = (pawnMasks().mask_forward(pawnColor, p1) | set_mask_bit(p1));
    for(; current != 0ULL;)
    {
      BitMask next{};
      for(; current != 0ULL;)
      {
        int i = clear_lsb(current);
        auto moves = movesTable().caps(Figure::TypeKnight, i);
        if(moves & target)
          return true;
        next |= moves & blocked & (~visited);
      }
      blocked |= movesTable().pawnCaps(pawnColor, p1);
      target &= ~set_mask_bit(p1);
      if(target == 0ULL)
        return false;
      p1 += deltay;
      if(p1 < 0 || p1 > 63)
        return false;
      blocked &= ~movesTable().pawnCaps(pawnColor, p1);
      current = next;
      visited |= current;
    }
    return false;
  };

  auto bishopGoToPawn = [](Board const& board, Figure::Color pawnColor, BitMask const& blocked_additional) -> bool
  {
    int deltay = delta_y_[pawnColor];
    auto bishopColor = Figure::otherColor(pawnColor);
    auto p = _lsb64(board.fmgr().pawn_mask(pawnColor));
    auto b = _lsb64(board.fmgr().bishop_mask(bishopColor));
    int kp = board.kingPos(pawnColor);
    int kb = board.kingPos(bishopColor);
    BitMask current = set_mask_bit(b);
    BitMask visited{ current };
    int p1 = p + deltay * (board.color() == pawnColor);
    BitMask blocked = ~(movesTable().pawnCaps(pawnColor, p1)
                        | board.fmgr().king_mask(pawnColor) | board.fmgr().bishop_mask(bishopColor)
                        | (movesTable().caps(Figure::TypeKing, kp) & ~movesTable().caps(Figure::TypeKing, kb))
                        | blocked_additional);
    BitMask target = (pawnMasks().mask_forward(pawnColor, p1) | set_mask_bit(p1));
    BitMask mask_all = board.fmgr().mask(Figure::ColorBlack) | board.fmgr().mask(Figure::ColorWhite);
    for(; current != 0ULL;)
    {
      BitMask next{};
      for(; current != 0ULL;)
      {
        int i = clear_lsb(current);
        auto moves = magic_ns::bishop_moves(i, mask_all);
        if(moves & target)
          return true;
        next |= moves & blocked & (~visited);
      }
      blocked |= movesTable().pawnCaps(pawnColor, p1);
      auto p1_mask_inv = ~set_mask_bit(p1);
      target &= p1_mask_inv;
      mask_all &= p1_mask_inv;
      if(target == 0ULL)
        return false;
      p1 += deltay;
      if(p1 < 0 || p1 > 63)
        return false;
      mask_all |= set_mask_bit(p1);
      blocked &= ~movesTable().pawnCaps(pawnColor, p1);
      current = next;
      visited |= current;
    }
    return false;
  };

  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [knighGoToPawn](Board const& board) -> ScoreType
  {
    BitMask blocked = movesTable().caps(Figure::TypeBishop, _lsb64(board.fmgr().bishop_mask(Figure::ColorBlack)));
    ScoreType score = evalKings1Pawn(board, Figure::ColorBlack);
    if(!knighGoToPawn(board, Figure::ColorBlack, blocked))
    {
      if(kpkPassed(board, Figure::ColorBlack))
        return -Figure::figureWeight_[Figure::TypePawn] + score;
    }
    return -20 + score;
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [knighGoToPawn](Board const& board) -> ScoreType
  {
    BitMask blocked = movesTable().caps(Figure::TypeBishop, _lsb64(board.fmgr().bishop_mask(Figure::ColorWhite)));
    ScoreType score = evalKings1Pawn(board, Figure::ColorWhite);
    if(!knighGoToPawn(board, Figure::ColorWhite, blocked))
    {
      if(kpkPassed(board, Figure::ColorWhite))
        return +Figure::figureWeight_[Figure::TypePawn] + score;
    }
    return +20 + score;
  };

  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [bishopGoToPawn](Board const& board)
  {
    BitMask blocked = movesTable().caps(Figure::TypeBishop, _lsb64(board.fmgr().bishop_mask(Figure::ColorBlack)));
    ScoreType score = evalKings1Pawn(board, Figure::ColorBlack);
    if(!bishopGoToPawn(board, Figure::ColorBlack, blocked))
    {
      if(kpkPassed(board, Figure::ColorBlack))
        return -Figure::figureWeight_[Figure::TypePawn] + score;
    }
    return -20 + score;
  };
  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [bishopGoToPawn](Board const& board)
  {
    BitMask blocked = movesTable().caps(Figure::TypeBishop, _lsb64(board.fmgr().bishop_mask(Figure::ColorWhite)));
    ScoreType score = evalKings1Pawn(board, Figure::ColorWhite);
    if(!bishopGoToPawn(board, Figure::ColorWhite, blocked))
    {
      if(kpkPassed(board, Figure::ColorWhite))
        return +Figure::figureWeight_[Figure::TypePawn] + score;
    }
    return +20 + score;
  };

  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [knighGoToPawn](Board const& board)
  {
    BitMask blocked = movesTable().caps(Figure::TypeKnight, _lsb64(board.fmgr().knight_mask(Figure::ColorBlack)));
    ScoreType score = evalKings1Pawn(board, Figure::ColorBlack);
    if(!knighGoToPawn(board, Figure::ColorBlack, blocked))
    {
      if(kpkPassed(board, Figure::ColorBlack))
        return -Figure::figureWeight_[Figure::TypePawn] + score;
    }
    return -20 + score;
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [knighGoToPawn](Board const& board)
  {
    BitMask blocked = movesTable().caps(Figure::TypeKnight, _lsb64(board.fmgr().knight_mask(Figure::ColorWhite)));
    ScoreType score = evalKings1Pawn(board, Figure::ColorWhite);
    if(!knighGoToPawn(board, Figure::ColorWhite, blocked))
    {
      if(kpkPassed(board, Figure::ColorWhite))
        return +Figure::figureWeight_[Figure::TypePawn] + score;
    }
    return +20 + score;
  };

  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [bishopGoToPawn](Board const& board)
  {
    BitMask blocked = movesTable().caps(Figure::TypeKnight, _lsb64(board.fmgr().knight_mask(Figure::ColorBlack)));
    ScoreType score = evalKings1Pawn(board, Figure::ColorBlack);
    if(!bishopGoToPawn(board, Figure::ColorBlack, blocked))
    {
      if(kpkPassed(board, Figure::ColorBlack))
        return -Figure::figureWeight_[Figure::TypePawn] + score;
    }
    return -20 + score;
  };
  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [bishopGoToPawn](Board const& board)
  {
    BitMask blocked = movesTable().caps(Figure::TypeKnight, _lsb64(board.fmgr().knight_mask(Figure::ColorWhite)));
    ScoreType score = evalKings1Pawn(board, Figure::ColorWhite);
    if(!bishopGoToPawn(board, Figure::ColorWhite, blocked))
    {
      if(kpkPassed(board, Figure::ColorWhite))
        return +Figure::figureWeight_[Figure::TypePawn] + score;
    }
    return +20 + score;
  };

  auto bishop_vs_pawn = [bishopGoToPawn](Board const& board, Figure::Color pawnColor) -> ScoreType
  {
    ScoreType score = evalKings1Pawn(board, pawnColor);
    if(!bishopGoToPawn(board, pawnColor, 0ULL))
    {
      if(kpkPassed(board, pawnColor))
        return (pawnColor ? 50 : -50) + score;
    }
    return (pawnColor ? 20 : -20) + score/2;
  };

  auto knight_vs_pawn = [knighGoToPawn](Board const& board, Figure::Color pawnColor) -> ScoreType
  {
    ScoreType score = evalKings1Pawn(board, pawnColor);
    if(!knighGoToPawn(board, pawnColor, 0ULL))
    {
      if(kpkPassed(board, pawnColor))
        return (pawnColor ? 50 : -50) + score;
    }
    return (pawnColor ? 20 : -20) + score/2;
  };

  auto figure_vs_pawns = [](Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    int kw = board.kingPos(winnerColor);
    int kl = board.kingPos(loserColor);
    ScoreType score = board.fmgr().pawns(winnerColor) * 10;
    auto pmask = board.fmgr().pawn_mask(winnerColor);
    for(; pmask;)
    {
      Index p(clear_lsb(pmask));
      Index pp(p.x(), winnerColor*7);
      int y = pawn_colored_y_[winnerColor][p.y()];
      score += EvalCoefficients::passerPawnSc_[y];
      score += (7 - distanceCounter().getDistance(kw, pp) + distanceCounter().getDistance(kl, pp)) * EvalCoefficients::kingToPawnDistanceMulti_;
    }
    if(winnerColor == Figure::ColorBlack)
      score = -score;
    return score;
  };

  // figure against pawns
  for(int i = 1; i < 8; ++i)
  {
    winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, static_cast<char>(i) } })] = [figure_vs_pawns, bishop_vs_pawn, i](Board const& board)
    {
      if(i == 1)
        return bishop_vs_pawn(board, Figure::ColorBlack);
      else
        return figure_vs_pawns(board, Figure::ColorBlack);
    };
    winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, static_cast<char>(i) } })] = [figure_vs_pawns, bishop_vs_pawn, i](Board const& board)
    {
      if(i == 1)
        return bishop_vs_pawn(board, Figure::ColorWhite);
      else
        return figure_vs_pawns(board, Figure::ColorWhite);
    };
    winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, static_cast<char>(i) } })] = [figure_vs_pawns, knight_vs_pawn, i](Board const& board)
    {
      if(i == 1)
        return knight_vs_pawn(board, Figure::ColorBlack);
      else
        return figure_vs_pawns(board, Figure::ColorBlack);
    };
    winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, static_cast<char>(i) } })] = [figure_vs_pawns, knight_vs_pawn, i](Board const& board)
    {
      if(i == 1)
        return knight_vs_pawn(board, Figure::ColorWhite);
      else
        return figure_vs_pawns(board, Figure::ColorWhite);
    };

    winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
    { Figure::TypePawn, Figure::ColorBlack, static_cast<char>(i) } })] = [figure_vs_pawns, i](Board const& board)
    {
      return figure_vs_pawns(board, Figure::ColorBlack);
    };
    winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
    { Figure::TypePawn, Figure::ColorWhite, static_cast<char>(i) } })] = [figure_vs_pawns, i](Board const& board)
    {
      return figure_vs_pawns(board, Figure::ColorWhite);
    };
  }
}

inline SpecialCasesDetector::Scase toScpecialCase(FiguresManager const& fmgr)
{
  SpecialCasesDetector::Scase sc{};
  for(int c = Figure::ColorBlack; c <= Figure::ColorWhite; ++c)
  {
    for(int t = Figure::TypePawn; t < Figure::TypeKing; ++t)
    {
      auto type = (Figure::Type)t;
      auto color = (Figure::Color)c;
      sc |= ((uint64)fmgr.tcount(type, color)) << calc_shift(type, color);
    }
  }
  return sc;
}

std::pair<bool, ScoreType> SpecialCasesDetector::evalBishopAndPawns(Board const& board) const
{
  auto const& fmgr = board.fmgr();
  for (Figure::Color winColor : {Figure::ColorBlack, Figure::ColorWhite})
  {
    auto losColor = Figure::otherColor(winColor);
    if (fmgr.bishops(winColor) == 1 && fmgr.allFigures(losColor) == 0 && fmgr.allFigures(winColor) == 1 && fmgr.pawns(winColor) > 0)
    {
      auto pw_mask = fmgr.pawn_mask(winColor);
      if (((pw_mask & pawnMasks().mask_column(0)) != pw_mask) &&
          ((pw_mask & pawnMasks().mask_column(7)) != pw_mask))
      {
        return { false, ScoreType{} };
      }
      ScoreType losScore = 0;
      auto b = _lsb64(fmgr.bishop_mask(winColor));
      auto opw_mask = fmgr.pawn_mask(losColor);
      while (opw_mask)
      {
        int n = clear_lsb(opw_mask);
        Index op{n};
        int cy = pawn_colored_y_[losColor][op.y()];
        if (cy > 4)
          losScore += 5*cy;
      }
      Index p(_lsb64(pw_mask));
      Index pp(p.x(), winColor * 7);
      if (FiguresCounter::s_whiteColors_[b] != FiguresCounter::s_whiteColors_[pp])
      {
        int kw = board.kingPos(winColor);
        int kl = board.kingPos(losColor);
        int pr_dist = distanceCounter().getDistance(p, pp);
        int l_pr_dist = distanceCounter().getDistance(kl, pp);
        int w_pr_dist = distanceCounter().getDistance(kw, pp);
        if ((pr_dist > l_pr_dist && l_pr_dist <= w_pr_dist) || (l_pr_dist < 2 && pr_dist > 0))
        {
          ScoreType score = winColor == Figure::ColorBlack ? -20 + losScore : +20 - losScore;
          return { true, score };
        }
      }
      return { false, ScoreType{} };
    }
    else if(fmgr.allFigures(losColor) == 0 && fmgr.allFigures(winColor) == 0 && fmgr.pawns(winColor) > 0 && fmgr.pawns(losColor) == 0)
    {
      auto pw_mask = fmgr.pawn_mask(winColor);
      if (((pw_mask & pawnMasks().mask_column(0)) != pw_mask) &&
        ((pw_mask & pawnMasks().mask_column(7)) != pw_mask))
      {
        return { false, ScoreType{} };
      }
      int ppos = winColor == Figure::ColorWhite ? _msb64(pw_mask) : _lsb64(pw_mask);
      if(!kpkPassed(board, winColor, ppos))
      {
        ScoreType score = winColor == Figure::ColorBlack ? -20 : +20;
        return { true, score };
      }
      return { false, ScoreType{} };
    }
  }
  return { false, ScoreType{} };
}

std::pair<bool, ScoreType> SpecialCasesDetector::eval(Board const& board) const
{
  auto const& fmgr = board.fmgr();
  if(fmgr.weight(Figure::ColorWhite) == 0 || fmgr.weight(Figure::ColorBlack) == 0)
  {
    auto sc = toScpecialCase(fmgr);
    auto iter = matCases_.find(sc);
    if(iter != matCases_.end())
      return { true, (iter->second)(board) };
    X_ASSERT(scases_.find(sc) != scases_.end() || winnerLoser_.find(sc) != winnerLoser_.end(), "special case was not detected");
    return evalBishopAndPawns(board);
  }
  if(fmgr.allFigures(Figure::ColorBlack) > 2 || fmgr.allFigures(Figure::ColorWhite) > 2)
  {
    return {false, 0};
  }
  auto sc = toScpecialCase(fmgr);
  {
    auto iter = scases_.find(sc);
    if(iter != scases_.end())
      return { true, (iter->second)(board) };
  }
  {
    auto iter = winnerLoser_.find(sc);
    if(iter != winnerLoser_.end())
      return { true, (iter->second)(board) };
  }
  return evalBishopAndPawns(board);
}

}