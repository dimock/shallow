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
  void reset();

  NTime::duration timeLimit_{};
  int  depthMax_{};
  bool analyze_mode_{};
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
  bool threadSearch(int ictx);

  void setBoard(Board const& board);
  Board & getBoard() { return scontexts_.at(0).board_; }
  const Board & getBoard() const { return scontexts_.at(0).board_; }

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
  void testTimer(int ictx);
  void testInput(int ictx);
  bool stopped(int ictx) const { return scontexts_.at(ictx).stop_; }

  // search routine
  ScoreType alphaBetta0(int ictx);
  ScoreType processMove0(int ictx, SMove const& move, ScoreType const alpha, ScoreType const betta, bool const pv);
  ScoreType alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool allow_nm);
  CapturesResult captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0 = -ScoreMax);
  int depthIncrement(int ictx, Move const& move, bool pv, bool singular) const;
  void assemblePV(int ictx, Move const & move, bool checking, int ply);

  void sortMoves0(int ictx);

  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  bool isRealThreat(int ictx, const Move & move);

#ifdef USE_HASH
  void prefetchHash(int ictx);
  // we should return alpha if flag is Alpha, or Betta if flag is Betta
  GHashTable::Flag getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv, bool& singular);
  void putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat, bool singular, bool pv);
  void putCaptureHash(int ictx, const Move & move, bool pv);
#endif // USE_HASH

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

#ifdef RELEASEDEBUGINFO
  bool findSequence(int ictx, int ply, bool exactMatch) const;
  void verifyGenerators(int ictx, const Move & hmove);
#endif

public:
  void saveHash(std::string const& fname) const;
  void loadHash(std::string const& fname);
};

} // NEngine