/*************************************************************
xtests.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <Helpers.h>
#include <xoptimize.h>
#include <list>
#include <functional>
#include <regex>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <boost/algorithm/string.hpp>

namespace NEngine
{

template <typename BOARD, typename MOVE, typename UNDO>
struct xEPD
{
  xEPD()
  {}

  xEPD(xEPD&& other) :
    board_(std::move(other.board_)),
    moves_(std::move(other.moves_)),
    score_(other.score_),
    fen_(std::move(other.fen_))
  {
  }

  xEPD& operator = (xEPD&& other)
  {
    board_ = std::move(other.board_);
    moves_ = std::move(other.moves_);
    score_ = other.score_;
    fen_   = std::move(other.fen_);
  }

  SBoard<BOARD, UNDO, 32> board_;
  std::vector<MOVE> moves_;
  int score_{};
  std::string fen_;
  std::string err_;
};

template <typename BOARD, typename MOVE, typename UNDO>
using xTestFen_Callback = std::function<void(size_t, xEPD<BOARD, MOVE, UNDO>&)>;

template <typename BOARD, typename MOVE, typename UNDO>
using xProcessFen_Callback = std::function<int(size_t, xEPD<BOARD, MOVE, UNDO>&)>;

using xTestFen_ErrorCallback = std::function<void(std::string const&)>;
using xDoOptimize_Callback = std::function<bool(int)>;

template <typename B, typename M, typename U>
inline void appendMove(xEPD<B, M, U>& epd, std::string const& smove)
{
  if(Move move = strToMove(smove, epd.board_))
  {
    epd.moves_.push_back(move);
  }
}

template <typename BOARD, typename MOVE, typename UNDO>
class FenTest : public std::vector<xEPD<BOARD, MOVE, UNDO>>
{
  using base_class = std::vector<xEPD<BOARD, MOVE, UNDO>>;
public:
  FenTest(std::string const& ffname, xTestFen_ErrorCallback const& ecbk)
  {
    std::regex r("([0-9a-hpnbrqkPNBRQKw/\\s\\-]+)([\\s]*bm[\\s]*)?([0-9a-hpnrqkPNBRQKOx+=!\\s\\-]+)?([\\s]*;[\\s]*\\\")?([\\w\\-]+)?");
    ::std::ifstream ifs(ffname);
    for(; ifs;)
    {
      ::std::string sline;
      ::std::getline(ifs, sline);
      trim(sline);
      if(sline.empty())
        continue;
      if(sline[0] == '#')
        continue;
      std::cout << sline << std::endl;
      std::smatch m;
      if(!std::regex_search(sline, m, r) || m.size() < 4)
      {
        ecbk("regex failed on line: " + sline);
        continue;
      }
      std::string fstr = m[1];
      std::string mstr = m[3];
      xEPD<BOARD, MOVE, UNDO> epd;
      if(!fromFEN(fstr, epd.board_))
      {
        ecbk("invalid fen: " + fstr);
        continue;
      }
      epd.fen_ = sline;
      std::vector<std::string> str_moves;
      boost::algorithm::split(str_moves, mstr, boost::is_any_of(" \t"), boost::token_compress_on);
      for(auto const& smove : str_moves)
      {
        if(smove.empty())
          continue;
        appendMove(epd, smove);
      }
      if(m.size() >= 6)
      {
        std::string sscore = m[5];
        try
        {
          if(!sscore.empty())
            epd.score_ = std::stoi(sscore);
        }
        catch(std::exception const&)
        {
        }
      }
      base_class::push_back(std::move(epd));
    }
  }
};

template <typename BOARD, typename MOVE, typename UNDO>
void testFen(std::string const& ffname,
             xTestFen_Callback<BOARD, MOVE, UNDO> const& cbk,
             xTestFen_ErrorCallback const& ecbk)
{
  FenTest<BOARD, MOVE, UNDO> ft(ffname, ecbk);
  auto t = std::chrono::high_resolution_clock::now();
  for(size_t i = 0; i < ft.size(); ++i)
  {
    cbk(i, ft[i]);
  }
  auto dt = std::chrono::high_resolution_clock::now() - t;
  std::cout << ft.size() << " positions; time: "
    << std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() << " (ms)" << std::endl;
  std::cout << "nodes count: " << NEngine::x_movesCounter << std::endl;
  double ts = std::chrono::duration_cast<std::chrono::milliseconds>(dt).count()/1000.0;
  std::cout << "NPS: " << NEngine::x_movesCounter/ts << std::endl;

}

void testSee(std::string const& ffname);
void optimizeFen(std::string const& ffname);
void optimizeFenEval(std::string const& ffname);
void evaluateFen(std::string const& ffname);
void see_perf_test(std::string const& fname);
void kpkTable(std::string const& fname);
void speedTest();
std::vector<std::string> board2Test(std::string const& epdfile);
void epdFolder(std::string const& folder);
void pgnFolder(std::string const& folder);
void generateMoves(std::string const& ffname, std::string const& ofname);

} // NEngine
