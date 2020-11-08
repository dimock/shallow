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
    imoves_(std::move(other.imoves_)),
    score_(other.score_),
    fen_(std::move(other.fen_))
  {
  }

  xEPD& operator = (xEPD&& other)
  {
    board_ = std::move(other.board_);
    moves_ = std::move(other.moves_);
    imoves_ = std::move(other.imoves_);
    score_ = other.score_;
    fen_   = std::move(other.fen_);
  }

  SBoard<BOARD, UNDO, 32> board_;
  std::vector<MOVE> moves_;
  std::vector<std::string> imoves_;
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
#ifdef PROCESS_MOVES_SEQ
  epd.board_.stestMoves_.push_back(smove == "_" ? "" : smove);
#endif
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
    std::regex r("([0-9a-hpnbrqkPNBRQKw/\\s\\-]+)([\\s]*bm[\\s]*)?([0-9a-hpnullrqkPNBRQKOx+=!_\\s\\-]+)?([\\s]*dmin[\\s]+)?([0-9]+)?([\\s]*dmax[\\s]*)?([0-9]+)?([\\s]+hash[\\s]+)?([0-9]+)?([\\s]*;[\\s]*\\\")?([\\w\\-]+)?([\\s]*moves[\\s]*)?([0-9a-hprqkPNBRQKOx+=!_\\s\\-]+)?");
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
//      std::cout << sline << std::endl;
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
      epd.fen_ = fstr;
      auto str_moves = NEngine::split(mstr, [](char c) { return NEngine::is_any_of(" \t", c); });
      for(auto const& smove : str_moves)
      {
        if(smove.empty())
          continue;
        appendMove(epd, smove);
      }
#ifdef PROCESS_MOVES_SEQ
      if (m.size() > 5 && ((std::string)m[4]).find("dmin") != std::string::npos) {
        epd.board_.stestDepthMin_ = std::stoi(m[5]);
      }
      if (m.size() > 7 && ((std::string)m[6]).find("dmax") != std::string::npos) {
        epd.board_.stestDepthMax_ = std::stoi(m[7]);
      }
      if (m.size() > 9 && ((std::string)m[8]).find("hash") != std::string::npos) {
        epd.board_.stestHashKey_ = std::stoull(m[9]);
      }
      if (m.size() > 13 && ((std::string)m[12]).find("moves") != std::string::npos) {
        std::string imstr = m[13];
        auto str_imoves = NEngine::split(imstr, [](char c) { return NEngine::is_any_of(" \t", c); });
        for (auto const& smove : str_imoves)
        {
          if (smove.empty())
            continue;
          epd.imoves_.push_back(smove);
        }
      }
#endif
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

void testMovegen(std::string const& ffname);
void testSee(std::string const& ffname);
void evaluateFen(std::string const& ffname, std::string const& refname);
void saveFen(std::string const& ffname, std::string const& refname);
void see_perf_test(std::string const& fname);
void kpkTable(std::string const& fname);
void speedTest();
std::vector<std::string> board2Test(std::string const& epdfile);
void epdFolder(std::string const& folder);
void pgnFolder(std::string const& folder);
void generateMoves(std::string const& ffname, std::string const& ofname);
void analyzeFen(std::string const& fname, std::string const& bestfname, std::string const& seqfname);

} // NEngine
