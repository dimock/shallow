
/*************************************************************
  xboard.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <io.h>
#include <iostream>
#include <string>
#include <fstream>
#include "xboard.h"

using namespace std;

static xBoardMgr * g_xboard_mgr_ = 0;

void queryInput()
{
  if ( !g_xboard_mgr_ )
    return;

  if ( !g_xboard_mgr_->peekInput() )
    return;

  g_xboard_mgr_->do_cmd();
}

//void sendOutput(SearchResult * sres)
//{
//  if ( !g_xboard_mgr_ )
//    return;
//
//  g_xboard_mgr_->printPV(sres);
//}

//void sendStats(SearchData * sdata)
//{
//  if ( !g_xboard_mgr_ )
//    return;
//
//  g_xboard_mgr_->printStat(sdata);
//}

//void sendFinished(SearchResult * sres)
//{
//  if ( !g_xboard_mgr_ )
//    return;
//
//  g_xboard_mgr_->printBM(sres);
//}

xBoardMgr::xBoardMgr() :
  os_(cout), uci_protocol_(false)
{
  vNum_ = 0;
  stop_ = false;
  force_ = false;
  fenOk_ = true;

  g_xboard_mgr_ = this;

  hinput_ = GetStdHandle(STD_INPUT_HANDLE);
  if ( hinput_ )
  {
    DWORD mode = 0;
    in_pipe_ = !GetConsoleMode(hinput_, &mode);
    if ( !in_pipe_ )
    {
      BOOL ok = SetConsoleMode(hinput_, mode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
      FlushConsoleInputBuffer(hinput_);
    }
  }

  //CallbackStruct cs;

  //cs.sendOutput_ = &sendOutput;
  //cs.sendStats_ = &sendStats;
  //cs.queryInput_ = &queryInput;
  //cs.sendFinished_ = &sendFinished;
  //thk_.setPlayerCallbacks(cs);
}

xBoardMgr::~xBoardMgr()
{
  g_xboard_mgr_ = 0;
  //thk_.clearPlayerCallbacks();
}
  
bool xBoardMgr::peekInput()
{
  if ( !hinput_ )
    return false;

  if ( in_pipe_ )
  {
    DWORD avaliable = 0;
    if ( !PeekNamedPipe(hinput_, 0, 0, 0, &avaliable, NULL) )
      return false;

    return avaliable != 0;
  }
  else
  {
    DWORD num = 0;
    if ( GetNumberOfConsoleInputEvents(hinput_, &num) )
    {
      if ( num == 0 )
        return false;

      INPUT_RECORD irecords[256];
      DWORD nread = 0;
      if ( PeekConsoleInput(hinput_, irecords, num, &nread) )
      {
        for (DWORD i = 0; i < nread; ++i)
        {
          if ( irecords[i].EventType & KEY_EVENT )
            return true;
        }
      }
    }
    return false;
  }
}


void xBoardMgr::out_state(ostream & os, uint8_t state, bool white)
{
  //if ( Board::ChessMat & state )
  //{
  //  if ( white )
  //    os << "1-0 {White}" << endl;
  //  else
  //    os << "0-1 {Black}" << endl;
  //}
  //else if ( Board::DrawInsuf & state )
  //{
  //  os << "1/2-1/2 {Draw by material insufficient}" << endl;
  //}
  //else if ( Board::Stalemat & state )
  //{
  //  os << "1/2-1/2 {Stalemate}" << endl;
  //}
  //else if ( Board::DrawReps & state )
  //{
  //  os << "1/2-1/2 {Draw by repetition}" << endl;
  //}
  //else if ( Board::Draw50Moves & state )
  //{
  //  os << "1/2-1/2 {Draw by fifty moves rule}" << endl;
  //}
}

void xBoardMgr::write_error(const std::exception * e /*= 0*/)
{
}

//void xBoardMgr::printPV(SearchResult * sres)
//{
//  if ( !sres || !sres->best_ || sres->best_ != sres->pv_[0] )
//    return;
//
//  if ( uci_protocol_ )
//  {
//    printInfo(sres);
//    return;
//  }
//
//  Board board = sres->board_;
//  UndoInfo undoStack[Board::GameLength];
//  board.set_undoStack(undoStack);
//
//  os_ << sres->depth_ << " " << sres->score_ << " " << (int)(sres->dt_/10) << " " << sres->totalNodes_;
//  for (int i = 0; i < sres->depth_ && sres->pv_[i]; ++i)
//  {
//    os_ << " ";
//
//    Move pv = sres->pv_[i];
//    uint8 captured = pv.capture_;
//    pv.clearFlags();
//    pv.capture_ = captured;
//
//    if ( !board.possibleMove(pv) )
//      break;
//
//    char str[64];
//    if ( !printSAN(board, pv, str) )
//      break;
//
//    THROW_IF( !board.validateMove(pv), "move is invalid but it is not detected by printSAN()");
//
//    board.makeMove(pv);
//
//    os_ << str;
//  }
//  os_ << std::endl;
//}
//
//void xBoardMgr::printStat(SearchData * sdata)
//{
//  if ( !sdata || sdata->depth_ <= 0 )
//    return;
//
//  if ( uci_protocol_ )
//  {
//    printUciStat(sdata);
//    return;
//  }
//
//  Board board = sdata->board_;
//  UndoInfo undoStack[Board::GameLength];
//  board.set_undoStack(undoStack);
//
//  char outstr[1024];
//  clock_t t = clock() - sdata->tstart_;
//  int movesLeft = sdata->numOfMoves_ - sdata->counter_;
//  sprintf(outstr, "stat01: %d %d %d %d %d", t/10, sdata->totalNodes_, sdata->depth_-1, movesLeft, sdata->numOfMoves_);
//
//  Move mv = sdata->best_;
//  uint8 captured = mv.capture_;
//  mv.clearFlags();
//  mv.capture_ = captured;
//
//  char str[64];
//  if ( mv && board.validateMove(mv) && printSAN(board, mv, str) )
//  {
//    strcat(outstr, " ");
//    strcat(outstr, str);
//  }
//
//  os_ << outstr << std::endl;
//}

void xBoardMgr::setProtocol(bool uci)
{
  uci_protocol_ = uci;
}

bool xBoardMgr::do_cmd()
{
  xCmd cmd;
  read_cmd(cmd);

  if ( !cmd )
    return !stop_;

  process_cmd(cmd);

  return !stop_;
}

void xBoardMgr::read_cmd(xCmd & cmd)
{
  const int slineSize = 65536;
  char sline[slineSize];
  cin.getline(sline, slineSize);

  char * s = strchr(sline, '\n');
  if ( s )
    *s = 0;

#ifdef WRITE_LOG_FILE_
  ofs_log_ << string(sline) << endl;
#endif

  cmd = parser_.parse(sline, uci_protocol_);
}

void xBoardMgr::process_cmd(xCmd & cmd)
{
  switch ( cmd.type() )
  {
  case xCmd::UCI:
    uci_protocol_ = true;
    os_ << "id name Shallow" << std::endl;
    os_ << "id author Dmitry Sultanov" << std::endl;
    os_ << "option name Hash type spin default 256 min 1 max 512" << std::endl;
    os_ << "uciok" << std::endl;
    break;

  case xCmd::xBoard:
    uci_protocol_ = false;
    break;

  case xCmd::SetOption:
    uciSetOption(cmd);
    break;

  case xCmd::IsReady:
    os_ << "readyok" << std::endl;
    break;

  case xCmd::UCInewgame:
    thk_.init();
    break;

  case xCmd::Position:
    uciPosition(cmd);
    break;

  case xCmd::UCIgo:
    uciGo(cmd);
    break;

  case xCmd::xPing:
    os_ << "pong " << cmd.asInt(0) << endl;
    break;

  case xCmd::xNew:
    thk_.init();
    break;

  case xCmd::xOption:
    {
      int v = cmd.getOption("enablebook");
      if ( v >= 0 )
        thk_.enableBook(v);

#ifdef WRITE_LOG_FILE_
      ofs_log_ << "  book was " << (v ? "enables" : "disabled") << endl;
#endif
    }
    break;

  case xCmd::xMemory:
    thk_.setMemory(cmd.asInt(0));
    break;

  case xCmd::xProtover:
    vNum_ = cmd.asInt(0);
    if ( vNum_ > 1 )
    {
      os_ << "feature done=0" << endl;
      os_ << "feature setboard=1" << endl;
      os_ << "feature myname=\"shallow\" memory=1" << endl;
      os_ << "feature done=1" << endl;
    }
    break;

  case xCmd::xSaveBoard:
    thk_.save();
    break;

  case xCmd::xSetboardFEN:
    if ( !(fenOk_ = thk_.fromFEN(cmd)) )
    {
#ifdef WRITE_LOG_FILE_
      if ( cmd.paramsNum() > 0 )
        ofs_log_ << "invalid FEN given: " << cmd.packParams() << endl;
      else
        ofs_log_ << "there is no FEN in setboard command" << endl;
#endif
      os_ << "tellusererror Illegal position" << endl;
    }
    break;

  case xCmd::xEdit:
  case xCmd::xChgColor:
  case xCmd::xClearBoard:
  case xCmd::xSetFigure:
  case xCmd::xLeaveEdit:
    thk_.editCmd(cmd);
    break;

  case xCmd::xPost:
  case xCmd::xNopost:
    thk_.setPost(cmd.type() == xCmd::xPost);
    break;

  case xCmd::xAnalyze:
    thk_.analyze();
    break;

  case xCmd::xGoNow:
    thk_.stop();
    break;

  case xCmd::xExit:
    thk_.stop();
    break;

  case xCmd::xQuit:
    stop_ = true;
    thk_.stop();
#ifdef WRITE_LOG_FILE_
    thk_.save();
#endif
    break;

  case xCmd::xForce:
    force_ = true;
    break;

  case xCmd::xSt:
    thk_.setTimePerMove(cmd.asInt(0)*1000);
    break;

  case xCmd::xSd:
    thk_.setDepth(cmd.asInt(0));
    break;

  case xCmd::xTime:
    thk_.setXtime(cmd.asInt(0)*10);
    break;

  case xCmd::xOtime:
    break;

  case xCmd::xLevel:
    thk_.setMovesLeft(cmd.asInt(0));
    break;

  case xCmd::xUndo:
    thk_.undo();
    {
      char fen[256];
      thk_.toFEN(fen);
    }
    break;

  case xCmd::xRemove:
    thk_.undo();
    thk_.undo();
    break;

  case xCmd::xGo:
    if ( !fenOk_ )
    {
      os_ << "Illegal move" << endl;
    }
    else
    {
      force_ = false;
      char str[256];
      uint8_t state = 0;//Board::Invalid;
      bool white;
      bool b = thk_.reply(str, state, white, true);
      if ( b )
      {
#ifdef WRITE_LOG_FILE_
        ofs_log_ << " " << str << endl;
#endif

        os_ << str << endl;

        //if ( Board::isDraw(state) || (state & Board::ChessMat) )
        {
          out_state(os_, state, white);

#ifdef WRITE_LOG_FILE_
          out_state(ofs_log_, state, white);
          //thk_.hash2file("hash");
#endif
        }
      }
    }
    break;

  case xCmd::xMove:
    if ( !fenOk_ )
    {
#ifdef WRITE_LOG_FILE_
      ofs_log_ << " illegal move. fen is invalid" << endl;
#endif
      os_ << "Illegal move" << endl;
    }
    else
    {
#ifdef WRITE_LOG_FILE_
      if ( thk_.is_thinking() )
        ofs_log_ << " can't move - thinking" << endl;
#endif

      uint8_t state;
      bool white;
      if ( thk_.move(cmd, state, white) )
      {
        //if ( Board::isDraw(state) || (Board::ChessMat & state) )
        {
          out_state(os_, state, white);
        }
//        else if ( !force_ )
//        {
//          char str[256];
//          bool b = thk_.reply(str, state, white, true);
//          if ( b )
//          {
//#ifdef WRITE_LOG_FILE_
//            ofs_log_ << " ... " << str << endl; 
//#endif
//
//            os_ << str << endl;
//
//            if ( Board::isDraw(state) || (state & Board::ChessMat) )
//            {
//              out_state(os_, state, white);
//
//#ifdef WRITE_LOG_FILE_
//              out_state(ofs_log_, state, white);
//              //thk_.hash2file("hash");
//#endif
//            }
//          }
//        }
      }
//      else
//      {
//        os_ << "Illegal move: " << cmd.str() << endl;
//
//#ifdef WRITE_LOG_FILE_
//        ofs_log_ << " Illegal move: " << cmd.str() << endl;
//#endif
//      }
    }
    break;
  }

}

void xBoardMgr::uciSetOption(const xCmd & cmd)
{
  if ( cmd.paramsNum() < 4 )
    return;

  if ( cmd.param(0) == "name" && cmd.param(1) == "Hash" && cmd.param(2) == "value" )
    thk_.setMemory(cmd.asInt(3));
}

void xBoardMgr::uciPosition(const xCmd & cmd)
{
  if ( cmd.paramsNum() < 1 )
    return;

  if ( cmd.param(0) == "fen" )
  {
    std::string fen = cmd.packParams(1);
    thk_.fromFEN(fen.c_str());
#ifdef WRITE_LOG_FILE_
    ofs_log_ << "fen: " << fen << endl;
#endif
  }
  else if ( cmd.param(0) == "startpos" || cmd.param(0) == "moves" )
  {
    thk_.fromFEN(0);
  }

  bool moves_found = false;
  for (size_t i = 0; i < cmd.paramsNum(); ++i)
  {
    /// start moves
    if ( cmd.param(i) == "moves" )
    {
      moves_found = true;
      continue;
    }

    if ( !moves_found )
      continue;

    char smove[256];
    strncpy(smove, cmd.param(i).c_str(), 256);

    if ( xParser::parseMove(smove) )
    {
      thk_.makeMove(smove);

#ifdef WRITE_LOG_FILE_
      ofs_log_ << smove << " ";
#endif
    }
  }

#ifdef WRITE_LOG_FILE_
  char fen[256];
  thk_.toFEN(fen);
  ofs_log_ << std::endl;
  ofs_log_ << "used fen: " << fen << std::endl;
#endif
}

void xBoardMgr::uciGo(const xCmd & cmd)
{
  bool white = false;//thk_.color() == Figure::ColorWhite;
  bool analize_mode = false;

  /// read timing params
  for (size_t i = 0; i < cmd.paramsNum(); ++i)
  {
    if ( cmd.param(i) == "infinite" )
    {
      analize_mode = true;
      break;
    }

    if ( i+1 >= cmd.paramsNum() )
      break;

    if ( ((cmd.param(i) == "wtime" && white) || (cmd.param(i) == "btime" && !white)) )
    {
      thk_.setXtime(cmd.asInt(i+1));
#ifdef WRITE_LOG_FILE_
      ofs_log_ << "set time: " << cmd.asInt(i+1) << std::endl;
#endif
    }
    else if ( cmd.param(i) == "movestogo" )
      thk_.setMovesToGo(cmd.asInt(i+1));
    else if ( cmd.param(i) == "movetime"  )
      thk_.setTimePerMove(cmd.asInt(i+1));
    else if ( cmd.param(i) == "depth"  )
      thk_.setDepth(cmd.asInt(i+1));
  }

  if ( analize_mode )
  {
    thk_.analyze();
    return;
  }

  char smove[256];
  uint8_t state;

  smove[0] = 0;
  if ( !thk_.reply(smove, state, white, false) )
    return;

  if ( smove[0] == 0 )
    return;
  os_ << "bestmove " << smove << std::endl;
}

//void xBoardMgr::printUciStat(SearchData * sdata)
//{
//  if ( !sdata || !sdata->best_ )
//    return;
//
//  char smove[256];
//  if ( !moveToStr(sdata->best_, smove, false) )
//    return;
//
//  os_ << "info ";
//  os_ << "currmove " << smove << " ";
//  os_ << "currmovenumber " << sdata->counter_+1 << " ";
//  os_ << "nodes " << sdata->totalNodes_ << " ";
//  os_ << "depth " << sdata->depth_ << std::endl;
//}

//void xBoardMgr::printInfo(SearchResult * sres)
//{
//  if ( !sres )
//    return;
//
//  Board board = sres->board_;
//  UndoInfo undoStack[Board::GameLength];
//  board.set_undoStack(undoStack);
//
//  std::string pv_str;
//  for (int i = 0; i < MaxPly && sres->pv_[i]; ++i)
//  {
//    if ( i )
//      pv_str += " ";
//
//    Move pv = sres->pv_[i];
//    uint8 captured = pv.capture_;
//    pv.clearFlags();
//    pv.capture_ = captured;
//
//    if ( !board.possibleMove(pv) )
//      break;
//
//    char str[64];
//    if ( !printSAN(board, pv, str) )
//      break;
//
//    THROW_IF( !board.validateMove(pv), "move is invalid but it is not detected by printSAN()");
//
//    board.makeMove(pv);
//
//    pv_str += str;
//  }
//
//  double dt = sres->dt_;
//  if ( dt < 1 )
//    dt = 1;
//
//  int nps = (int)((double)(sres->totalNodes_)*1000.0 / dt);
//
//  os_ << "info ";
//
//  os_ << "depth " << sres->depth_ << " ";
//  os_ << "seldepth " << sres->depthMax_ << " ";
//
//  if ( sres->score_ >= Figure::MatScore-MaxPly )
//  {
//    int n = (Figure::MatScore - sres->score_) / 2;
//    os_ << "score mate " << n << " ";
//  }
//  else if ( sres->score_ <= MaxPly-Figure::MatScore )
//  {
//    int n = (-Figure::MatScore - sres->score_) / 2;
//    os_ << "score mate " << n << " ";
//  }
//  else
//    os_ << "score cp " << sres->score_ << " ";
//
//  os_ << "time " << (int)sres->dt_ << " ";
//  os_ << "nodes " << sres->totalNodes_ << " ";
//  os_ << "nps " << nps << " ";
//
//  if ( sres->best_ )
//  {
//    char smove[256];
//    moveToStr(sres->best_, smove, false);
//    os_ << "currmove " << smove << " ";
//  }
//
//  os_ << "currmovenumber " << sres->counter_+1 << " ";
//  os_ << "pv " << pv_str;
//
//  os_ << std::endl;
//}
//
//void xBoardMgr::printBM(SearchResult * sres)
//{
//  if ( !sres || !sres->best_ )
//    return;
//
//  char smove[256];
//  if ( moveToStr(sres->best_, smove, false) )
//    os_ << "bestmove " << smove << std::endl;
//}
