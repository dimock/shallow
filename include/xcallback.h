/*************************************************************
xcallback.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <xoptimize.h>
//#include <Move.h>
#include <xtime.h>
#include <array>
#include <functional>
#include <memory>

namespace NEngine
{
class SearchResult
{
public:
  SearchResult();

  SearchResult& operator = (SearchResult const& other);

  /// statistic
  NTime::duration dt_{};
  int nodesCount_{};
  int totalNodes_{};
  int depthMax_{};
  int numOfMoves_{};
  int counter_{};
  Board board_;

  /// result
  Move best_{};
  int  depth_{};
  ScoreType score_;
  std::array<Move, MaxPly + 4> pv_;
};

struct SearchData
{
  void reset();
  void restart();
  void inc_nc()
  {
    totalNodes_++;
    nodesCount_++;
  }

  Board board_;
  int counter_{};
  int numOfMoves_{};
  int depth_{};
  int nodesCount_{};
  int totalNodes_{};
  int plyMax_{};
  NTime::point tstart_{};
  NTime::point tprev_{};
  SMove best_;
};

using query_input_command    = std::function<bool()>;
using give_more_time_command = std::function<NTime::duration()>;
using send_result_command    = std::function<void(SearchResult const&)>;
using send_stats_command     = std::function<void(SearchData const&)>;

struct xCallback
{
  query_input_command     queryInput_;
  give_more_time_command  giveTime_;
  send_result_command     sendOutput_;
  send_result_command     sendFinished_;
  send_stats_command      sendStats_;
  std::ofstream*          slog_{};
};

} // NEngine
