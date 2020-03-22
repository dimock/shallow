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

void Engine::SearchContext::reset()
{
  sdata_.reset();
  sres_.reset();
  for (auto& pls : plystack_)
    pls.clearKiller();
  stop_ = false;
}

Engine::SearchContext& Engine::SearchContext::operator = (SearchContext const& other)
{
  board_ = static_cast<Board>(other.board_);
  for(size_t i = 0; i < plystack_.size(); ++i)
    plystack_[i] = other.plystack_[i];
  for(size_t i = 0; i < moves_.size(); ++i)
    moves_[i] = other.moves_[i];
  sdata_ = other.sdata_;
  sres_ = other.sres_;
  stop_ = other.stop_;
  return *this;
}

//////////////////////////////////////////////////////////////////////////
Engine::Engine() :
#ifdef USE_HASH
  hash_(0)
#endif
{
  setMemory(HASH_SIZE_DEFAULT);
  setThreadsNumber(N_THREADS_DEFAULT);

  initGlobals();
}

//////////////////////////////////////////////////////////////////////////
void Engine::needUpdate()
{
  updateRequested_ = true;
}

void Engine::setMemory(int mb)
{
  if(mb < 1)
    return;

#ifdef USE_HASH
  int bytesN = mb*1024*1024;
  int hbucketSize = sizeof(HBucket);
  int hsize = log2((uint64)(bytesN)/hbucketSize);
  hash_.resize(hsize);
#endif
}

void Engine::setThreadsNumber(int n)
{
  if (n < 1)
    return;

  if (n > N_THREADS_MAX)
    n = N_THREADS_MAX;

  if (!scontexts_.empty())
  {
    // stash/restore existing context
    auto sctx0 = scontexts_.at(0);
    scontexts_.resize(n);
    scontexts_.at(0) = sctx0;
  }
  else
  {
    scontexts_.resize(n);
  }
  
  for (auto& scontext : scontexts_)
  {
    scontext.eval_.initialize(&scontext.board_);
  }
}

bool Engine::fromFEN(std::string const& fen)
{
  if (scontexts_.empty())
    return false;

  for(auto& sctx : scontexts_)
    sctx.stop_ = false;

  SBoard<Board, UndoInfo, Board::GameLength> tboard(scontexts_.at(0).board_);

  // verify FEN first
  if(!NEngine::fromFEN(fen, tboard))
    return false;

  clear_history();

  return NEngine::fromFEN(fen, scontexts_.at(0).board_);
}

void Engine::setBoard(Board const& board)
{
  if (scontexts_.empty())
    return;
  for (auto& sctx : scontexts_)
    sctx.stop_ = false;
  clear_history();
  scontexts_.at(0).board_ = board;
}

std::string Engine::toFEN() const
{
  return NEngine::toFEN(scontexts_.at(0).board_);
}

void Engine::clearHash()
{
#ifdef USE_HASH
  hash_.clear();
#endif
}

void Engine::reset()
{
  for (auto& sctx : scontexts_)
  {
    sctx.reset();
  }

  clear_history();

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

void Engine::pleaseStop(int ictx)
{
  if (ictx != 0)
    return;
  scontexts_.at(ictx).stop_ = true;
}

bool Engine::checkForStop(int ictx)
{
  if (ictx != 0)
    return false;
  auto& sdata = scontexts_.at(ictx).sdata_;
  if(sdata.totalNodes_ && !(sdata.totalNodes_ & TIMING_FLAG))
  {
    if(sparams_.timeLimit_ > NTime::duration(0))
      testTimer(ictx);
    else
      testInput(ictx);
  }
  return scontexts_.at(ictx).stop_;
}

void Engine::testTimer(int ictx)
{
  if (ictx != 0)
    return;
  auto& sdata = scontexts_.at(ictx).sdata_;
  if((NTime::now() - sdata.tstart_) > sparams_.timeLimit_)
    pleaseStop(ictx);

  testInput(ictx);
}

void Engine::testInput(int ictx)
{
  if (ictx != 0)
    return;
  auto& sdata = scontexts_.at(ictx).sdata_;
  if(callbacks_.queryInput_)
  {
    (callbacks_.queryInput_)();
  }

  if(updateRequested_)
  {
    updateRequested_ = false;
    if (callbacks_.sendStats_)
    {
      SearchData sdataTotal = sdata;
      for (size_t i = 1; i < scontexts_.size(); ++i)
        sdataTotal.nodesCount_ += scontexts_.at(i).sdata_.nodesCount_;
      (callbacks_.sendStats_)(sdataTotal);
    }
  }
}

void Engine::assemblePV(int ictx, const Move & move, bool checking, int ply)
{
  if(ply >= MaxPly-1)
    return;

  scontexts_.at(ictx).plystack_[ply].pv_[ply] = move;
  scontexts_.at(ictx).plystack_[ply].pv_[ply+1] = Move{true};

  for(int i = ply+1; i < MaxPly-1; ++i)
  {
    scontexts_.at(ictx).plystack_[ply].pv_[i] = scontexts_.at(ictx).plystack_[ply+1].pv_[i];
    if(!scontexts_.at(ictx).plystack_[ply].pv_[i])
    {
      break;
    }
  }
}

}