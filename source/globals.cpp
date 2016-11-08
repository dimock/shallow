/*************************************************************
Board.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <globals.h>
#include <memory>

namespace NEngine
{

namespace details
{
  DeltaPosCounter const* g_deltaPosCounter_{};
  BetweenMask const*     g_betweenMasks_{};
  DistanceCounter const* g_distanceCounter_{};
  MovesTable const*      g_movesTable_{};
  FigureDir const*       g_figureDir_{};
  PawnMasks const*       g_pawnMasks_{};
}

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

    details::g_deltaPosCounter_ = deltaPosCounter_.get();
    details::g_betweenMasks_ = betweenMasks_.get();
    details::g_distanceCounter_ = distanceCounter_.get();
    details::g_movesTable_ = movesTable_.get();
    details::g_figureDir_ = figureDir_.get();
    details::g_pawnMasks_ = pawnMasks_.get();
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

} // NEngine
