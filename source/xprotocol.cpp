/*************************************************************
  xboard.cpp - Copyright (C) 2016 by Dmitry Sultanov
  *************************************************************/

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <xprotocol.h>
#include <Helpers.h>
#include <xoptions.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>

namespace NShallow
{

xProtocolMgr::xProtocolMgr() :
  os_(std::cout)
#ifdef LOG_PV
  ,ofs_log_(new std::ofstream("log.txt", std::ios::out))
#endif
{
  NEngine::xCallback xcbk;
  xcbk.slog_ = ofs_log_.get();

  xcbk.queryInput_ = [this]()
  {
    if(auto cmd = cmds_.peek())
    {
      if(!this->swallowCmd(cmd))
      {
        cmds_.push(cmd);
      }
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

  if(cmds_.isUci())
  {
    printInfo(sres);
    return;
  }

  NEngine::SBoard<NEngine::Board::GameLength> board(sres.board_);
  std::ostringstream oss;

  oss << sres.depth_ << " " << sres.score_ << " " << NTime::centi_seconds<int>(sres.dt_) << " " << sres.totalNodes_;
  for(int i = 0; i < sres.depth_ && sres.pv_[i]; ++i)
  {
    oss << " ";

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

    oss << str;
  }
  os_ << oss.str() << std::endl;
}

void xProtocolMgr::printStat(NEngine::SearchData const& sdata)
{
  if(sdata.depth_ <= 0)
    return;

  if(cmds_.isUci())
  {
    printUciStat(sdata);
    return;
  }

  NEngine::SBoard<NEngine::Board::GameLength> board(sdata.board_);

  int movesLeft = sdata.numOfMoves_ - sdata.counter_;
  std::ostringstream oss;
  oss << "stat01: " << NTime::centi_seconds<int>(NTime::now() - sdata.tstart_)
    << " " << sdata.totalNodes_ << " " << sdata.depth_-1 << " " << movesLeft << " " << sdata.numOfMoves_;
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

void xProtocolMgr::setOption(const xCmd & cmd)
{
  NEngine::xOptions xopts;
  xopts.hash_size_ = cmd.param("Hash");
  xopts.use_nullmove_ = cmd.param("Nullmove");
  proc_.setOptions(xopts);
}

void xProtocolMgr::uciPosition(const xCmd & cmd)
{
  if(!proc_.fromFEN(cmd.fen()))
    return;

  for(auto const& mvstr : cmd.moves())
  {
    proc_.makeMove(mvstr);
  }
}

bool xProtocolMgr::uciGo(const xCmd & cmd)
{
  if(cmd.infinite())
  {
    return proc_.analyze();
  }

  bool const white = proc_.color() == NEngine::Figure::ColorWhite;
  int xtime = white ? cmd.param("wtime") : cmd.param("btime");
  if(xtime > 0)
  {
    proc_.setXtime(NTime::from_milliseconds(xtime));
  }
  if(cmd.param("movestogo") > 0)
  {
    proc_.setMovesToGo(cmd.param("movestogo"));
  }
  if(cmd.param("movetime") > 0)
  {
    proc_.setTimePerMove(NTime::from_milliseconds(cmd.param("movetime")));
  }
  if(cmd.param("depth") > 0)
  {
    proc_.setDepth(cmd.param("depth"));
  }

  if(auto r = proc_.reply(false))
  {
    if(!r->moveStr_.empty())
      os_ << "bestmove " << r->moveStr_ << std::endl;
    return true;
  }
  else
  {
    return false;
  }
}

void xProtocolMgr::printUciStat(NEngine::SearchData const& sdata)
{
  if(!sdata.best_)
    return;

  auto smove = moveToStr(sdata.best_, false);
  if(smove.empty())
    return;

  os_ << "info "
      << "currmove " << smove << " "
      << "currmovenumber " << sdata.counter_+1 << " "
      << "nodes " << sdata.totalNodes_ << " "
      << "depth " << sdata.depth_ << std::endl;
}

void xProtocolMgr::printInfo(NEngine::SearchResult const& sres)
{
  NEngine::SBoard<NEngine::Board::GameLength> board(sres.board_);

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

    auto str = moveToStr(pv, false);
    if(str.empty())
      break;

    X_ASSERT(!board.validateMove(pv), "move is invalid but it is not detected by printSAN()");

    board.makeMove(pv);

    pv_str += str;
  }

  auto dt = NTime::seconds<double>(sres.dt_) + 0.001;
  int nps = static_cast<int>(sres.totalNodes_ / dt);
  std::ostringstream oss;

  oss << "info "
      << "depth " << sres.depth_ << " "
      << "seldepth " << sres.depthMax_ << " ";

  if(sres.score_ >= NEngine::Figure::MatScore-MaxPly)
  {
    int n = (NEngine::Figure::MatScore - sres.score_) / 2;
    oss << "score mate " << n << " ";
  }
  else if(sres.score_ <= MaxPly-NEngine::Figure::MatScore)
  {
    int n = (-NEngine::Figure::MatScore - sres.score_) / 2;
    oss << "score mate " << n << " ";
  }
  else
    oss << "score cp " << sres.score_ << " ";

  oss << "time " << NTime::milli_seconds<int>(sres.dt_) << " ";
  oss << "nodes " << sres.totalNodes_ << " ";
  oss << "nps " << nps << " ";

  if(sres.best_)
  {
    oss << "currmove " << moveToStr(sres.best_, false) << " ";
  }

  oss << "currmovenumber " << sres.counter_+1 << " ";
  oss << "pv " << pv_str;

  os_ << oss.str() << std::endl;
}

void xProtocolMgr::printBM(NEngine::SearchResult const& sres)
{
  if(!sres.best_)
    return;

  os_ << "bestmove " << moveToStr(sres.best_, false) << std::endl;
}

bool xProtocolMgr::doCmd()
{
  if(auto cmd = cmds_.next())
  {
    processCmd(cmd);
  }

  return !stop_;
}

bool xProtocolMgr::swallowCmd(xCmd const& cmd)
{
  bool swallow = false;
  bool stop    = true;
  switch(cmd.type())
  {
  case xType::xUndo:
  case xType::xRemove:
    stop = true;
    break;

  case xType::xQuit:
    stop = stop_ = true;
    swallow = true;
    break;

  case xType::xGoNow:
  case xType::xExit:
    stop = true;
    swallow = true;
    break;

  case xType::xLeaveEdit:
    proc_.editCmd(cmd);
    stop = false;
    swallow = true;
    break;
  }
  if(stop)
    proc_.stop();
  return swallow;
}

void xProtocolMgr::processCmd(xCmd const& cmd)
{
  switch(cmd.type())
  {
  case xType::UCI:
    cmds_.setUci(true);
    os_ << "id name Shallow" << std::endl;
    os_ << "id author Dmitry Sultanov" << std::endl;
    uciOutputOptions();
    os_ << "uciok" << std::endl;
    break;

  case xType::xBoard:
    cmds_.setUci(false);
    break;

  case xType::xOption:
  case xType::SetOption:
    setOption(cmd);
    break;

  case xType::IsReady:
    os_ << "readyok" << std::endl;
    break;

  case xType::UCInewgame:
  case xType::xNew:
    proc_.init();
    break;

  case xType::Position:
    uciPosition(cmd);
    break;

  case xType::UCIgo:
    uciGo(cmd);
    break;

  case xType::xPing:
    os_ << "pong " << cmd.value() << std::endl;
    break;

  case xType::xMemory:
    {
      NEngine::xOptions xopts;
      xopts.hash_size_ = cmd.value();
      proc_.setOptions(xopts);
    }
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
    proc_.save(cmd.param(0));
    break;

  case xType::xLoadBoard:
    proc_.load(cmd.param(0));
    break;

  case xType::xSetboardFEN:
    {
      fenOk_ = false;
      auto ok = proc_.fromFEN(cmd);
      if(!ok)
      {
        os_ << "tellusererror Illegal position" << std::endl;
      }
      else if(*ok)
        fenOk_ = true;
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
    proc_.setTimePerMove(NTime::duration(cmd.value()));
    break;

  case xType::xSd:
    proc_.setDepth(cmd.value());
    break;

  case xType::xTime:
    proc_.setXtime(NTime::from_centiseconds(cmd.value()));
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
        if(!rs->moveStr_.empty())
        {
          os_ << rs->moveStr_ << std::endl;
          outState(rs->state_, rs->white_);
        }
      }
    }
    break;

  case xType::xMove:
    if(!fenOk_)
    {
      os_ << "Illegal move" << std::endl;
    }
    else
    {
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
      }
    }
    break;
  }
}

void xProtocolMgr::uciOutputOptions()
{
  for(auto const& oinfo : NEngine::all_options())
  {
    std::ostringstream oss;
    oss << "option name " << oinfo.name
      << " type " << oinfo.type;
    if(!oinfo.min_val.empty())
      oss << " min " << oinfo.min_val;
    if(!oinfo.max_val.empty())
      oss << " max " << oinfo.max_val;
    if(!oinfo.def_val.empty())
      oss << " default " << oinfo.def_val;
    if(!oinfo.vars.empty())
    {
      std::vector<std::string> temp;
      std::transform(oinfo.vars.begin(), oinfo.vars.end(), std::back_inserter(temp),
        [](std::string const& s) { return "var " + s; });
      auto str = boost::algorithm::join(temp, " ");
      oss << str;
    }
    os_ << oss.str() << std::endl;
  }
}

void xProtocolMgr::printCmdDbg(xCmd const& cmd) const
{
  std::ofstream ofs("commands.txt", std::ios::app);
  ofs << to_string(cmd) << std::endl;
}

} // NShallow
