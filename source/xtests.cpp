/*************************************************************
xtests.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xtests.h>
#include <iostream>
#include <fstream>
#include <regex>

namespace NEngine
{

FenTest::FenTest(std::string const& ffname)
{
  std::regex r("([0-9pnbrqkPNBRQKw/\\s\\-]+)([\\s]*bm[\\s]*)([0-9a-hpnrqkPNBRQKOx+!\\-]+)([\\s]*[;])");
  std::ifstream ifs(ffname);
  for(; ifs;)
  {
    std::string line;
    std::getline(ifs, line);
    std::smatch m;
    if(!std::regex_search(line, m, r) || m.size() < 4)
      continue;
    auto fstr = m[1];
    auto mstr = m[3];
    xtests_details_ns::FBoard board;
    if(!fromFEN(fstr, board))
      continue;
    Move move = strToMove(mstr, board);
    if(!move)
      continue;
    emplace_back(board, move);
  }
}

void testFen(std::string const& ffname, xTestFen_Callback const& cbk)
{
  FenTest ft(ffname);
  for(auto& x : ft)
  {
    cbk(x.first, x.second);
  }
}

} // NEngine
