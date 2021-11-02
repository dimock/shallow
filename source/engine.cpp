/*************************************************************
  engine.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include "engine.h"
#include "MovesGenerator.h"
#include "Helpers.h"

namespace NEngine
{

void Engine::SearchContext::reset()
{
  sdata_.reset();
  sres_.reset();
  eval_.reset();
  clearStack();
  for (auto& m : moves_) {
    m = SMove{true};
  }
  stop_ = false;
}

void Engine::SearchContext::clearStack()
{
  for (auto& pls : plystack_) {
    pls.clearKiller();
    pls.clearPV(MaxPly);
  }
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
Engine::Engine()
#ifdef USE_HASH
  : hash_(0)
#elif(defined USE_EVAL_HASH_ALL)
  :
#endif
#if ((defined USE_HASH) && (defined USE_EVAL_HASH_ALL))
  ,
#endif
#ifdef USE_EVAL_HASH_ALL
  ev_hash_(0)
#endif
{
  setThreadsNumber(N_THREADS_DEFAULT);
  initGlobals();
  setMemory(HASH_SIZE_DEFAULT);
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
  size_t bytesN = mb*1024*1024;
  size_t hitemSize = sizeof(NEngine::GHashTable::ItemType);
  size_t hitemsN = log2((BitMask)(bytesN)/hitemSize);
  hash_.resize(hitemsN);
#ifdef USE_EVAL_HASH_ALL
  int evbytesN = (int)bytesN - (1<<hitemsN)*hitemSize;
  if (evbytesN > 0) {
    size_t evitemSize = sizeof(NEngine::FHashTable::ItemType);
    size_t evitemsN = log2((BitMask)(evbytesN) / evitemSize);
    ev_hash_.resize(evitemsN);
  }
#endif
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
    auto sctx0 = scontexts_[0];
    scontexts_.resize(n);
    scontexts_[0] = sctx0;
  }
  else
  {
    scontexts_.resize(n);
  }
  
  for (auto& scontext : scontexts_)
  {
    scontext.clearStack();
    scontext.eval_.initialize(&scontext.board_
#ifdef USE_EVAL_HASH_ALL
      , &ev_hash_
#endif
    );
  }
}

bool Engine::fromFEN(std::string const& fen)
{
  if (scontexts_.empty())
    return false;

  for(auto& sctx : scontexts_)
    sctx.stop_ = false;

  SBoard<Board, UndoInfo, Board::GameLength> tboard(scontexts_[0].board_);

  // verify FEN first
  if(!NEngine::fromFEN(fen, tboard))
    return false;

  clear_history();
  for (auto& sctx : scontexts_) {
    sctx.board_.clearStack();
  }
  return NEngine::fromFEN(fen, scontexts_[0].board_);
}

void Engine::setBoard(Board const& board)
{
  if (scontexts_.empty())
    return;
  for (auto& sctx : scontexts_)
    sctx.stop_ = false;
  clear_history();
  for (auto& sctx : scontexts_) {
    sctx.board_.clearStack();
  }
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
#endif
}

void Engine::reset()
{
  sparams_.timeAdded_ = false;
  for (auto& sctx : scontexts_) {
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
  scontexts_[ictx].stop_ = true;
}

bool Engine::checkForStop(int ictx)
{
  if (ictx != 0)
    return false;
  auto& sdata = scontexts_[ictx].sdata_;
  if (sdata.totalNodes_ && !(sdata.totalNodes_ & TIMING_FLAG))
  {
    if (sparams_.timeLimit_ > NTime::duration(0))
      testTimer(ictx);
    else
      testInput(ictx);
  }
  return scontexts_[ictx].stop_;
}

void Engine::testTimer(int ictx)
{
  if (ictx != 0)
    return;
  auto& sdata = scontexts_[ictx].sdata_;
  if(((NTime::now() - sdata.tstart_) > sparams_.timeLimit_) && !tryToAddTime())
    pleaseStop(ictx);

  testInput(ictx);
}

bool Engine::tryToAddTime()
{
  if (sparams_.analyze_mode_ || !callbacks_.giveTime_) {
    return false;
  }

  auto& sctx = scontexts_[0];
  auto& sres = sctx.sres_;
  auto& sdata = sctx.sdata_;

  if ((sdata.best_ && (sdata.counter_ < sdata.numOfMoves_) &&
      (sres.best_ != sdata.best_ || sres.best_ != sres.prevBest_ || sdata.scoreBest_ < sres.score_-10)) ||
      (!sdata.best_ && (sres.best_ != sres.prevBest_)))
  {
    auto t_add = (callbacks_.giveTime_)();
    if (NTime::milli_seconds<int>(t_add) > 0)
    {
      sparams_.timeLimit_ += t_add;
      sparams_.timeAdded_ = true;
      return true;
    }
  }
  return false;
}

void Engine::testInput(int ictx)
{
  if (ictx != 0)
    return;
  auto& sdata = scontexts_[ictx].sdata_;
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
        sdataTotal.nodesCount_ += scontexts_[i].sdata_.nodesCount_;
      (callbacks_.sendStats_)(sdataTotal);
    }
  }
}

void Engine::assemblePV(int ictx, const Move move, bool checking, int ply)
{
  if(ply >= MaxPly-1)
    return;

  auto& sctx = scontexts_[ictx];
  sctx.plystack_[ply].pv_[ply] = move;
  sctx.plystack_[ply].pv_[ply+1] = Move{true};

  for(int i = ply+1; i < MaxPly-1; ++i)
  {
    sctx.plystack_[ply].pv_[i] = sctx.plystack_[ply+1].pv_[i];
    if(!sctx.plystack_[ply].pv_[i])
    {
      break;
    }
  }
}

}