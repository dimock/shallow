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

class CapsGenerator;
class MovesGenerator;
class EscapeGenerator;
class ChecksGenerator;

struct PlyStack
{
  PlyStack() {}

  Move killer_;
  Move pv_[MaxPly+4];

  void clearPV(int depth)
  {
    for(int i = 0; i < depth; ++i)
      pv_[i].clear();
  }

  void clear(int ply)
  {
    pv_[ply].clear();
  }

  void setKiller(const Move & killer)
  {
    if(!killer.capture_ && !killer.new_type_)
      killer_ = killer;
  }

  void clearKiller()
  {
    killer_.clear();
  }
};

struct SearchParams
{
  void reset();

  NTime::duration timeLimit_{};
  int  depthMax_{};
  bool analyze_mode_{};
};

class Engine
{
  friend class CapsGenerator;
  friend class MovesGenerator;
  friend class EscapeGenerator;
  friend class ChecksGenerator;

public:

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

  Board & getBoard() { return scontexts_[0].board_; }
  const Board & getBoard() const { return scontexts_[0].board_; }

  void pleaseStop();

  void setAnalyzeMode(bool);
  void setTimeLimit(NTime::duration const& tm);
  void setMaxDepth(int d);

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

  // testing of move generator
  void enumerate();
  void enumerate(int depth);

  // search routine
  ScoreType alphaBetta0();
  ScoreType alphaBetta(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, bool nm = false);
  ScoreType captures(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, bool pv, ScoreType score0 = -ScoreMax);
  int depthIncrement(int ictx, Move & move, bool pv) const;
  void assemblePV(int ictx, const Move & move, bool checking, int ply);


  // is given movement caused by previous. this mean that if we don't do this move we loose
  // we actually check if moved figure was attacked by previously moved one or from direction it was moved from
  bool isRealThreat(int ictx, const Move & move);

#ifdef USE_HASH
  void prefetchHash(int ictx);
  // we should return alpha if flag is Alpha, or Betta if flag is Betta
  GHashTable::Flag getHash(int ictx, int depth, int ply, ScoreType alpha, ScoreType betta, Move & hmove, ScoreType & hscore, bool pv);
  void putHash(int ictx, const Move & move, ScoreType alpha, ScoreType betta, ScoreType score, int depth, int ply, bool threat);
#endif // USE_HASH

  // analyze mode support
  bool updateRequested_{ false };
  xCallback callbacks_;

  // search data
  static const int depth0_ = 1; // start depth
  volatile bool stop_;

  struct SearchContext
  {
    //SearchContext();

    SBoard<Board::GameLength>     board_;
    Evaluator eval_;
    std::array<PlyStack, MaxPly+4>    plystack_;
    std::array<Move, Board::MovesMax> moves_;
//    std::array<UndoInfo, Board::GameLength> undoStack_;
  };

  // will be used in multi-threading mode???
  SearchContext scontexts_[1];

  SearchData sdata_;
  SearchParams sparams_;

  xOptions options_;


#ifdef USE_HASH
  GHashTable hash_;
  EHashTable ehash_;
#endif


  /// for DEBUG, verification

#ifdef VERIFY_ESCAPE_GENERATOR
  void verifyEscapeGen(int ictx, const Move & hmove);
#endif

#ifdef VERIFY_CHECKS_GENERATOR
  void verifyChecksGenerator(int ictx);
#endif

#ifdef VERIFY_CAPS_GENERATOR
  void verifyCapsGenerator(int ictx);
#endif

#ifdef VERIFY_FAST_GENERATOR
  void verifyFastGenerator(int ictx, const Move & hmove, const Move & killer);
#endif

#ifdef VERIFY_TACTICAL_GENERATOR
  void verifyTacticalGenerator(int ictx);
#endif

  void findSequence(int ictx, const Move & move, int ply, int depth, int counter, ScoreType alpha, ScoreType betta) const;
  void verifyGenerators(int ictx, const Move & hmove);

public:
  void saveHash(std::string const& fname) const;
  void loadHash(std::string const& fname);
};

} // NEngine