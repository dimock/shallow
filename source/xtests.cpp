/*************************************************************
xtests.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xtests.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <chrono>
#include <boost/algorithm/string/trim.hpp>

namespace NEngine
{

FenTest::FenTest(std::string const& ffname, xTestFen_ErrorCallback const& ecbk)
{
  std::regex r("([0-9pnbrqkPNBRQKw/\\s\\-]+)([\\s]*bm[\\s]*)([0-9a-hpnrqkPNBRQKOx+!\\-]+)([\\s]*[;])");
  std::ifstream ifs(ffname);
  for(; ifs;)
  {
    std::string line;
    std::getline(ifs, line);
    boost::algorithm::trim(line);
    if(line.empty())
      continue;
    if(line[0] == '#')
      continue;
    std::smatch m;
    if(!std::regex_search(line, m, r) || m.size() < 4)
    {
      ecbk("regex failed on line: " + line);
      continue;
    }
    std::string fstr = m[1];
    std::string mstr = m[3];
    xtests_details_ns::FBoard board;
    if(!fromFEN(fstr, board))
    {
      ecbk("invalid fen: " + fstr);
      continue;
    }
    Move move = strToMove(mstr, board);
    if(!move)
    {
      ecbk("invalid move: " + mstr + " in position: " + fstr);
      continue;
    }
    emplace_back(board, move);
  }
}

void testFen(std::string const& ffname, xTestFen_Callback const& cbk, xTestFen_ErrorCallback const& ecbk)
{
  FenTest ft(ffname, ecbk);
  auto t = std::chrono::high_resolution_clock::now();
  for(size_t i = 0; i < ft.size(); ++i)
  {
    auto& x = ft[i];
    cbk(i, x.first, x.second);
  }
  auto dt = std::chrono::high_resolution_clock::now() - t;
  std::cout << "time: " << std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() << " (ms)" << std::endl;
}

void testSee(std::string const& ffname)
{
  NEngine::testFen(ffname,
    [](size_t i, NEngine::Board& board, NEngine::Move& move)
    {
      std::cout << i << ": "
        << NEngine::toFEN(board) << "  "
        << NEngine::printSAN(board, move)
        << "  see: " << board.see_old(move)
        << "  see new: " << board.see(move)
        << std::endl;
    },
    [](std::string const& err)
    {
      std::cout << "error: " << err << std::endl;
    });
}

} // NEngine
