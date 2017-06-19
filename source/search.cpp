/*************************************************************
  search.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/


#include <engine.h>
#include <MovesGenerator.h>
#include "Evaluator.h"
#include <algorithm>
#include <Helpers.h>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
inline Figure::Type delta2type(int delta)
{
  Figure::Type minimalType = Figure::TypePawn;

#ifdef USE_DELTA_PRUNING
  if ( delta > Figure::figureWeight_[Figure::TypeQueen] )
    minimalType = Figure::TypeKing;
  else if ( delta > Figure::figureWeight_[Figure::TypeRook] )
    minimalType = Figure::TypeQueen;
  else if ( delta > Figure::figureWeight_[Figure::TypeBishop] )
    minimalType = Figure::TypeRook;
  else if ( delta > Figure::figureWeight_[Figure::TypeKnight] )
    minimalType = Figure::TypeBishop;
  else if ( delta > Figure::figureWeight_[Figure::TypePawn] )
    minimalType = Figure::TypeKnight;
#endif

  return minimalType;
}

inline int calculateDelta(ScoreType alpha, ScoreType score)
{
  int delta = (int)alpha - (int)score - Position_Gain;
  return delta;
}

bool Engine::search(SearchResult& sres)
{
#ifdef USE_HASH
  hash_.inc();
#endif

  reset();

  // stash board to correctly print status later
  sres.board_ = scontexts_[0].board_;
  sdata_.board_ = scontexts_[0].board_;

  {
    auto moves = generate<Board, SMove>(scontexts_[0].board_);
    for(auto move : moves)
    {
      move.sort_value = 0;
      if(scontexts_[0].board_.validateMoveBruteforce(move))
        scontexts_[0].moves_[sdata_.numOfMoves_++] = move;
    }
    scontexts_[0].moves_[sdata_.numOfMoves_] = SMove{ true };
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

    if(scontexts_[ictx].board_.getField(move.to()) && (prev.move_.to() == curr.move_.to() || curr.enpassant() == curr.move_.to()))
      return depthInc + ONE_PLY;
  }

  return depthInc;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::alphaBetta0()
{
  if(stopped())
    return scontexts_[0].eval_(-ScoreMax, +ScoreMax);

  if(sdata_.numOfMoves_ == 0)
  {
    scontexts_[0].board_.setNoMoves();
    ScoreType score = scontexts_[0].eval_(-ScoreMax, +ScoreMax);
    return score;
  }

  ScoreType alpha = -ScoreMax;
  ScoreType betta = +ScoreMax;
  const int depth = sdata_.depth_ * ONE_PLY;

  bool check_escape = scontexts_[0].board_.underCheck();
  int sortDepth = 4;
  int numMovesNoReduce = 5;

  const bool bDoSort = sdata_.depth_ <= sortDepth;

  for(sdata_.counter_ = 0; sdata_.counter_ < sdata_.numOfMoves_; ++sdata_.counter_)
  {
    if(checkForStop())
      break;

    auto& move = scontexts_[0].moves_[sdata_.counter_];
    ScoreType score = -ScoreMax;

    scontexts_[0].board_.makeMove(move);
    sdata_.inc_nc();

    int depthInc = (scontexts_[0].board_.underCheck()) ? ONE_PLY : 0;
    if(!stopped())
      score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -betta, -alpha, true, true);

    scontexts_[0].board_.unmakeMove(move);

    if(!stopped())
    {
      move.sort_value = score;
      if(score > alpha)
      {
        sdata_.best_ = move;
        alpha = score;
        assemblePV(0, move, scontexts_[0].board_.underCheck(), 0);
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
    auto iter = scontexts_[0].moves_.begin();
    std::advance(iter, sdata_.numOfMoves_);
    std::sort(scontexts_[0].moves_.begin(), iter, [](SMove const& m1, SMove const& m2)
    {
      return m1 > m2;
    });
    if(!(scontexts_[0].moves_[0] == sdata_.best_))
    {
      for(int i = 0; i < sdata_.numOfMoves_; ++i)
      {
        if(sdata_.best_ == scontexts_[0].moves_[i])
        {
          std::swap(scontexts_[0].moves_[i], scontexts_[0].moves_[0]);
          break;
        }
      }
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

  if(ply < MaxPly-1)
  {
    scontexts_[ictx].plystack_[ply].clear(ply);
    scontexts_[ictx].plystack_[ply+1].clearKiller();
  }

  if(scontexts_[ictx].board_.drawState() || scontexts_[ictx].board_.hasReps())
    return Figure::DrawScore;

  if(stopped() || ply >= MaxPly)
    return scontexts_[ictx].eval_(alpha, betta);

  ScoreType alpha0 = alpha;

  if(depth <= 0)
    return captures(ictx, depth, ply, alpha, betta, pv);

  ScoreType scoreBest = -ScoreMax;
  Move best{true};
  int counter{};

  FastGenerator<Board, Move> fg(scontexts_[ictx].board_);
  for(; alpha < betta && !checkForStop();)
  {
    auto* pmove = fg.next();
    if(!pmove)
      break;

    auto& move = *pmove;
    if(!scontexts_[ictx].board_.validateMove(move))
      continue;

    ScoreType score = -ScoreMax;

    scontexts_[ictx].board_.makeMove(move);
    sdata_.inc_nc();

    int depthInc = (scontexts_[0].board_.underCheck()) ? ONE_PLY : 0;
    score = -alphaBetta(ictx, depth + depthInc - ONE_PLY, ply+1, -betta, -alpha, pv, true);

    scontexts_[ictx].board_.unmakeMove(move);

    if(!stopped())
    {
      if(score > scoreBest)
      {
        best = move;
        scoreBest = score;
        if(score > alpha)
        {
          alpha = score;
          if(pv)
            assemblePV(ictx, move, scontexts_[ictx].board_.underCheck(), ply);
        }
      }
    }

    ++counter;
  }

  if(stopped())
    return scoreBest;

  if(!counter)
  {
    scontexts_[ictx].board_.setNoMoves();
    scoreBest = scontexts_[ictx].eval_(alpha, betta);
    if(scontexts_[ictx].board_.matState())
      scoreBest += ply;
  }

  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
{
  if(alpha >= Figure::MatScore-ply)
    return alpha;

  if(scontexts_[ictx].board_.drawState() || scontexts_[ictx].board_.hasReps())
    return Figure::DrawScore;

  if(stopped() || ply >= MaxPly)
    return Figure::DrawScore;

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  int threshold = 0;

  if(!scontexts_[ictx].board_.underCheck())
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

  TacticalGenerator<Board, Move> tg(scontexts_[ictx].board_, depth);
  for(; alpha < betta && !checkForStop();)
  {
    auto* pmove = tg.next();
    if(!pmove)
      break;

    auto& move = *pmove;
    if(!scontexts_[ictx].board_.validateMove(move))
      continue;

    if((!scontexts_[ictx].board_.underCheck() || counter > 0) && !scontexts_[ictx].board_.see(move, threshold))
      continue;

    ScoreType score = -ScoreMax;

    scontexts_[ictx].board_.makeMove(move);
    sdata_.inc_nc();

    int depthInc = (scontexts_[0].board_.underCheck()) ? ONE_PLY : 0;
    score = -captures(ictx, depth + depthInc - ONE_PLY, ply+1, -betta, -alpha, pv, -ScoreMax);

    scontexts_[ictx].board_.unmakeMove(move);

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
    if(scontexts_[ictx].board_.underCheck())
      scoreBest = -Figure::MatScore+ply;
    else
      scoreBest = score0;
  }

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
  const HItem * hitem = hash_.find(scontexts_[ictx].board_.fmgr().hashCode());
  if(!hitem)
    return GHashTable::NoFlag;

  X_ASSERT(hitem->hkey_ != scontexts_[ictx].board_.fmgr().hashCode(), "invalid hash item found");

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
      if(!scontexts_[ictx].board_.hasReps(hmove))
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
  hash_.push(scontexts_[ictx].board_.fmgr().hashCode(), score, depth / ONE_PLY, flag, move, threat);
}

void Engine::putCaptureHash(int ictx, const Move & move)
{
  if(!move || scontexts_[ictx].board_.hasReps())
    return;
  hash_.push(scontexts_[ictx].board_.fmgr().hashCode(), 0, 0, GHashTable::NoFlag, move, false);
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
