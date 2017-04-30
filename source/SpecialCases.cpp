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

  // king position eval for BN-mat
  const ScoreType bishopKnightMat_[64] =
  {
    16, 10, 6, 1, -2, -5, -12, -16,
    10, 12, 5, -1, -3, -6, -14, -12,
    5, 5, 4, -2, -4, -8, -8, -10,
    -1, -1, -2, -6, -6, -6, -5, -4,
    -4, -5, -6, -6, -6, -2, -1, -1,
    -10, -8, -8, -4, -2, 4, 5, 5,
    -12, -14, -6, -3, -1, 5, 12, 10,
    -16, -12, -5, -2, 1, 6, 10, 16
  };
}

SpecialCasesDetector::SpecialCasesDetector()
{
  initSimple();
  initComplex();
  initWinnerLoser();
}

void SpecialCasesDetector::initSimple()
{
  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = -10;
  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = -10;

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = +10;
  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = +10;

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = +20;
  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = +20;

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = -20;
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = -20;

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 2 } })] = +10;
  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 2 } })] = +10;

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 2 } })] = -10;
  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 2 } })] = -10;

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = +10;

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = -10;

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = -30;

  scases_[format({ { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = -30;

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = +30;

  scases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = +30;

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = -30;

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 2 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = +30;

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = -10;

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = +10;

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = -10;

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 2 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = +10;

  scases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = -20;

  scases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = +20;

  scases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 },
    { Figure::TypeRook, Figure::ColorWhite, 1 } })] = -20;

  scases_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 },
    { Figure::TypeRook, Figure::ColorBlack, 1 } })] = +20;
}

void SpecialCasesDetector::initComplex()
{
  auto bishops_vs_knight = [](Board const& board, Figure::Color bishopColor)
  {
    auto knightColor = Figure::otherColor(bishopColor);
    int npos = _lsb64(board.fmgr().knight_mask(knightColor));
    int kpos = _lsb64(board.fmgr().king_mask(knightColor));
    ScoreType score = 70;
    score += distanceCounter().getDistance(npos, 32);
    score += distanceCounter().getDistance(npos, 32) * 2;
    if(bishopColor == Figure::ColorBlack)
      score = -score;
    return score;
  };

  complexScases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = [bishops_vs_knight](Board const& board)
  {
    return bishops_vs_knight(board, Figure::ColorBlack);
  };

  complexScases_[format({ { Figure::TypeBishop, Figure::ColorWhite, 2 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 } })] = [bishops_vs_knight](Board const& board)
  {
    return bishops_vs_knight(board, Figure::ColorWhite);
  };

  // force exchange 1 x 1 same-weight figures to avoid 50 moves draw
  {
    auto force_xchg = [](Board const& board)
    {
      // consider current move side
      // it should always be < 0 after consider move side again in caller
      return -10 + 20*(Figure::ColorBlack == board.getColor());
    };

    complexScases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
      { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 },
      { Figure::TypeBishop, Figure::ColorWhite, 2 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
      { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeBishop, Figure::ColorBlack, 2 },
      { Figure::TypeKnight, Figure::ColorWhite, 2 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
      { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
      { Figure::TypeBishop, Figure::ColorBlack, 1 },
      { Figure::TypeKnight, Figure::ColorWhite, 1 },
      { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
      { Figure::TypeKnight, Figure::ColorWhite, 1 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeKnight, Figure::ColorBlack, 2 },
      { Figure::TypeKnight, Figure::ColorWhite, 2 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeRook, Figure::ColorBlack, 1 },
      { Figure::TypeRook, Figure::ColorWhite, 1 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeRook, Figure::ColorBlack, 2 },
      { Figure::TypeRook, Figure::ColorWhite, 2 } })] = force_xchg;

    complexScases_[format({ { Figure::TypeQueen, Figure::ColorBlack, 1 },
      { Figure::TypeQueen, Figure::ColorWhite, 1 } })] = force_xchg;
  }
}

void SpecialCasesDetector::initWinnerLoser()
{
  auto pawnOnly = [](Board const& board, Figure::Color pawnColor)
  {
    ScoreType score{+10};
    auto loserColor = Figure::otherColor(pawnColor);
    auto xpawnColor = pawnColor;
    auto moveColor = board.getColor();
    int p  = _lsb64(board.fmgr().pawn_mask(pawnColor));
    int kw = _lsb64(board.fmgr().king_mask(pawnColor));
    int kl = _lsb64(board.fmgr().king_mask(loserColor));
    if(pawnColor == Figure::ColorBlack)
    {
      p  = Figure::mirrorIndex_[p];
      kw = Figure::mirrorIndex_[kw];
      kl = Figure::mirrorIndex_[kl];
      moveColor = Figure::otherColor(moveColor);
      xpawnColor = Figure::otherColor(xpawnColor);
    }

    //SBoard<8> sboard;
    //sboard.initEmpty(moveColor);
    //sboard.addFigure(xpawnColor, Figure::TypePawn, p);
    //sboard.addFigure(xpawnColor, Figure::TypeKing, kw);
    //sboard.addFigure(Figure::otherColor(xpawnColor), Figure::TypeKing, kl);
    //sboard.invalidate();
    //std::string sfen = toFEN(sboard);

    Index index(p);
    if(kpk_[kw][kl][moveColor] & (1ULL<<p))
    {
      score = 2*Figure::figureWeight_[Figure::TypePawn] + pawn_colored_y_[xpawnColor][index.y()] * 20;
    }
    if(pawnColor == Figure::ColorBlack)
      score = -score;
    return score;
  };
  winnerLoser_[format({ { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [pawnOnly](Board const& board)
  {
    return pawnOnly(board, Figure::ColorBlack);
  };
  winnerLoser_[format({ { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [pawnOnly](Board const& board)
  {
    return pawnOnly(board, Figure::ColorWhite);
  };

  auto bishopKnightMat = [](Board const& board, Figure::Color winnerColor)
  {
    auto loserColor = Figure::otherColor(winnerColor);
    int kw = _lsb64(board.fmgr().king_mask(winnerColor));
    int kl = _lsb64(board.fmgr().king_mask(loserColor));
    int bp = _lsb64(board.fmgr().bishop_mask(winnerColor));
    int kn = _lsb64(board.fmgr().knight_mask(winnerColor));
    int dist = distanceCounter().getDistance(kw, kl);
    ScoreType score = board.fmgr().weight(winnerColor) - dist;
    int kp = kl;
    if(FiguresCounter::s_whiteColors_[bp])
    {
      kp = Figure::mirrorIndex_[kp];
    }
    score += bishopKnightMat_[kp];
    int ndist = distanceCounter().getDistance(kn, kl);
    score -= ndist >> 1;
    return score;
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 } })] = [bishopKnightMat](Board const& board)
  {
    return bishopKnightMat(board, Figure::ColorBlack);
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 } })] = [bishopKnightMat](Board const& board)
  {
    return bishopKnightMat(board, Figure::ColorWhite);
  };

  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return -20;
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return +20;
  };

  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return -20;
  };
  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return +20;
  };

  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return -20;
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return +20;
  };

  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [](Board const& board)
  {
    return -20;
  };
  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [](Board const& board)
  {
    return +20;
  };

  auto figure_vs_pawns = [](Board const& board, Figure::Color winnerColor)
  {
    ScoreType score = board.fmgr().pawns(winnerColor) * 10;
    auto pmask = board.fmgr().pawn_mask(winnerColor);
    for(; pmask;)
    {
      Index p(clear_lsb(pmask));
      int y = winnerColor ? p.y() : 7 - p.y();
      score += y * 5;
    }
    if(winnerColor == Figure::ColorBlack)
      score = -score;
    return score;
  };
  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorBlack);
  };
  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorWhite);
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 1 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorBlack);
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 1 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorWhite);
  };

  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 2 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorBlack);
  };
  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 2 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorWhite);
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 2 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorBlack);
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 2 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorWhite);
  };

  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 3 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorBlack);
  };
  winnerLoser_[format({ { Figure::TypeBishop, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 3 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorWhite);
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorWhite, 1 },
    { Figure::TypePawn, Figure::ColorBlack, 3 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorBlack);
  };
  winnerLoser_[format({ { Figure::TypeKnight, Figure::ColorBlack, 1 },
    { Figure::TypePawn, Figure::ColorWhite, 3 } })] = [figure_vs_pawns](Board const& board)
  {
    return figure_vs_pawns(board, Figure::ColorWhite);
  };
}

boost::optional<ScoreType> SpecialCasesDetector::eval(Board const& board) const
{
  auto const& fmgr = board.fmgr();
  Scase sc{};
  for(int c = Figure::ColorBlack; c <= Figure::ColorWhite; ++c)
  {
    for(int t = Figure::TypePawn; t < Figure::TypeKing; ++t)
    {
      auto type  = (Figure::Type)t;
      auto color = (Figure::Color)c;
      sc |= ((uint64)fmgr.tcount(type, color)) << calc_shift(type, color);
    }
  }
  {
    auto iter = scases_.find(sc);
    if(iter != scases_.end())
      return iter->second;
  }
  {
    auto iter = complexScases_.find(sc);
    if(iter != complexScases_.end())
      return (iter->second)(board);
  }
  if(board.isWinnerLoser())
  {
    auto iter = winnerLoser_.find(sc);
    if(iter != winnerLoser_.end())
      return (iter->second)(board);
  }
  return boost::none;
}

}