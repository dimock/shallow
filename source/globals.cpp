/*************************************************************
Board.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <globals.h>
#include <memory>

namespace NEngine
{

namespace
{
class Globals
{
  std::unique_ptr<DeltaPosCounter> deltaPosCounter_;
  std::unique_ptr<BetweenMask>     betweenMasks_;
  std::unique_ptr<DistanceCounter> distanceCounter_;
  std::unique_ptr<MovesTable>      movesTable_;
  std::unique_ptr<FigureDir>       figureDir_;
  std::unique_ptr<PawnMasks>       pawnMasks_;

public:
  Globals()
  {
    deltaPosCounter_ = std::unique_ptr<DeltaPosCounter>(new DeltaPosCounter);
    betweenMasks_    = std::unique_ptr<BetweenMask>(new BetweenMask(*deltaPosCounter_));
    distanceCounter_ = std::unique_ptr<DistanceCounter>(new DistanceCounter);
    movesTable_      = std::unique_ptr<MovesTable>(new MovesTable);
    figureDir_       = std::unique_ptr<FigureDir>(new FigureDir);
    pawnMasks_       = std::unique_ptr<PawnMasks>(new PawnMasks);
  }

  MovesTable const& movesTable()
  {
    return *movesTable_;
  }

  FigureDir const& figureDir()
  {
    return *figureDir_;
  }

  PawnMasks const& pawnMasks()
  {
    return *pawnMasks_;
  }

  BetweenMask const& betweenMasks()
  {
    return *betweenMasks_;
  }

  DeltaPosCounter const& deltaPosCounter()
  {
    return *deltaPosCounter_;
  }

  DistanceCounter const& distanceCounter()
  {
    return *distanceCounter_;
  }
};

std::unique_ptr<Globals> globals_;

} // end of {}

void initGlobals()
{
  if(!globals_)
  {
    globals_ = std::unique_ptr<Globals>(new Globals);
  }
}

MovesTable const& movesTable()
{
  return globals_->movesTable();
}

FigureDir const& figureDir()
{
  return globals_->figureDir();
}

PawnMasks const& pawnMasks()
{
  return globals_->pawnMasks();
}

BetweenMask const& betweenMasks()
{
  return globals_->betweenMasks();
}

DeltaPosCounter const& deltaPosCounter()
{
  return globals_->deltaPosCounter();
}

DistanceCounter const& distanceCounter()
{
  return globals_->distanceCounter();
}

} // NEngine
