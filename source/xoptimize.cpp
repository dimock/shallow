#include <xoptimize.h>
#include <MovesGenerator.h>
#include <xindex.h>
#include <Helpers.h>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>

namespace NEngine
{

int64 x_movesCounter = 0;

void xcaptures(Board& board, int depth)
{
  if(board.drawState() || board.hasReps() || depth < -10)
    return;
  TacticalGenerator<Board, Move> tg(board, depth);
  //try
  //{
    for(;;)
    {
      auto* pmove = tg.next();
      if(!pmove)
        break;
      auto& move = *pmove;
      X_ASSERT(!board.validateMove(move), "invalid move");
      board.see(move, 0);
      //Board brd{ board };
      //std::string fen = toFEN(board);
      board.makeMove(move);
      x_movesCounter++;
      xcaptures(board, depth-1);
      board.unmakeMove(move);
      //X_ASSERT(brd != board, "board was not correctly restored");
    }
  //}
  //catch(std::exception const& e)
  //{
  //  throw std::runtime_error(toFEN(board) + "; " + e.what());
  //}
}

void xsearch(Board& board, int depth)
{
  if(board.drawState() || board.hasReps())
    return;
  if(depth <= 0)
  {
    xcaptures(board, depth);
    return;
  }
  //try
  //{
    FastGenerator<Board, Move> fg(board);
    for(;;)
    {
      auto* pmove = fg.next();
      if(!pmove)
        break;
      auto& move = *pmove;
      X_ASSERT(!board.validateMove(move), "invalid move");
      board.see(move, 0);
      //std::string fen = toFEN(board);
      //Board brd{ board };

      board.makeMove(move);
      x_movesCounter++;
      xsearch(board, depth-1);
      board.unmakeMove(move);

      //X_ASSERT(brd != board, "board was not correctly restored");
    }
  //}
  //catch(std::exception const& e)
  //{
  //  throw std::runtime_error(toFEN(board) + "; " + e.what());
  //}
}

template <class MOVE>
bool compare(std::vector<MOVE> const& moves1, std::vector<MOVE> const& moves2)
{
  //if(moves1.size() != moves2.size())
  //  return false;
  for(auto const& move1 : moves1)
  {
    if(std::find_if(moves2.begin(), moves2.end(), [&move1](Move const& m) { return m == move1; })
       == moves2.end())
       return false;
  }
  for(auto move2 : moves2)
  {
    if(std::find_if(moves1.begin(), moves1.end(), [&move2](Move const& m) { return m == move2; })
       == moves1.end())
       return false;
  }
  return true;
}

bool xverifyMoves(Board& board)
{
  bool ok = false;
  std::vector<Move> fmoves;
  FastGenerator<Board, Move> fg(board);
  for(;;)
  {
    auto* pmove = fg.next();
    if(!pmove)
      break;
    auto& move = *pmove;
    X_ASSERT(board.validateMove(move) != board.validateMoveBruteforce(move), "validate move error");
    if(!board.validateMove(move))
      continue;
    Board brd{ board };
    brd.makeMove(move);
    brd.unmakeMove(move);
    X_ASSERT(board != brd, "board was not restored correctly after move");
    fmoves.push_back(move);
    ok = true;
  }
  auto moves = generate<Board, Move>(board);
  std::vector<Move> etalons;
  std::vector<Move> checks;
  auto ocolor = Figure::otherColor(board.color());
  for(auto& move : moves)
  {
    if(!board.validateMoveBruteforce(move))
      continue;
    if(move.new_type() == Figure::TypeBishop || move.new_type() == Figure::TypeRook)
      continue;
    if(move.new_type() == Figure::TypeKnight
       && !(movesTable().caps(Figure::TypeKnight, move.to()) & set_mask_bit(board.kingPos(ocolor)))
       )
       continue;
    etalons.push_back(move);
    Board brd{ board };
    brd.makeMove(move);
    if(brd.underCheck() && !brd.lastUndo().capture() && !move.new_type())
      checks.push_back(move);
    brd.unmakeMove(move);
    X_ASSERT(board != brd, "board was not restored correctly after move");
  }
  if(!compare(etalons, fmoves))
    return false;
  if(board.underCheck())
    return true;
  ChecksGenerator<Board, Move> cg(board);
  cg.generate();
  std::vector<Move> checks2;
  for(auto& move : cg.moves_)
  {
    X_ASSERT(board.validateMoveBruteforce(move) != board.validateMove(move), "inalid move validator");
    if(!board.validateMoveBruteforce(move))
      continue;
    if(std::find_if(etalons.begin(), etalons.end(), [&move](Move const& m) { return m == move; })
       == etalons.end())
       return false;
    Board brd{ board };
    brd.makeMove(move);
    X_ASSERT(!brd.underCheck(), "check was not detected");
    checks2.push_back(move);
  }
  if(!compare(checks, checks2))
    return false;
  return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ScoreType Engine::alphaBetta0()
//{
//  if(stopped())
//    return scontexts_[0].eval_(-ScoreMax, +ScoreMax);
//
//  if(sdata_.numOfMoves_ == 0)
//  {
//    scontexts_[0].board_.setNoMoves();
//    ScoreType score = scontexts_[0].eval_(-ScoreMax, +ScoreMax);
//    return score;
//  }
//
//  ScoreType alpha = -ScoreMax;
//  ScoreType betta = +ScoreMax;
//  const int depth = sdata_.depth_ * ONE_PLY;
//
//  bool check_escape = scontexts_[0].board_.underCheck();
//  int sortDepth = 4;
//  int numMovesNoReduce = 5;
//
//  const bool bDoSort = sdata_.depth_ <= sortDepth;
//
//#ifdef LOG_PV
//  logMovies();
//#endif
//
//  for(sdata_.counter_ = 0; sdata_.counter_ < sdata_.numOfMoves_; ++sdata_.counter_)
//  {
//    if(checkForStop())
//      break;
//
//    auto& move = scontexts_[0].moves_[sdata_.counter_];
//    ScoreType score = -ScoreMax;
//
//    //if(!move.seen_)
//    //{
//    //  move.see_good_ = scontexts_[0].board_.see(move) >= 0;
//    //  move.seen_ = 1;
//    //}
//
//    //if(scontexts_[0].board_.isMoveThreat(move))
//    //  move.threat_ = 1;
//
//    scontexts_[0].board_.makeMove(move);
//    sdata_.inc_nc();
//
//    bool fullRange = false;
//
//    {
//      int depthInc = depthIncrement(0, move, true);
//      bool strongMove = (move.sort_value > (alpha-500)) || (sdata_.counter_ < numMovesNoReduce);
//
//      if(bDoSort || sdata_.counter_ < 1)
//      {
//        score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -betta, bDoSort ? ScoreMax : -alpha, true, true);
//        fullRange = true;
//      }
//      else
//      {
//        int depthInc2 = depthIncrement(0, move, strongMove);
//        int R = 0;
//
//
//#ifdef USE_LMR
//        if(!check_escape &&
//           sdata_.counter_ >= numMovesNoReduce &&
//           depth > LMR_MinDepthLimit &&
//           //alpha > -Figure::MatScore-MaxPly && 
//           scontexts_[0].board_.canBeReduced(move))
//        {
//          R = ONE_PLY;
//        }
//#endif
//
//        score = -alphaBetta(0, depth + depthInc2 - ONE_PLY - R, 1, -alpha-1, -alpha, strongMove, true);
//
//        if(!stopped() && score > alpha && R > 0)
//          score = -alphaBetta(0, depth + depthInc2 - ONE_PLY, 1, -alpha - 1, -alpha, strongMove, true);
//
//        if(!stopped() && score > alpha)
//        {
//          score = -alphaBetta(0, depth + depthInc - ONE_PLY, 1, -betta, -alpha, true, true);
//          fullRange = true;
//        }
//      }
//    }
//
//    scontexts_[0].board_.unmakeMove(move);
//
//    if(!stopped())
//    {
//      if(bDoSort || sdata_.counter_ < 1)
//        move.sort_value = score;
//
//      if(score > alpha)
//      {
//        move.sort_value = score;
//        sdata_.best_ = move;
//        alpha = score;
//
//        assemblePV(0, move, scontexts_[0].board_.underCheck(), 0);
//
//        // bring best move to front, shift other moves 1 position right
//        if(!bDoSort)
//        {
//          for(int j = sdata_.counter_; j > 0; --j)
//            scontexts_[0].moves_[j] = scontexts_[0].moves_[j-1];
//          scontexts_[0].moves_[0] = sdata_.best_;
//        }
//      }
//
//#ifdef LOG_PV
//      logPV();
//#endif
//    }
//
//
//    if(callbacks_.sendStats_ && sparams_.analyze_mode_)
//      (callbacks_.sendStats_)(sdata_);
//  }
//
//  // don't need to continue
//  if(!stopped() && sdata_.counter_ == 1 && !sparams_.analyze_mode_)
//    pleaseStop();
//
//  if(!stopped())
//  {
//    // full sort only on first iterations
//    if(bDoSort)
//    {
//      auto iter = scontexts_[0].moves_.begin();
//      std::advance(iter, sdata_.numOfMoves_);
//      std::sort(scontexts_[0].moves_.begin(), iter);
//    }
//    // sort all moves but 1st
//    else if(sdata_.numOfMoves_ > 2)
//    {
//      auto iter1 = scontexts_[0].moves_.begin();
//      auto iter2 = scontexts_[0].moves_.begin();
//      iter1++;
//      std::advance(iter2, sdata_.numOfMoves_);
//      std::sort(iter1, iter2);
//    }
//  }
//
//  return alpha;
//}
//
////////////////////////////////////////////////////////////////////////////
//ScoreType Engine::alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool allow_nm)
//{
//  prefetchHash(ictx);
//
//  if(alpha >= Figure::MatScore-ply)
//    return alpha;
//
//  if(ply < MaxPly-1)
//  {
//    scontexts_[ictx].plystack_[ply].clear(ply);
//    scontexts_[ictx].plystack_[ply+1].clearKiller();
//  }
//
//  if(scontexts_[ictx].board_.drawState() || scontexts_[ictx].board_.hasReps())
//    return Figure::DrawScore;
//
//  if(stopped() || ply >= MaxPly)
//    return scontexts_[ictx].eval_(alpha, betta);
//
//  ScoreType alpha0 = alpha;
//
//  Move hmove{};
//
//#ifdef USE_HASH
//  ScoreType hscore = -ScoreMax;
//  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv);
//  if(flag == GHashTable::Alpha || flag == GHashTable::Betta)
//  {
//    return hscore;
//  }
//#endif
//
//  if(depth <= 0)
//    return captures(ictx, depth, ply, alpha, betta, pv);
//
//  bool nm_threat = false, real_threat = false, mat_threat = false;
//
//  auto const& prev = scontexts_[ictx].board_.lastUndo();
//  bool check_escape = scontexts_[ictx].board_.underCheck();
//
//  if(options_.use_nullmove_)
//  {
//    if(
//      !scontexts_[ictx].board_.underCheck()
//      && !pv
//      && allow_nm
//      && scontexts_[ictx].board_.allowNullMove()
//      && depth > scontexts_[ictx].board_.nullMoveDepthMin()
//      && std::abs(betta) < Figure::MatScore+MaxPly
//      )
//    {
//      int null_depth = scontexts_[ictx].board_.nullMoveDepth(depth, betta);
//
//      // do null-move
//      scontexts_[ictx].board_.makeNullMove();
//      ScoreType nullScore = -alphaBetta(ictx, null_depth, ply+1, -betta, -(betta-1), false, false);
//      scontexts_[ictx].board_.unmakeNullMove();
//
//      // verify null-move with shortened depth
//      if(nullScore >= betta)
//      {
//        depth = null_depth;
//        if(depth <= 0)
//          return captures(ictx, depth, ply, alpha, betta, pv);
//      }
//      else // may be we are in danger?
//      {
//        if(nullScore <= -Figure::MatScore+MaxPly) // mat threat?
//        {
//          if(prev.is_reduced())
//            return betta-1;
//          else
//            mat_threat = true;
//        }
//
//        nm_threat = true;
//      }
//    }
//  } // use nullmove
//
//#ifdef USE_FUTILITY_PRUNING
//  if(!pv &&
//     !scontexts_[ictx].board_.underCheck() &&
//     alpha > -Figure::MatScore+MaxPly &&
//     alpha < Figure::MatScore-MaxPly &&
//     depth <= 3*ONE_PLY && ply > 1)
//  {
//    ScoreType score0 = scontexts_[ictx].eval_(alpha, betta);
//    int delta = calculateDelta(alpha, score0);
//    if(!scontexts_[ictx].board_.isWinnerLoser())
//    {
//      if(depth <= ONE_PLY && delta > 0)
//      {
//        return captures(ictx, depth, ply, alpha, betta, pv, score0);
//      }
//      else if(depth <= 2*ONE_PLY && delta > Figure::figureWeight_[Figure::TypeQueen])
//      {
//        return captures(ictx, depth, ply, alpha, betta, pv, score0);
//      }
//      else if(delta > Figure::figureWeight_[Figure::TypeQueen] + Figure::figureWeight_[Figure::TypeKnight])
//      {
//        return captures(ictx, depth, ply, alpha, betta, pv, score0);
//      }
//    }
//  }
//#endif
//
//#ifdef USE_IID
//  if(!hmove && depth >= 4 * ONE_PLY)
//  {
//    alphaBetta(ictx, depth - 3*ONE_PLY, ply, alpha, betta, pv, true);
//    if(const HItem * hitem = hash_.find(scontexts_[ictx].board_.fmgr().hashCode()))
//    {
//      X_ASSERT(hitem->hkey_ != scontexts_[ictx].board_.fmgr().hashCode(), "invalid hash item found");
//      hmove = hitem->move_;
//    }
//  }
//#endif
//
//  int counter = 0;
//  int depthInc = 0;
//  ScoreType scoreBest = -ScoreMax;
//  Move best{};// , killer{};
//
//  FastGenerator<Board, Move> fg(scontexts_[ictx].board_);// , hmove, scontexts_[ictx].plystack_[ply].killer_);
//  if(fg.singleReply())
//    depthInc += ONE_PLY;
//
//#ifdef SINGULAR_EXT
//  int  aboveAlphaN = 0;
//  bool wasExtended = false;
//  bool allMovesIterated = false;
//#endif
//
//  for(; alpha < betta && !checkForStop();)
//  {
//    auto* pmove = fg.next();
//    if(!pmove)
//    {
//#ifdef SINGULAR_EXT
//      allMovesIterated = true;
//#endif
//      break;
//    }
//    auto& move = *pmove;
//    //if(!scontexts_[ictx].board_.validateMove(move))
//    //  continue;
//
//    ScoreType score = -ScoreMax;
//
//    //if(pv && !move.seen_)
//    //{
//    //  move.see_good_ = scontexts_[ictx].board_.see(move) >= 0;
//    //  move.seen_ = 1;
//    //}
//
//    //// detect some threat, like pawn's attack etc...
//    //// don't LMR such moves
//    //if(scontexts_[ictx].board_.isMoveThreat(move))
//    //  move.threat_ = 1;
//
//    scontexts_[ictx].board_.makeMove(move);
//    sdata_.inc_nc();
//
//    //findSequence(ictx, move, ply, depth, counter, alpha, betta);
//
//    auto& curr = scontexts_[ictx].board_.lastUndo();
//
//    int depthInc1 = depthIncrement(ictx, move, pv) + depthInc;
//
//    {
//      ScoreType betta_pc = betta + 300;
//#ifdef USE_PROBCUT
//      if(!pv
//         && move.capture_
//         && move.see_good_
//         && !check_escape
//         && depth >= Probcut_Depth
//         && std::abs(betta) < Figure::MatScore-MaxPly
//         && std::abs(alpha) < Figure::MatScore-MaxPly)
//      {
//        score = -alphaBetta(ictx, depth - Probcut_PlyReduce, ply+1, -betta_pc, -(betta_pc-1), pv, true);
//      }
//#endif
//
//      if(score < betta_pc)
//      {
//        if(!counter)
//          score = -alphaBetta(ictx, depth + depthInc1 - ONE_PLY, ply+1, -betta, -alpha, pv, true);
//        else
//        {
//          int depthInc2 = depthIncrement(ictx, move, false) + depthInc;
//          int R = 0;
//
//#ifdef USE_LMR
//          if(!check_escape &&
//             sdata_.depth_ * ONE_PLY > LMR_MinDepthLimit &&
//             depth >= LMR_DepthLimit &&
//             alpha > -Figure::MatScore-MaxPly &&
//             scontexts_[ictx].board_.canBeReduced(move))
//          {
//            R = ONE_PLY;
//            curr.mflags_ |= UndoInfo::Reduced;
//          }
//#endif
//
//          score = -alphaBetta(ictx, depth + depthInc2 - R - ONE_PLY, ply + 1, -alpha - 1, -alpha, false, true);
//          curr.mflags_ &= ~UndoInfo::Reduced;
//
//          if(!stopped() && score > alpha && R > 0)
//            score = -alphaBetta(ictx, depth + depthInc2 - ONE_PLY, ply + 1, -alpha - 1, -alpha, false, true);
//
//          if(!stopped() && score > alpha && score < betta)
//            score = -alphaBetta(ictx, depth + depthInc1 - ONE_PLY, ply + 1, -betta, -alpha, pv, true);
//        }
//      }
//    }
//
//    scontexts_[ictx].board_.unmakeMove(move);
//
//    if(!stopped())
//    {
//#ifdef SINGULAR_EXT
//      if(pv && score > alpha0)
//      {
//        aboveAlphaN++;
//        wasExtended = depthInc1 > 0;
//      }
//#endif
//
//      History & hist = history(scontexts_[ictx].board_.color(), move.from(), move.to());
//      if(score > scoreBest)
//      {
//        hist.inc_good();
//        best = move;
//        scoreBest = score;
//        if(score > alpha)
//        {
//          auto capture = scontexts_[ictx].board_.getField(move.to()) || (move.to() > 0 && scontexts_[ictx].board_.enpassant());
//          if(!capture)
//            scontexts_[ictx].plystack_[ply].killer_ = move;
//          alpha = score;
//          if(pv)
//            assemblePV(ictx, move, scontexts_[ictx].board_.underCheck(), ply);
//        }
//      }
//      else
//        hist.inc_bad();
//    }
//
//    // should be increased here to consider invalid moves!!!
//    ++counter;
//  }
//
//  if(stopped())
//    return scoreBest;
//
//  if(!counter)
//  {
//    scontexts_[ictx].board_.setNoMoves();
//
//    scoreBest = scontexts_[ictx].eval_(alpha, betta);
//
//    if(scontexts_[ictx].board_.matState())
//      scoreBest += ply;
//  }
//
//#ifdef SINGULAR_EXT
//  if(aboveAlphaN == 1 && !fg.singleReply() && !wasExtended && allMovesIterated)
//  {
//    X_ASSERT(!best, "best move wasn't found but one move was");
//
//    scontexts_[ictx].board_.makeMove(best);
//    sdata_.inc_nc();
//
//    alpha = alpha0;
//    ScoreType score = -alphaBetta(ictx, depth, ply+1, -betta, -alpha, pv, true);
//
//    scontexts_[ictx].board_.unmakeMove(best);
//
//    if(!stopped())
//    {
//      scoreBest = score;
//      if(score > alpha)
//      {
//        alpha = score;
//        assemblePV(ictx, best, scontexts_[ictx].board_.underCheck(), ply);
//      }
//    }
//  }
//#endif
//
//  if(best)
//  {
//    History & hist = history(scontexts_[ictx].board_.color(), best.from(), best.to());
//    hist.inc_score(depth / ONE_PLY);
//
//    //#if ((defined USE_LMR) && (defined VERIFY_LMR))
//    //    // have to recalculate with full depth, or indicate threat in hash
//    //    if ( !stopped() && (mat_threat || (alpha >= betta && nm_threat && isRealThreat(ictx, best))) )
//    //    {
//    //      if(prev.reduced_)
//    //        return betta-1;
//    //      else
//    //        real_threat = true;
//    //    }
//    //#endif
//  }
//
//#ifdef USE_HASH
//  putHash(ictx, best, alpha0, betta, scoreBest, depth, ply, real_threat);
//#endif
//
//  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
//
//  return scoreBest;
//}
//
////////////////////////////////////////////////////////////////////////////
//ScoreType Engine::captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0)
//{
//  if(alpha >= Figure::MatScore-ply)
//    return alpha;
//
//  if(scontexts_[ictx].board_.drawState() || scontexts_[ictx].board_.hasReps())
//    return Figure::DrawScore;
//
//  if(stopped() || ply >= MaxPly)
//    return Figure::DrawScore;
//
//  Move hmove{};
//
//#ifdef USE_HASH
//  ScoreType hscore = -ScoreMax;
//  GHashTable::Flag flag = getHash(ictx, depth, ply, alpha, betta, hmove, hscore, pv);
//  if(flag == GHashTable::Alpha || flag == GHashTable::Betta)
//  {
//    return hscore;
//  }
//#endif
//
//  int counter = 0;
//  ScoreType scoreBest = -ScoreMax;
//  int delta = 0;
//  int depthInc = 0;
//
//  if(!scontexts_[ictx].board_.underCheck())
//  {
//    // not initialized yet
//    if(score0 == -ScoreMax)
//      score0 = scontexts_[ictx].eval_(alpha, betta);
//
//    if(score0 >= betta)
//      return score0;
//
//    delta = calculateDelta(alpha, score0);
//    if(score0 > alpha)
//      alpha = score0;
//
//    scoreBest = score0;
//  }
//
//  Move best{};
//  Figure::Type thresholdType = scontexts_[ictx].board_.isWinnerLoser() ? Figure::TypePawn : delta2type(delta);
//
//  TacticalGenerator<Board, Move> tg(scontexts_[ictx].board_, depth);// , hmove, thresholdType, depth);
//  if(tg.singleReply())
//    depthInc += ONE_PLY;
//
//  for(; alpha < betta && !checkForStop();)
//  {
//    auto* pmove = tg.next();
//    if(!pmove)
//      break;
//
//    auto& move = *pmove;
//    //if(!scontexts_[ictx].board_.validateMove(move))
//    //  continue;
//
//    ScoreType score = -ScoreMax;
//
//    scontexts_[ictx].board_.makeMove(move);
//    sdata_.inc_nc();
//
//    {
//      int depthInc1 = depthInc + depthIncrement(ictx, move, false);
//      score = -captures(ictx, depth + depthInc1 - ONE_PLY, ply+1, -betta, -alpha, pv, -ScoreMax);
//    }
//
//    scontexts_[ictx].board_.unmakeMove(move);
//
//    if(!stopped() && score > scoreBest)
//    {
//      best = move;
//      scoreBest = score;
//      if(score > alpha)
//      {
//        alpha = score;
//      }
//    }
//    counter++;
//  }
//
//  if(stopped())
//    return scoreBest;
//
//  if(!counter)
//  {
//    if(scontexts_[ictx].board_.underCheck())
//      scoreBest = -Figure::MatScore+ply;
//    else
//      scoreBest = score0;
//  }
//
//#ifdef USE_HASH
//  putCaptureHash(ictx, best);
//#endif
//
//  X_ASSERT(scoreBest < -Figure::MatScore || scoreBest > +Figure::MatScore, "invalid score");
//
//  return scoreBest;
//}


} // NEngine
