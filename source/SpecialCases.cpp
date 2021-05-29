#include <SpecialCases.h>
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
    int           count_;
  };

  BitMask format(std::initializer_list<Triplet>&& figures)
  {
    BitMask hkey = 0ULL;
    hkey ^= FiguresManager::code(Figure::ColorBlack, Figure::TypeKing, 1);
    hkey ^= FiguresManager::code(Figure::ColorWhite, Figure::TypeKing, 1);
    for(auto const& f : figures) {
      if(f.count_)
        hkey ^= FiguresManager::code(f.color_, f.type_, f.count_);
    }
    return hkey;
  }

  const int pawn_colored_y_[2][8] = {
    { 7, 6, 5, 4, 3, 2, 1, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7 } };

  const int delta_y_[2] = { -8, 8 };

  const BitMask FORTRESS_PAWNS[2] = { 35538699416403456ULL, 35604670110137856ULL };

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

  std::pair<SpecialCaseResult, ScoreType> pawnOnly(Board const& board, Figure::Color pawnColor)
  {
    if(kpkPassed(board, pawnColor))
    {
      ScoreType score{ +10 };
      Index index(_lsb64(board.fmgr().pawn_mask(pawnColor)));
      auto xpawnColor = (pawnColor == Figure::ColorWhite) ? pawnColor : Figure::otherColor(pawnColor);
      score = 4*Figure::figureWeight_[Figure::TypePawn] + pawn_colored_y_[xpawnColor][index.y()] * 20;
      if (pawnColor == Figure::ColorBlack)
        score = -score;
      return { SpecialCaseResult::SCORE, score };
    }
    else {
      return { SpecialCaseResult::DRAW, 0 };
    }
  }
  
  bool couldIntercept(Board const& board, Figure::Color pawnColor, Index pawnPos)
  {
    auto const& fmgr = board.fmgr();
    int promo_pos = pawnPos.x() | (pawnColor * 56);
    int stepsMax = std::abs(pawnColor * 7 - pawnPos.y());
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
  
  std::pair<SpecialCaseResult, ScoreType> pawnAndHeavy(Board const& board, Figure::Color pawnColor)
  {
    auto ocolor = Figure::otherColor(pawnColor);
    Index index(_lsb64(board.fmgr().pawn_mask(pawnColor)));
    Index kingP(board.kingPos(pawnColor));
    Index kingO(board.kingPos(ocolor));
    int pcy = pawn_colored_y_[pawnColor][index.y()];
    if (std::abs(kingO.x() - index.x()) < 2 && pawn_colored_y_[pawnColor][kingO.y()] > pcy) {
      if (pawn_colored_y_[pawnColor][kingP.y()] < pcy) {
        return { SpecialCaseResult::ALMOST_DRAW, 0 };
      }
      else {
        return { SpecialCaseResult::PROBABLE_DRAW, 0 };
      }
    }
    bool kpk = kpkPassed(board, pawnColor);
    if (index.x() == 0 || index.x() == 7) {
      if (!kpk) {
        return { SpecialCaseResult::ALMOST_DRAW, 0 };
      }
      else {
        return { SpecialCaseResult::PROBABLE_DRAW, 0 };
      }
    }
    if (!kpk) {
      return { SpecialCaseResult::PROBABLE_DRAW, 0 };
    }
    return { SpecialCaseResult::NO_RESULT, 0 };
  }

  std::pair<SpecialCaseResult, ScoreType> pawnsAndRook(Board const& board, Figure::Color pawnColor)
  {
    auto const& fmgr = board.fmgr();
    auto pmask = fmgr.pawn_mask(pawnColor);
    int xmin = 7;
    int xmax = 0;
    int ymin = 7;
    int ymax = 0;
    while (pmask) {
      auto n = clear_lsb(pmask);
      Index index{ n };
      xmin = std::min(xmin, index.x());
      xmax = std::max(xmax, index.x());
      ymin = std::min(ymin, index.y());
      ymax = std::max(ymax, index.y());
    }
    if (std::abs(xmax - xmin) > 1) {
      return { SpecialCaseResult::NO_RESULT, 0 };
    }

    Index pkpos{ board.kingPos(pawnColor) };
    Index okpos{ board.kingPos(Figure::otherColor(pawnColor)) };
    if (xmin == xmax) {
      bool kingsOk =
        (pawnColor == Figure::ColorBlack && okpos.y() < ymin && okpos.y() < pkpos.y()) ||
        (pawnColor == Figure::ColorWhite && okpos.y() > ymax && okpos.y() > pkpos.y());
      if (xmin == 0 || xmin == 7) {
        if (kingsOk) {
          if (std::abs(okpos.x() - xmin) > std::abs(pkpos.x() - xmin))
            return { SpecialCaseResult::PROBABLE_DRAW, 0 };
          else
            return { SpecialCaseResult::ALMOST_DRAW, 0 };
        }
        else {
          return { SpecialCaseResult::MAYBE_DRAW, 0 };
        }
      }
      else if (kingsOk && (std::abs(okpos.x() - xmin) <= 1 && std::abs(okpos.x() - xmax) <= 1)) {
          return { SpecialCaseResult::PROBABLE_DRAW, 0 };
      }
    }
    else {
      bool kingsOk =
        (pawnColor == Figure::ColorBlack && okpos.y() < ymin && pkpos.y() >= ymax) ||
        (pawnColor == Figure::ColorWhite && okpos.y() > ymax && pkpos.y() <= ymin);
      if (kingsOk && (std::abs(okpos.x() - xmin) <= 1 && std::abs(okpos.x() - xmax) <= 1)) {
        return { SpecialCaseResult::PROBABLE_DRAW, 0 };
      }
    }
    return { SpecialCaseResult::NO_RESULT, 0 };
  }

  std::pair<SpecialCaseResult, ScoreType> bishopAndPawnsDraw(Board const& board, Figure::Color winColor)
  {
    auto const& fmgr = board.fmgr();
    auto losColor = Figure::otherColor(winColor);
    auto pw_mask = fmgr.pawn_mask(winColor);
    auto const pw_0 = pw_mask & pawnMasks().mask_column(0);
    auto const pw_7 = pw_mask & pawnMasks().mask_column(7);
    auto const pw_1_6 = ~(pw_0 | pw_7) & pw_mask;
    if ((pw_1_6 != 0ULL) || (pw_0 != 0ULL && pw_7 != 0ULL)) {
      return { SpecialCaseResult::NO_RESULT, 0 };
    }
    auto b = _lsb64(fmgr.bishop_mask(winColor));
    Index p(winColor ? _msb64(pw_mask) : _lsb64(pw_mask));
    Index pp(p.x(), winColor * 7);
    if (FiguresCounter::s_whiteColors_[b] != FiguresCounter::s_whiteColors_[pp])
    {
      int kw = board.kingPos(winColor);
      int kl = board.kingPos(losColor);
      int pr_dist = distanceCounter().getDistance(p, pp);
      int l_pr_dist = distanceCounter().getDistance(kl, pp);
      int w_pr_dist = distanceCounter().getDistance(kw, pp);
      if ((pr_dist > l_pr_dist && l_pr_dist <= w_pr_dist) || (l_pr_dist < 2 && pr_dist > 0)) {
        return { SpecialCaseResult::DRAW, 0 };
      }
    }
    return { SpecialCaseResult::NO_RESULT, 0 };
  }

  std::pair<SpecialCaseResult, ScoreType> queenAgainstRookVsPawnDraw(Board const& board, Figure::Color winnerColor)
  {
    auto ocolor = Figure::otherColor(winnerColor);
    Index kingW(board.kingPos(winnerColor));
    Index kingL(board.kingPos(ocolor));
    int queenW = _lsb64(board.fmgr().queen_mask(winnerColor));
    Index rookL(_lsb64(board.fmgr().rook_mask(ocolor)));
    bool fortress = false;
    auto pw_mask = board.fmgr().pawn_mask(ocolor) & FORTRESS_PAWNS[ocolor];
    while (pw_mask && !fortress)
    {
      int n = clear_lsb(pw_mask);
      Index pawn{n};
      int py = pawn_colored_y_[ocolor][pawn.y()];
      if ((movesTable().pawnCaps(ocolor, n) & board.fmgr().rook_mask(ocolor)) &&
          distanceCounter().getDistance(kingL, n) == 1)
      {
        fortress =
          ( (pawn_colored_y_[ocolor][rookL.y()] == 2) &&
            (pawn_colored_y_[ocolor][kingW.y()] > pawn_colored_y_[ocolor][rookL.y()]) &&
            (pawn_colored_y_[ocolor][kingL.y()] < pawn_colored_y_[ocolor][rookL.y()]) &&
            (py == 1)) ||
          (pawn.x() == 1 && rookL.x() == 2 && kingL.x() < rookL.x() && kingW.x() > rookL.x()) ||
          (pawn.x() == 6 && rookL.x() == 5 && kingL.x() > rookL.x() && kingW.x() < rookL.x()) ||
          ((py == 5 || py == 6) && ((kingW.x()-rookL.x())*(pawn.x()-rookL.x()) < 0));
      }
    }
    return { fortress ? SpecialCaseResult::ALMOST_DRAW : SpecialCaseResult::NO_RESULT, 0 };
  }
  
  std::pair<SpecialCaseResult, ScoreType> queenAgainstRook(Board const& board, Figure::Color winnerColor)
  {
    auto ocolor = Figure::otherColor(winnerColor);
    ScoreType score = Figure::figureWeight_[Figure::TypeQueen] - Figure::figureWeight_[Figure::TypeRook];
    Index kingW(board.kingPos(winnerColor));
    Index kingL(board.kingPos(ocolor));
    int queenW = _lsb64(board.fmgr().queen_mask(winnerColor));
    int rookL = _lsb64(board.fmgr().rook_mask(ocolor));
    score -= EvalCoefficients::positionEvaluations_[0][Figure::TypeKing][kingL].eval1();
    score -= distanceCounter().getDistance(kingW, kingL) * 2;
    score -= distanceCounter().getDistance(queenW, kingL) * 2;
    score += distanceCounter().getDistance(rookL, kingL) * 2;
    if (winnerColor == Figure::ColorBlack)
      score = -score;
    return { SpecialCaseResult::SCORE, score };
  }
  
  std::pair<SpecialCaseResult, ScoreType> evalMatCases(Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    auto kw = board.kingPos(winnerColor);
    auto kl = board.kingPos(loserColor);
    auto score = (7 - distanceCounter().getDistance(kw, kl)) * EvalCoefficients::kingToKingDistanceMulti_;
    score -= EvalCoefficients::positionEvaluations_[0][Figure::TypeKing][kl].eval1();
    score += board.fmgr().weight(winnerColor).eval1() + EvalCoefficients::additionalMatBonus_;
    score -= board.fiftyMovesCount();
    if (!winnerColor)
      score = -score;
    return { SpecialCaseResult::SCORE, score };
  }

  // mat with Bishop & Knight
  ScoreType bishopKnightMat(Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    int kw = board.kingPos(winnerColor);
    int kl = board.kingPos(loserColor);
    int bp = _lsb64(board.fmgr().bishop_mask(winnerColor));
    int kn = _lsb64(board.fmgr().knight_mask(winnerColor));
    int dist = distanceCounter().getDistance(kw, kl);
    ScoreType score = board.fmgr().weight(winnerColor).eval1() - dist;
    int kp = kl;
    if (FiguresCounter::s_whiteColors_[bp])
    {
      kp = Figure::mirrorIndex_[kp];
    }
    score += EvalCoefficients::bishopKnightMat_[kp] * 2 + EvalCoefficients::additionalMatBonus_;
    int ndist = distanceCounter().getDistance(kn, kl);
    int bdist = distanceCounter().getDistance(bp, kl);
    score -= ndist;
    score -= bdist >> 1;
    score -= board.fiftyMovesCount();
    return score;
  };

  ScoreType evalKings(Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    auto kw = board.kingPos(winnerColor);
    auto kl = board.kingPos(loserColor);
    auto score = (7 - distanceCounter().getDistance(kw, kl)) * EvalCoefficients::kingToKingDistanceMulti_;
    score -= EvalCoefficients::positionEvaluations_[0][Figure::TypeKing][kl].eval1();
    score += 30;
    if (auto n_mask = board.fmgr().knight_mask(winnerColor))
    {
      auto np = _lsb64(n_mask);
      score += (7 - distanceCounter().getDistance(kl, np)) * EvalCoefficients::figureToKingDistanceMulti_;
    }
    if (auto b_mask = board.fmgr().bishop_mask(winnerColor))
    {
      auto bp = _lsb64(b_mask);
      score += (7 - distanceCounter().getDistance(kl, bp)) * EvalCoefficients::figureToKingDistanceMulti_;
    }
    if (auto n_mask = board.fmgr().knight_mask(loserColor))
    {
      auto np = _lsb64(n_mask);
      score += distanceCounter().getDistance(kl, np) * EvalCoefficients::figureToKingDistanceMulti_;
    }
    if (auto b_mask = board.fmgr().bishop_mask(loserColor))
    {
      auto bp = _lsb64(b_mask);
      score += distanceCounter().getDistance(kl, bp) * EvalCoefficients::figureToKingDistanceMulti_;
    }
    return score;
  };

  bool knighGoToPawn(Board const& board, Figure::Color pawnColor, BitMask const& blocked_additional)
  {
    int deltay = delta_y_[pawnColor];
    auto knightColor = Figure::otherColor(pawnColor);
    auto p = _lsb64(board.fmgr().pawn_mask(pawnColor));
    auto n = _lsb64(board.fmgr().knight_mask(knightColor));
    int kp = board.kingPos(pawnColor);
    int kn = board.kingPos(knightColor);
    BitMask current = set_mask_bit(n);
    BitMask visited{ current };
    int p1 = p + deltay * (board.color() == pawnColor);
    BitMask blocked = ~(movesTable().pawnCaps(pawnColor, p1)
      | board.fmgr().king_mask(pawnColor) | board.fmgr().king_mask(knightColor)
      | (movesTable().caps(Figure::TypeKing, kp) & ~movesTable().caps(Figure::TypeKing, kn))
      | blocked_additional);
    BitMask target = (pawnMasks().mask_forward(pawnColor, p1) | set_mask_bit(p1));
    for (; current != 0ULL;)
    {
      BitMask next{};
      for (; current != 0ULL;)
      {
        int i = clear_lsb(current);
        auto moves = movesTable().caps(Figure::TypeKnight, i);
        if (moves & target)
          return true;
        next |= moves & blocked & (~visited);
      }
      blocked |= movesTable().pawnCaps(pawnColor, p1);
      target &= ~set_mask_bit(p1);
      if (target == 0ULL)
        return false;
      p1 += deltay;
      if (p1 < 0 || p1 > 63)
        return false;
      blocked &= ~movesTable().pawnCaps(pawnColor, p1);
      current = next;
      visited |= current;
    }
    return false;
  };

  bool bishopGoToPawn(Board const& board, Figure::Color pawnColor, BitMask const& blocked_additional)
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
    for (; current != 0ULL;)
    {
      BitMask next{};
      for (; current != 0ULL;)
      {
        int i = clear_lsb(current);
        auto moves = magic_ns::bishop_moves(i, mask_all);
        if (moves & target)
          return true;
        next |= moves & blocked & (~visited);
      }
      blocked |= movesTable().pawnCaps(pawnColor, p1);
      auto p1_mask_inv = ~set_mask_bit(p1);
      target &= p1_mask_inv;
      mask_all &= p1_mask_inv;
      if (target == 0ULL)
        return false;
      p1 += deltay;
      if (p1 < 0 || p1 > 63)
        return false;
      mask_all |= set_mask_bit(p1);
      blocked &= ~movesTable().pawnCaps(pawnColor, p1);
      current = next;
      visited |= current;
    }
    return false;
  };

  ScoreType evalKings1Pawn(Board const& board, Figure::Color pawnColor)
  {
    Figure::Color ocolor = Figure::otherColor(pawnColor);
    Index p(_lsb64(board.fmgr().pawn_mask(pawnColor)));
    int kw = board.kingPos(pawnColor);
    int kl = board.kingPos(ocolor);
    Index pp(p.x(), pawnColor * 7);
    int dist_w = distanceCounter().getDistance(kw, pp);
    int dist_l = distanceCounter().getDistance(kl, pp);
    int y = pawn_colored_y_[pawnColor][p.y()];
    ScoreType score = (7 - dist_w + dist_l) * EvalCoefficients::kingToPawnDistanceMulti_ + EvalCoefficients::passerPawnSc_[y];
    auto kn_mask = board.fmgr().knight_mask(ocolor);
    for (; kn_mask;)
    {
      int n = clear_lsb(kn_mask);
      score += distanceCounter().getDistance(n, pp) * EvalCoefficients::knightToPawnDistanceMulti_;
    }
    if (!pawnColor)
      score = -score;
    return score;
  }

  ScoreType bishopVsPawn(Board const& board, Figure::Color pawnColor)
  {
    ScoreType score = evalKings1Pawn(board, pawnColor);
    if (!bishopGoToPawn(board, pawnColor, 0ULL))
    {
      if (kpkPassed(board, pawnColor))
        return (pawnColor ? 50 : -50) + score;
    }
    return (pawnColor ? 20 : -20) + score / 2;
  };

  ScoreType knightVsPawn(Board const& board, Figure::Color pawnColor)
  {
    ScoreType score = evalKings1Pawn(board, pawnColor);
    if (!knighGoToPawn(board, pawnColor, 0ULL))
    {
      if (kpkPassed(board, pawnColor))
        return (pawnColor ? 50 : -50) + score;
    }
    return (pawnColor ? 20 : -20) + score / 2;
  };

  ScoreType figureVsPawns(Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    int kw = board.kingPos(winnerColor);
    int kl = board.kingPos(loserColor);
    ScoreType score = board.fmgr().pawns(winnerColor) * 10;
    auto pmask = board.fmgr().pawn_mask(winnerColor);
    for (; pmask;)
    {
      Index p(clear_lsb(pmask));
      Index pp(p.x(), winnerColor * 7);
      int y = pawn_colored_y_[winnerColor][p.y()];
      score += EvalCoefficients::passerPawnSc_[y];
      score += (7 - distanceCounter().getDistance(kw, pp) + distanceCounter().getDistance(kl, pp)) * EvalCoefficients::kingToPawnDistanceMulti_;
    }
    if (winnerColor == Figure::ColorBlack)
      score = -score;
    return score;
  };

  std::pair<SpecialCaseResult, ScoreType> evalRookPawn(Board const& board, Figure::Color rcolor)
  {
    auto pcolor = Figure::otherColor(rcolor);
    bool rmove = rcolor == board.color();
    Index kr{ board.kingPos(rcolor) };
    Index kp{ board.kingPos(pcolor) };
    Index r{ _lsb64(board.fmgr().rook_mask(rcolor)) };
    Index p{ _lsb64(board.fmgr().pawn_mask(pcolor)) };
    Index pp(p.x(), pcolor * 7);
    auto dist_kp = distanceCounter().getDistance(kp, p);
    auto dist_kr = distanceCounter().getDistance(kr, p);
    auto dist_kpp = distanceCounter().getDistance(kp, pp);
    auto dist_krp = distanceCounter().getDistance(kr, pp);
    auto dist_pp = distanceCounter().getDistance(p, pp);
    if (dist_pp+rmove < dist_krp && dist_kp+rmove <= 3 && dist_pp <= 4) {
      if (dist_kp + rmove + 1 < dist_kr) {
        return { SpecialCaseResult::ALMOST_DRAW, 0 };
      }
      if ((dist_kp + rmove < dist_kr)) {
        return { SpecialCaseResult::PROBABLE_DRAW, 0 };
      }
    }
    return { SpecialCaseResult::NO_RESULT, 0 };
  }

  std::pair<SpecialCaseResult, ScoreType> bishopRooks(Board const& board, Figure::Color bishopColor)
  {
    auto const& fmgr = board.fmgr();
    auto loserColor = Figure::otherColor(bishopColor);
    Index bi{ _lsb64(fmgr.bishop_mask(bishopColor)) };
    Index kw{ board.kingPos(bishopColor) };
    Index kl{ board.kingPos(loserColor) };
    Index rw{ _lsb64(fmgr.rook_mask(bishopColor)) };
    Index rl{ _lsb64(fmgr.rook_mask(loserColor)) };
    if (kl.x() != 0 && kl.x() != 7 && kl.y() != 0 && kl.y() != 7) {
      return { SpecialCaseResult::ALMOST_DRAW, 0 };
    }
    if (kl.x() == 0 && kl.y() == kw.y() && kw.x() == 2 && bi.x() == 3 && bi.y() == kl.y() && rw.x() == 1) {
      return { SpecialCaseResult::MAYBE_DRAW, 0 };
    }
    if (kl.x() == 7 && kl.y() == kw.y() && kw.x() == 5 && bi.x() == 4 && bi.y() == kl.y() && rw.x() == 6) {
      return { SpecialCaseResult::MAYBE_DRAW, 0 };
    }
    if (kl.y() == 0 && kl.x() == kw.x() && kw.y() == 2 && bi.y() == 3 && bi.x() == kl.x() && rw.y() == 1) {
      return { SpecialCaseResult::MAYBE_DRAW, 0 };
    }
    if (kl.y() == 7 && kl.x() == kw.x() && kw.y() == 5 && bi.y() == 4 && bi.x() == kl.x() && rw.y() == 6) {
      return { SpecialCaseResult::MAYBE_DRAW, 0 };
    }
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  }

} // namespace {}

SpecialCasesDetector::SpecialCasesDetector()
{
  initCases();
}

void SpecialCasesDetector::initCases()
{
  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };
  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };
  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, evalKings(board, Figure::ColorWhite) };
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, evalKings(board, Figure::ColorWhite) };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, -evalKings(board, Figure::ColorBlack) };
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, -evalKings(board, Figure::ColorBlack) };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 2 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 2 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
  { Figure::TypeKnight, Figure::ColorBlack, 2 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
  { Figure::TypeBishop, Figure::ColorBlack, 2 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return bishopRooks(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return bishopRooks(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 2 },
  { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
  { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::PROBABLE_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 2 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::PROBABLE_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
  { Figure::TypeBishop, Figure::ColorBlack, 1 } })] =
    [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, -bishopKnightMat(board, Figure::ColorBlack) };
  };
  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
  { Figure::TypeBishop, Figure::ColorWhite, 1 } })] =
    [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, +bishopKnightMat(board, Figure::ColorWhite) };
  };

  // figure against pawns
  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
  { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, bishopVsPawn(board, Figure::ColorBlack) };
  };
  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
  { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, bishopVsPawn(board, Figure::ColorWhite) };
  };
  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
  { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, knightVsPawn(board, Figure::ColorBlack) };
  };
  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
  { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, knightVsPawn(board, Figure::ColorWhite) };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
  { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, figureVsPawns(board, Figure::ColorBlack) };
  };

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
  { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::SCORE, figureVsPawns(board, Figure::ColorWhite) };
  };

  // rook against 2 pawns & rook
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
  { Figure::TypePawn, Figure::ColorBlack, 2 },
  { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return pawnsAndRook(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 2 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return pawnsAndRook(board, Figure::ColorWhite);
  };
  
  // rook against pawn & rook
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
  { Figure::TypePawn, Figure::ColorBlack, 1 },
  { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return pawnAndHeavy(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return pawnAndHeavy(board, Figure::ColorWhite);
  };

  // queen against pawn & queen
  scases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeQueen, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return pawnAndHeavy(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeQueen, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeQueen, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return pawnAndHeavy(board, Figure::ColorWhite);
  };

  // queen against figure & queen
  scases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeQueen, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeQueen, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeQueen, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeQueen, Figure::ColorWhite, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  scases_[format({ { Figure::TypeQueen, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeQueen, Figure::ColorBlack, 1 } })] = [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return { SpecialCaseResult::ALMOST_DRAW, 0 };
  };

  // only 1 pawn
  scases_[format({ { Figure::TypePawn, Figure::ColorBlack, 1 } })] =
    [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return pawnOnly(board, Figure::ColorBlack);
  };
  scases_[format({ { Figure::TypePawn, Figure::ColorWhite, 1 } })] =
    [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
  {
    return pawnOnly(board, Figure::ColorWhite);
  };

  // fortress
  for (int i = 1; i < 3; ++i)
  {
    scases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 },
      { Figure::TypeRook, Figure::ColorWhite, 1 },
      { Figure::TypePawn, Figure::ColorWhite, i } })] = [](Board const& board)
    {
      return queenAgainstRookVsPawnDraw(board, Figure::ColorBlack);
    };

    scases_[format({ { Figure::TypeQueen, Figure::ColorWhite, 1 },
      { Figure::TypeRook, Figure::ColorBlack, 1 },
      { Figure::TypePawn, Figure::ColorBlack, i } })] = [](Board const& board)
    {
      return queenAgainstRookVsPawnDraw(board, Figure::ColorWhite);
    };
  }
  
  // queen vs. rook
  scases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return queenAgainstRook(board, Figure::ColorBlack);
  };

  scases_[format({ { Figure::TypeQueen, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return queenAgainstRook(board, Figure::ColorWhite);
  };

  // some obvious mat cases
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return evalMatCases(board, Figure::ColorBlack);
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return evalMatCases(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return evalMatCases(board, Figure::ColorBlack);
  };
  scases_[format({ { Figure::TypeQueen, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return evalMatCases(board, Figure::ColorWhite);
  };

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 } })] = [](Board const& board)
  {
    return evalMatCases(board, Figure::ColorBlack);
  };
  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 2 } })] = [](Board const& board)
  {
    return evalMatCases(board, Figure::ColorWhite);
  };

  // bishop and corner pawn draw
  for (int w = 1; w <= 6; ++w)
  {
    for (int l = 0; l < 6-l; ++l)
    {
      scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
        { Figure::TypePawn, Figure::ColorWhite, w },
        { Figure::TypePawn, Figure::ColorBlack, l } })] = [](Board const& board)
      {
        return bishopAndPawnsDraw(board, Figure::ColorWhite);
      };
      scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
        { Figure::TypePawn, Figure::ColorBlack, w },
        { Figure::TypePawn, Figure::ColorWhite, l } })] = [](Board const& board)
      {
        return bishopAndPawnsDraw(board, Figure::ColorBlack);
      };
    }
  }

  // bishops and pawns. may be draw
  for (int w = 0; w <= 8; ++w)
  {
    for (int b = 0; b <= 8; ++b)
    {
      scases_[format({
        { Figure::TypeBishop, Figure::ColorWhite, 1 },
        { Figure::TypeBishop, Figure::ColorBlack, 1 },
        { Figure::TypePawn, Figure::ColorWhite, w },
        { Figure::TypePawn, Figure::ColorBlack, b } })] =
        [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
      {
        auto const& fmgr = board.fmgr();
        bool w = (fmgr.bishop_mask(Figure::ColorWhite) & FiguresCounter::s_whiteMask_) != 0ULL;
        bool b = (fmgr.bishop_mask(Figure::ColorBlack) & FiguresCounter::s_whiteMask_) != 0ULL;
        return { (b != w) ? SpecialCaseResult::MAYBE_DRAW : SpecialCaseResult::NO_RESULT, 0 };
      };
    }
  }

  // bishop|knight + rook (+pawn?) vs. rook + pawns -> probable draw
  for (Figure::Type ft : {Figure::TypeKnight, Figure::TypeBishop}) {
    for (Figure::Color fc : {Figure::ColorBlack, Figure::ColorWhite}) {
      for (int fp = 0; fp <= 1; ++fp) {
        for (int rp = fp+1; rp <= fp+3; ++rp) {
          scases_[format({
            { ft, fc, 1 },
            { Figure::TypeRook, fc, 1 },
            { Figure::TypePawn, fc, fp },
            { Figure::TypeRook, Figure::otherColor(fc), 1 },
            { Figure::TypePawn, Figure::otherColor(fc), rp } })] =
            [fp](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
          {
            return { fp ? SpecialCaseResult::PROBABLE_DRAW : SpecialCaseResult::ALMOST_DRAW, 0 };
          };
        }
      }
    }
  }

  // bishop|knight + pawn vs. rook + pawns -> may be draw
  for (Figure::Type ft : {Figure::TypeKnight, Figure::TypeBishop}) {
    for (Figure::Color fc : {Figure::ColorBlack, Figure::ColorWhite}) {
        scases_[format({
          { ft, fc, 1 },
          { Figure::TypePawn, fc, 1 },
          { Figure::TypeRook, Figure::otherColor(fc), 1 },
          { Figure::TypePawn, Figure::otherColor(fc), 1 } })] =
          [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
        {
          return { SpecialCaseResult::MAYBE_DRAW, 0 };
        };
    }
  }

  // bishop|knight + (+pawn?) vs. knight|bishop + pawns -> probable draw
  for (Figure::Type wft : {Figure::TypeKnight, Figure::TypeBishop}) {
    for (Figure::Color wfc : {Figure::ColorBlack, Figure::ColorWhite}) {
      for (int wfp = 0; wfp <= 2; ++wfp) {
        for (int lfp = 0; lfp <= 2; ++lfp) {
          if (wfp == 0 && lfp == 0) {
            continue;
          }
          auto lfc = Figure::otherColor(wfc);
          auto lft = (wft == Figure::TypeKnight) ? Figure::TypeBishop : Figure::TypeKnight;
          scases_[format({
            { wft, wfc, 1 },
            { Figure::TypePawn, wfc, wfp },
            { lft, lfc, 1 },
            { Figure::TypePawn, lfc, lfp },
            })] =
            [](Board const& board) -> std::pair<SpecialCaseResult, ScoreType>
          {
            return { SpecialCaseResult::MAYBE_DRAW, 0 };
          };
        }
      }
    }
  }

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 }, { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return evalRookPawn(board, Figure::ColorBlack);
  };
  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 }, { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return evalRookPawn(board, Figure::ColorWhite);
  };

  ///
}

} // NEngine