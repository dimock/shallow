/*************************************************************
Board.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <MovesTable.h>
#include <FigureDirs.h>

namespace NEngine
{
  void initGlobals();

  MovesTable      const& movesTable();
  FigureDir       const& figureDir();
  PawnMasks       const& pawnMasks();
  BetweenMask     const& betweenMasks();
  DeltaPosCounter const& deltaPosCounter();
  DistanceCounter const& distanceCounter();

} // NEngine
