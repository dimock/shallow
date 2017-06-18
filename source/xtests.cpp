/*************************************************************
xtests.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xtests.h>
#include <xoptimize.h>
#include <xprotocol.h>
#include <kpk.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

namespace NEngine
{

namespace
{

template <typename BOARD, typename MOVE, typename UNDO>
std::ostream& operator << (std::ostream& os, xEPD<BOARD, MOVE, UNDO> const& epd)
{
  os << toFEN(epd.board_) << "; moves: ";
  for(auto const& move : epd.moves_)
    os << moveToStr(move, false) << " ";
  os << "; score = " << epd.score_ << std::endl;
  return os;
}

void optimizeFen(std::string const& ffname,
                 xProcessFen_Callback<Board, Move, UndoInfo> const& process_cbk,
                 xDoOptimize_Callback const& optimize_cbk,
                 xTestFen_ErrorCallback const& err_cbk)
{
  FenTest<Board, Move, UndoInfo> ft(ffname, err_cbk);
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


void testSee(std::string const& ffname)
{
  testFen<Board, Move, UndoInfo>(ffname,
                                 [](size_t i, xEPD<Board, Move, UndoInfo>& epd)
    {
      for(auto const& move : epd.moves_)
      {
        auto v= epd.board_.see(move);
        std::cout << i << ": "
          << toFEN(epd.board_) << "  "
          << printSAN(epd.board_, move)
          << "  see result: " << v
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
  auto see_old_cbk = [N](size_t i, xEPD<Board, Move, UndoInfo>& epd)
  {
    int x = 0;
    for(int n = 0; n < N; ++n)
    {
      for(auto& move : epd.moves_)
      {
        x += epd.board_.see(move);
      }
    }
  };
  auto see_new_cbk = [N](size_t i, xEPD<Board, Move, UndoInfo>& epd)
  {
    int x = 0;
    for(int n = 0; n < N; ++n)
    {
      for(auto& move : epd.moves_)
      {
        x += epd.board_.see(move);
      }
    }
  };
  testFen<Board, Move, UndoInfo>(fname, see_new_cbk, [](std::string const& err)
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
  optimizeFen(ffname, [&proc, depth](size_t i, xEPD<Board, Move, UndoInfo>& epd)
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
    auto iter = std::find_if(epd.moves_.begin(), epd.moves_.end(), [&best](Move const& move) { return move == best; });
    if(iter == epd.moves_.end())
    {
      std::cout << "-" << std::endl;
      return 1;
    }
    std::cout << printSAN(epd.board_, r->best_) << std::endl;
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
  testFen<Board, Move, UndoInfo>(
    ffname,
    [](size_t, xEPD<Board, Move, UndoInfo>& e)
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
          SBoard<Board, UndoInfo, 256> board;
          board.initEmpty(ccolor);
          board.addFigure(Figure::ColorWhite, Figure::TypePawn, p);
          board.addFigure(Figure::ColorWhite, Figure::TypeKing, kw);
          board.addFigure(Figure::ColorBlack, Figure::TypeKing, kl);
          if(!board.invalidate())
            continue;
          if(board.state() != State::Ok && board.state() != State::UnderCheck)
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


void speedTest()
{
  SBoard<Board, UndoInfo, 512> board;
  fromFEN("", board);
  auto t = std::chrono::high_resolution_clock::now();
  xsearch(board, 4);
  auto dt = std::chrono::high_resolution_clock::now() - t;
  double dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dt).count();
  int nps = (x_movesCounter / dt_ms) * 1000;
  std::cout << x_movesCounter
    << " moves; time: " << dt_ms/1000.0 << " (s);"
    << " nps " << nps << std::endl;
}

std::vector<std::string> board2Test(std::string const& epdfile)
{
  std::vector<std::string> errors;
  testFen<Board, Move, UndoInfo>(epdfile, [&errors](size_t i, xEPD<Board, Move, UndoInfo>& epd)
  {
    bool ok{true};
    try
    {
      ok = xverifyMoves(epd.board_);
      //xsearch(epd.board_, 2);
      std::cout << std::setw(4) << i << " verified with result: " << std::boolalpha << ok << std::endl;
    }
    catch(std::exception const& e)
    {
      epd.err_ = e.what();
      ok = false;
      std::cout << std::setw(4) << i << " failed with " << e.what() << std::endl;
    }
    if(!ok)
      errors.push_back(epd.fen_ + "; error -> "+ epd.err_);
  },
  [](std::string const& err)
  {
    std::cout << "Error: " << err << std::endl;
  });
  return errors;
}

void epdFolder(std::string const& folder)
{
  std::vector<std::string> errors;
  boost::filesystem::path p(folder);
  if(boost::filesystem::is_regular_file(boost::filesystem::status(p)))
  {
    auto errs = board2Test(folder);
    errors.insert(errors.end(), errs.begin(), errs.end());
  }
  else if(boost::filesystem::is_directory(boost::filesystem::status(p)))
  {
    boost::filesystem::directory_iterator i{ p };
    for(; i != boost::filesystem::directory_iterator{}; i++)
    {
      boost::filesystem::path pp{ *i };
      std::string e = pp.extension().string();
      boost::algorithm::to_lower(e);
      if(e != ".epd")
        continue;
      auto errs = board2Test(pp.string());
      errors.insert(errors.end(), errs.begin(), errs.end());
    }
  }
  std::cout << "errors count: " << errors.size() << std::endl;
  std::ofstream ofs("errors.log");
  ofs << "errors count: " << errors.size() << std::endl;
  for(auto const& e : errors)
  {
    std::cout << e << std::endl;
    ofs << e << std::endl;
  }
}

} // NEngine
