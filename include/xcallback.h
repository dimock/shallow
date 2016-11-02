/*************************************************************
xcallback.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <Board.h>
#include <Move.h>
#include <time.h>
#include <array>
#include <functional>
#include <memory>

namespace NEngine
{
class SearchResult
{
public:
  SearchResult();

  /// statistic
  clock_t dt_{};
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
  Move pv_[MaxPly+4];
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
  clock_t tstart_{};
  clock_t tprev_{};
  Move best_;
};

using query_input_command    = std::function<void()>;
using give_more_time_command = std::function<int()>;
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

enum class xPostType
{
  xpNone,
  xpUndo,
  xpUpdate,
  xpHint,
  xpNew,
  xpFen
};

struct xPostCmd
{
  xPostType   type_{ xPostType::xpNone };
  std::string fen_;

  xPostCmd(xPostType t, std::string const& f = std::string{}) :
    type_(t), fen_(f)
  {}
};

} // NEngine
