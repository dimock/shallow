/*************************************************************
  engine.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <engine.h>
#include <MovesGenerator.h>
#include <Helpers.h>

namespace NEngine
{

void SearchParams::reset()
{
  timeLimit_ = NTime::duration(0);
  depthMax_ = 0;
  analyze_mode_ = false;
  scoreLimit_ = Figure::MatScore;
}

//////////////////////////////////////////////////////////////////////////
Engine::Engine() :
  stop_(false)
#ifdef USE_HASH
  , hash_(0)
  , ehash_(0)
#endif
{
  setMemory(options_.hash_size_);

  initGlobals();

  for(auto& scontext : scontexts_)
  {
#ifdef USE_HASH
    scontext.eval_.initialize(&scontext.board_, &ehash_, &hash_);
#else
    scontext.eval_.initialize(&scontext.board_, nullptr, nullptr);
#endif
  }
}

//////////////////////////////////////////////////////////////////////////
void Engine::needUpdate()
{
  updateRequested_ = true;
}

void Engine::setOptions(xOptions const& opts)
{
  options_ = opts;

  setMemory(options_.hash_size_);
}

void Engine::setMemory(int mb)
{
  if(mb < 1)
    return;

  int bytesN = mb*1024*1024;

#ifdef USE_HASH
  int hitemSize = sizeof(HItem);
  int hsize2 = log2((uint64)(bytesN)/hitemSize) - 3;
  if(hsize2 >= 10)
  {
    hash_.resize(hsize2);
    ehash_.resize(hsize2+1);
  }
#endif
}


bool Engine::fromFEN(std::string const& fen)
{
  stop_ = false;

  SBoard<Board, UndoInfo, Board::GameLength> tboard(scontexts_[0].board_);

  // verify FEN first
  if(!NEngine::fromFEN(fen, tboard))
    return false;

  clear_history();

  return NEngine::fromFEN(fen, scontexts_[0].board_);
}

void Engine::setBoard(Board const& board)
{
  stop_ = false;
  clear_history();
  scontexts_[0].board_ = board;
}

std::string Engine::toFEN() const
{
  return NEngine::toFEN(scontexts_[0].board_);
}

void Engine::clearHash()
{
#ifdef USE_HASH
  hash_.clear();
  ehash_.clear();
#endif
}

void Engine::reset()
{
  sdata_.reset();
  stop_ = false;
  clear_history();

  for(int i = 0; i < MaxPly; ++i)
    scontexts_[0].plystack_[i].clearKiller();
}

void Engine::setCallbacks(xCallback cs)
{
  callbacks_ = cs;
}

void Engine::setAnalyzeMode(bool analyze)
{
  sparams_.analyze_mode_ = analyze;
}

void Engine::setTimeLimit(NTime::duration const& tm)
{
  sparams_.timeLimit_ = tm;
}

void Engine::setMaxDepth(int d)
{
  if(d >= 1 && d <= 32)
    sparams_.depthMax_ = d;
}

void Engine::setScoreLimit(ScoreType score)
{
  sparams_.scoreLimit_ = score;
}

void Engine::pleaseStop()
{
  stop_ = true;
}

bool Engine::checkForStop()
{
  if(sdata_.totalNodes_ && !(sdata_.totalNodes_ & TIMING_FLAG))
  {
    if(sparams_.timeLimit_ > NTime::duration(0))
      testTimer();
    else
      testInput();
  }
  return stop_;
}

void Engine::testTimer()
{
  if((NTime::now() - sdata_.tstart_) > sparams_.timeLimit_)
    pleaseStop();

  testInput();
}

void Engine::testInput()
{
  if(callbacks_.queryInput_)
  {
    (callbacks_.queryInput_)();
  }

  if(updateRequested_)
  {
    updateRequested_ = false;
    if(callbacks_.sendStats_)
      (callbacks_.sendStats_)(sdata_);
  }
}

void Engine::assemblePV(int ictx, const Move & move, bool checking, int ply)
{
  if(ply >= MaxPly-1)
    return;

  scontexts_[ictx].plystack_[ply].pv_[ply] = move;
  scontexts_[ictx].plystack_[ply].pv_[ply+1] = Move{true};

  for(int i = ply+1; i < MaxPly-1; ++i)
  {
    scontexts_[ictx].plystack_[ply].pv_[i] = scontexts_[ictx].plystack_[ply+1].pv_[i];
    if(!scontexts_[ictx].plystack_[ply].pv_[i])
    {
      break;
    }
  }
}

}