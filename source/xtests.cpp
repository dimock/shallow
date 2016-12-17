/*************************************************************
xtests.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xtests.h>
#include <xprotocol.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <chrono>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>

namespace NEngine
{

namespace
{

std::ostream&
operator << (std::ostream& os, xEPD const& epd)
{
  os << toFEN(epd.board_) << "; moves: ";
  for(auto const& move : epd.moves_)
    os << moveToStr(move, false) << " ";
  os << "; score = " << epd.score_ << std::endl;
  return os;
}

void optimizeFen(std::string const& ffname,
                 xProcessFen_Callback const& process_cbk,
                 xDoOptimize_Callback const& optimize_cbk,
                 xTestFen_ErrorCallback const& err_cbk)
{
  FenTest ft(ffname, err_cbk);
  auto t = std::chrono::high_resolution_clock::now();
  for(;;)
  {
    int summ{};
    for(size_t i = 0; i < ft.size(); ++i)
    {
      summ += process_cbk(i, ft[i]);
    }
    if(!optimize_cbk(summ))
      break;
  }
  auto dt = std::chrono::high_resolution_clock::now() - t;
  std::cout << ft.size() << " moves; time: " << std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() << " (ms)" << std::endl;
}

}


FenTest::FenTest(std::string const& ffname, xTestFen_ErrorCallback const& ecbk)
{
  std::regex r("([0-9pnbrqkPNBRQKw/\\s\\-]+)([\\s]*bm[\\s]*)([0-9a-hpnrqkPNBRQKOx+=!\\-\\s]+)(;[\\s]*)?([\\-]?[\\d]+)?");
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
    xEPD epd;
    if(!fromFEN(fstr, epd.board_))
    {
      ecbk("invalid fen: " + fstr);
      continue;
    }
    std::vector<std::string> str_moves;
    boost::algorithm::split(str_moves, mstr, boost::is_any_of(" \t"), boost::token_compress_on);
    for(auto const& smove : str_moves)
    {
      if(smove.empty())
        continue;
      if(Move move = strToMove(smove, epd.board_))
      {
        epd.moves_.push_back(move);
      }
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
      {}
    }
    push_back(std::move(epd));
  }
}

void testFen(std::string const& ffname, xTestFen_Callback const& cbk, xTestFen_ErrorCallback const& ecbk)
{
  FenTest ft(ffname, ecbk);
  auto t = std::chrono::high_resolution_clock::now();
  for(size_t i = 0; i < ft.size(); ++i)
  {
    cbk(i, ft[i]);
  }
  auto dt = std::chrono::high_resolution_clock::now() - t;
  std::cout << ft.size() << " moves; time: " << std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() << " (ms)" << std::endl;
}

void testSee(std::string const& ffname)
{
  NEngine::testFen(ffname,
    [](size_t i, NEngine::xEPD& epd)
    {
      for(auto const& move : epd.moves_)
      {
        auto v_old = epd.board_.see_old(move);
        auto v_new = epd.board_.see(move);
        if((v_old < 0) == (v_new < 0))
          //if(v_old == v_new)
          return;
        std::cout << i << ": "
          << NEngine::toFEN(epd.board_) << "  "
          << NEngine::printSAN(epd.board_, move)
          << "  see old: " << v_old
          << "  see new: " << v_new
          << std::endl;
      }
    },
    [](std::string const& err)
    {
      std::cout << "error: " << err << std::endl;
    });
}

void see_perf_test(std::string const& fname)
{
  int N = 1000000;
  auto see_old_cbk = [N](size_t i, xEPD& epd)
  {
    int x = 0;
    for(int n = 0; n < N; ++n)
    {
      for(auto& move : epd.moves_)
      {
        x += epd.board_.see_old(move);
        move.seen_ = x != 0;
      }
    }
  };
  auto see_new_cbk = [N](size_t i, xEPD& epd)
  {
    int x = 0;
    for(int n = 0; n < N; ++n)
    {
      for(auto& move : epd.moves_)
      {
        x += epd.board_.see(move);
        move.seen_ = x != 0;
      }
    }
  };
  testFen(fname, see_new_cbk, [](std::string const& err)
  {
    std::cout << "error: " << err << std::endl;
  });
}

void optimizeFen(std::string const& ffname)
{
  NShallow::Processor proc;
  int summ_min{ -1 };
  int iters_num{};
  optimizeFen(ffname, [&proc](size_t i, xEPD& epd)
  {
    proc.setDepth(10);
    proc.setBoard(epd.board_);
    proc.clear();
    auto r = proc.reply(false);
    if(!r)
      return 1;
    auto best = r->best_;
    auto iter = std::find_if(epd.moves_.begin(), epd.moves_.end(), [&best](NEngine::Move const& move) { return move == best; });
    if(iter == epd.moves_.end())
      return 1;
    return 0;
  },
    [&summ_min, &iters_num](int summ) -> bool
  {
    if(summ_min < 0 || summ_min > summ)
    {
      summ_min = summ;
    }
    return false;// ++iters_num > 10;
  },
    [](std::string const& err)
  {
    std::cout << "error: " << err << std::endl;
  });
  std::cout << summ_min << std::endl;
}

} // NEngine
