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
  if (scontexts_.size() <= ictx)
    return false;

  auto& sres = scontexts_.at(ictx).sres_;
  auto& board = scontexts_.at(ictx).board_;
  auto& sdata = scontexts_.at(ictx).sdata_;
  
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
      move.sort_value = scontexts_.at(ictx).eval_(-ScoreMax, +ScoreMax);
      if (!move.see_ok())
        move.sort_value -= 10000;
      board.unmakeMove(move);
    }
    scontexts_.at(ictx).moves_[sdata.numOfMoves_++] = move;
  }
  scontexts_.at(ictx).moves_[sdata.numOfMoves_] = SMove{ true };
  auto* b = scontexts_.at(ictx).moves_.data();
  auto* e = b + sdata.numOfMoves_;
  std::sort(b, e, [](SMove const& m1, SMove const& m2) { return m1 > m2; });
  auto const* hitem = hash_.find(board.fmgr().hashCode());
  if (hitem && hitem->move_)
  {
    SMove best{ hitem->move_.from(), hitem->move_.to(), (Figure::Type)hitem->move_.new_type() };
    X_ASSERT(!board.possibleMove(best), "move from hash is impossible");
    X_ASSERT(!board.validateMove(best), "move from hash is invalid");
    bring_to_front(b, e, best);
  }

  sres.numOfMoves_ = sdata.numOfMoves_;
  return sdata.numOfMoves_ > 0;
}

SearchResult const& Engine::result() const
{
  return scontexts_.at(0).sres_;
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
    scontexts_.at(ictx) = scontexts_.at(0);
  }

  // start threads
  std::vector<std::unique_ptr<std::thread>> threads;
  for (int ictx = 1; ictx < scontexts_.size(); ++ictx)
  {
    auto thread = std::unique_ptr<std::thread>(new std::thread{ [this](int ictx) {
      this->threadSearch(ictx);
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
  auto& sres = scontexts_.at(ictx).sres_;
  // stash board to correctly print status later
  sres.board_ = scontexts_.at(ictx).board_;
  auto& board = scontexts_.at(ictx).board_;
  auto& sdata = scontexts_.at(ictx).sdata_;
  // copy to print stats later
  sdata.board_ = board;

  for(sdata.depth_ = depth0_; !stopped(ictx) && sdata.depth_ <= sparams_.depthMax_; ++sdata.depth_)
  {
    scontexts_.at(ictx).plystack_[0].clearPV(sparams_.depthMax_);

    sdata.restart();

    ScoreType score = alphaBetta0(ictx);

    if(sdata.best_)
    {
      if(ictx == 0 && stopped(ictx) && sdata.depth_ > 2 && sdata.counter_ < sdata.numOfMoves_ && 
        ((sres.score_ - score >= Figure::figureWeight_[Figure::TypePawn]/4) || (sdata.best_ != sres.best_)) &&
        callbacks_.giveTime_ &&
        !sparams_.analyze_mode_)
      {
        auto t_add = (callbacks_.giveTime_)();
        if(NTime::milli_seconds<int>(t_add) > 0)
        {
          scontexts_.at(ictx).stop_ = false;
          sparams_.timeLimit_ += t_add;
          if(sdata.counter_ < sdata.numOfMoves_)
            sdata.depth_--;
          continue;
        }
      }

      auto t  = NTime::now();
      auto dt = t - sdata.tstart_;
      sdata.tprev_ = t;

      sres.score_ = score;
      sres.best_ = sdata.best_;
      sres.depth_ = sdata.depth_;
      sres.nodesCount_ = sdata.nodesCount_;
      sres.totalNodes_ = sdata.totalNodes_;
      sres.depthMax_ = 0;
      sres.dt_ = dt;
      sres.counter_ = sdata.counter_;

      for(int i = 0; i < MaxPly; ++i)
      {
        sres.depthMax_ = i;
        sres.pv_[i] = scontexts_.at(ictx).plystack_[0].pv_[i];
        if(!sres.pv_[i])
          break;
      }

      X_ASSERT(sres.pv_[0] != sdata.best_, "invalid PV found");

      if (ictx == 0 && callbacks_.sendOutput_)
      {
        for (size_t j = 1; j < scontexts_.size(); ++j)
          sres.totalNodes_ += scontexts_.at(j).sdata_.totalNodes_;
        (callbacks_.sendOutput_)(sres);
      }
    }
    // we haven't found move and spend more time for search it than on prev. iteration
    else if(ictx == 0 && stopped(ictx) && sdata.depth_ > 2 && callbacks_.giveTime_ && !sparams_.analyze_mode_)
    {
      auto t = NTime::now();
      if((t - sdata.tprev_) >= (sdata.tprev_ - sdata.tstart_))
      {
        auto t_add = (callbacks_.giveTime_)();
        if(t_add > NTime::duration(0))
        {
          scontexts_.at(ictx).stop_ = false;
          sparams_.timeLimit_ += t_add;
          sdata.depth_--;
          continue;
        }
      }

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
            sres.totalNodes_ += scontexts_.at(j).sdata_.totalNodes_;
          (callbacks_.sendOutput_)(sres);
        }
      }
#endif // SYNCHRONIZE_LAST_ITER
    }

    if(!sdata.best_ ||
       ((score >= sparams_.scoreLimit_-MaxPly || score <= MaxPly-sparams_.scoreLimit_) &&
      !sparams_.analyze_mode_))
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

bool Engine::threadSearch(int ictx)
{
  // stash board to correctly print status later
  auto& sres  = scontexts_.at(ictx).sres_;
  sres.board_ = scontexts_.at(ictx).board_;
  auto& board = scontexts_.at(ictx).board_;
  auto& sdata = scontexts_.at(ictx).sdata_;
  // copy to print stats later
  sdata.board_ = board;

  int depth0 = depth0_;
  for (auto const& sctx : scontexts_)
    depth0 = std::max(sctx.sdata_.depth_, depth0);
  depth0++;
  for (sdata.depth_ = depth0; !stopped(ictx) && sdata.depth_ <= sparams_.depthMax_;)
  {
    scontexts_.at(ictx).plystack_[0].clearPV(sparams_.depthMax_);

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
          sres.pv_[i] = scontexts_.at(ictx).plystack_[0].pv_[i];
          if (!sres.pv_[i])
            break;
        }
      }
#endif // SYNCHRONIZE_LAST_ITER

      X_ASSERT(sres.pv_[0] != sdata.best_, "invalid PV found");
    }

    if (!sdata.best_ ||
      ((score >= sparams_.scoreLimit_ - MaxPly || score <= MaxPly - sparams_.scoreLimit_) &&
        !sparams_.analyze_mode_))
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
  auto& board = scontexts_.at(ictx).board_;
  
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
  if(stopped(ictx))
    return scontexts_.at(ictx).eval_(-ScoreMax, +ScoreMax);

  auto& sdata = scontexts_.at(ictx).sdata_;
  X_ASSERT(sdata.numOfMoves_ == 0, "no moves");

  auto& board = scontexts_.at(ictx).board_;
  if(sdata.numOfMoves_ == 0)
  {
    board.setNoMoves();
    ScoreType score = scontexts_.at(ictx).eval_(-ScoreMax, +ScoreMax);
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

    auto& move = scontexts_.at(ictx).moves_[sdata.counter_];
    ScoreType score = processMove0(ictx, move, alpha, betta, !sdata.counter_);
    X_ASSERT(board != board0, "board undo error");

    if(stopped(ictx))
      break;

    auto& hist = history(board.color(), move.from(), move.to());
    if(score > alpha)
    {
      hist.inc_good();
      sdata.best_ = move;
      alpha = score;
      assemblePV(ictx, sdata.best_, board.underCheck(), 0);
    }
    else
      hist.inc_bad();

    if (ictx == 0 && callbacks_.sendStats_ && sparams_.analyze_mode_)
    {
      SearchData sdataTotal = sdata;
      for (size_t j = 1; j < scontexts_.size(); ++j)
        sdataTotal.nodesCount_ += scontexts_.at(j).sdata_.nodesCount_;
      (callbacks_.sendStats_)(sdataTotal);
    }
  } // end of for loop

  // don't need to continue
  if(!stopped(ictx) && sdata.numOfMoves_ == 1 && !sparams_.analyze_mode_)
    pleaseStop(ictx);

  if(sdata.numOfMoves_ == 1)
    return alpha;

  if(sdata.best_)
  {
    auto& hist = history(board.color(), sdata.best_.from(), sdata.best_.to());
    hist.inc_score(sdata.depth_ / ONE_PLY);
    sortMoves0(ictx);

    X_ASSERT(!(scontexts_[ictx].moves_[0] == sdata.best_), "not best move first");
  }
  return alpha;
}

void Engine::sortMoves0(int ictx)
{
  auto& sdata = scontexts_.at(ictx).sdata_;
  auto& moves = scontexts_.at(ictx).moves_;
  auto& board = scontexts_.at(ictx).board_;

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
  ScoreType score = -ScoreMax;
  auto& sdata = scontexts_.at(ictx).sdata_;
  const int depth = sdata.depth_ * ONE_PLY;

  auto& board = scontexts_.at(ictx).board_;

#ifdef USE_LMR0
  bool check_escape = board.underCheck();
#endif

  board.makeMove(move);
  sdata.inc_nc();

  if(!stopped(ictx))
  {
    if(pv)
    {
      int depthInc = depthIncrement(ictx, move, true, false);
      score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, 1, -betta, -alpha, true, true);
    }
    else
    {
      int depthInc = depthIncrement(ictx, move, false, false);

      int R = 0;
  #ifdef USE_LMR0
      if(!check_escape &&
         sdata.depth_ * ONE_PLY > LMR_MinDepthLimit &&
         depth >= LMR_DepthLimit &&
         alpha > -Figure::MatScore-MaxPly &&
         board.canBeReduced(move))
      {
        R = ONE_PLY;
      }
  #endif
      score = -alphaBetta(ictx, depth + depthInc - R - ONE_PLY, 1, -alpha-1, -alpha, false, true);
      if(!stopped(ictx) && score > alpha && R > 0)
        score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, 1, -alpha - 1, -alpha, false, true);

      if(!stopped(ictx) && score > alpha)
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
  prefetchHash(ictx);

  if(alpha >= Figure::MatScore-ply)
    return alpha;

  auto& board = scontexts_.at(ictx).board_;
  auto& sdata = scontexts_.at(ictx).sdata_;

  if(ply < MaxPly-1)
  {
    scontexts_.at(ictx).plystack_[ply].clear(ply);
    scontexts_.at(ictx).plystack_[ply+1].clearKiller();
  }

  if(board.drawState() || board.hasReps())
    return Figure::DrawScore;

  if(stopped(ictx) || ply >= MaxPly)
    return scontexts_.at(ictx).eval_(alpha, betta);

  ScoreType const alpha0 = alpha;
  SMove hmove{true};
  bool singular = false;

#ifdef USE_HASH
  ScoreType hscore = -ScoreMax;
  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv, singular);
  if(flag == GHashTable::Alpha || flag == GHashTable::Betta)
  {
    return hscore;
  }
  X_ASSERT(hmove && !board.possibleMove(hmove), "impossible move in hash"); 
#endif

  if (depth <= 0)
  {
    auto capResult = captures(ictx, depth, ply, alpha, betta, pv);
#ifdef PROCESS_DANGEROUS_EVAL
    if (!capResult.dangerous || depth < 0 || capResult.score <= alpha)
#endif // #ifdef PROCESS_DANGEROUS_EVAL
      return capResult.score;
  }

  bool mat_threat{false};
  bool nm_threat{false};
  auto const& prev = board.lastUndo();

#ifdef USE_NULL_MOVE
  if(
    !board.underCheck()
    && !pv
    && allow_nm
    && board.allowNullMove()
    && depth > board.nullMoveDepthMin()
    && std::abs(betta) < Figure::MatScore+MaxPly
    && !board.isWinnerLoser()
    )
  {
    int null_depth = board.nullMoveDepth(depth, betta);

    // do null-move
    board.makeNullMove();
    ScoreType nullScore = -alphaBetta(ictx, null_depth, ply+1, -betta, -(betta-1), false, false);
    board.unmakeNullMove();

    // verify null-move with shortened depth
    if(nullScore >= betta)
    {
      depth = null_depth;
      if (depth <= 0)
      {
        return captures(ictx, depth, ply, alpha, betta, pv).score;
      }
    }
    else // may be we are in danger?
    {
      if(nullScore <= -Figure::MatScore+MaxPly) // mat threat?
      {
        if(prev.is_reduced())
          return betta-1;
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
      && depth > 0 && depth <= ONE_PLY && ply > 1
      && !board.isWinnerLoser())
  {
    static const int thresholds_[4] = { 0,
      0,
      Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypePawn],
      Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypeRook] };

    ScoreType score0 = scontexts_.at(ictx).eval_(alpha, betta);
    int threshold = (int)alpha - (int)score0 - Position_Gain;
    if (threshold > thresholds_[(depth / ONE_PLY) & 3])
      return captures(ictx, depth, ply, alpha, betta, pv, score0).score;
  }
#endif // futility pruning


#ifdef SINGULAR_EXT
  int  aboveAlphaN = 0;
  int  depthIncBest = 0;
  bool allMovesIterated = false;
#endif

#ifdef USE_IID
  if(!hmove && depth >= 4 * ONE_PLY)
  {
    alphaBetta(ictx, depth - 3*ONE_PLY, ply, alpha, betta, pv, true);
    if(const HItem * hitem = hash_.find(board.fmgr().hashCode()))
    {
      X_ASSERT(hitem->hkey_ != board.fmgr().hashCode(), "invalid hash item found");
      (Move&)hmove = hitem->move_;
      singular = hitem->singular_;
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
  FastGenerator<Board, SMove> fg(board, hmove, scontexts_.at(ictx).plystack_[ply].killer_);
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

    bool danger_pawn = board.isDangerPawn(move);

    ScoreType score = -ScoreMax;
    board.makeMove(move);
    sdata.inc_nc();
    int depthInc = 0;
    auto& curr = board.lastUndo();
    bool check_or_cap = curr.capture() || board.underCheck();

    //if (sdata.depth_ == 11 && findSequence(ictx, move, ply, true))
    //{
    //  depthInc = depthInc;
    //}

#ifdef USE_PROBCUT
    ScoreType betta_pc = betta < ScoreMax - 1000 ? betta + 300 : betta;
    if (!pv
      //&& curr.capture()
      && !danger_pawn
      && move.see_ok()
      && !check_escape
      && !board.isWinnerLoser()
      && depth >= Probcut_Depth
      && std::abs(betta) < Figure::MatScore - MaxPly
      && std::abs(alpha) < Figure::MatScore - MaxPly)
    {
      score = -alphaBetta(ictx, depth - Probcut_PlyReduce, ply + 1, -betta_pc, -(betta_pc - 1), pv, allow_nm);
    }

    if (score < alpha + 300)
#endif

    {

      if (!counter)
      {
        depthInc = depthIncrement(ictx, move, pv, singular);
        score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply + 1, -betta, -alpha, pv, allow_nm);
      }
      else
      {
        depthInc = depthIncrement(ictx, move, false, false);

        int R = 0;
#ifdef USE_LMR
        if (!check_escape &&
          !danger_pawn &&
          sdata.depth_ * ONE_PLY > LMR_MinDepthLimit &&
          depth >= LMR_DepthLimit &&
          alpha > -Figure::MatScore - MaxPly &&
          board.canBeReduced(move) &&
          !board.isWinnerLoser())
        {
          R = ONE_PLY;
          //auto const& hist = history(Figure::otherColor(board.color()), move.from(), move.to());
          //if (depth > LMR_DepthLimit && (!move.see_ok() || hist.good() < hist.bad()))
          //{
          //  R += ONE_PLY + ONE_PLY*counter/ 10;
          //}
          //R = std::min(3*ONE_PLY, R);

#ifdef LMR_REDUCE_MORE
          auto const& hist = history(Figure::otherColor(board.color()), move.from(), move.to());
          if(depth > LMR_DepthLimit && (!move.see_ok() || hist.good()*20 < hist.bad()))
          {
            R += ONE_PLY;
            if(depth > LMR_DepthLimit+ONE_PLY && counter > 10)
              R += ONE_PLY;
          }
          curr.mflags_ |= UndoInfo::Reduced;
        }
        else if(!check_escape &&
                counter > 10 &&
                sdata.depth_ * ONE_PLY > LMR_MinDepthLimit &&
                depth > LMR_DepthLimit &&
                alpha > -Figure::MatScore-MaxPly &&
                !move.see_ok() &&
                !curr.castle() &&
                !board.underCheck())
        {
          R = ONE_PLY;
#endif // LMR_REDUCE_MORE
          curr.mflags_ |= UndoInfo::Reduced;
        }
#endif //USE_LMR

        score = -alphaBetta(ictx, depth + depthInc - R - ONE_PLY, ply + 1, -alpha - 1, -alpha, false, allow_nm);
        curr.mflags_ &= ~UndoInfo::Reduced;

        if (!stopped(ictx) && score > alpha && R > 0)
          score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply + 1, -alpha - 1, -alpha, false, allow_nm);

        if (!stopped(ictx) && score > alpha && score < betta && pv)
        {
          depthInc = depthIncrement(ictx, move, pv, singular);
          score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply + 1, -betta, -alpha, pv, allow_nm);
        }
      }

    } // end of (score < betta_pc)
#ifdef USE_PROBCUT
    else if(betta < Figure::MatScore - MaxPly)
    {
      score = betta;
    }
#endif

    board.unmakeMove(move);
    X_ASSERT(board != board0, "board undo error");

    if(!stopped(ictx))
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
        hist.inc_good();
        best = move;
        dont_reduce = check_or_cap;
        scoreBest = score;
        if(score > alpha)
        {
          bool capture = move.new_type() || board.getField(move.to())
            || (move.to() > 0 && board.enpassant() == move.to() && board.getField(move.from()).type() == Figure::TypePawn);
          if(!capture)
            scontexts_.at(ictx).plystack_[ply].killer_ = move;
          alpha = score;
          if(pv)
            assemblePV(ictx, move, board.underCheck(), ply);
        }
      }
      else
        hist.inc_bad();
    }

    ++counter;
  }

  if(stopped(ictx))
    return scoreBest;

  if(!counter)
  {
    board.setNoMoves();
    scoreBest = scontexts_.at(ictx).eval_(alpha, betta);
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

    board.makeMove(best);
    sdata.inc_nc();

    alpha = alpha0;
    ScoreType score = -alphaBetta(ictx, depth+depthIncBest, ply+1, -betta, -alpha, pv, true);

    board.unmakeMove(best);

    if(!stopped(ictx))
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
    auto& hist = history(board.color(), best.from(), best.to());
    hist.inc_score(depth / ONE_PLY);

#if ((defined USE_LMR) && (defined VERIFY_LMR))
    // have to recalculate with full depth, or indicate threat in hash
    if ( !stopped(ictx) && !dont_reduce && (mat_threat || (alpha >= betta && nm_threat && isRealThreat(ictx, best))) )
    {
      if(prev.is_reduced())
        return betta-1;
      else
        threat = true;
    }
#endif
  }

#ifdef USE_HASH
  putHash(ictx, best, alpha0, betta, scoreBest, depth, ply, threat, singular);
#endif

  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
Engine::CapturesResult Engine::captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
{
  if(alpha >= Figure::MatScore-ply)
    return { alpha, false };

  auto& board = scontexts_.at(ictx).board_;
  auto& sdata = scontexts_.at(ictx).sdata_;
  auto& eval = scontexts_.at(ictx).eval_;

  if(board.drawState() || board.hasReps() || stopped(ictx) || ply >= MaxPly)
    return { Figure::DrawScore, false };

  SMove hmove{true};
  
#ifdef USE_HASH
  ScoreType hscore = -ScoreMax;
  bool singular = false;
  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv, singular);
  if(flag == GHashTable::Alpha || flag == GHashTable::Betta)
  {
    return { hscore, false };
  }
  X_ASSERT(hmove && !board.possibleMove(hmove), "impossible move in hash");
#endif

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  int threshold = 0;
  bool dangerous = false;

  if(!board.underCheck())
  {
    // not initialized yet
    if (score0 == -ScoreMax)
      score0 = eval(alpha, betta);

#ifdef PROCESS_DANGEROUS_EVAL
    if(score0 > alpha)
      dangerous = eval.dangerous();
#endif // PROCESS_DANGEROUS_EVAL

    if(score0 >= betta && (depth >= 0 || !dangerous))
      return { score0, dangerous };

    threshold = (int)alpha - (int)score0 - Position_Gain;
    if(threshold > Figure::figureWeight_[Figure::TypePawn] && board.isWinnerLoser())
      threshold = Figure::figureWeight_[Figure::TypePawn];
    
    if (!dangerous)
    {
      if (score0 > alpha)
        alpha = score0;

      scoreBest = score0;
    }
  }

  if (!board.underCheck() && hmove && !board.is_capture(hmove) && hmove.new_type() == 0 && depth < 0)
  {
    hmove = SMove{ true };
  }

  Move best{true};

#ifndef NDEBUG
  Board board0{ board };
#endif

  TacticalGenerator<Board, SMove> tg(board, hmove, depth);
  for(; alpha < betta && !checkForStop(ictx);)
  {
    auto* pmove = tg.next(dangerous, threshold, counter);
    if(!pmove)
      break;

    auto& move = *pmove;
    X_ASSERT(!board.validateMove(move), "invalid move got from generator");

    if((!board.underCheck() || counter > 0) && !move.see_ok() && !board.see(move, threshold))
      continue;

    ScoreType score = -ScoreMax;

    board.makeMove(move);
    sdata.inc_nc();

    int depthInc = board.underCheck() ? ONE_PLY : 0;
    score = -captures(ictx, depth + depthInc - ONE_PLY, ply+1, -betta, -alpha, pv, -ScoreMax).score;

    board.unmakeMove(move);
    X_ASSERT(board != board0, "board undo error");

    if(score > -Figure::MatScore+MaxPly)
      dangerous = false;

    if (!stopped(ictx) && score > scoreBest)
    {
      if (score > alpha) {
        alpha = score;
      }

      best = move;
      scoreBest = score;
    }
    counter++;
  }

  if(stopped(ictx))
    return { scoreBest, dangerous };

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
  putCaptureHash(ictx, best);
#endif

  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
  return { scoreBest, dangerous };
}


/// extract data from hash table
#ifdef USE_HASH
void Engine::prefetchHash(int ictx)
{
  hash_.prefetch(scontexts_.at(ictx).board_.fmgr().hashCode());
}

GHashTable::Flag Engine::getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv, bool& singular)
{
  auto& board = scontexts_.at(ictx).board_;
  auto const* pitem = hash_.find(board.fmgr().hashCode());
  if(!pitem)
    return GHashTable::NoFlag;

  auto hitem = *pitem;
  if(hitem.hkey_ != board.fmgr().hashCode())
    return GHashTable::NoFlag;

  singular = hitem.singular_;
  hmove = hitem.move_;
  if(pv || board.hashValueForbidden())
    return GHashTable::AlphaBetta;

  depth = (depth + ONE_PLY - 1) / ONE_PLY;

  hscore = hitem.score_;
  if(hscore >= Figure::MatScore-MaxPly)
    hscore = hscore - ply;
  else if(hscore <= MaxPly-Figure::MatScore)
    hscore = hscore + ply;

  X_ASSERT(hscore > 32760 || hscore < -32760, "invalid value in hash");

  if(hitem.flag_  != GHashTable::NoFlag && (int)hitem.depth_ >= depth && ply > 0 && hscore != 0)
  {
    if(GHashTable::Alpha == hitem.flag_ && hscore <= alpha)
    {
      X_ASSERT(!scontexts_.at(ictx).stop_ && alpha < -32760, "invalid hscore");
      return GHashTable::Alpha;
    }

    if(hitem.flag_ > GHashTable::Alpha && hscore >= betta && hmove)
    {
      if(betta > 0 && board.hasReps(hmove))
      {
        return GHashTable::AlphaBetta;
      }
#ifdef VERIFY_LMR
      else
      {
        /// danger move was reduced - recalculate it with full depth
        auto const& prev = board.lastUndo();
        if(hitem.threat_ && prev.is_reduced())
        {
          hscore = betta-1;
          return GHashTable::Alpha;
        }
        return GHashTable::Betta;
      }
#endif
    }
  }

  return GHashTable::AlphaBetta;
}

/// insert data to hash table
void Engine::putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat, bool singular)
{
  auto& board = scontexts_.at(ictx).board_;
  if (board.hasReps())
    return;

  GHashTable::Flag flag = GHashTable::NoFlag;
  if(score != 0)
  {
    if ( score <= alpha || !move )
      flag = GHashTable::Alpha;
    else if(score >= betta)
      flag = GHashTable::Betta;
    else
      flag = GHashTable::AlphaBetta;
  }
  if ( score >= +Figure::MatScore-MaxPly )
    score += ply;
  else if ( score <= -Figure::MatScore+MaxPly )
    score -= ply;
  hash_.push(board.fmgr().hashCode(), score, depth / ONE_PLY, flag, move, threat, singular);
}

void Engine::putCaptureHash(int ictx, const Move & move)
{
  auto& board = scontexts_.at(ictx).board_;
  if(!move || board.hasReps())
    return;
  hash_.push(board.fmgr().hashCode(), 0, 0, GHashTable::NoFlag, move, false, false);
}
#endif


//////////////////////////////////////////////////////////////////////////
// is given movement caused by previous? this mean that if we don't do this move we loose
// we actually test if moved figure was/will be attacked by previously moved one or from direction it was moved from
// or we should move our king even if we have a lot of other figures
//////////////////////////////////////////////////////////////////////////
bool Engine::isRealThreat(int ictx, const Move & move)
{
  auto const& board = scontexts_.at(ictx).board_;

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
