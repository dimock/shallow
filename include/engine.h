/*************************************************************
engine.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <HashTable.h>
#include <Evaluator.h>
#include <xcallback.h>
#include <xoptions.h>
#include <queue>
#include <array>
#include <mutex>
#include <Helpers.h>

namespace NEngine
{

struct PlyStack
{
  PlyStack() {}

  SMove killer_;
  Move pv_[MaxPly+4];

  void clearPV(int depth)
  {
    for(int i = 0; i < depth; ++i)
      pv_[i] = Move{true};
  }

  void clear(int ply)
  {
    pv_[ply] = Move{ true };
  }

  void clearKiller()
  {
    killer_ = SMove{ true };
  }
};

struct SearchParams
{
  NTime::duration timeLimit_{};
  int  depthMax_{};
  bool analyze_mode_{};
  bool timeAdded_{};
  ScoreType scoreLimit_{ Figure::MatScore };
};

class Engine
{
public:

  static int xcounter_;

  // initialize global arrays, tables, masks, etc. write them to it's board_
  Engine();

  void clearHash();
  void setCallbacks(xCallback callback);

  void needUpdate();

  bool fromFEN(std::string const& fen);
  std::string toFEN() const;

  // call it to start search
  bool search();

  // return result of last search
  SearchResult const& result() const;
  // prepare and start in threads
  bool generateStartposMoves(int ictx);
  bool mainThreadSearch(int ictx);
  bool threadSearch(const int ictx, const int depth0);

  void setBoard(Board const& board);
  Board & getBoard() { return scontexts_[0].board_; }
  const Board & getBoard() const { return scontexts_[0].board_; }

  void pleaseStop(int ictx);

  void setAnalyzeMode(bool);
  void setTimeLimit(NTime::duration const& tm);
  void setMaxDepth(int d);
  void setScoreLimit(ScoreType score);

  void setMemory(int mb);
  void setThreadsNumber(int n);

private:

  struct CapturesResult
  {
    CapturesResult(ScoreType s, bool d) : score{s}, dangerous{d}
    {}

    ScoreType score{ -ScoreMax };
    bool dangerous{ false };
  };

  bool checkForStop(int ictx);
  void reset();

//  // logging
  void logPV(int ictx);
  void logMovies(int ictx);

  // time control
  bool tryToAddTime();
  void testTimer(int ictx);
  void testInput(int ictx);

  // search routine
  ScoreType alphaBetta0(int ictx);
  ScoreType processMove0(int ictx, SMove const& move, ScoreType const alpha, ScoreType const betta, bool const pv);
  ScoreType alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool allow_nm, int signular_count);

  ScoreType captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0 = -ScoreMax);

  int depthIncrement(int ictx, Move const& move, bool pv, bool singular) const
  {
    X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");

    auto& board = scontexts_[ictx].board_;

#ifdef EXTEND_CHECK_SEE_ONLY
    if (!move.see_ok() && !pv )
      return 0;
#endif

    if (board.underCheck())
      return ONE_PLY;

    if (!pv || !move.see_ok())
      return 0;

    if (move.new_type() || singular)
      return ONE_PLY;

    // recapture
    if (board.halfmovesCount() > 1)
    {
      auto const& prev = board.reverseUndo(1);
      auto const& curr = board.lastUndo();

      if (prev.move_.to() == curr.move_.to() && prev.eaten_type_ > 0)
      {
        return ONE_PLY;
      }
    }

    return 0;
  }
  
  void assemblePV(int ictx, Move const & move, bool checking, int ply);

  void sortMoves0(int ictx);

  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  bool isRealThreat(int ictx, const Move & move);

  // analyze mode support
  bool updateRequested_{ false };
  xCallback callbacks_;

  // search data
  static const int depth0_ = 1; // start depth

  struct SearchContext
  {
    SBoard<Board, UndoInfo, Board::GameLength> board_;
    Evaluator eval_;
    std::array<PlyStack, MaxPly+4> plystack_;
    std::array<SMove, Board::MovesMax> moves_;
    SearchData sdata_;
    SearchResult sres_;
    volatile bool stop_{ false };

    SearchContext& operator = (SearchContext const& other);
    void reset();
    void clearStack();
  };

  // multi-threading mode
  std::vector<SearchContext> scontexts_;

#ifdef SYNCHRONIZE_LAST_ITER
  std::mutex m_mutex;
#endif // SYNCHRONIZE_LAST_ITER

  SearchParams sparams_;

#ifdef USE_HASH
  GHashTable hash_;
#endif

#ifdef PROCESS_MOVES_SEQ
  bool findSequence(int ictx, int ply, bool exactMatch) const;
#endif

public:
  void saveHash(std::string const& fname) const;
  void loadHash(std::string const& fname);

/// HASH Table
private:

#ifdef USE_HASH
  // we should return alpha if flag is Alpha, or Betta if flag is Betta
  inline Flag getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv, bool& singular, HItem*& pitem)
  {
    X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
    auto& sctx = scontexts_[ictx];

    auto& board = sctx.board_;
    pitem = hash_.get(board.fmgr().hashCode());
    auto const& hitem = *pitem;
    if (hitem.hkey_ == board.fmgr().hashKey())
    {
      singular = hitem.singular_;
      auto hflag = hitem.flag();
      if (!pv && board.fmgr().weight(Figure::ColorBlack).eval32_ && board.fmgr().weight(Figure::ColorWhite).eval32_)
      {
        X_ASSERT(hscore > 32760 || hscore < -32760, "invalid value in hash");

        if (hflag != NoFlag && (int)hitem.depth_ >= depth && ply > 0)
        {
          hscore = hitem.score_;
          if (hscore >= Figure::MatScore - MaxPly)
            hscore = hscore - ply;
          else if (hscore <= MaxPly - Figure::MatScore)
            hscore = hscore + ply;

          if (hscore >= betta && hflag == Betta)
            return Betta;
          else if (hscore < alpha && hflag == Alpha)
            return Alpha;
        }
      }
      if (hitem.move_ && board.validateMoveExpress(hitem.move_)) {
        hmove = hitem.move_;
        return AlphaBetta;
      }
    }

    return NoFlag;
  }

  /// insert data to hash table
  inline void putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat, bool singular, bool pv, HItem* pitem)
  {
    X_ASSERT((size_t)ictx >= scontexts_.size(), "Invalid context index");
    auto& sctx = scontexts_[ictx];

    auto& board = sctx.board_;

    Flag flag = NoFlag;
    if (score != 0)
    {
      if (score >= betta)
        flag = Betta;
      else if (move && score > alpha)
        flag = AlphaBetta;
      else
        flag = Alpha;
    }
    if (score >= +Figure::MatScore - MaxPly)
      score += ply;
    else if (score <= -Figure::MatScore + MaxPly)
      score -= ply;
    {
      const auto hk = board.fmgr().hashKey();
      auto& hitem = *pitem;
      if (hitem.hkey_ != hk) {
        hitem.eval_ = -ScoreMax;
      }
      if (
        (hitem.hkey_ != hk) ||
        (depth > hitem.depth_) ||
        (AlphaBetta == flag)
        )
      {
        X_ASSERT(score > 32760, "write wrong value to the hash");
        hitem = HItem{ hk, score, depth, flag, move, threat, singular, pv};
      }

    }
  }
#endif // USE_HASH
};

} // NEngine