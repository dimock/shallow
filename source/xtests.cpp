/*************************************************************
xtests.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xtests.h>
#include <xprotocol.h>
#include <kpk.h>
#include <iostream>
#include <fstream>
#include <regex>
#include <chrono>
#include <iomanip>
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
  std::regex r("([0-9pnbrqkPNBRQKw/\\s\\-]+)([\\s]*bm[\\s]*)([0-9a-hpnrqkPNBRQKOx+=!\\s\\-]+)(;[\\s]*)?([\\-]?[\\d]+)?");
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
    std::cout << line << std::endl;
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
  NEngine::EvalCoefficients evals{ evalCoeffs() };
  int summ_min{ -1 };
  int iters_num{};
  int steps_num{};
  int depth = 8;
  int Niters = 5;
  int Nsteps = 1;
  double r  = 0.1;
  double dr = 0.5;
  optimizeFen(ffname, [&proc, depth](size_t i, xEPD& epd)
  {
    std::cout << i << ": ";
    proc.setDepth(depth);
    proc.setBoard(epd.board_);
    proc.clear();
    auto r = proc.reply(false);
    if(!r)
    {
      std::cout << "-" << std::endl;
      return 1;
    }
    auto best = r->best_;
    auto iter = std::find_if(epd.moves_.begin(), epd.moves_.end(), [&best](NEngine::Move const& move) { return move == best; });
    if(iter == epd.moves_.end())
    {
      std::cout << "-" << std::endl;
      return 1;
    }
    std::cout << NEngine::printSAN(epd.board_, r->best_) << std::endl;
    return 0;
  },
  [&](int summ) -> bool
  {
    if(summ_min < 0 || summ_min > summ)
    {
      summ_min = summ;
      //proc.saveEval("eval.txt");
      //evals = proc.getEvals();
      evals.currentToIninital();
    }
    if(++iters_num >= Niters)
    {
      iters_num = 0;
      steps_num++;
      //proc.setEvals(evals);
      r *= dr;
    }
    //proc.adjustEval({}, {}, r);
    std::cout << iters_num << " iteration. " << steps_num << " step. current fails = " << summ << ". minimum fails = " << summ_min << std::endl;
    return steps_num < Nsteps;
  },
    [](std::string const& err)
  {
    std::cout << "error: " << err << std::endl;
  });
  std::cout << summ_min << std::endl;
}

void evaluateFen(std::string const& ffname)
{
  NEngine::testFen(
    ffname,
    [](size_t, NEngine::xEPD& e)
  {
    NShallow::Processor proc;
    NEngine::Evaluator eval;
    eval.initialize(&e.board_, nullptr);
    auto score = eval(-NEngine::Figure::MatScore, NEngine::Figure::MatScore);
    std::cout << score << std::endl;
  },
    [](std::string const& err_str)
  {
    std::cout << "Error: " << err_str << std::endl;
  });
}

void kpkTable(std::string const& fname)
{
  NShallow::Processor proc;
  //NTime::duration tm(NTime::from_milliseconds(500));
  std::array<std::array<std::array<uint64, 2>, 64>, 64> kpk = {};
  for(int kw = 0; kw < 64; ++kw)
  {
    for(int kl = 0; kl < 64; ++kl)
    {
      if(distanceCounter().getDistance(kl, kw) < 2)
        continue;
      for(int p = 8; p < 56; ++p)
      {
        if(p == kl || p == kw)
          continue;
        for(int color = 0; color < 2; ++color)
        {
          Figure::Color ccolor = (Figure::Color)color;
          SBoard<256> board;
          board.initEmpty(ccolor);
          board.addFigure(Figure::ColorWhite, Figure::TypePawn, p);
          board.addFigure(Figure::ColorWhite, Figure::TypeKing, kw);
          board.addFigure(Figure::ColorBlack, Figure::TypeKing, kl);
          if(!board.invalidate())
            continue;
          if(board.getState() != Board::State::Ok && board.getState() != Board::State::UnderCheck)
            continue;
          //proc.setTimePerMove(tm);
          proc.setDepth(22);
          proc.setScoreLimit(500);
          proc.setBoard(board);
          proc.clear();
          std::cout << "processing " << kw << " " << kl << " " << p << " " << color << std::endl;
          auto r = proc.reply(false);
          if(!r)
          {
            std::cout << "Error" << std::endl;
            return;
          }
          auto rr = *r;
          if(std::abs(rr.score_) > 500)
          {
            kpk[kw][kl][color] |= 1ULL << p;
            std::cout << " winner: " << r->score_ << std::endl;
          }
          else
          {
            std::cout << " draw: " << r->score_ << std::endl;
          }
        }
      }
    }
  }
  std::ofstream f(fname);
  f << "#include <kpk.h>" << std::endl << std::endl;
  f << "namespace NEngine" << std::endl;
  f << "{" << std::endl << std::endl;
  f << "std::vector<std::vector<std::array<uint64, 2>>> kpk_ = {" << std::endl;
  for(size_t kw = 0; kw < 64; ++kw)
  {
    f << "  {";
    for(size_t kl = 0; kl < 64; ++kl)
    {
      if((kl & 3) == 0)
      {
        f << std::endl << "    ";
      }
      f << "{ ";
      f << "0x" << std::hex << std::setfill('0') << std::setw(16) << kpk[kw][kl][0] << "ULL";
      f << ", " << "0x" << std::hex << std::setfill('0') << std::setw(16) << kpk[kw][kl][1] << "ULL";
      f << " },";
    }
    f << std::endl << "  }," << std::endl;
  }
  f << "};" << std::endl << std::endl;
  f << "} // NEngine" << std::endl;
}

} // NEngine
