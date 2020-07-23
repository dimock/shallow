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
  save(scontexts_.at(0).board_, ofs);
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
  //load(scontexts_.at(0).board_, ifs);
  load_history(hfname);
}

////////////////////////////////////////////////////////////////////////////
/// for DEBUG
////////////////////////////////////////////////////////////////////////////
void Engine::logPV(int ictx)
{
  if(!callbacks_.slog_)
    return;

  auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&tm), "%d:%m:%Y %H:%M:%S ");

  auto& sdata = scontexts_.at(ictx).sdata_;
  SBoard<Board, UndoInfo, Board::GameLength> board(scontexts_.at(ictx).board_);

  oss << "iter " << sdata.depth_ << " ";
  if(sdata.best_)
  {
    auto strbest = printSAN(board, sdata.best_);
    if(!strbest.empty())
    {
      oss << " bm " << strbest << " ";
    }
  }

  oss << "pv ";
  for(int i = 0; i < MaxPly; ++i)
  {
    auto const& move = scontexts_.at(ictx).plystack_[0].pv_[i];
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

void Engine::logMovies(int ictx)
{
  if(!callbacks_.slog_)
    return;

  auto& sdata = scontexts_.at(ictx).sdata_;
  SBoard<Board, UndoInfo, Board::GameLength> board(scontexts_.at(ictx).board_);

  auto fen = NEngine::toFEN(board);

  auto tm = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::ostringstream oss;
  oss << std::put_time(std::localtime(&tm), "%d:%m:%Y %H:%M:%S ");
  oss << " position " << fen << " ";
  oss << " halfmovies count " << scontexts_.at(ictx).board_.halfmovesCount() << " ";
  oss << "iter " << sdata.depth_ << " ";
  oss << "movies ";

  for(int i = 0; i < sdata.numOfMoves_; ++i)
  {
    if(checkForStop(ictx))
      break;

    Move move = scontexts_.at(ictx).moves_[i];
    auto str = printSAN(board, move);
    if(str.empty())
      break;
    oss << str << " ";
  }
  (*callbacks_.slog_) << oss.str() << std::endl;
}


#ifdef RELEASEDEBUGINFO

static int TEST_DEPTH_MIN = 8;
static int TEST_DEPTH_MAX = 12;

bool compare_depth(int depth)
{
  return depth >= TEST_DEPTH_MIN && depth <= TEST_DEPTH_MAX;
}

bool Engine::findSequence(int ictx, int ply, bool exactMatch) const
{
  bool identical = false;
  auto& board = scontexts_.at(ictx).board_;
  int depth = scontexts_.at(ictx).sdata_.depth_;

  if (!compare_depth(depth))
    return false;

  const auto& sequence = board.stestMoves_;
  if((ply < sequence.size() && !exactMatch) || (ply == sequence.size()-1 && exactMatch))
  {
    for(int i = ply; i >= 0; --i)
    {
      identical = true;
      int j = ply-i;
      if(j >= board.halfmovesCount())
        break;
      const UndoInfo & undo = board.reverseUndo(j);
      auto smove = sequence[i];
      if (!smove.empty()) {
        if (undo.is_nullmove()) {
          if (smove != "null")
          {
            identical = false;
            break;
          }
        }
        else {
          auto move = strToMove(smove);
          if (undo.move_.from() != move.from() || undo.move_.to() != move.to())
          {
            identical = false;
            break;
          }
        }
      }
    }
  }

  //if(identical)
  //{
  //  std::ostringstream oss;
  //  NEngine::save(board, oss, false);
  //  oss << "PLY: " << ply << std::endl;
  //  oss << "depth_ = " << sdata_.depth_ << "; depth = " << depth << "; ply = "
  //    << ply << "; alpha = " << alpha << "; betta = " << betta << "; counter = " << counter << std::endl;
  //  oss << "===================================================================" << std::endl << std::endl;

  //  std::ofstream ofs("sequence.txt", std::ios_base::app);
  //  ofs << oss.str();
  //}

  return identical;
}
#endif // RELEASEDEBUGINFO

} // NEngine
