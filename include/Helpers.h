/*************************************************************
Helpers.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#undef _USE_LOG

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

bool load(Board &, std::istream &);
bool save(const Board &, std::ostream &, bool = true);

/// initialize from FEN
bool fromFEN(std::string const& i_fen, Board& board);

/// save current position to FEN
std::string toFEN(Board const& board);

#ifdef _USE_LOG
void addLog(std::string const& str);
#else
inline void addLog(std::string const&) {}
#endif

} // NEngine
