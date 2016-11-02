
/*************************************************************
  xboard.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
  *************************************************************/

#include <io.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <xprotocol.h>
#include <Helpers.h>

namespace NShallow
{

xProtocolMgr::xProtocolMgr() :
  os_(std::cout)
{
  NEngine::xCallback xcbk;

  xcbk.queryInput_ = [this]()
  {
    if(auto cmd = parse(this->input_.peekInput(), this->isUci_))
    {
      this->processCmd(cmd);
    }
    return !this->stop_;
  };

  xcbk.sendOutput_ = [this](NEngine::SearchResult const& sres)
  {
    this->printPV(sres);
  };

  xcbk.sendFinished_ = [this](NEngine::SearchResult const& sres)
  {
    this->printBM(sres);
  };

  xcbk.sendStats_ = [this](NEngine::SearchData const& sdata)
  {
    this->printStat(sdata);
  };

  proc_.setCallback(xcbk);
}

void xProtocolMgr::outState(NEngine::Board::State state, bool white)
{
  if(NEngine::Board::State::ChessMat & state)
  {
    if(white)
      os_ << "1-0 {White}" << std::endl;
    else
      os_ << "0-1 {Black}" << std::endl;
  }
  else if(NEngine::Board::State::DrawInsuf & state)
  {
    os_ << "1/2-1/2 {Draw by material insufficient}" << std::endl;
  }
  else if(NEngine::Board::State::Stalemat & state)
  {
    os_ << "1/2-1/2 {Stalemate}" << std::endl;
  }
  else if(NEngine::Board::State::DrawReps & state)
  {
    os_ << "1/2-1/2 {Draw by repetition}" << std::endl;
  }
  else if(NEngine::Board::State::Draw50Moves & state)
  {
    os_ << "1/2-1/2 {Draw by fifty moves rule}" << std::endl;
  }
}

void xProtocolMgr::printPV(NEngine::SearchResult const& sres)
{
  if(!sres.best_ || sres.best_ != sres.pv_[0])
    return;

  if(isUci_)
  {
    printInfo(sres);
    return;
  }

  auto board = sres.board_;
  NEngine::UndoInfo undoStack[NEngine::Board::GameLength];
  board.set_undoStack(undoStack);

  os_ << sres.depth_ << " " << sres.score_ << " " << (int)(sres.dt_/10) << " " << sres.totalNodes_;
  for(int i = 0; i < sres.depth_ && sres.pv_[i]; ++i)
  {
    os_ << " ";

    auto pv = sres.pv_[i];
    auto captured = pv.capture_;
    pv.clearFlags();
    pv.capture_ = captured;

    if(!board.possibleMove(pv))
      break;

    auto str = NEngine::printSAN(board, pv);
    if(str.empty())
      break;

    X_ASSERT(!board.validateMove(pv), "move is invalid but it is not detected by printSAN()");

    board.makeMove(pv);

    os_ << str;
  }
  os_ << std::endl;
}

void xProtocolMgr::printStat(NEngine::SearchData const& sdata)
{
  if(sdata.depth_ <= 0)
    return;

  if(isUci_)
  {
    printUciStat(sdata);
    return;
  }

  auto board = sdata.board_;
  NEngine::UndoInfo undoStack[NEngine::Board::GameLength];
  board.set_undoStack(undoStack);

  clock_t t = clock() - sdata.tstart_;
  int movesLeft = sdata.numOfMoves_ - sdata.counter_;
  std::ostringstream oss;
  oss << "stat01: " << t/10 << " " << sdata.totalNodes_ << " " << sdata.depth_-1 << " " << movesLeft << " " << sdata.numOfMoves_;
  std::string outstr = oss.str();

  auto mv = sdata.best_;
  uint8 captured = mv.capture_;
  mv.clearFlags();
  mv.capture_ = captured;

  if(mv && board.validateMove(mv))
  {
    auto str = printSAN(board, mv);
    outstr += " " + str;
  }

  os_ << outstr << std::endl;
}

void xProtocolMgr::uciSetOption(const xCmd & cmd)
{
  proc_.setMemory(cmd.value());
}

void xProtocolMgr::uciPositionFEN(const xCmd & cmd)
{
  proc_.fromFEN(cmd.fen());
#ifdef WRITE_LOG_FILE_
  auto fen = proc_.toFEN();
  ofs_log_ << std::endl;
  ofs_log_ << "used fen: " << fen << std::endl;
#endif
}

void xProtocolMgr::uciPositionMoves(const xCmd & cmd)
{
  proc_.fromFEN("");

  bool moves_found = false;
  for(auto const& mvstr : cmd.moves())
  {
    proc_.makeMove(mvstr);

#ifdef WRITE_LOG_FILE_
    ofs_log_ << smove << " ";
#endif
  }

#ifdef WRITE_LOG_FILE_
  auto fen = proc_.toFEN();
  ofs_log_ << std::endl;
  ofs_log_ << "used fen: " << fen << std::endl;
#endif
}

void xProtocolMgr::uciGo(const xCmd & cmd)
{
  bool white = proc_.color() == NEngine::Figure::ColorWhite;

  if(cmd.infinite())
  {
    proc_.analyze();
    return;
  }

  proc_.setXtime(white ? cmd.wtime() : cmd.btime());
  proc_.setMovesToGo(cmd.movestogo());
  proc_.setTimePerMove(cmd.movetime());
  proc_.setDepth(cmd.depth());

  if(auto r = proc_.reply(false))
  {
    os_ << "bestmove " << r->moveStr_ << std::endl;
  }
}

void xProtocolMgr::printUciStat(NEngine::SearchData const& sdata)
{
  if(!sdata.best_)
    return;

  auto smove = moveToStr(sdata.best_, false);
  if(smove.empty())
    return;

  os_ << "info ";
  os_ << "currmove " << smove << " ";
  os_ << "currmovenumber " << sdata.counter_+1 << " ";
  os_ << "nodes " << sdata.totalNodes_ << " ";
  os_ << "depth " << sdata.depth_ << std::endl;
}

void xProtocolMgr::printInfo(NEngine::SearchResult const& sres)
{
  auto board = sres.board_;
  NEngine::UndoInfo undoStack[NEngine::Board::GameLength];
  board.set_undoStack(undoStack);

  std::string pv_str;
  for(int i = 0; i < MaxPly && sres.pv_[i]; ++i)
  {
    if(i)
      pv_str += " ";

    auto pv = sres.pv_[i];
    uint8 captured = pv.capture_;
    pv.clearFlags();
    pv.capture_ = captured;

    if(!board.possibleMove(pv))
      break;

    auto str = printSAN(board, pv);
    if(str.empty())
      break;

    X_ASSERT(!board.validateMove(pv), "move is invalid but it is not detected by printSAN()");

    board.makeMove(pv);

    pv_str += str;
  }

  double dt = sres.dt_;
  if(dt < 1)
    dt = 1;

  int nps = (int)((double)(sres.totalNodes_)*1000.0 / dt);

  os_ << "info ";

  os_ << "depth " << sres.depth_ << " ";
  os_ << "seldepth " << sres.depthMax_ << " ";

  if(sres.score_ >= NEngine::Figure::MatScore-MaxPly)
  {
    int n = (NEngine::Figure::MatScore - sres.score_) / 2;
    os_ << "score mate " << n << " ";
  }
  else if(sres.score_ <= MaxPly-NEngine::Figure::MatScore)
  {
    int n = (-NEngine::Figure::MatScore - sres.score_) / 2;
    os_ << "score mate " << n << " ";
  }
  else
    os_ << "score cp " << sres.score_ << " ";

  os_ << "time " << static_cast<int>(sres.dt_) << " ";
  os_ << "nodes " << sres.totalNodes_ << " ";
  os_ << "nps " << nps << " ";

  if(sres.best_)
  {
    os_ << "currmove " << moveToStr(sres.best_, false) << " ";
  }

  os_ << "currmovenumber " << sres.counter_+1 << " ";
  os_ << "pv " << pv_str;

  os_ << std::endl;
}

void xProtocolMgr::printBM(NEngine::SearchResult const& sres)
{
  if(!sres.best_)
    return;

  os_ << "bestmove " << moveToStr(sres.best_, false) << std::endl;
}

bool xProtocolMgr::doCmd()
{
  if(auto cmd = parse(input_.getInput(), isUci_))
  {
    processCmd(cmd);
  }

  return !stop_;
}

void xProtocolMgr::processCmd(xCmd const& cmd)
{
  switch(cmd.type())
  {
  case xType::UCI:
    isUci_ = true;
    os_ << "id name Shallow" << std::endl;
    os_ << "id author Dmitry Sultanov" << std::endl;
    os_ << "option name Hash type spin default 256 min 1 max 1024" << std::endl;
    os_ << "uciok" << std::endl;
    break;

  case xType::xBoard:
    isUci_ = false;
    break;

  case xType::SetOption:
    uciSetOption(cmd);
    break;

  case xType::IsReady:
    os_ << "readyok" << std::endl;
    break;

  case xType::UCInewgame:
    proc_.init();
    break;

  case xType::PositionFEN:
    uciPositionFEN(cmd);
    break;

  case xType::PositionMoves:
    uciPositionMoves(cmd);
    break;

  case xType::UCIgo:
    uciGo(cmd);
    break;

  case xType::xPing:
    os_ << "pong " << cmd.value() << std::endl;
    break;

  case xType::xNew:
    proc_.init();
    break;

  case xType::xOption:
    break;

  case xType::xMemory:
    proc_.setMemory(cmd.value());
    break;

  case xType::xProtover:
    if(cmd.value() > 1)
    {
      os_ << "feature done=0" << std::endl;
      os_ << "feature setboard=1" << std::endl;
      os_ << "feature myname=\"shallow\" memory=1" << std::endl;
      os_ << "feature done=1" << std::endl;
    }
    break;

  case xType::xSaveBoard:
    proc_.save();
    break;

  case xType::xSetboardFEN:
    if(!(fenOk_ = proc_.fromFEN(cmd)))
    {
#ifdef WRITE_LOG_FILE_
      if(cmd.paramsNum() > 0)
        ofs_log_ << "invalid FEN given: " << cmd.packParams() << endl;
      else
        ofs_log_ << "there is no FEN in setboard command" << endl;
#endif
      os_ << "tellusererror Illegal position" << std::endl;
    }
    break;

  case xType::xEdit:
  case xType::xChgColor:
  case xType::xClearBoard:
  case xType::xSetFigure:
  case xType::xLeaveEdit:
    proc_.editCmd(cmd);
    break;

  case xType::xPost:
  case xType::xNopost:
    proc_.setPost(cmd.type() == xType::xPost);
    break;

  case xType::xAnalyze:
    proc_.analyze();
    break;

  case xType::xGoNow:
    proc_.stop();
    break;

  case xType::xExit:
    proc_.stop();
    break;

  case xType::xQuit:
    stop_ = true;
    proc_.stop();
#ifdef WRITE_LOG_FILE_
    thk_.save();
#endif
    break;

  case xType::xForce:
    force_ = true;
    break;

  case xType::xSt:
    proc_.setTimePerMove(cmd.value()*1000);
    break;

  case xType::xSd:
    proc_.setDepth(cmd.value());
    break;

  case xType::xTime:
    proc_.setXtime(cmd.value()*10);
    break;

  case xType::xOtime:
    break;

  case xType::xLevel:
    proc_.setMovesLeft(cmd.value());
    break;

  case xType::xUndo:
    proc_.undo();
    break;

  case xType::xRemove:
    proc_.undo();
    proc_.undo();
    break;

  case xType::xGo:
    if(!fenOk_)
    {
      os_ << "Illegal move" << std::endl;
    }
    else
    {
      force_ = false;
      if(auto rs = proc_.reply(true))
      {
#ifdef WRITE_LOG_FILE_
        ofs_log_ << " " << str << endl;
#endif

        os_ << rs->moveStr_ << std::endl;
        outState(rs->state_, rs->white_);
      }
    }
    break;

  case xType::xMove:
    if(!fenOk_)
    {
#ifdef WRITE_LOG_FILE_
      ofs_log_ << " illegal move. fen is invalid" << endl;
#endif
      os_ << "Illegal move" << std::endl;
    }
    else
    {
#ifdef WRITE_LOG_FILE_
      if(thk_.is_thinking())
        ofs_log_ << " can't move - thinking" << endl;
#endif

      if(auto rs = proc_.move(cmd))
      {
        if(NEngine::Board::isDraw(rs->state_) || NEngine::Board::ChessMat & rs->state_)
        {
          outState(rs->state_, rs->white_);
        }
        else if(!force_)
        {
          if(auto rs = proc_.reply(true))
          {
            os_ << rs->moveStr_ << std::endl;
            outState(rs->state_, rs->white_);
          }
        }
      }
      else
      {
        os_ << "Illegal move: " << cmd.str() << std::endl;

#ifdef WRITE_LOG_FILE_
        ofs_log_ << " Illegal move: " << cmd.str() << endl;
#endif
      }
    }
    break;
  }
}

} // NShallow