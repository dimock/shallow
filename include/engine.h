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
  void setOptions(xOptions const& opts);
  void setCallbacks(xCallback callback);

  void needUpdate();

  bool fromFEN(std::string const& fen);
  std::string toFEN() const;

  // call it to start search
  bool search(SearchResult& result);

  void setBoard(Board const& board);
  Board & getBoard() { return scontexts_[0].board_; }
  const Board & getBoard() const { return scontexts_[0].board_; }

  void pleaseStop();

  void setAnalyzeMode(bool);
  void setTimeLimit(NTime::duration const& tm);
  void setMaxDepth(int d);
  void setScoreLimit(ScoreType score);

private:

  void setMemory(int mb);

  bool checkForStop();
  void reset();

//  // logging
  void logPV();
  void logMovies();

  // time control
  void testTimer();
  bool stopped() const { return stop_; }
  void testInput();

  // search routine
  ScoreType alphaBetta0();
  ScoreType alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool allow_nm);
  ScoreType captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0 = -ScoreMax);
  int depthIncrement(int ictx, Move const& move, bool pv) const;
  void assemblePV(int ictx, Move const & move, bool checking, int ply);


  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  //bool isRealThreat(int ictx, const Move & move);

#ifdef USE_HASH
  void prefetchHash(int ictx);
  // we should return alpha if flag is Alpha, or Betta if flag is Betta
  GHashTable::Flag getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv);
  void putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat);
  void putCaptureHash(int ictx, const Move & move);
#endif // USE_HASH

  // analyze mode support
  bool updateRequested_{ false };
  xCallback callbacks_;

  // search data
  static const int depth0_ = 2; // start depth
  volatile bool stop_;

  struct SearchContext
  {
    SBoard<Board, UndoInfo, Board::GameLength>     board_;
    Evaluator eval_;
    std::array<PlyStack, MaxPly+4>    plystack_;
    std::array<SMove, Board::MovesMax> moves_;
  };

  // will be used in multi-threading mode???
  std::array<SearchContext, 1> scontexts_;

  SearchData sdata_;
  SearchParams sparams_;
  xOptions options_;

#ifdef USE_HASH
  GHashTable hash_;
  EHashTable ehash_;
#endif


  void findSequence(int ictx, const Move & move, int ply, int depth, int counter, ScoreType alpha, ScoreType betta) const;
  void verifyGenerators(int ictx, const Move & hmove);

public:
  void saveHash(std::string const& fname) const;
  void loadHash(std::string const& fname);
};

} // NEngine