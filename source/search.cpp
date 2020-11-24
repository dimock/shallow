/*************************************************************
  search.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/


#include <engine.h>
#include <MovesGenerator.h>
#include "Evaluator.h"
#include <algorithm>
#include <Helpers.h>
#include <History.h>
#include <xalgorithm.h>
#include <thread>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
bool Engine::generateStartposMoves(int ictx)
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];

  auto& sres =  sctx.sres_;
  auto& board = sctx.board_;
  auto& sdata = sctx.sdata_;
  
  // generate moves from startpos
  auto moves = generate<Board, SMove>(board);
  for (auto move : moves)
  {
    move.sort_value = 0;
    if (!board.validateMoveBruteforce(move))
      continue;
    if (board.see(move, 0))
      move.set_ok();
    if (move.new_type() || board.is_capture(move))
    {
      if (move.see_ok())
        move.sort_value = 10000000 + board.sortValueOfCap(move.from(), move.to(), (Figure::Type)move.new_type());
      else
        move.sort_value = board.sortValueOfCap(move.from(), move.to(), (Figure::Type)move.new_type()) - 10000;
    }
    else
    {
      board.makeMove(move);
      move.sort_value = sctx.eval_(-ScoreMax, +ScoreMax);
      if (!move.see_ok())
        move.sort_value -= 10000;
      board.unmakeMove(move);
    }
    sctx.moves_[sdata.numOfMoves_++] = move;
  }
  sctx.moves_[sdata.numOfMoves_] = SMove{ true };
  auto* b = sctx.moves_.data();
  auto* e = b + sdata.numOfMoves_;
  std::sort(b, e, [](SMove const& m1, SMove const& m2) { return m1 > m2; });

#ifdef USE_HASH
  auto * hitem = hash_.get(board.fmgr().hashCode());
  X_ASSERT(hitem == nullptr, "HItem not found in Hash table");
  HKeyType hk = (HKeyType)(board.fmgr().hashCode() >> (sizeof(uint64) - sizeof(HKeyType)) * 8);
  if (hitem->hkey_ == hk && hitem->move_)
  {
    SMove best{ hitem->move_.from(), hitem->move_.to(), (Figure::Type)hitem->move_.new_type() };
    X_ASSERT(!board.possibleMove(best), "move from hash is impossible");
    X_ASSERT(!board.validateMove(best), "move from hash is invalid");
    bring_to_front(b, e, best);
  }
#endif

  sres.numOfMoves_ = sdata.numOfMoves_;
  return sdata.numOfMoves_ > 0;
}

SearchResult const& Engine::result() const
{
  auto const& sctx0 = scontexts_[0];
  return sctx0.sres_;
}

bool Engine::search()
{
  if (scontexts_.empty())
    return false;

#ifdef USE_HASH
  hash_.inc();
#endif

  reset();
 
  if (!generateStartposMoves(0))
    return false;

  for (size_t ictx = 1; ictx < scontexts_.size(); ++ictx)
  {
    scontexts_[ictx] = scontexts_[0];
  }

  // start threads
  std::vector<std::unique_ptr<std::thread>> threads;
  int depth0 = depth0_+1;
  for (int ictx = 1; ictx < scontexts_.size(); ++ictx, ++depth0)
  {
    auto thread = std::unique_ptr<std::thread>(new std::thread{ [this, depth0](int ictx) {
      this->threadSearch(ictx, depth0);
    }, ictx});
    threads.emplace_back(std::move(thread));
  }

  // search in main thread
  auto bres = mainThreadSearch(0);

  for (auto& thread : threads)
  {
    thread->join();
  }

  return bres;
}

bool Engine::mainThreadSearch(int ictx)
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];

  auto& sres = sctx.sres_;
  // stash board to correctly print status later
  sres.board_ = sctx.board_;
  auto& board = sctx.board_;
  auto& sdata = sctx.sdata_;
  // copy to print stats later
  sdata.board_ = board;

  for(sdata.depth_ = depth0_; !sctx.stop_ && sdata.depth_ <= sparams_.depthMax_; ++sdata.depth_)
  {
    sctx.plystack_[0].clearPV(sparams_.depthMax_);
    sdata.restart();
    
    ScoreType score = alphaBetta0(ictx);

    if(sdata.best_)
    {
      if (sdata.counter_ == sdata.numOfMoves_)
      {
        auto t = NTime::now();
        auto dt = t - sdata.tstart_;
        sdata.tprev_ = t;

        sres.score_ = score;
        sres.prevBest_ = sres.best_;
        sres.best_ = sdata.best_;
        sres.depth_ = sdata.depth_;
        sres.nodesCount_ = sdata.nodesCount_;
        sres.totalNodes_ = sdata.totalNodes_;
        sres.depthMax_ = 0;
        sres.dt_ = dt;
        sres.counter_ = sdata.counter_;

        for (int i = 0; i < MaxPly; ++i)
        {
          sres.depthMax_ = i;
          sres.pv_[i] = sctx.plystack_[0].pv_[i];
          if (!sres.pv_[i])
            break;
        }

        X_ASSERT(sres.pv_[0] != sdata.best_, "invalid PV found");

        if (ictx == 0 && callbacks_.sendOutput_)
        {
          for (size_t j = 1; j < scontexts_.size(); ++j)
            sres.totalNodes_ += scontexts_[j].sdata_.totalNodes_;
          (callbacks_.sendOutput_)(sres);
        }

        if (!sparams_.analyze_mode_ && sparams_.timeAdded_) {
          pleaseStop(ictx);
          break;
        }
      }
    }
    else
    {
#ifdef SYNCHRONIZE_LAST_ITER
      int jcbest = -1;
      {
        std::lock_guard<std::mutex> g{ m_mutex };

        int depth_best = sdata.depth_;
        bool best_found = false;
        for (int jctx = 1; jctx < scontexts_.size(); ++jctx)
        {
          auto const& jsres = scontexts_[jctx].sres_;
          if (!jsres.best_)
            continue;
          if ((jsres.depth_ > depth_best) || (jsres.depth_ == depth_best && !best_found))
          {
            jcbest = jctx;
            depth_best = jsres.depth_;
            best_found = true;
          }
        }

        if (jcbest > 0)
        {
          sres = scontexts_[jcbest].sres_;
          sres.depth_ = sdata.depth_;
          sres.nodesCount_ = sdata.nodesCount_;
          sres.totalNodes_ = sdata.totalNodes_;
          sres.dt_ = t - sdata.tstart_;
          sdata.best_ = SMove{ sres.best_.from(), sres.best_.to(), static_cast<Figure::Type>(sres.best_.new_type()) };
          sortMoves0(ictx);
        }
      }

      if (sres.best_ && jcbest > 0)
      {
        X_ASSERT(sres.pv_[0] != sdata.best_, "invalid PV found");

        if (ictx == 0 && callbacks_.sendOutput_)
        {
          for (size_t j = 1; j < scontexts_.size(); ++j)
            sres.totalNodes_ += scontexts_[j].sdata_.totalNodes_;
          (callbacks_.sendOutput_)(sres);
        }
      }
#endif // SYNCHRONIZE_LAST_ITER
    }

    if(!sres.best_ ||
       ( (score >= sparams_.scoreLimit_-MaxPly || score <= MaxPly-sparams_.scoreLimit_) &&
         !sparams_.analyze_mode_ &&
         sres.depth_ > NumUsualAfterHorizon+1))
    {
      break;
    }
  }

  sres.totalNodes_ = sdata.totalNodes_;
  sres.dt_ = NTime::now() - sdata.tstart_;

  if(ictx == 0 && sparams_.analyze_mode_ && callbacks_.sendFinished_)
    (callbacks_.sendFinished_)(sres);

  if (ictx == 0)
  {
    // stop all threads
    for(auto& sctx : scontexts_)
    {
      sctx.stop_ = true;
    }
  }

  return sres.best_;
}

bool Engine::threadSearch(const int ictx, const int depth0)
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];

  // stash board to correctly print status later
  auto& sres  = sctx.sres_;
  sres.board_ = sctx.board_;
  auto& board = sctx.board_;
  auto& sdata = sctx.sdata_;
  // copy to print stats later
  sdata.board_ = board;

  for (sdata.depth_ = depth0; !sctx.stop_ && sdata.depth_ <= sparams_.depthMax_;)
  {
    sctx.plystack_[0].clearPV(sparams_.depthMax_);

    sdata.restart();

    ScoreType score = alphaBetta0(ictx);

    if (sdata.best_)
    {
      auto t = NTime::now();
      auto dt = t - sdata.tstart_;
      sdata.tprev_ = t;

#ifdef SYNCHRONIZE_LAST_ITER
      {
        std::lock_guard<std::mutex> g{ m_mutex };
        sres.score_ = score;
        sres.best_ = sdata.best_;
        sres.depth_ = sdata.depth_;
        sres.nodesCount_ = sdata.nodesCount_;
        sres.totalNodes_ = sdata.totalNodes_;
        sres.depthMax_ = 0;
        sres.dt_ = dt;
        sres.counter_ = sdata.counter_;

        for (int i = 0; i < MaxPly; ++i)
        {
          sres.depthMax_ = i;
          sres.pv_[i] = sctx.plystack_[0].pv_[i];
          if (!sres.pv_[i])
            break;
        }
      }
#endif // SYNCHRONIZE_LAST_ITER

      X_ASSERT(sres.pv_[0] != sdata.best_, "invalid PV found");
    }

    if (!sdata.best_ ||
       ( (score >= sparams_.scoreLimit_ - MaxPly || score <= MaxPly - sparams_.scoreLimit_) &&
         !sparams_.analyze_mode_ &&
         sres.depth_ > NumUsualAfterHorizon + 1))
    {
      break;
    }

    for (auto const& sctx : scontexts_)
      sdata.depth_ = std::max(sctx.sdata_.depth_, sdata.depth_);
    sdata.depth_++;
  }

  sres.totalNodes_ = sdata.totalNodes_;
  sres.dt_ = NTime::now() - sdata.tstart_;

  return sres.best_;
}

//////////////////////////////////////////////////////////////////////////
int Engine::depthIncrement(int ictx, Move const & move, bool pv, bool singular) const
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");

  auto& sctx = scontexts_[ictx];
  auto& board = sctx.board_;
  int depthInc = 0;

  if(board.underCheck())
    depthInc += ONE_PLY;

  if(!pv || !move.see_ok())
    return depthInc;

  if(move.new_type() || singular)
    return depthInc + ONE_PLY;

  // recapture
  if(board.halfmovesCount() > 1)
  {
    auto const& prev = board.reverseUndo(1);
    auto const& curr = board.lastUndo();

    if(prev.move_.to() == curr.move_.to() && prev.eaten_type_ > 0)
    {
      return depthInc + ONE_PLY;
    }
  }

  return depthInc;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::alphaBetta0(int ictx)
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];

  if(sctx.stop_)
    return sctx.eval_(-ScoreMax, +ScoreMax);

  auto& sdata = sctx.sdata_;
  X_ASSERT(sdata.numOfMoves_ == 0, "no moves");

  auto& board = sctx.board_;
  if(sdata.numOfMoves_ == 0)
  {
    board.setNoMoves();
    ScoreType score = sctx.eval_(-ScoreMax, +ScoreMax);
    return score;
  }

  ScoreType alpha = -ScoreMax;
  ScoreType betta = +ScoreMax;

#ifndef NDEBUG
  Board board0{ board };
#endif

  for(sdata.counter_ = 0; sdata.counter_ < sdata.numOfMoves_; ++sdata.counter_)
  {
    if(checkForStop(ictx))
      break;

    auto& move = sctx.moves_[sdata.counter_];
    ScoreType score = processMove0(ictx, move, alpha, betta, !sdata.counter_);
    X_ASSERT(board != board0, "board undo error");

    if(sctx.stop_)
      break;

    auto& hist = history(board.color(), move.from(), move.to());
    if(score > alpha)
    {
      sdata.best_ = move;
      alpha = score;
      assemblePV(ictx, sdata.best_, board.underCheck(), 0);
    }

    if (ictx == 0 && callbacks_.sendStats_ && sparams_.analyze_mode_)
    {
      SearchData sdataTotal = sdata;
      for (size_t j = 1; j < scontexts_.size(); ++j)
        sdataTotal.nodesCount_ += scontexts_[j].sdata_.nodesCount_;
      (callbacks_.sendStats_)(sdataTotal);
    }
  } // end of for loop

  // don't need to continue
  if(!sctx.stop_ && sdata.numOfMoves_ == 1 && !sparams_.analyze_mode_)
    pleaseStop(ictx);

  if(sdata.numOfMoves_ == 1)
    return alpha;

  if(sdata.best_)
  {
    auto& hist = history(board.color(), sdata.best_.from(), sdata.best_.to());
    hist.inc_score(sdata.depth_);
    sortMoves0(ictx);

    X_ASSERT(!(scontexts_[ictx].moves_[0] == sdata.best_), "not best move first");
  }
  return alpha;
}

void Engine::sortMoves0(int ictx)
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];

  auto& sdata = sctx.sdata_;
  auto& moves = sctx.moves_;
  auto& board = sctx.board_;

  bring_to_front(moves.data(), moves.data() + sdata.numOfMoves_, sdata.best_);
  
  if (sdata.numOfMoves_ > 2)
  {
    auto* b = moves.data();
    auto* e = b + sdata.numOfMoves_;
    b++;
    for (auto* m = b; m != e; ++m)
    {
      if (m->new_type() || board.is_capture(*m))
        continue;
      auto& hist = history(board.color(), m->from(), m->to());
      m->sort_value = hist.score();
    }
    std::stable_sort(b, e, [](SMove const& m1, SMove const& m2) { return m1 > m2; });
  }
}

ScoreType Engine::processMove0(int ictx, SMove const& move, ScoreType const alpha, ScoreType const betta, bool const pv)
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];

  ScoreType score = -ScoreMax;
  auto& sdata = sctx.sdata_;
  const int depth = sdata.depth_ * ONE_PLY;

  auto& board = sctx.board_;

  board.makeMove(move);
  sdata.inc_nc();

#ifdef RELEASEDEBUGINFO
  auto ff = (FIELD_NAME)move.from();
  auto ft = (FIELD_NAME)move.to();
  if (findSequence(ictx, 0, true))
  {
    int t = 1;
  }
#endif

  if(!sctx.stop_)
  {
    if(pv)
    {
      int depthInc = depthIncrement(ictx, move, true, false);
      score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, 1, -betta, -alpha, true, true);
    }
    else
    {
      int depthInc = depthIncrement(ictx, move, false, false);

      score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, 1, -alpha-1, -alpha, false, true);

      if(!sctx.stop_ && score > alpha)
      {
        depthInc = depthIncrement(ictx, move, true, false);
        score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, 1, -betta, -alpha, true, true);
      }
    }
  }

  board.unmakeMove(move);
  return score;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool allow_nm)
{
  if(alpha >= Figure::MatScore-ply)
    return alpha;

  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];
  auto& board = sctx.board_;
  auto& sdata = sctx.sdata_;

  if(ply < MaxPly-1)
  {
    sctx.plystack_[ply].clear(ply);
    sctx.plystack_[ply+1].clearKiller();
  }

  if(board.drawState() || board.hasReps())
    return Figure::DrawScore;

  if(sctx.stop_ || ply >= MaxPly)
    return sctx.eval_(alpha, betta);

  ScoreType const alpha0 = alpha;
  SMove hmove{true};
  bool singular = false;
  ScoreType hscore = -ScoreMax;

  if (depth <= 0) {
    return captures(ictx, depth, ply, alpha, betta, pv);
  }

#ifdef USE_HASH
  HItem *pitem = nullptr;
  Flag flag = getHash(ictx, depth>>4 /* /ONE_PLY */, ply, alpha, betta, hmove, hscore, pv, singular, pitem);
  if (flag == Alpha || flag == Betta)
  {
    return hscore;
  }
  X_ASSERT(hmove && !board.possibleMove(hmove), "impossible move in hash");
#endif

#ifdef RELEASEDEBUGINFO
  if (board.fmgr().hashCode() == board.stestHashKey_)
  {
    int x = 0;
  }
#endif

  bool mat_threat{false};
  bool nm_threat{false};
  auto const& prev = board.lastUndo();

#ifdef USE_NULL_MOVE
  if(
    !pv
    && !board.underCheck()
    && allow_nm
    && std::abs(betta) < Figure::MatScore - MaxPly
    && depth > board.nullMoveDepthMin()
    && board.allowNullMove()
    )
  {
    int null_depth = board.nullMoveDepth(depth, betta);
    // do null-move
    board.makeNullMove();
#ifdef RELEASEDEBUGINFO
    if (findSequence(ictx, ply, true))
    {
      null_depth = null_depth;
    }
#endif // RELEASEDEBUGINFO
    ScoreType nullScore = -alphaBetta(ictx, null_depth, ply + 1, -betta, -(betta - 1), false, false);
    board.unmakeNullMove();

    // verify null-move with shortened depth
    if (nullScore >= betta)
    {
      depth = null_depth;
      if (depth <= 0)
        return captures(ictx, depth, ply, alpha, betta, pv);
    }
    else // may be we are in danger?
    {
      if (nullScore <= -Figure::MatScore + MaxPly) // mat threat?
      {
        if (prev.is_reduced())
          return betta - 1;
        else
          mat_threat = true;
      }

      nm_threat = true;
    }
  }
#endif // null-move

#ifdef USE_FUTILITY_PRUNING
  if(!pv
      && !board.underCheck()
      && alpha > -Figure::MatScore+MaxPly
      && alpha < Figure::MatScore-MaxPly
      && depth > 0 && depth <= ONE_PLY && ply > 1)
  {
    static const int thresholds_[4] = { 0,
      0,
      Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypePawn],
      Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypeRook] };

    ScoreType mscore = sctx.eval_.materialScore();
    ScoreType score0 = sctx.eval_(alpha, betta);
    if (score0 > mscore)
      mscore = score0;
    int threshold = (int)alpha - (int)mscore - Position_GainFP;
    if (threshold > thresholds_[(depth >> 4 /* / ONE_PLY */) & 3])
      return captures(ictx, depth, ply, alpha, betta, pv, score0);
  }
#endif // futility pruning

#ifdef SINGULAR_EXT
  int  aboveAlphaN = 0;
  int  depthIncBest = 0;
  bool allMovesIterated = false;
#endif

#ifdef USE_IID
  if(!hmove && depth >= (ONE_PLY<<2))
  {
    alphaBetta(ictx, depth - 3*ONE_PLY, ply, alpha, betta, pv, true);
    if(const HItem * hitem = hash_.get(board.fmgr().hashCode()))
    {
      if (hitem->hkey_ == (HKeyType)(board.fmgr().hashCode() >> (sizeof(uint64) - sizeof(HKeyType)) * 8)) {
        (Move&)hmove = hitem->move_;
        singular = hitem->singular_;
      }
    }
  }
#endif

  ScoreType scoreBest = -ScoreMax;
  Move best{true};
  bool dont_reduce{ false };
  int counter{};

#ifndef NDEBUG
  Board board0{ board };
#endif
  
  bool check_escape = board.underCheck();
  FastGenerator<Board, SMove> fg(board, hmove, sctx.plystack_[ply].killer_);
  for (; alpha < betta && !checkForStop(ictx);)
  {
    auto* pmove = fg.next();
    if (!pmove)
    {
#ifdef SINGULAR_EXT
      allMovesIterated = true;
#endif
      break;
    }

    auto& move = *pmove;
    X_ASSERT(!board.validateMove(move), "invalid move got from generator");

#ifdef USE_HASH
    hash_.prefetch(board.hashAfterMove(move));
#endif

    bool danger_pawn = board.isDangerPawn(move);

    ScoreType score = -ScoreMax;
    board.makeMove(move);
    sdata.inc_nc();
    int depthInc = 0;
    auto& curr = board.lastUndo();
    bool check_or_cap = curr.capture() || board.underCheck();

#ifdef RELEASEDEBUGINFO
    auto ff = (FIELD_NAME)move.from();
    auto ft = (FIELD_NAME)move.to();
    if (findSequence(ictx, ply, true))
    {
      depthInc = depthInc;
    }
#endif // RELEASEDEBUGINFO

    if (!counter)
    {
      depthInc = depthIncrement(ictx, move, pv, singular);
      score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply + 1, -betta, -alpha, pv, allow_nm);
    }
    else
    {
      depthInc = depthIncrement(ictx, move, false, false);
      auto const& hist = history(Figure::otherColor(board.color()), move.from(), move.to());

      int R = 0;
#ifdef USE_LMR
      if(!check_escape &&         
         !danger_pawn &&
          (sdata.depth_<<4) /* *ONE_PLY*/ > LMR_MinDepthLimit &&
          depth >= LMR_DepthLimit &&
          alpha > -Figure::MatScore-MaxPly &&
          scontexts_[ictx].board_.canBeReduced(move))
      {
        R = ONE_PLY;
        curr.mflags_ |= UndoInfo::Reduced;
      }
#endif

      score = -alphaBetta(ictx, depth + depthInc - R - ONE_PLY, ply + 1, -alpha - 1, -alpha, false, allow_nm);
      curr.mflags_ &= ~UndoInfo::Reduced;

      if (!sctx.stop_ && score > alpha && R > 0)
        score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply + 1, -alpha - 1, -alpha, false, allow_nm);

      if (!sctx.stop_ && score > alpha && score < betta && pv)
      {
        depthInc = depthIncrement(ictx, move, pv, singular);
        score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply + 1, -betta, -alpha, pv, allow_nm);
      }
    }

#ifdef RELEASEDEBUGINFO
    if (findSequence(ictx, ply, true) && score > alpha)
    {
      depthInc = depthInc;
    }
#endif // RELEASEDEBUGINFO

    board.unmakeMove(move);
    X_ASSERT(board != board0, "board undo error");

    if(!sctx.stop_)
    {
#ifdef SINGULAR_EXT
      if(pv && score > alpha0)
      {
        aboveAlphaN++;
        depthIncBest = depthInc;
      }
#endif

      auto& hist = history(board.color(), move.from(), move.to());
      if(score > scoreBest)
      {
        best = move;
        dont_reduce = check_or_cap;
        scoreBest = score;
        if(score > alpha)
        {
          bool capture = board.getField(move.to())  || move.new_type() || (move.to() > 0 && board.enpassant() == move.to() && board.getField(move.from()).type() == Figure::TypePawn);
          if (!capture) {
            sctx.plystack_[ply].killer_ = move;
          }
          auto d = depth >> 4;
          hist.inc_score(d);
          alpha = score;
          if(pv)
            assemblePV(ictx, move, board.underCheck(), ply);
        }
      }
      else {
        auto d = depth >> 4;
        hist.dec_score(d);
      }
    }

    ++counter;
  }

  if(sctx.stop_)
    return scoreBest;

  if(!counter)
  {
    board.setNoMoves();
    scoreBest = sctx.eval_(alpha, betta);
    if(board.matState())
    {
      scoreBest += ply;
      X_ASSERT(board.hasMove(), "invalid mat detection");
    }
  }

#ifdef SINGULAR_EXT
  if(pv && !singular && aboveAlphaN == 1 && allMovesIterated)
  {
    X_ASSERT(!best, "best move wasn't found but one move was");
    singular = true;

    hash_.prefetch(board.hashAfterMove(best));

    board.makeMove(best);
    sdata.inc_nc();

#ifdef RELEASEDEBUGINFO
    auto ff = (FIELD_NAME)best.from();
    auto ft = (FIELD_NAME)best.to();
    if (findSequence(ictx, ply, true))
    {
      int y = 0;
    }
#endif // RELEASEDEBUGINFO

    alpha = alpha0;
    ScoreType score = -alphaBetta(ictx, depth+depthIncBest, ply+1, -betta, -alpha, pv, true);

    board.unmakeMove(best);

    if(!sctx.stop_)
    {
      scoreBest = score;
      if(score > alpha)
      {
        alpha = score;
        assemblePV(ictx, best, board.underCheck(), ply);
      }
    }
  }
#endif

  bool threat{ false };
  if(best)
  {
#if ((defined USE_LMR) && (defined VERIFY_LMR))
    // have to recalculate with full depth, or indicate threat in hash
    if ( !sctx.stop_ && !dont_reduce && (mat_threat || (alpha >= betta && nm_threat && isRealThreat(ictx, best))) )
    {
      if(prev.is_reduced())
        return betta-1;
      else
        threat = true;
    }
#endif
  }

#ifdef RELEASEDEBUGINFO
  if (board.fmgr().hashCode() == board.stestHashKey_)
  {
    int x = 0;
  }
#endif

#ifdef USE_HASH
  putHash(ictx, best, alpha0, betta, scoreBest, depth>>4 /* /ONE_PLY */, ply, threat, singular, pv, pitem);
#endif

  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];

  if (alpha >= Figure::MatScore - ply)
    return alpha;

  auto& board = sctx.board_;
  auto& sdata = sctx.sdata_;
  auto& eval =  sctx.eval_;

  if (board.drawState() || board.hasReps() || sctx.stop_ || ply >= MaxPly)
    return Figure::DrawScore;

  SMove hmove{ true };

#ifdef USE_HASH
  HItem* pitem = nullptr;
  ScoreType hscore = -ScoreMax;
  bool singular = false;
  int cdepth = depth < 0 ? -1 : 0;
  Flag flag = getHash(ictx, cdepth, ply, alpha, betta, hmove, hscore, pv, singular, pitem);
  if (flag == Alpha || flag == Betta)
  {
    return hscore;
  }
  X_ASSERT(hmove && !board.possibleMove(hmove), "impossible move in hash");
#endif

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  int threshold = 0;

  if(!board.underCheck())
  {
    if (hmove && !board.is_capture(hmove) && hmove.new_type() == 0 && depth < 0) {
      hmove = SMove{ true };
    }

    // not initialized yet
    if (score0 == -ScoreMax) {
      score0 = eval(alpha, betta);
    }

    if (score0 >= betta) {
#ifdef USE_HASH
      putHash(ictx, hmove, alpha, betta, score0, cdepth, ply, false, false, pv, pitem);
#endif
      return score0;
    }

#ifdef GENERATE_MAT_MOVES_IN_EVAL
    if (eval.move(board.color()) && !hmove) {
      hmove = eval.move(board.color());
    }
#endif // GENERATE_MAT_MOVES_IN_EVAL

    ScoreType mscore = eval.materialScore();
    if (score0 > mscore)
      mscore = score0;
    threshold = (int)alpha - (int)mscore - Position_GainThr;
    if(threshold > Figure::figureWeight_[Figure::TypePawn] && !board.allowNullMove())
      threshold = Figure::figureWeight_[Figure::TypePawn];
    
    {
      if (score0 > alpha)
        alpha = score0;

      scoreBest = score0;
    }
  }

  Move best{true};

#ifndef NDEBUG
  Board board0{ board };
#endif

  int thr = std::min(threshold, 0);
  TacticalGenerator<Board, SMove> tg(board, hmove, depth);
  for(; alpha < betta && !checkForStop(ictx);)
  {
    auto* pmove = tg.next();
    if(!pmove)
      break;

    auto& move = *pmove;
    X_ASSERT(!board.validateMove(move), "invalid move got from generator");

#ifdef USE_HASH
    hash_.prefetch(board.hashAfterMove(move));
#endif

#ifdef RELEASEDEBUGINFO
    auto ff = (FIELD_NAME)move.from();
    auto ft = (FIELD_NAME)move.to();
#endif

#ifdef SEE_PRUNING
    if((!board.underCheck()) && !move.see_ok() && !board.see(move, thr))
      continue;
#endif

    ScoreType score = -ScoreMax;

    board.makeMove(move);
    sdata.inc_nc();

#ifdef RELEASEDEBUGINFO
    if (findSequence(ictx, ply, true))
    {
      thr = thr;
    }
#endif // RELEASEDEBUGINFO

    int depthInc = board.underCheck() * ONE_PLY;
    score = -captures(ictx, depth + depthInc - ONE_PLY, ply + 1, -betta, -alpha, pv);

#ifdef RELEASEDEBUGINFO
    if (findSequence(ictx, ply, true) && score > alpha)
    {
      thr = thr;
    }
#endif // RELEASEDEBUGINFO

    board.unmakeMove(move);
    X_ASSERT(board != board0, "board undo error");

    if (score > -Figure::MatScore + MaxPly) {
      thr = threshold;
    }

    if (!sctx.stop_ && score > scoreBest)
    {
      if (score > alpha) {
        alpha = score;
      }

      best = move;
      scoreBest = score;
    }

    counter++;
  }

  if (sctx.stop_)
    return scoreBest;

  if(!counter)
  {
    if(board.underCheck())
    {
      scoreBest = -Figure::MatScore+ply;
      X_ASSERT(board.hasMove(), "invalid mat detection");
    }
    else
      scoreBest = score0;
  }

#ifdef USE_HASH
  else {
    putHash(ictx, best, alpha, betta, scoreBest, cdepth, ply, false, false, pv, pitem);
  }
#endif

  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
// is given movement caused by previous? this mean that if we don't do this move we loose
// we actually test if moved figure was/will be attacked by previously moved one or from direction it was moved from
// or we should move our king even if we have a lot of other figures
//////////////////////////////////////////////////////////////////////////
bool Engine::isRealThreat(int ictx, const Move & move)
{
  X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
  auto& sctx = scontexts_[ictx];
  auto const& board = sctx.board_;

  auto const& prev = board.lastUndo();
  if(prev.is_nullmove())
    return false;

  auto const color = board.color();
  auto const ocolor = Figure::otherColor(color);

  auto const& pfield = board.getField(prev.move_.to());
  X_ASSERT(!pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat");

  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
  if(prev.capture() || prev.move_.new_type() > 0 || prev.check() || prev.threat())
    return false;

  /// we have to move king even if there a lot of figures
  if(board.getField(move.from()).type() == Figure::TypeKing &&
     board.fmgr().queens(color) > 0 &&
     board.fmgr().rooks(color) > 0 &&
     board.fmgr().knights(color) + board.fmgr().bishops(color) > 0)
  {
    return true;
  }

  auto const& cfield = board.getField(move.from());
  X_ASSERT(!cfield || cfield.color() != color, "no figure of required color in while detecting threat");

  // we have to put figure under attack
  if(board.ptAttackedBy(move.to(), prev.move_.to()))
    return true;

  // put our figure under attack, opened by prev movement
  if(board.ptAttackedFrom(ocolor, move.to(), prev.move_.from()))
    return true;

  // prev move was attack, and we should escape from it
  if(board.ptAttackedBy(move.from(), prev.move_.to()))
    return true;

  // our figure was attacked from direction, opened by prev movement
  if(board.ptAttackedFrom(ocolor, move.from(), prev.move_.from()))
    return true;

  return false;
}

int Engine::xcounter_ = 0;

} // NEngine
