/*************************************************************
xtests.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xtests.h>
#include <MovesGenerator.h>
#include <xoptimize.h>
#include <xprotocol.h>
#include <kpk.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_set>
#include <boost/filesystem.hpp>

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
  std::cout << ft.size() << " cases; time: " << std::chrono::duration_cast<std::chrono::milliseconds>(dt).count() << " (ms)" << std::endl;
}

}

void testMovegen(std::string const& ffname)
{
  testFen<Board, Move, UndoInfo>(ffname,
    [](size_t i, xEPD<Board, Move, UndoInfo>& epd)
  {
    std::cout << toFEN(epd.board_) << std::endl;
    ChecksGenerator<Board, SMove> ckg{ epd.board_ };
    ckg.generateStrongest();
    Move* move = nullptr;
    for (int j = 1; move = ckg.next();)
    {
      if (epd.board_.see(*move, 0))
        std::cout << "    " << i + 1 << "[" << j++ << "]: " << moveToStr(*move, false) << std::endl;
    }
  },
    [](std::string const& err)
  {
    std::cout << "error: " << err << std::endl;
  });
}

void testSee(std::string const& ffname)
{
  testFen<Board, Move, UndoInfo>(ffname,
                                 [](size_t i, xEPD<Board, Move, UndoInfo>& epd)
    {
      for(auto const& move : epd.moves_)
      {
        auto v= epd.board_.see(move, 500);
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
        x += epd.board_.see(move, 0);
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
        x += epd.board_.see(move, 0);
      }
    }
  };
  testFen<Board, Move, UndoInfo>(fname, see_new_cbk, [](std::string const& err)
  {
    std::cout << "error: " << err << std::endl;
  });
}

void evaluateFen(std::string const& ffname, std::string const& refname)
{
  std::vector<int> refEvals;
  if (!refname.empty()) {
    std::ifstream rfs{ refname };
    std::string line;
    for (;std::getline(rfs, line);) {
      auto evals = NEngine::split(line, [](char c) { return NEngine::is_any_of(" ,;cp", c); });
      if (evals.size() != 2)
        break;
      refEvals.push_back(std::stoi(evals[1]));
    }
  }
  std::cout << "N: score, refScore, diff, cp" << std::endl;
  float totalError = 0.0f;

  NShallow::Processor proc;
  testFen<Board, Move, UndoInfo>(
    ffname,
    [&totalError, &refEvals](size_t i, xEPD<Board, Move, UndoInfo>& e)
  {
    NEngine::Evaluator eval;
    eval.initialize(&e.board_);
    auto score = eval(-NEngine::Figure::MatScore, NEngine::Figure::MatScore);
    int diff = -100000;
    float t = 0.0f;
    int refScore = -10000;
    if (i < refEvals.size()) {
      refScore = refEvals[i];
      diff = score - refEvals[i];
      t = static_cast<float>(std::abs(diff)) / 100.f;// (std::abs(refEvals[i]) + std::abs(score)) / 2.0f;
      totalError += t;
    }
    std::cout << i+1 << ": " << score << ", " << refScore << ", " << diff << ", " << std::setprecision(2) << t << std::endl;
  },
  [](std::string const& err_str)
  {
    std::cout << "Error: " << err_str << std::endl;
  });
  std::cout << "total error: " << totalError << std::endl;
  std::cout << "average error: " << totalError/refEvals.size() << std::endl;
}

void generateMoves(std::string const& ffname, std::string const& ofname)
{
  using GBoard = SBoard<Board, UndoInfo, 16>;
  std::ofstream ofs{ofname};
  testFen<Board, Move, UndoInfo>(
    ffname,
    [&ofs](size_t, xEPD<Board, Move, UndoInfo>& e)
  {
    auto fen = toFEN(e.board_);
    ofs << fen << ";";
    auto moves = generate<Board, Move>(e.board_);
    for (auto move : moves)
    {
      move.clear_ok();
      if (!e.board_.validateMoveBruteforce(move))
        continue;

      auto smove = moveToStr(move, false);

      GBoard board{e.board_, true};
      board.makeMove(move);
      if (board.underCheck())
        smove += "!";

      ofs << " " << smove;
    }
    ofs << "\n";
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
          if(std::abs(r.score_) > 500)
          {
            kpk[kw][kl][color] |= 1ULL << p;
            std::cout << " winner: " << r.score_ << std::endl;
          }
          else
          {
            std::cout << " draw: " << r.score_ << std::endl;
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
  auto dt_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dt).count();
  int nps = int((x_movesCounter / (double)dt_ms) * 1000);
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
      NEngine::to_lower(e);
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

void appendEPD(std::string const& fen, std::string const& bm, int score)
{
  static int counter = 0;
  std::ostringstream oss;
  oss << fen << " bm " << bm << "; \"test_" << counter << "\"" << std::endl;
  std::ofstream ofs("result.epd", std::ostream::app);
  std::cout << oss.str();
  ofs << oss.str();
  counter++;
}

void processBoardPGN(std::string const& pgn_file)
{
  static std::unordered_set<BitMask> existing_;
  NTime::duration tm(NTime::from_milliseconds(500));
  std::ifstream ifs(pgn_file);
  for(;;)
  {
    SBoard<Board, UndoInfo, Board::GameLength> board;
    std::cout << "loading from " << pgn_file << std::endl;
    if(!load(board, ifs))
    {
      std::cout << "no games any more" << std::endl;
      break;
    }
    std::cout << "loaded ok" << std::endl;

    int num = board.halfmovesCount();
    while(board.halfmovesCount() > 0)
      board.unmakeMove(board.lastUndo().move_);

    for(int i = 0; i < num; ++i)
    {
      Move move = board.undoInfo(i).move_;
      board.makeMove(move);
      if(!board.invalidate() || board.drawState() || board.matState())
        break;
      if(board.underCheck())
      {
        std::cout << "under check, skipping\n";
        continue;
      }
      if(existing_.count(board.fmgr().hashCode()) > 0)
      {
        std::cout << "already exsists, skipping\n";
        continue;
      }
      existing_.insert(board.fmgr().hashCode());

      SBoard<Board, UndoInfo, Board::GameLength> sboard(board, true);
      NEngine::Evaluator eval;
      eval.initialize(&sboard);
      auto score0 = eval(-NEngine::Figure::MatScore, NEngine::Figure::MatScore);

      NShallow::Processor proc;
      //proc.setTimePerMove(tm);
      proc.setDepth(4);
      //proc.setScoreLimit(500);
      proc.setBoard(sboard);
      proc.clear();
      auto r = proc.reply(false);
      if(!r)
      {
        std::cout << "Error" << std::endl;
        return;
      }
      if(sboard.is_capture(r.best_))
      {
        std::cout << "capture, skipping\n";
        continue;
      }
      if(std::abs(r.score_ - score0) > 20 || std::abs(r.score_) > 300)
      {
        std::cout << "big score " << std::abs(r.score_)  << " or diff " << std::abs(r.score_ - score0) << ", skipping\n";
        continue;
      }
      auto bm = printSAN(sboard, r.best_);
      auto sfen = toFEN(sboard);
//      std::cout << sfen << " bm " << bm << " score " << rr.score_ << std::endl;
      appendEPD(sfen, bm, r.score_);
    }
  }
}

void pgnFolder(std::string const& folder)
{
  std::vector<std::string> errors;
  boost::filesystem::path p(folder);
  if(boost::filesystem::is_regular_file(boost::filesystem::status(p)))
  {
    std::string e = p.extension().string();
    NEngine::to_lower(e);
    if(e != ".pgn")
      return;
    processBoardPGN(folder);
  }
  else if(boost::filesystem::is_directory(boost::filesystem::status(p)))
  {
    boost::filesystem::directory_iterator i{ p };
    for(; i != boost::filesystem::directory_iterator{}; i++)
    {
      boost::filesystem::path pp{ *i };
      pgnFolder(pp.string());
    }
  }
}

} // NEngine
