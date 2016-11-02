/*************************************************************
Helpers.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once


#include <Board.h>

namespace NEngine
{
struct Move;
class Board;

enum class eMoveNotation
{
  mnUnknown,
  mnSmith,
  mnSAN
};

eMoveNotation detectNotation(std::string const& str);

/// e2e4 - Smith notation
std::string moveToStr(Move const& move, bool wbf);
Move strToMove(std::string const& str, Board const& board);

/// Rxf5+ - Standard algebraic notation
Move parseSAN(Board const& board, std::string const& str);
std::string printSAN(Board & board, const Move & move);

} // NEngine
