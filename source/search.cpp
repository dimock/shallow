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

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
bool Engine::search(SearchResult& sres)
{
#ifdef USE_HASH
  hash_.inc();
#endif

  reset();

  // stash board to correctly print status later
  sres.board_ = scontexts_[0].board_;
  sdata_.board_ = scontexts_[0].board_;
  auto& board = scontexts_[0].board_;

  {
    auto moves = generate<Board, SMove>(board);
    for(auto move : moves)
    {
      move.sort_value = 0;
      if(!board.validateMoveBruteforce(move))
        continue;
      if(board.see(move, 0))
        move.set_ok();
      if(move.new_type() || board.is_capture(move))
      {
        if(move.see_ok())
          move.sort_value = 10000000 + board.sortValueOfCap(move.from(), move.to(), (Figure::Type)move.new_type());
        else
          move.sort_value = board.sortValueOfCap(move.from(), move.to(), (Figure::Type)move.new_type()) - 10000;
      }
      else
      {
        board.makeMove(move);
        move.sort_value = scontexts_[0].eval_(-ScoreMax, +ScoreMax);
        if(!move.see_ok())
          move.sort_value -= 10000;
        board.unmakeMove(move);
      }
      scontexts_[0].moves_[sdata_.numOfMoves_++] = move;
    }
    scontexts_[0].moves_[sdata_.numOfMoves_] = SMove{ true };
    auto* b = scontexts_[0].moves_.data();
    auto* e = b + sdata_.numOfMoves_;
    std::sort(b, e, [](SMove const& m1, SMove const& m2) { return m1 > m2; });
    auto const* hitem = hash_.find(board.fmgr().hashCode());
    if(hitem && hitem->move_)
    {
      SMove best{ hitem->move_.from(), hitem->move_.to(), (Figure::Type)hitem->move_.new_type() };
      X_ASSERT(!board.possibleMove(best), "move from hash is impossible");
      X_ASSERT(!board.validateMove(best), "move from hash is invalid");
      bring_to_front(b, e, best);
    }
  }

  sres.numOfMoves_ = sdata_.numOfMoves_;
  if(!sdata_.numOfMoves_)
    return false;

  for(sdata_.depth_ = depth0_; !stopped() && sdata_.depth_ <= sparams_.depthMax_; ++sdata_.depth_)
  {
    scontexts_[0].plystack_[0].clearPV(sparams_.depthMax_);

    sdata_.restart();

    ScoreType score = alphaBetta0();

    if(sdata_.best_)
    {
      if(stop_ && sdata_.depth_ > 2 &&
        (abs(score-sres.score_) >= Figure::figureWeight_[Figure::TypePawn]/3 || (sdata_.best_ != sres.best_ /*&& abs(score-sres->score_) >= 5*/)) &&
        callbacks_.giveTime_ &&
        !sparams_.analyze_mode_)
      {
        auto t_add = (callbacks_.giveTime_)();
        if(NTime::milli_seconds<int>(t_add) > 0)
        {
          stop_ = false;
          sparams_.timeLimit_ += t_add;
          if(sdata_.counter_ < sdata_.numOfMoves_)
            sdata_.depth_--;
          continue;
        }
      }

      auto t  = NTime::now();
      auto dt = t - sdata_.tstart_;
      sdata_.tprev_ = t;

      sres.score_ = score;
      sres.best_ = sdata_.best_;
      sres.depth_ = sdata_.depth_;
      sres.nodesCount_ = sdata_.nodesCount_;
      sres.totalNodes_ = sdata_.totalNodes_;
      sres.depthMax_ = 0;
      sres.dt_ = dt;
      sres.counter_ = sdata_.counter_;

      for(int i = 0; i < MaxPly; ++i)
      {
        sres.depthMax_ = i;
        sres.pv_[i] = scontexts_[0].plystack_[0].pv_[i];
        if(!sres.pv_[i])
          break;
      }

      X_ASSERT(sres.pv_[0] != sdata_.best_, "invalid PV found");

      if(callbacks_.sendOutput_)
        (callbacks_.sendOutput_)(sres);
    }
    // we haven't found move and spend more time for search it than on prev. iteration
    else if(stop_ && sdata_.depth_ > 2 && callbacks_.giveTime_ && !sparams_.analyze_mode_)
    {
      auto t = NTime::now();
      if((t - sdata_.tprev_) >= (sdata_.tprev_ - sdata_.tstart_))
      {
        auto t_add = (callbacks_.giveTime_)();
        if(t_add > NTime::duration(0))
        {
          stop_ = false;
          sparams_.timeLimit_ += t_add;
          sdata_.depth_--;
          continue;
        }
      }
    }

    if(!sdata_.best_ ||
       ((score >= sparams_.scoreLimit_-MaxPly || score <= MaxPly-sparams_.scoreLimit_) &&
      !sparams_.analyze_mode_))
    {
      break;
    }
  }

  sres.totalNodes_ = sdata_.totalNodes_;

  sres.dt_ = NTime::now() - sdata_.tstart_;

  if(sparams_.analyze_mode_ && callbacks_.sendFinished_)
    (callbacks_.sendFinished_)(sres);

  return sres.best_;
}

//////////////////////////////////////////////////////////////////////////
int Engine::depthIncrement(int ictx, Move const & move, bool pv, bool singular) const
{
  int depthInc = 0;

  if(scontexts_[ictx].board_.underCheck())
    depthInc += ONE_PLY;

  if(!pv || !move.see_ok())
    return depthInc;

  if(move.new_type() || singular)
    return depthInc + ONE_PLY;

  // recapture
  if(scontexts_[ictx].board_.halfmovesCount() > 1)
  {
    auto const& prev = scontexts_[ictx].board_.reverseUndo(1);
    auto const& curr = scontexts_[ictx].board_.lastUndo();

    if(prev.move_.to() == curr.move_.to() && prev.eaten_type_ > 0)
    {
      return depthInc + ONE_PLY;
    }
  }

  return depthInc;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::alphaBetta0()
{
  if(stopped())
    return scontexts_[0].eval_(-ScoreMax, +ScoreMax);

  X_ASSERT(sdata_.numOfMoves_ == 0, "no moves");

  auto& board = scontexts_[0].board_;
  if(sdata_.numOfMoves_ == 0)
  {
    board.setNoMoves();
    ScoreType score = scontexts_[0].eval_(-ScoreMax, +ScoreMax);
    return score;
  }

  ScoreType alpha = -ScoreMax;
  ScoreType betta = +ScoreMax;

#ifndef NDEBUG
  Board board0{ board };
#endif

  for(sdata_.counter_ = 0; sdata_.counter_ < sdata_.numOfMoves_; ++sdata_.counter_)
  {
    if(checkForStop())
      break;

    auto& move = scontexts_[0].moves_[sdata_.counter_];
    ScoreType score = processMove0(move, alpha, betta, !sdata_.counter_);
    X_ASSERT(board != board0, "board undo error");

    if(stopped())
      break;

    auto& hist = history(board.color(), move.from(), move.to());
    if(score > alpha)
    {
      hist.inc_good();
      sdata_.best_ = move;
      alpha = score;
      assemblePV(0, sdata_.best_, board.underCheck(), 0);
    }
    else
      hist.inc_bad();

    if(callbacks_.sendStats_ && sparams_.analyze_mode_)
      (callbacks_.sendStats_)(sdata_);
  } // end of for loop

  // don't need to continue
  if(!stopped() && sdata_.numOfMoves_ == 1 && !sparams_.analyze_mode_)
    pleaseStop();

  if(sdata_.numOfMoves_ == 1)
    return alpha;

  if(sdata_.best_)
  {
    auto& hist = history(board.color(), sdata_.best_.from(), sdata_.best_.to());
    hist.inc_score(sdata_.depth_ / ONE_PLY);
    bring_to_front(scontexts_[0].moves_.data(),
                   scontexts_[0].moves_.data()+sdata_.numOfMoves_,
                   sdata_.best_);
  
    if(sdata_.numOfMoves_ > 2)
    {
      auto* b = scontexts_[0].moves_.data();
      auto* e = b + sdata_.numOfMoves_;
      b++;
      for(auto* m = b; m != e; ++m)
      {
	    if(m->new_type() || board.is_capture(*m))
          continue;
        auto& hist = history(board.color(), m->from(), m->to());
        m->sort_value = hist.score();
      }
      std::sort(b, e, [](SMove const& m1, SMove const& m2) { return m1 > m2; });
    }

    X_ASSERT(!(scontexts_[0].moves_[0] == sdata_.best_), "not best move first");
  }
  return alpha;
}

ScoreType Engine::processMove0(SMove const& move, ScoreType const alpha, ScoreType const betta, bool const pv)
{
  ScoreType score = -ScoreMax;
  const int depth = sdata_.depth_ * ONE_PLY;

  auto& board = scontexts_[0].board_;

#ifdef USE_LMR0
  bool check_escape = board.underCheck();
#endif

  board.makeMove(move);
  sdata_.inc_nc();

  if(!stopped())
  {
    if(pv)
    {
      int depthInc = depthIncrement(0, move, true, false);
      score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -betta, -alpha, true, true);
    }
    else
    {
      int depthInc = depthIncrement(0, move, false, false);

      int R = 0;
  #ifdef USE_LMR0
      if(!check_escape &&
         sdata_.depth_ * ONE_PLY > LMR_MinDepthLimit &&
         depth >= LMR_DepthLimit &&
         alpha > -Figure::MatScore-MaxPly &&
         board.canBeReduced(move))
      {
        R = ONE_PLY;
      }
  #endif
      score = -alphaBetta(0, depth + depthInc - R - ONE_PLY, 1, -alpha-1, -alpha, false, true);
      if(!stopped() && score > alpha && R > 0)
        score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -alpha - 1, -alpha, false, true);

      if(!stopped() && score > alpha)
      {
        depthInc = depthIncrement(0, move, true, false);
        score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -betta, -alpha, true, true);
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

  auto& board = scontexts_[ictx].board_;

  if(ply < MaxPly-1)
  {
    scontexts_[ictx].plystack_[ply].clear(ply);
    scontexts_[ictx].plystack_[ply+1].clearKiller();
  }

  if(board.drawState() || board.hasReps())
    return Figure::DrawScore;

  if(stopped() || ply >= MaxPly)
    return scontexts_[ictx].eval_(alpha, betta);

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

  bool dangerous = false;
  if (depth <= 0)
  {
    auto score = captures(ictx, depth, ply, alpha, betta, pv, dangerous);
    if (!dangerous || depth < 0 || score <= alpha)// || !pv)
      return score;
  }

  bool mat_threat{false};
  bool nm_threat{false};
  auto const& prev = board.lastUndo();

#ifdef USE_NULL_MOVE
  if(
    !scontexts_[ictx].board_.underCheck()
    && !pv
    && allow_nm
    && scontexts_[ictx].board_.allowNullMove()
    && depth > scontexts_[ictx].board_.nullMoveDepthMin()
    && std::abs(betta) < Figure::MatScore+MaxPly
    )
  {
    int null_depth = scontexts_[ictx].board_.nullMoveDepth(depth, betta);

    // do null-move
    scontexts_[ictx].board_.makeNullMove();
    ScoreType nullScore = -alphaBetta(ictx, null_depth, ply+1, -betta, -(betta-1), false, false);
    scontexts_[ictx].board_.unmakeNullMove();

    // verify null-move with shortened depth
    if(nullScore >= betta)
    {
      depth = null_depth;
      bool dng = false;
      if(depth <= 0)
        return captures(ictx, depth, ply, alpha, betta, pv, dng);
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
      && !scontexts_[ictx].board_.underCheck()
      && alpha > -Figure::MatScore+MaxPly
      && alpha < Figure::MatScore-MaxPly
      && depth > 0 && depth <= ONE_PLY && ply > 1
      && !scontexts_[ictx].board_.isWinnerLoser())
  {
    static const int thresholds_[4] = { 0,
      0,
      Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypePawn],
      Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypeRook] };

    ScoreType score0 = scontexts_[ictx].eval_(alpha, betta);
    bool dang = scontexts_[ictx].eval_.dangerous();
    int threshold = (int)alpha - (int)score0 - Position_Gain;
    if (threshold > thresholds_[(depth / ONE_PLY) & 3])
    {
      auto score = captures(ictx, depth, ply, alpha, betta, pv, dang, score0, true);
      if (!dang || score <= alpha)
        return score;
    }
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
    if(const HItem * hitem = hash_.find(scontexts_[ictx].board_.fmgr().hashCode()))
    {
      X_ASSERT(hitem->hkey_ != scontexts_[ictx].board_.fmgr().hashCode(), "invalid hash item found");
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
  FastGenerator<Board, SMove> fg(board, hmove, scontexts_[ictx].plystack_[ply].killer_);
  for (; alpha < betta && !checkForStop();)
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
    sdata_.inc_nc();
    int depthInc = 0;
    auto& curr = board.lastUndo();
    bool check_or_cap = curr.capture() || board.underCheck();

    //if (sdata_.depth_ == 11 && findSequence(ictx, move, ply, true))
    //{
    //  depthInc = depthInc;
    //}

#ifdef USE_PROBCUT
    ScoreType betta_pc = betta < ScoreMax - 1000 ? betta + 300 : betta;
    if (!pv
      && curr.capture()
      && !danger_pawn
      && move.see_ok()
      && !check_escape
      && !scontexts_[ictx].board_.isWinnerLoser()
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
          sdata_.depth_ * ONE_PLY > LMR_MinDepthLimit &&
          depth >= LMR_DepthLimit &&
          alpha > -Figure::MatScore - MaxPly &&
          scontexts_[ictx].board_.canBeReduced(move))
        {
          R = ONE_PLY;

#ifdef LMR_REDUCE_MORE
          auto const& hist = history(Figure::otherColor(board.color()), move.from(), move.to());
          if(depth > LMR_DepthLimit && (!move.see_ok() || hist.good()*20 < hist.bad()))
          {
            R += ONE_PLY;
            if(depth > LMR_DepthLimit+ONE_PLY && counter > 10)
              R += ONE_PLY;
          }
        //  curr.mflags_ |= UndoInfo::Reduced;
        //}
        //else if(!check_escape &&
        //      counter > 10 &&
        //      sdata_.depth_ * ONE_PLY > LMR_MinDepthLimit &&
        //      depth > LMR_DepthLimit &&
        //      alpha > -Figure::MatScore-MaxPly &&
        //      !move.see_ok() &&
        //      !curr.castle() &&
        //      !board.underCheck())
        //{
        //  R = ONE_PLY;
#endif // LMR_REDUCE_MORE

          curr.mflags_ |= UndoInfo::Reduced;
        }
#endif

        score = -alphaBetta(ictx, depth + depthInc - R - ONE_PLY, ply + 1, -alpha - 1, -alpha, false, allow_nm);
        curr.mflags_ &= ~UndoInfo::Reduced;

        if (!stopped() && score > alpha && R > 0)
          score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply + 1, -alpha - 1, -alpha, false, allow_nm);

        if (!stopped() && score > alpha && score < betta && pv)
        {
          depthInc = depthIncrement(0, move, pv, singular);
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

    if(!stopped())
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
            scontexts_[ictx].plystack_[ply].killer_ = move;
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

  if(stopped())
    return scoreBest;

  if(!counter)
  {
    board.setNoMoves();
    scoreBest = scontexts_[ictx].eval_(alpha, betta);
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
    sdata_.inc_nc();

    alpha = alpha0;
    ScoreType score = -alphaBetta(ictx, depth+depthIncBest, ply+1, -betta, -alpha, pv, true);

    board.unmakeMove(best);

    if(!stopped())
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
    if ( !stopped() && !dont_reduce && (mat_threat || (alpha >= betta && nm_threat && isRealThreat(ictx, best))) )
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
ScoreType Engine::captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool& dangerous, ScoreType score0, bool fpr)
{
  if(alpha >= Figure::MatScore-ply)
    return alpha;

  auto& board = scontexts_[ictx].board_;

  if(board.drawState() || board.hasReps())
    return Figure::DrawScore;

  if(stopped() || ply >= MaxPly)
    return Figure::DrawScore;

  SMove hmove{true};
  
#ifdef USE_HASH
  ScoreType hscore = -ScoreMax;
  bool singular = false;
  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv, singular);
  if(flag == GHashTable::Alpha || flag == GHashTable::Betta)
  {
    return hscore;
  }
  X_ASSERT(hmove && !board.possibleMove(hmove), "impossible move in hash");
  if(!board.underCheck() && depth < -12*ONE_PLY && hmove && !board.is_capture(hmove))
    hmove = SMove{true};
#endif

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  int threshold = 0;

  if(!board.underCheck())
  {
    // not initialized yet
    if (score0 == -ScoreMax)
    {
      score0 = scontexts_[ictx].eval_(alpha, betta);
      dangerous = scontexts_[ictx].eval_.dangerous();
    }

    if(score0 >= betta)
      return score0;

    threshold = (int)alpha - (int)score0 - Position_Gain;
    if(threshold > Figure::figureWeight_[Figure::TypePawn] && board.isWinnerLoser())
      threshold = Figure::figureWeight_[Figure::TypePawn];
    if(score0 > alpha)
      alpha = score0;

    scoreBest = score0;
  }

  Move best{true};

#ifndef NDEBUG
  Board board0{ board };
#endif

  TacticalGenerator<Board, SMove> tg(board, hmove, depth, fpr);
  for(; alpha < betta && !checkForStop();)
  {
    auto* pmove = tg.next();
    if(!pmove)
      break;

    auto& move = *pmove;
    X_ASSERT(!board.validateMove(move), "invalid move got from generator");

    if((!board.underCheck() || counter > 0) && !move.see_ok() && !board.see(move, threshold))
      continue;

    dangerous = false;
    ScoreType score = -ScoreMax;

    board.makeMove(move);
    sdata_.inc_nc();

    int depthInc = board.underCheck() ? ONE_PLY : 0;
    bool dng = false;
    score = -captures(ictx, depth + depthInc - ONE_PLY, ply+1, -betta, -alpha, pv, dng, -ScoreMax);

    board.unmakeMove(move);
    X_ASSERT(board != board0, "board undo error");

    if(!stopped() && score > scoreBest)
    {
      best = move;
      scoreBest = score;
      if(score > alpha)
      {
        alpha = score;
      }
    }
    counter++;
  }

  if(stopped())
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
  putCaptureHash(ictx, best);
#endif

  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
  return scoreBest;
}


/// extract data from hash table
#ifdef USE_HASH
void Engine::prefetchHash(int ictx)
{
  hash_.prefetch(scontexts_[ictx].board_.fmgr().hashCode());
}

GHashTable::Flag Engine::getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv, bool& singular)
{
  auto& board = scontexts_[ictx].board_;
  auto const* hitem = hash_.find(board.fmgr().hashCode());
  if(!hitem)
    return GHashTable::NoFlag;

  X_ASSERT(hitem->hkey_ != board.fmgr().hashCode(), "invalid hash item found");

  singular = hitem->singular_;
  hmove = hitem->move_;
  if(pv)
    return GHashTable::AlphaBetta;

  depth = (depth + ONE_PLY - 1) / ONE_PLY;

  hscore = hitem->score_;
  if(hscore >= Figure::MatScore-MaxPly)
    hscore = hscore - ply;
  else if(hscore <= MaxPly-Figure::MatScore)
    hscore = hscore + ply;

  X_ASSERT(hscore > 32760 || hscore < -32760, "invalid value in hash");

  if(hitem->flag_  != GHashTable::NoFlag && (int)hitem->depth_ >= depth && ply > 0 && hscore != 0)
  {
    if(GHashTable::Alpha == hitem->flag_ && hscore <= alpha)
    {
      X_ASSERT(!stop_ && alpha < -32760, "invalid hscore");
      return GHashTable::Alpha;
    }

    if(hitem->flag_ > GHashTable::Alpha && hscore >= betta && hmove)
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
        if(hitem->threat_ && prev.is_reduced())
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
  if (scontexts_[ictx].board_.hasReps())
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
  hash_.push(scontexts_[ictx].board_.fmgr().hashCode(), score, depth / ONE_PLY, flag, move, threat, singular);
}

void Engine::putCaptureHash(int ictx, const Move & move)
{
  if(!move || scontexts_[ictx].board_.hasReps())
    return;
  hash_.push(scontexts_[ictx].board_.fmgr().hashCode(), 0, 0, GHashTable::NoFlag, move, false, false);
}
#endif


//////////////////////////////////////////////////////////////////////////
// is given movement caused by previous? this mean that if we don't do this move we loose
// we actually test if moved figure was/will be attacked by previously moved one or from direction it was moved from
// or we should move our king even if we have a lot of other figures
//////////////////////////////////////////////////////////////////////////
bool Engine::isRealThreat(int ictx, const Move & move)
{
  auto const& board = scontexts_[ictx].board_;

  auto const& prev = board.lastUndo();
  if(prev.is_nullmove())
    return false;

  auto const color = board.color();
  auto const ocolor = Figure::otherColor(color);

  auto const& pfield = scontexts_[ictx].board_.getField(prev.move_.to());
  X_ASSERT(!pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat");

  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
  if(prev.capture() || prev.move_.new_type() > 0 || prev.check() || prev.threat())
    return false;

  /// we have to move king even if there a lot of figures
  if(scontexts_[ictx].board_.getField(move.from()).type() == Figure::TypeKing &&
    scontexts_[ictx].board_.fmgr().queens(color) > 0 &&
    scontexts_[ictx].board_.fmgr().rooks(color) > 0 &&
    scontexts_[ictx].board_.fmgr().knights(color) + scontexts_[ictx].board_.fmgr().bishops(color) > 0)
  {
    return true;
  }

  auto const& cfield = scontexts_[ictx].board_.getField(move.from());
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
