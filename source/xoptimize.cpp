#include <xoptimize.h>
#include <MovesGenerator.h>

namespace NEngine
{

int64 x_movesCounter = 0;

void xcaptures(Board& board, int depth)
{
  if(board.drawState() || board.countReps() > 1 || depth < -10)
    return;
  TacticalGenerator tg(board, Figure::TypeNone, depth);
  for(;;)
  {
    auto* pmove = tg.test_move();
    if(!pmove)
      break;
    auto& move = *pmove;
    if(!board.validateMove(move))
      continue;
    board.makeMove(move);
    x_movesCounter++;
    xcaptures(board, depth-1);
    board.unmakeMove();
  }
}

void xsearch(Board& board, int depth)
{
  if(board.drawState() || board.countReps() > 1)
    return;
  if(depth <= 0)
  {
    xcaptures(board, depth);
    return;
  }
  FastGenerator fg(board);
  for(;;)
  {
    auto* pmove = fg.test_move();
    if(!pmove)
      break;
    auto& move = *pmove;
    if(!board.validateMove(move))
      continue;
    board.makeMove(move);
    x_movesCounter++;
    xsearch(board, depth-1);
    board.unmakeMove();
  }
}

} // NEngine