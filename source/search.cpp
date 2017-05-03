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
    MovesGenerator mg(scontexts_[0].board_);
    for(auto move : mg.moves())
    {
      move.vsort_ = 0;
      if(scontexts_[0].board_.validateMove(move))
        scontexts_[0].moves_[sdata_.numOfMoves_++] = move;
    }
    scontexts_[0].moves_[sdata_.numOfMoves_].clear();
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
int Engine::depthIncrement(int ictx, Move & move, bool pv) const
{
  int depthInc = 0;

  if(scontexts_[ictx].board_.underCheck())
    depthInc += ONE_PLY;

  if(!pv || !move.see_good_)
    return depthInc;

  if(move.new_type_ == Figure::TypeQueen || move.new_type_ == Figure::TypeKnight)
    return depthInc + ONE_PLY;

  // recapture
  if(scontexts_[ictx].board_.halfmovesCount() > 1)
  {
    const UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(-1);
    const UndoInfo & curr = scontexts_[ictx].board_.undoInfoRev(0);

    if(move.capture_ && (prev.to_ == curr.to_ || curr.en_passant_ == curr.to_))
      return depthInc + ONE_PLY;
  }

  //// entering pawns endgame
  //if (move.capture_)
  //{
  //  int numFigures = scontexts_[ictx].board_.fmgr().queens(Figure::ColorBlack) + scontexts_[ictx].board_.fmgr().queens(Figure::ColorWhite) +
  //    scontexts_[ictx].board_.fmgr().rooks(Figure::ColorBlack) + scontexts_[ictx].board_.fmgr().rooks(Figure::ColorWhite) +
  //    scontexts_[ictx].board_.fmgr().bishops(Figure::ColorBlack) + scontexts_[ictx].board_.fmgr().bishops(Figure::ColorWhite) +
  //    scontexts_[ictx].board_.fmgr().knights(Figure::ColorBlack) + scontexts_[ictx].board_.fmgr().knights(Figure::ColorWhite);

  //  int numPawns = scontexts_[ictx].board_.fmgr().pawns(Figure::ColorBlack) + scontexts_[ictx].board_.fmgr().pawns(Figure::ColorWhite);
  //  if (numFigures == 0 && numPawns > 0)
  //    return depthInc + ONE_PLY;
  //}

  //// pawn go to 7th line
  //if ( move.threat_ )
  //  depthInc += ONE_PLY / 4;

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

#ifdef LOG_PV
  logMovies();
#endif

  for(sdata_.counter_ = 0; sdata_.counter_ < sdata_.numOfMoves_; ++sdata_.counter_)
  {
    if(checkForStop())
      break;

    Move & move = scontexts_[0].moves_[sdata_.counter_];
    ScoreType score = -ScoreMax;

    if(!move.seen_)
    {
      move.see_good_ = scontexts_[0].board_.see(move) >= 0;
      move.seen_ = 1;
    }

    if(scontexts_[0].board_.isMoveThreat(move))
      move.threat_ = 1;

    scontexts_[0].board_.makeMove(move);
    sdata_.inc_nc();

    bool fullRange = false;

    {
      int depthInc = depthIncrement(0, move, true);
      bool strongMove = (((int)move.vsort_ - (int)ScoreMax) > (alpha-500)) || (sdata_.counter_ < numMovesNoReduce);

      if(bDoSort || sdata_.counter_ < 1)
      {
        score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -betta, bDoSort ? ScoreMax : -alpha, true);
        fullRange = true;
      }
      else
      {
        int depthInc2 = depthIncrement(0, move, strongMove);
        int R = 0;


#ifdef USE_LMR
        if ( !check_escape &&
          sdata_.counter_ >= numMovesNoReduce &&
          depth > LMR_MinDepthLimit &&
          //alpha > -Figure::MatScore-MaxPly && 
          scontexts_[0].board_.canBeReduced() )
        {
          R = ONE_PLY;
        }
#endif

        score = -alphaBetta(0, depth + depthInc2 - ONE_PLY - R, 1, -alpha-1, -alpha, strongMove);

        if(!stopped() && score > alpha && R > 0)
          score = -alphaBetta(0, depth + depthInc2 - ONE_PLY, 1, -alpha - 1, -alpha, strongMove);

        if(!stopped() && score > alpha)
        {
          score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -betta, -alpha, true);
          fullRange = true;
        }
      }
    }

    scontexts_[0].board_.unmakeMove();

    if(!stopped())
    {
      if(bDoSort)
        move.vsort_ = score + ScoreMax;

      if(score > alpha)
      {
        move.vsort_ = score + ScoreMax;
        sdata_.best_ = move;
        alpha = score;

        assemblePV(0, move, scontexts_[0].board_.underCheck(), 0);

        // bring best move to front, shift other moves 1 position right
        if(!bDoSort)
        {
          for(int j = sdata_.counter_; j > 0; --j)
            scontexts_[0].moves_[j] = scontexts_[0].moves_[j-1];
          scontexts_[0].moves_[0] = sdata_.best_;
        }
      }

#ifdef LOG_PV
      logPV();
#endif
    }


    if(callbacks_.sendStats_ && sparams_.analyze_mode_)
      (callbacks_.sendStats_)(sdata_);
  }

  // don't need to continue
  if(!stopped() && sdata_.counter_ == 1 && !sparams_.analyze_mode_)
    pleaseStop();

  if(!stopped())
  {
    // full sort only on first iterations
    if(bDoSort)
    {
      auto iter = scontexts_[0].moves_.begin();
      std::advance(iter, sdata_.numOfMoves_);
      std::sort(scontexts_[0].moves_.begin(), iter);
    }
    //// then sort all moves but 1st
    //else if ( sdata_.numOfMoves_ > 2 )
    //  std::sort(scontexts_[0].moves_+1, scontexts_[0].moves_ + sdata_.numOfMoves_);
  }

  return alpha;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv)
{
  prefetchHash(ictx);

  if(alpha >= Figure::MatScore-ply)
    return alpha;

  if(ply < MaxPly-1)
  {
    scontexts_[ictx].plystack_[ply].clear(ply);
    scontexts_[ictx].plystack_[ply+1].clearKiller();
  }

  if(scontexts_[ictx].board_.drawState() || scontexts_[ictx].board_.countReps() > 1)
    return Figure::DrawScore;

  if(stopped() || ply >= MaxPly)
    return scontexts_[ictx].eval_(alpha, betta);

  ////bool light_null_move{};
  //if(options_.use_nullmove_
  //   && !pv
  //   && !scontexts_[ictx].board_.underCheck()
  //   && scontexts_[ictx].board_.allowNullMove()
  //   && depth >= scontexts_[ictx].board_.nullMoveDepthMin()
  //   && betta < Figure::MatScore+MaxPly
  //   && betta > -Figure::MatScore-MaxPly)
  //{
  //  auto score_x = scontexts_[ictx].board_.fmgr().weight();
  //  auto my_color = scontexts_[ictx].board_.getColor();
  //  auto other_color = Figure::otherColor(my_color);
  //  if(Figure::ColorBlack  == my_color )
  //    score_x = -score_x;
  //  if((score_x > betta + Figure::figureWeight_[Figure::TypeQueen]+Figure::figureWeight_[Figure::TypeRook])
  //     || (score_x > betta + Figure::figureWeight_[Figure::TypeRook] && scontexts_[ictx].board_.isWinnerColor(my_color) && scontexts_[ictx].board_.isWinnerLoser()))
  //  {
  //    return betta;
  //    //light_null_move = true;
  //  }
  //}
  ////XCounter xcntr(light_null_move ? &sdata_.nodesCount_ : nullptr);

  ScoreType alpha0 = alpha;

  Move hmove(0);

#ifdef USE_HASH
  ScoreType hscore = -ScoreMax;
  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv);
  if(flag == GHashTable::Alpha || flag == GHashTable::Betta)
  {
    return hscore;
  }
#endif

  if(depth <= 0)
    return captures(ictx, depth, ply, alpha, betta, pv);

  bool nm_threat = false, real_threat = false, mat_threat = false;

  UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(0);
  bool check_escape = scontexts_[ictx].board_.underCheck();

  if(options_.use_nullmove_)
  {
    if(!scontexts_[ictx].board_.underCheck() &&
      scontexts_[ictx].board_.allowNullMove() &&
      depth >= scontexts_[ictx].board_.nullMoveDepthMin() &&
      betta < Figure::MatScore+MaxPly &&
      betta > -Figure::MatScore-MaxPly)
    {
      int null_depth = std::max(0, depth - NullMove_PlyReduce);// scontexts_[ictx].board_.nullMoveDepth(depth, betta);

      // do null-move
      scontexts_[ictx].board_.makeNullMove();
      ScoreType nullScore = -alphaBetta(ictx, null_depth, ply+1, -betta, -(betta-1), false);
      scontexts_[ictx].board_.unmakeNullMove();

      // verify null-move with shortened depth
      if(nullScore >= betta)
      {
        depth -= NullMove_PlyVerify;// null_depth;
        if(depth <= 0)
          return captures(ictx, depth, ply, alpha, betta, pv);
      }
      else // may be we are in danger?
      {
        if(nullScore <= -Figure::MatScore+MaxPly) // mat threat?
        {
          if(prev.reduced_)
            return betta-1;
          else
            mat_threat = true;
        }

        nm_threat = true;
      }
    }
  } // use nullmove

#ifdef USE_FUTILITY_PRUNING
  if ( !pv &&
    !scontexts_[ictx].board_.underCheck() &&       
    alpha > -Figure::MatScore+MaxPly &&
    alpha < Figure::MatScore-MaxPly &&
    depth <= 3*ONE_PLY && ply > 1 )
  {
    ScoreType score0 = scontexts_[ictx].eval_(alpha, betta);
    int delta = calculateDelta(alpha, score0);
    if (!scontexts_[ictx].board_.isWinnerLoser())
    {
      if(depth <= ONE_PLY && delta > 0)
      {
        return captures(ictx, depth, ply, alpha, betta, pv, score0);
      }
      else if(depth <= 2*ONE_PLY && delta > Figure::figureWeight_[Figure::TypeQueen])
      {
        return captures(ictx, depth, ply, alpha, betta, pv, score0);
      }
      else if(delta > Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypeKnight])
      {
        return captures(ictx, depth, ply, alpha, betta, pv, score0);
      }
    }
  }
#endif

#ifdef USE_IID
  if (!hmove && depth >= 5 * ONE_PLY)
  {
    alphaBetta(ictx, depth - 2 * ONE_PLY, ply, alpha, betta, pv);
  }
#endif

  int counter = 0;
  int depthInc = 0;
  ScoreType scoreBest = -ScoreMax;
  Move best(0);

  Move killer(0);
  scontexts_[ictx].board_.extractKiller(scontexts_[ictx].plystack_[ply].killer_, hmove, killer);

  FastGenerator fg(scontexts_[ictx].board_, hmove, killer);
  if(fg.singleReply())
    depthInc += ONE_PLY;

#ifdef SINGULAR_EXT
  int  aboveAlphaN = 0;
  bool wasExtended = false;
  bool allMovesIterated = false;
#endif

  for(; alpha < betta && !checkForStop();)
  {
    auto* pmove = fg.move();
    if(!pmove)
    {
#ifdef SINGULAR_EXT
      allMovesIterated = true;
#endif
      break;
    }
    auto& move = *pmove;

    if(!scontexts_[ictx].board_.validateMove(move))
      continue;

    ScoreType score = -ScoreMax;

    if(pv && !move.seen_)
    {
      move.see_good_ = scontexts_[ictx].board_.see(move) >= 0;
      move.seen_ = 1;
    }

    // detect some threat, like pawn's attack etc...
    // don't LMR such moves
    if(scontexts_[ictx].board_.isMoveThreat(move))
      move.threat_ = 1;

    scontexts_[ictx].board_.makeMove(move);
    sdata_.inc_nc();

    //findSequence(ictx, move, ply, depth, counter, alpha, betta);

    UndoInfo & curr = scontexts_[ictx].board_.undoInfoRev(0);

    int depthInc1 = depthIncrement(ictx, move, pv) + depthInc;

    {

      if(!counter)
        score = -alphaBetta(ictx, depth + depthInc1 - ONE_PLY, ply+1, -betta, -alpha, pv);
      else
      {
        int depthInc2 = depthIncrement(ictx, move, false) + depthInc;
        int R = 0;

#ifdef USE_LMR
        if ( !check_escape &&
          sdata_.depth_ * ONE_PLY > LMR_MinDepthLimit &&
          depth >= LMR_DepthLimit &&
          alpha > -Figure::MatScore-MaxPly &&             
          scontexts_[ictx].board_.canBeReduced() )
        {
          R = ONE_PLY;
          curr.reduced_ = true;
        }
#endif

        score = -alphaBetta(ictx, depth + depthInc2 - R - ONE_PLY, ply + 1, -alpha - 1, -alpha, false);
        curr.reduced_ = false;

        if(!stopped() && score > alpha && R > 0)
          score = -alphaBetta(ictx, depth + depthInc2 - ONE_PLY, ply + 1, -alpha - 1, -alpha, false);

        if(!stopped() && score > alpha && score < betta)
          score = -alphaBetta(ictx, depth + depthInc1 - ONE_PLY, ply + 1, -betta, -alpha, pv);
      }
    }

    scontexts_[ictx].board_.unmakeMove();

    if(!stopped())
    {
#ifdef SINGULAR_EXT
      if ( pv && score > alpha0 )
      {
        aboveAlphaN++;
        wasExtended = depthInc1 > 0;
      }
#endif

      History & hist = history(move.from_, move.to_);

      if(score > scoreBest)
      {
        hist.inc_good();
        best = move;
        scoreBest = score;
        if(score > alpha)
        {
          scontexts_[ictx].plystack_[ply].setKiller(move);
          alpha = score;
          if(pv)
            assemblePV(ictx, move, scontexts_[ictx].board_.underCheck(), ply);
        }
      }
      else
        hist.inc_bad();
    }

    // should be increased here to consider invalid moves!!!
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

#ifdef SINGULAR_EXT
  if ( aboveAlphaN == 1 && !fg.singleReply() && !wasExtended && allMovesIterated )
  {
    X_ASSERT( !best, "best move wasn't found but one move was" );

    scontexts_[ictx].board_.makeMove(best);
    sdata_.inc_nc();

    alpha = alpha0;
    ScoreType score = -alphaBetta(ictx, depth, ply+1, -betta, -alpha, pv);

    scontexts_[ictx].board_.unmakeMove();

    if ( !stopped() )
    {
      scoreBest = score;
      if ( score > alpha )
      {
        alpha = score;
        assemblePV(ictx, best, scontexts_[ictx].board_.underCheck(), ply);
      }
    }
  }
#endif

  if(best)
  {
    History & hist = history(best.from_, best.to_);
    hist.inc_score(depth / ONE_PLY);

#if ((defined USE_LMR) && (defined VERIFY_LMR))
    // have to recalculate with full depth, or indicate threat in hash
    if ( !stopped() && (mat_threat || (alpha >= betta && nm_threat && isRealThreat(ictx, best))) )
    {
      if(prev.reduced_)
        return betta-1;
      else
        real_threat = true;
    }
#endif
  }

#ifdef USE_HASH
  putHash(ictx, best, alpha0, betta, scoreBest, depth, ply, real_threat);
#endif

  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");

  return scoreBest;
}

//////////////////////////////////////////////////////////////////////////
ScoreType Engine::captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
{
  if(alpha >= Figure::MatScore-ply)
    return alpha;

  //if ( pv && sdata_.plyMax_ < ply )
  //  sdata_.plyMax_ = ply;

  if(scontexts_[ictx].board_.drawState() || scontexts_[ictx].board_.countReps() > 1)
    return Figure::DrawScore;

  if(stopped() || ply >= MaxPly)
    return Figure::DrawScore;

  int counter = 0;
  ScoreType scoreBest = -ScoreMax;
  int delta = 0;
  int depthInc = 0;

  if(!scontexts_[ictx].board_.underCheck())
  {
    // not initialized yet
    if(score0 == -ScoreMax)
      score0 = scontexts_[ictx].eval_(alpha, betta);

    if(score0 >= betta)
      return score0;

    delta = calculateDelta(alpha, score0);
    if(score0 > alpha)
      alpha = score0;

    scoreBest = score0;
  }

  Move best(0);
  Figure::Type thresholdType = scontexts_[ictx].board_.isWinnerLoser() ? Figure::TypePawn : delta2type(delta);

  TacticalGenerator tg(scontexts_[ictx].board_, thresholdType, depth);
  if(tg.singleReply())
    depthInc += ONE_PLY;

  for(; alpha < betta && !checkForStop();)
  {
    auto* pmove = tg.move();
    if(!pmove)
      break;

    auto& move = *pmove;
    if(!scontexts_[ictx].board_.validateMove(move))
      continue;

    ScoreType score = -ScoreMax;

    scontexts_[ictx].board_.makeMove(move);
    sdata_.inc_nc();

    {
      int depthInc1 = depthInc + depthIncrement(ictx, move, false);
      score = -captures(ictx, depth + depthInc1 - ONE_PLY, ply+1, -betta, -alpha, pv, -ScoreMax);
    }

    scontexts_[ictx].board_.unmakeMove();

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
  hash_.prefetch(scontexts_[ictx].board_.hashCode());
}

GHashTable::Flag Engine::getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv)
{
  const HItem * hitem = hash_.find(scontexts_[ictx].board_.hashCode());
  if ( !hitem )
    return GHashTable::NoFlag;

  X_ASSERT( hitem->hkey_ != scontexts_[ictx].board_.hashCode(), "invalid hash item found" );

  scontexts_[ictx].board_.unpackMove(hitem->move_, hmove);

  if ( pv )
    return GHashTable::AlphaBetta;

  depth = (depth + ONE_PLY - 1) / ONE_PLY;

  hscore = hitem->score_;
  if ( hscore >= Figure::MatScore-MaxPly )
    hscore = hscore - ply;
  else if ( hscore <= MaxPly-Figure::MatScore )
    hscore = hscore + ply;

  X_ASSERT(hscore > 32760 || hscore < -32760, "invalid value in hash");

  if ( (int)hitem->depth_ >= depth && ply > 0 )
  {
    if ( GHashTable::Alpha == hitem->flag_ && hscore <= alpha )
    {
      X_ASSERT( !stop_ && alpha < -32760, "invalid hscore" );
      return GHashTable::Alpha;
    }

    if ( hitem->flag_ > GHashTable::Alpha && hscore >= betta && hmove )
    {
      if ( scontexts_[ictx].board_.calculateReps(hmove) < 2 )
      {
        /// danger move was reduced - recalculate it with full depth
        const UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(0);
        if ( hitem->threat_ && prev.reduced_ )
        {
          hscore = betta-1;
          return GHashTable::Alpha;
        }

        return GHashTable::Betta;
      }
    }
  }

  return GHashTable::AlphaBetta;
}

/// insert data to hash table
void Engine::putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat)
{
  if (scontexts_[ictx].board_.repsCount() >= 3)
    return;

  PackedMove pm = scontexts_[ictx].board_.packMove(move);
  GHashTable::Flag flag = GHashTable::NoFlag;
  if ( scontexts_[ictx].board_.repsCount() < 2 )
  {
    if ( score <= alpha || !move )
      flag = GHashTable::Alpha;
    else if ( score >= betta )
      flag = GHashTable::Betta;
    else
      flag = GHashTable::AlphaBetta;
  }
  if ( score >= +Figure::MatScore-MaxPly )
    score += ply;
  else if ( score <= -Figure::MatScore+MaxPly )
    score -= ply;
  hash_.push(scontexts_[ictx].board_.hashCode(), score, depth / ONE_PLY, flag, pm, threat);
}
#endif


//////////////////////////////////////////////////////////////////////////
// is given movement caused by previous? this mean that if we don't do this move we loose
// we actually test if moved figure was/will be attacked by previously moved one or from direction it was moved from
// or we should move our king even if we have a lot of other figures
//////////////////////////////////////////////////////////////////////////
bool Engine::isRealThreat(int ictx, const Move & move)
{
  // don't need to forbid if our answer is capture or check ???
  if(move.capture_ || move.checkFlag_)
    return false;

  const UndoInfo & prev = scontexts_[ictx].board_.undoInfoRev(0);
  if(!prev) // null-move
    return false;

  Figure::Color  color = scontexts_[ictx].board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  const Field & pfield = scontexts_[ictx].board_.getField(prev.to_);
  X_ASSERT(!pfield || pfield.color() != ocolor, "no figure of required color on the field it was move to while detecting threat");

  // don't need forbid reduction of captures, checks, promotions and pawn's attack because we've already done it
  if(prev.capture_ || prev.new_type_ > 0 || prev.checkingNum_ > 0 || prev.threat_)
    return false;

  /// we have to move king even if there a lot of figures
  if(scontexts_[ictx].board_.getField(move.from_).type() == Figure::TypeKing &&
    scontexts_[ictx].board_.fmgr().queens(color) > 0 &&
    scontexts_[ictx].board_.fmgr().rooks(color) > 0 &&
    scontexts_[ictx].board_.fmgr().knights(color) + scontexts_[ictx].board_.fmgr().bishops(color) > 0)
  {
    return true;
  }

  const Field & cfield = scontexts_[ictx].board_.getField(move.from_);
  X_ASSERT(!cfield || cfield.color() != color, "no figure of required color in while detecting threat");

  // we have to put figure under attack
  if(scontexts_[ictx].board_.ptAttackedBy(move.to_, prev.to_))
    return true;

  // put our figure under attack, opened by prev movement
  if(scontexts_[ictx].board_.getAttackedFrom(ocolor, move.to_, prev.from_) >= 0)
    return true;

  // prev move was attack, and we should escape from it
  if(scontexts_[ictx].board_.ptAttackedBy(move.from_, prev.to_))
    return true;

  // our figure was attacked from direction, opened by prev movement
  if(scontexts_[ictx].board_.getAttackedFrom(ocolor, move.from_, prev.from_) >= 0)
    return true;

  return false;
}

int Engine::xcounter_ = 0;

} // NEngine
