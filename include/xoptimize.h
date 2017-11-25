#pragma once

#include <Board.h>

namespace NEngine
{

extern int64 x_movesCounter;
void xsearch(Board& board, int depth);
bool xverifyMoves(Board&);

} // NEngine