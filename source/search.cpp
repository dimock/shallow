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
      if(board.validateMoveBruteforce(move))
        scontexts_[0].moves_[sdata_.numOfMoves_++] = move;
    }
    scontexts_[0].moves_[sdata_.numOfMoves_] = SMove{ true };
    if(auto const* hitem = hash_.find(board.fmgr().hashCode()))
    {
      if(hitem->move_)
      {
        SMove best{ hitem->move_.from(), hitem->move_.to(), (Figure::Type)hitem->move_.new_type() };
        X_ASSERT(!board.possibleMove(best), "move from hash is impossible");
        X_ASSERT(!board.validateMove(best), "move from hash is invalid");
        auto* b = scontexts_[0].moves_.data();
        auto* e = b + sdata_.numOfMoves_;
        if(*b != best)
          bring_to_front(b, e, best);
      }
    }
  }

  sres.numOfMoves_ = sdata_.numOfMoves_;

  for(sdata_.depth_ = depth0_; !stopped() && sdata_.depth_ <= sparams_.depthMax_; ++sdata_.depth_)
  {
    scontexts_[0].plystack_[0].clearPV(sparams_.depthMax_);

    sdata_.restart();

    ScoreType score = alphaBetta0();

    if(sdata_.best_)
    {
      if(stop_ && sdata_.depth_ > 2 &&
        (abs(score-sres.score_) >= Figure::figureWeight_[Figure::TypePawn]/2 || (sdata_.best_ != sres.best_ /*&& abs(score-sres->score_) >= 5*/)) &&
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
int Engine::depthIncrement(int ictx, Move const & move, bool pv) const
{
  int depthInc = 0;

  if(scontexts_[ictx].board_.underCheck())
    depthInc += ONE_PLY;

  if(!pv || !move.see_ok())
    return depthInc;

  if(move.new_type())
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

  auto& board = scontexts_[0].board_;
  if(sdata_.numOfMoves_ == 0)
  {
    board.setNoMoves();
    ScoreType score = scontexts_[0].eval_(-ScoreMax, +ScoreMax);
    return score;
  }

  ScoreType alpha = -ScoreMax;
  const int depth = sdata_.depth_ * ONE_PLY;

  for(sdata_.counter_ = 0; sdata_.counter_ < sdata_.numOfMoves_; ++sdata_.counter_)
  {
    if(checkForStop())
      break;

    auto& move = scontexts_[0].moves_[sdata_.counter_];
    ScoreType score = -ScoreMax;

    board.makeMove(move);
    sdata_.inc_nc();

    //int depthInc = depthIncrement(0, move, true);// board.underCheck() ? ONE_PLY : 0;
    if(!stopped())
    {
      if(!sdata_.counter_)
      {
        int depthInc = depthIncrement(0, move, true);
        score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -ScoreMax, -alpha, true, true);
      }
      else
      {
        int depthInc = depthIncrement(0, move, false);
        score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -alpha-1, -alpha, false, true);
        if(!stopped() && score > alpha)
        {
          depthInc = depthIncrement(0, move, true);
          score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -ScoreMax, -alpha, true, true);
        }
      }
    }

    board.unmakeMove(move);

    if(!stopped())
    {
      move.sort_value = score;
      if(score > alpha)
      {
        sdata_.best_ = move;
        alpha = score;
        assemblePV(0, sdata_.best_, board.underCheck(), 0);
      }
    }

    if(callbacks_.sendStats_ && sparams_.analyze_mode_)
      (callbacks_.sendStats_)(sdata_);
  }

  // don't need to continue
  if(!stopped() && sdata_.counter_ == 1 && !sparams_.analyze_mode_)
    pleaseStop();

  if(!stopped())
  {
    if(sdata_.numOfMoves_ > 1)
    {
      auto* b = scontexts_[0].moves_.data();
      auto* e = b + sdata_.numOfMoves_;
      std::sort(b, e, [](SMove const& m1, SMove const& m2) { return m1 > m2; });
      if(*b != sdata_.best_)
        bring_to_front(b, e, sdata_.best_);
    }
    X_ASSERT(!(scontexts_[0].moves_[0] == sdata_.best_), "not best move first");
  }

  return alpha;
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

  ScoreType alpha0 = alpha;
  SMove hmove{true};
  
#ifdef USE_HASH
  ScoreType hscore = -ScoreMax;
  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv);
  if(flag == GHashTable::Alpha || flag == GHashTable::Betta)
  {
    return hscore;
  }
  X_ASSERT(hmove && !board.possibleMove(hmove), "impossible move in hash"); 
#endif

  if(depth <= 0)
    return captures(ictx, depth, ply, alpha, betta, pv);

  ScoreType scoreBest = -ScoreMax;
  Move best{true};
  int counter{};

  FastGenerator<Board, SMove> fg(board, hmove, scontexts_[ictx].plystack_[ply].killer_);
  for(; alpha < betta && !checkForStop();)
  {
    auto* pmove = fg.next();
    if(!pmove)
      break;

    auto& move = *pmove;
    X_ASSERT(!board.validateMove(move), "invalid move got from generator");

    ScoreType score = -ScoreMax;
    board.makeMove(move);
    sdata_.inc_nc();

    if(!counter)
    {
      int depthInc = depthIncrement(ictx, move, pv);
      score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply+1, -betta, -alpha, pv, true);
    }
    else
    {
      int depthInc = depthIncrement(ictx, move, false);
      score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply+1, -alpha-1, -alpha, false, true);
      if(!stopped() && score > alpha && score < betta && pv)
      {
        depthInc = depthIncrement(0, move, pv);
        score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply+1, -betta, -alpha, pv, true);
      }
    }

    board.unmakeMove(move);

    if(!stopped())
    {
      auto& hist = history(board.color(), move.from(), move.to());
      if(score > scoreBest)
      {
        hist.inc_good();
        best = move;
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

  if(best)
  {
    auto& hist = history(board.color(), best.from(), best.to());
    hist.inc_score(depth / ONE_PLY);
  }

#ifdef USE_HASH
  putHash(ictx, best, alpha0, betta, scoreBest, depth, ply, false);
#endif

  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
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
  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv);
  if(flag == GHashTable::Alpha || flag == GHashTable::Betta)
  {
    return hscore;
  }
  X_ASSERT(hmove && !board.possibleMove(hmove), "impossible move in hash");
  if(hmove && !board.getField(hmove.to()))
    hmove = SMove{true};
#endif

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  int threshold = 0;

  if(!board.underCheck())
  {
    // not initialized yet
    if(score0 == -ScoreMax)
      score0 = scontexts_[ictx].eval_(alpha, betta);

    if(score0 >= betta)
      return score0;

    threshold = (int)alpha - (int)score0 - Position_Gain;
    if(score0 > alpha)
      alpha = score0;

    scoreBest = score0;
  }

  Move best{true};

  TacticalGenerator<Board, SMove> tg(board, hmove, depth);
  for(; alpha < betta && !checkForStop();)
  {
    auto* pmove = tg.next();
    if(!pmove)
      break;

    auto& move = *pmove;
    X_ASSERT(!board.validateMove(move), "invalid move got from generator");

    if((!board.underCheck() || counter > 0) && !move.see_ok() && !board.see(move, threshold))
      continue;

    ScoreType score = -ScoreMax;

    board.makeMove(move);
    sdata_.inc_nc();

    int depthInc = board.underCheck() ? ONE_PLY : 0;
    score = -captures(ictx, depth + depthInc - ONE_PLY, ply+1, -betta, -alpha, pv, -ScoreMax);

    board.unmakeMove(move);

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

GHashTable::Flag Engine::getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv)
{
  auto& board = scontexts_[ictx].board_;
  auto const* hitem = hash_.find(board.fmgr().hashCode());
  if(!hitem)
    return GHashTable::NoFlag;

  X_ASSERT(hitem->hkey_ != board.fmgr().hashCode(), "invalid hash item found");

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
      else
      {
        ///// danger move was reduced - recalculate it with full depth
        //auto const& prev = scontexts_[ictx].board_.lastUndo();
        //if(hitem->threat_ && prev.reduced_)
        //{
        //  hscore = betta-1;
        //  return GHashTable::Alpha;
        //}

        return GHashTable::Betta;
      }
    }
  }

  return GHashTable::AlphaBetta;
}

/// insert data to hash table
void Engine::putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat)
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
  hash_.push(scontexts_[ictx].board_.fmgr().hashCode(), score, depth / ONE_PLY, flag, move);
}

void Engine::putCaptureHash(int ictx, const Move & move)
{
  if(!move || scontexts_[ictx].board_.hasReps())
    return;
  hash_.push(scontexts_[ictx].board_.fmgr().hashCode(), 0, 0, GHashTable::NoFlag, move);
}
#endif


////////////////////////////////////////////////////////////////////////////
//// is given movement caused by previous? this mean that if we don't do this move we loose
//// we actually test if moved figure was/will be attacked by previously moved one or from direction it was moved from
//// or we should move our king even if we have a lot of other figures
////////////////////////////////////////////////////////////////////////////
//bool Engine::isRealThreat(int ictx, const Move & move)
//{
//  // don't need to forbid if our answer is capture or check ???
//  if(move.capture_ || move.checkFlag_)
//    return false;
//
//  const UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(0);
//  if(!prev) // null-move
//    return false;
//
//  Figure::Color  color = scontexts_[ictx].board_.getColor();
//  Figure::Color ocolor = Figure::otherColor(color);
//
//  const Field & pfield = scontexts_[ictx].board_.getField(prev.to_);
//  X_ASSERT(!pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat");
//
//  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
//  if(prev.capture_ || prev.new_type_ > 0 || prev.checkingNum_ > 0 || prev.threat_)
//    return false;
//
//  /// we have to move king even if there a lot of figures
//  if(scontexts_[ictx].board_.getField(move.from_).type() == Figure::TypeKing &&
//    scontexts_[ictx].board_.fmgr().queens(color) > 0 &&
//    scontexts_[ictx].board_.fmgr().rooks(color) > 0 &&
//    scontexts_[ictx].board_.fmgr().knights(color) + scontexts_[ictx].board_.fmgr().bishops(color) > 0)
//  {
//    return true;
//  }
//
//  const Field & cfield = scontexts_[ictx].board_.getField(move.from_);
//  X_ASSERT(!cfield || cfield.color() != color, "no figure of required color in while detecting threat");
//
//  // we have to put figure under attack
//  if(scontexts_[ictx].board_.ptAttackedBy(move.to_, prev.to_))
//    return true;
//
//  // put our figure under attack, opened by prev movement
//  if(scontexts_[ictx].board_.getAttackedFrom(ocolor, move.to_, prev.from_) >= 0)
//    return true;
//
//  // prev move was attack, and we should escape from it
//  if(scontexts_[ictx].board_.ptAttackedBy(move.from_, prev.to_))
//    return true;
//
//  // our figure was attacked from direction, opened by prev movement
//  if(scontexts_[ictx].board_.getAttackedFrom(ocolor, move.from_, prev.from_) >= 0)
//    return true;
//
//  return false;
//}

int Engine::xcounter_ = 0;

} // NEngine
