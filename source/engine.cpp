/*************************************************************
  engine.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <engine.h>
#include <MovesGenerator.h>

namespace NEngine
{

void SearchParams::reset()
{
  timeLimit_ = NTime::duration(0);
  depthMax_ = 0;
  analyze_mode_ = false;
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
Engine::SearchContext::SearchContext()
{
  undoStack_ = new UndoInfo[Board::GameLength];
  board_.set_undoStack(undoStack_);
}

Engine::SearchContext::~SearchContext()
{
  board_.set_undoStack(0);
  delete[] undoStack_;
}

//////////////////////////////////////////////////////////////////////////
Engine::Engine() :
  stop_(false)
#ifdef USE_HASH
  , hash_(20)
  , ehash_(20)
#endif
{
  setMemory(256);

  g_deltaPosCounter = new DeltaPosCounter;
  g_betweenMasks = new BetweenMask(g_deltaPosCounter);
  g_distanceCounter = new DistanceCounter;
  g_movesTable = new MovesTable;
  g_figureDir = new FigureDir;
  g_pawnMasks_ = new PawnMasks;

  for(int i = 0; i < sizeof(scontexts_)/sizeof(SearchContext); ++i)
  {
#ifdef USE_HASH
    scontexts_[i].eval_.initialize(&scontexts_[i].board_, &ehash_);
#else
    scontexts_[i].eval_.initialize(&scontexts_[i].board_, 0);
#endif

    scontexts_[i].board_.set_MovesTable(g_movesTable);
    scontexts_[i].board_.set_FigureDir(g_figureDir);
    scontexts_[i].board_.set_DeltaPosCounter(g_deltaPosCounter);
    scontexts_[i].board_.set_DistanceCounter(g_distanceCounter);
    scontexts_[i].board_.set_BetweenMask(g_betweenMasks);
    scontexts_[i].board_.set_PawnMasks(g_pawnMasks_);
  }
}

Engine::~Engine()
{
  for(int i = 0; i < sizeof(scontexts_)/sizeof(SearchContext); ++i)
  {
    scontexts_[i].board_.set_MovesTable(0);
    scontexts_[i].board_.set_FigureDir(0);
    scontexts_[i].board_.set_DeltaPosCounter(0);
    scontexts_[i].board_.set_DistanceCounter(0);
    scontexts_[i].board_.set_BetweenMask(0);
    scontexts_[i].board_.set_PawnMasks(0);
  }

  delete g_movesTable;
  delete g_figureDir;
  delete g_betweenMasks;
  delete g_deltaPosCounter;
  delete g_distanceCounter;
  delete g_pawnMasks_;
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

  Board tboard(scontexts_[0].board_);
  UndoInfo tundo[16];
  tboard.set_undoStack(tundo);

  // verify FEN first
  if(!tboard.fromFEN(fen))
    return false;

  MovesGenerator::clear_history();

  return scontexts_[0].board_.fromFEN(fen);
}

std::string Engine::toFEN() const
{
  return scontexts_[0].board_.toFEN();
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
  MovesGenerator::clear_history();

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
  if( /*ply > sdata_.depth_ || */ply >= MaxPly-1)
    return;

  scontexts_[ictx].plystack_[ply].pv_[ply] = move;
  scontexts_[ictx].plystack_[ply].pv_[ply].checkFlag_ = checking;
  scontexts_[ictx].plystack_[ply].pv_[ply+1].clear();

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