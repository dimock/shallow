/*************************************************************
Board.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <MovesTable.h>
#include <FigureDirs.h>
#include <SpecialCases.h>
#include <EvalCoefficients.h>

namespace NEngine
{

namespace details
{
  extern EvalCoefficients*            g_evalCoeffs_;
  extern DeltaPosCounter const*       g_deltaPosCounter_;
  extern BetweenMask const*           g_betweenMasks_;
  extern DistanceCounter const*       g_distanceCounter_;
  extern MovesTable const*            g_movesTable_;
  extern FigureDir const*             g_figureDir_;
  extern PawnMasks const*             g_pawnMasks_;
  extern SpecialCasesDetector const*  g_specialCases_;
}

void initGlobals();


inline MovesTable const& movesTable()
{
  return *details::g_movesTable_;
}

inline FigureDir const& figureDir()
{
  return *details::g_figureDir_;
}

inline PawnMasks const& pawnMasks()
{
  return *details::g_pawnMasks_;
}

inline BetweenMask const& betweenMasks()
{
  return *details::g_betweenMasks_;
}

inline DeltaPosCounter const& deltaPosCounter()
{
  return *details::g_deltaPosCounter_;
}

inline DistanceCounter const& distanceCounter()
{
  return *details::g_distanceCounter_;
}

inline SpecialCasesDetector const& specialCases()
{
  return *details::g_specialCases_;
}

inline EvalCoefficients const& evalCoeffs()
{
  return *details::g_evalCoeffs_;
}

inline EvalCoefficients& evalCoeffs0()
{
  return *details::g_evalCoeffs_;
}

} // NEngine
