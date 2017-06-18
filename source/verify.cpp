/*************************************************************
  verify.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <engine.h>
#include <MovesGenerator.h>
#include <Helpers.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>

namespace NEngine
{

////////////////////////////////////////////////////////////////////////
void Engine::saveHash(std::string const&  fname) const
{
  if(fname.empty())
    return;
  std::string gfname, cfname, bfname, hfname;
  gfname = fname + "_g.hash";
  cfname = fname + "_c.hash";
  bfname = fname + "_b.hash";
  hfname = fname + "_h.hash";

#ifdef USE_HASH
  hash_.save(gfname);
#endif

  std::ofstream ofs(bfname, std::ios::out);
  save(scontexts_[0].board_, ofs);
  save_history(hfname);
}

void Engine::loadHash(std::string const& fname)
{
  std::string gfname, cfname, bfname, hfname;
  gfname = fname + "_g.hash";
  cfname = fname + "_c.hash";
  bfname = fname + "_b.hash";
  hfname = fname + "_h.hash";

#ifdef USE_HASH
  hash_.load(gfname);
#endif

  //std::ifstream ifs(bfname, std::ios::in);
  //load(scontexts_[0].board_, ifs);
  load_history(hfname);
}

////////////////////////////////////////////////////////////////////////////
/// for DEBUG
////////////////////////////////////////////////////////////////////////////
void Engine::logPV()
{
  if(!callbacks_.slog_)
    return;

  auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&tm), "%d:%m:%Y %H:%M:%S ");

  SBoard<Board, UndoInfo, Board::GameLength> board(scontexts_[0].board_);

  oss << "iter " << sdata_.depth_ << " ";
  if(sdata_.best_)
  {
    auto strbest = printSAN(board, sdata_.best_);
    if(!strbest.empty())
    {
      oss << " bm " << strbest << " ";
    }
  }

  oss << "pv ";
  for(int i = 0; i < MaxPly; ++i)
  {
    auto const& move = scontexts_[0].plystack_[0].pv_[i];
    if(!move)
      break;
    auto str = printSAN(board, move);
    if(str.empty())
      break;
    board.makeMove(move);
    oss << str << " ";
  }
  (*callbacks_.slog_) << oss.str() << std::endl;
}

void Engine::logMovies()
{
  if(!callbacks_.slog_)
    return;

  SBoard<Board, UndoInfo, Board::GameLength> board(scontexts_[0].board_);

  auto fen = NEngine::toFEN(board);

  auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&tm), "%d:%m:%Y %H:%M:%S ");
  oss << " position " << fen << " ";
  oss << " halfmovies count " << scontexts_[0].board_.halfmovesCount() << " ";
  oss << "iter " << sdata_.depth_ << " ";
  oss << "movies ";

  for(int i = 0; i < sdata_.numOfMoves_; ++i)
  {
    if(checkForStop())
      break;

    Move move = scontexts_[0].moves_[i];
    auto str = printSAN(board, move);
    if(str.empty())
      break;
    oss << str << " ";
  }
  (*callbacks_.slog_) << oss.str() << std::endl;
}

void Engine::findSequence(int ictx, const Move & move, int ply, int depth,
  int counter, ScoreType alpha, ScoreType betta) const
{
  struct MOVE { int from_, to_; };
  bool identical = false;
  static MOVE sequence[] = {
    {52, 45},
    {9, 8},
     };

  if ( ply < sizeof(sequence)/sizeof(MOVE) && move.from() == sequence[ply].from_ && move.to() == sequence[ply].to_ )
  {
    for (int i = ply; i >= 0; --i)
    {
      identical = true;
      int j = ply-i;
      if ( j >= scontexts_[ictx].board_.halfmovesCount() )
        break;
      const UndoInfo & undo = scontexts_[ictx].board_.reverseUndo(j);
      if ( undo.move_.from() != sequence[i].from_ || undo.move_.to() != sequence[i].to_ )
      {
        identical = false;
        break;
      }
    }
  }

  if ( identical )
  {
    //if ( sdata_.depth_ == 5*ONE_PLY && ply == 1 )
    //{
    //  int ttt = 0;
    //}
    std::ostringstream oss;
    NEngine::save(scontexts_[ictx].board_, oss, false);
    oss << "PLY: " << ply << std::endl;
    oss << "depth_ = " << sdata_.depth_ << "; depth = " << depth << "; ply = "
      << ply << "; alpha = " << alpha << "; betta = " << betta << "; counter = " << counter << std::endl;
    oss << "===================================================================" << std::endl << std::endl;

    std::ofstream ofs("sequence.txt", std::ios_base::app);
    ofs << oss.str();
  }
}

} // NEngine
