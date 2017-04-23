#include <SpecialCases.h>
#include <Board.h>

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
}

SpecialCasesDetector::SpecialCasesDetector()
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
  auto iter = scases_.find(sc);
  if(iter != scases_.end())
    return iter->second;
  return boost::none;
}

}