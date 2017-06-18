#include <xoptimize.h>
#include <MovesGenerator.h>
#include <xindex.h>
#include <Helpers.h>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>

namespace NEngine
{

int64 x_movesCounter = 0;

void xcaptures(Board& board, int depth)
{
  if(board.drawState() || board.hasReps() || depth < -10)
    return;
  TacticalGenerator<Board, Move> tg(board, depth);
  //try
  //{
    for(;;)
    {
      auto* pmove = tg.next();
      if(!pmove)
        break;
      auto& move = *pmove;
      if(!board.validateMove(move))
        continue;
      //board.see(move);
      //Board brd{ board };
      //std::string fen = toFEN(board);
      board.makeMove(move);
      x_movesCounter++;
      xcaptures(board, depth-1);
      board.unmakeMove(move);
      //X_ASSERT(brd != board, "board was not correctly restored");
    }
  //}
  //catch(std::exception const& e)
  //{
  //  throw std::runtime_error(toFEN(board) + "; " + e.what());
  //}
}

void xsearch(Board& board, int depth)
{
  if(board.drawState() || board.hasReps())
    return;
  if(depth <= 0)
  {
    xcaptures(board, depth);
    return;
  }
  //try
  //{
    FastGenerator<Board, Move> fg(board);
    for(;;)
    {
      auto* pmove = fg.next();
      if(!pmove)
        break;
      auto& move = *pmove;
      if(!board.validateMove(move))
        continue;
      //board.see(move);
      //std::string fen = toFEN(board);
      //Board brd{ board };

      board.makeMove(move);
      x_movesCounter++;
      xsearch(board, depth-1);
      board.unmakeMove(move);

      //X_ASSERT(brd != board, "board was not correctly restored");
    }
  //}
  //catch(std::exception const& e)
  //{
  //  throw std::runtime_error(toFEN(board) + "; " + e.what());
  //}
}

template <class MOVE>
bool compare(std::vector<MOVE> const& moves1, std::vector<MOVE> const& moves2)
{
  //if(moves1.size() != moves2.size())
  //  return false;
  for(auto const& move1 : moves1)
  {
    if(std::find_if(moves2.begin(), moves2.end(), [&move1](Move const& m) { return m == move1; })
       == moves2.end())
       return false;
  }
  for(auto move2 : moves2)
  {
    if(std::find_if(moves1.begin(), moves1.end(), [&move2](Move const& m) { return m == move2; })
       == moves1.end())
       return false;
  }
  return true;
}

bool xverifyMoves(Board& board)
{
  bool ok = false;
  std::vector<Move> fmoves;
  FastGenerator<Board, Move> fg(board);
  for(;;)
  {
    auto* pmove = fg.next();
    if(!pmove)
      break;
    auto& move = *pmove;
    X_ASSERT(board.validateMove(move) != board.validateMoveBruteforce(move), "validate move error");
    if(!board.validateMove(move))
      continue;
    Board brd{ board };
    brd.makeMove(move);
    brd.unmakeMove(move);
    X_ASSERT(board != brd, "board was not restored correctly after move");
    fmoves.push_back(move);
    ok = true;
  }
  auto moves = generate<Board, Move>(board);
  std::vector<Move> etalons;
  std::vector<Move> checks;
  auto ocolor = Figure::otherColor(board.color());
  for(auto& move : moves)
  {
    if(!board.validateMoveBruteforce(move))
      continue;
    if(move.new_type() == Figure::TypeBishop || move.new_type() == Figure::TypeRook)
      continue;
    if(move.new_type() == Figure::TypeKnight
       && !(movesTable().caps(Figure::TypeKnight, move.to()) & set_mask_bit(board.kingPos(ocolor)))
       )
       continue;
    etalons.push_back(move);
    Board brd{ board };
    brd.makeMove(move);
    if(brd.underCheck() && !brd.lastUndo().capture() && !move.new_type())
      checks.push_back(move);
    brd.unmakeMove(move);
    X_ASSERT(board != brd, "board was not restored correctly after move");
  }
  if(!compare(etalons, fmoves))
    return false;
  if(board.underCheck())
    return true;
  ChecksGenerator<Board, Move> cg(board);
  cg.generate();
  std::vector<Move> checks2;
  for(auto& move : cg.moves_)
  {
    X_ASSERT(board.validateMoveBruteforce(move) != board.validateMove(move), "inalid move validator");
    if(!board.validateMoveBruteforce(move))
      continue;
    if(std::find_if(etalons.begin(), etalons.end(), [&move](Move const& m) { return m == move; })
       == etalons.end())
       return false;
    Board brd{ board };
    brd.makeMove(move);
    X_ASSERT(!brd.underCheck(), "check was not detected");
    checks2.push_back(move);
  }
  if(!compare(checks, checks2))
    return false;
  return true;
}

} // NEngine
