/*************************************************************
xtests.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <Helpers.h>
#include <list>
#include <functional>

namespace NEngine
{

namespace xtests_details_ns
{
using FBoard = SBoard<16>;
};

struct xEPD
{
  xEPD()
  {}

  xEPD(xEPD&& other) :
    board_(std::move(other.board_)),
    moves_(std::move(other.moves_)),
    score_(other.score_)
  {
  }

  xEPD& operator = (xEPD&& other)
  {
    board_ = std::move(other.board_);
    moves_ = std::move(other.moves_);
    score_ = other.score_;
  }

  xtests_details_ns::FBoard board_;
  std::vector<Move> moves_;
  int score_{};
};

using xTestFen_Callback = std::function<void(size_t, xEPD&)>;
using xTestFen_ErrorCallback = std::function<void(std::string const&)>;
using xProcessFen_Callback = std::function<int(size_t, xEPD&)>;
using xDoOptimize_Callback = std::function<bool(int)>;

class FenTest : public std::vector<xEPD>
{

public:
  FenTest(std::string const& ffname, xTestFen_ErrorCallback const&);
};

void testFen(std::string const& ffname, xTestFen_Callback const&, xTestFen_ErrorCallback const&);
void testSee(std::string const& ffname);
void optimizeFen(std::string const& ffname);
void evaluateFen(std::string const& ffname);
void see_perf_test(std::string const& fname);
void kpkTable(std::string const& fname);

} // NEngine
