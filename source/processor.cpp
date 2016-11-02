
/*************************************************************
  processor.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <fstream>
#include <map>
#include <processor.h>
#include <Helpers.h>
#include <xindex.h>

namespace NShallow
{

static const int DepthMaximum = 32;

void Processor::setCallback(NEngine::xCallback& xcbk)
{
  xcbk.giveTime_ = [this]() -> int
  {
    return this->giveMoreTime();
  };

  engine_.setCallbacks(xcbk);
}

void Processor::setPost(bool p)
{
  if(is_thinking())
    return;

  post_ = p;
}

void Processor::setDepth(int depth)
{
  if(is_thinking())
    return;

  xtimeMS_ = 0;
  movesLeft_ = 0;
  movesToGo_ = 0;
  timePerMoveMS_ = -1;
  if(depth < 2)
    depth = 2;
  maxDepth_ = depth;
  engine_.setMaxDepth(depth);
  engine_.setTimeLimit(timePerMoveMS_);
}

void Processor::setTimePerMove(int ms)
{
  if(is_thinking())
    return;

  maxDepth_ = -1;
  xtimeMS_ = 0;
  movesLeft_ = 0;
  movesToGo_ = 0;
  timePerMoveMS_ = ms;
  if(timePerMoveMS_ < 100)
    timePerMoveMS_ = 100;
  engine_.setTimeLimit(timePerMoveMS_);
  engine_.setMaxDepth(DepthMaximum);
}

void Processor::setXtime(int ms)
{
  if(is_thinking())
    return;

  maxDepth_ = -1;
  xtimeMS_ = ms;
  if(xtimeMS_ < 100)
    xtimeMS_ = 100;
  timePerMoveMS_ = -1;
  engine_.setMaxDepth(DepthMaximum);
}

void Processor::setMovesLeft(int mleft)
{
  if(is_thinking())
    return;

  maxDepth_ = -1;
  movesLeft_ = mleft;
  movesToGo_ = 0;
  timePerMoveMS_ = -1;
  engine_.setMaxDepth(DepthMaximum);
}

void Processor::setMovesToGo(int mtogo)
{
  if(is_thinking())
    return;

  maxDepth_ = -1;
  movesLeft_ = 0;
  movesToGo_ = mtogo;
  timePerMoveMS_ = -1;
  engine_.setMaxDepth(DepthMaximum);
}


int Processor::giveMoreTime()
{
  if(timePerMoveMS_ > 0 || xtimeMS_ <= 500 || givetimeCounter_ > 0)
    return 0;

  int add_t = 0;
  int mcount = 40;

  if(movesLeft_ > 0)
  {
    int mcount = engine_.getBoard().movesCount();
    mcount = movesLeft_ - (mcount-1) % movesLeft_;
  }
  else if(movesToGo_ > 0)
  {
    mcount = movesToGo_;
  }

  givetimeCounter_++;

  if(mcount < 2)
    return 0;

  add_t = xtimeMS_/mcount;
  if(mcount > 20)
    return add_t;
  else if(mcount > 13)
    add_t /= 2;
  else if(mcount > 7)
    add_t /= 4;
  else
    add_t /= 8;

  return add_t;
}

void Processor::enableBook(int v)
{
}

bool Processor::undo()
{
  if(is_thinking())
  {
    NEngine::xPostCmd cmd(NEngine::xPostType::xpUndo);
    engine_.postCommand(cmd);
    return false;
  }

  auto& board = engine_.getBoard();
  if ( board.halfmovesCount() > 0 )
  {
    board.unmakeMove();
    return true;
  }

  return false;
}

void Processor::setMemory(int mb)
{
  if(is_thinking())
    return;

  engine_.setMemory(mb);
}

NEngine::Figure::Color Processor::color() const
{
  return engine_.getBoard().getColor();
}

bool Processor::init()
{
  if(is_thinking())
  {
    NEngine::xPostCmd cmd(NEngine::xPostType::xpNew);
    engine_.postCommand(cmd);
    return true;
  }

  engine_.clearHash();
  return engine_.fromFEN("");
}

void Processor::analyze()
{
  if(is_thinking())
    return;

  thinking_ = true;
  engine_.setTimeLimit(0);
  engine_.setMaxDepth(DepthMaximum);

  NEngine::SearchResult sres;

  engine_.setAnalyzeMode(true);
  engine_.findMove(sres);
  engine_.setAnalyzeMode(false);

  engine_.setTimeLimit(timePerMoveMS_);
  engine_.setMaxDepth(maxDepth_ < 0 ? DepthMaximum : maxDepth_);
  thinking_ = false;
}

void Processor::stop()
{
  engine_.pleaseStop();
}

boost::optional<ReplyStruct> Processor::reply(bool winboardFormat)
{
  if(is_thinking())
    return boost::none;

  updateTiming();

  ReplyStruct rep;

  auto & board = engine_.getBoard();
  rep.white_ = NEngine::Figure::ColorWhite == board.getColor();
  rep.state_ = static_cast<NEngine::Board::State>(board.getState());

  if(board.drawState() || board.matState())
    return rep;

  engine_.setAnalyzeMode(false);
  thinking_ = true;
  givetimeCounter_ = 0;
  NEngine::SearchResult sres;
  if(engine_.findMove(sres))
  {
    if(board.validateMove(sres.best_))
    {
      board.makeMove(sres.best_);
      board.verifyState();
    }
    else
      sres.best_.clear();
  }
  thinking_ = false;

  rep.state_ = static_cast<NEngine::Board::State>(board.getState());
  rep.moveStr_ = moveToStr(sres.best_, winboardFormat);
  if(rep.moveStr_.empty())
    return boost::none;

  return rep;
}

boost::optional<ReplyStruct> Processor::move(xCmd const& moveCmd)
{
  if(is_thinking())
    return boost::none;

  if(moveCmd.type() != xType::xMove)
    return boost::none;

  ReplyStruct rep;
  auto& board = engine_.getBoard();
  auto color = board.getColor();
  auto ocolor = NEngine::Figure::otherColor(color);
  rep.state_ = static_cast<NEngine::Board::State>(board.getState());
  rep.white_ = NEngine::Figure::ColorWhite == color;

  if(board.drawState() || board.matState())
    return rep;

  auto move = strToMove(moveCmd.str(), board);
  if(!move)
    return boost::none;

  if(!board.validateMove(move))
    return boost::none;

  board.makeMove(move);
  board.verifyState();
  rep.state_ = static_cast<NEngine::Board::State>(board.getState());
  updateTiming();

  return rep;
}

bool Processor::makeMove(std::string const& moveStr)
{
  if(moveStr.empty())
    return false;

  NEngine::Board & board = engine_.getBoard();

  NEngine::Move move = NEngine::strToMove(moveStr, board);
  if(!move)
    return false;

  if ( !board.validateMove(move) )
    return false;

  board.makeMove(move);
  board.verifyState();
  updateTiming();

  return true;
}

void Processor::save()
{
  if(is_thinking())
    return;

  std::ofstream ofs("game_001.pgn");
  auto const& board = engine_.getBoard();
  NEngine::Board::save(board, ofs);
}

void Processor::fen2file(const char * fname)
{
  if(is_thinking())
    return;

  if(!fname)
    return;

  try
  {
    std::ofstream ofs(fname);
    auto fen = engine_.getBoard().toFEN();
    ofs << fen << std::endl;
  }
  catch(...)
  {
  }
}

void Processor::pgn2file(const char * fname)
{
  if(!fname)
    return;

  try
  {
    std::ofstream ofs(fname);
    NEngine::Board::save(engine_.getBoard(), ofs);
  }
  catch(...)
  {
  }
}

void Processor::hash2file(const char * fname)
{
  if(is_thinking() || !fname)
    return;

  engine_.saveHash(fname);
}

//////////////////////////////////////////////////////////////////////////
bool Processor::fromFEN(xCmd const& cmd)
{
  if(is_thinking())
  {
    NEngine::xPostCmd pcmd(NEngine::xPostType::xpFen, cmd.fen());
    engine_.postCommand(pcmd);
    return true;
  }
  return fromFEN(cmd.fen());
}

bool Processor::fromFEN(std::string const& fen)
{
  return engine_.fromFEN(fen);
}

std::string Processor::toFEN()
{
  if(is_thinking())
    return{};

  return engine_.toFEN();
}

//////////////////////////////////////////////////////////////////////////
// edit mode
//////////////////////////////////////////////////////////////////////////
void Processor::editCmd(xCmd const& cmd)
{
  if(is_thinking())
  {
    if(cmd.type() == xType::xLeaveEdit) // command '.'
    {
      NEngine::xPostCmd pcmd(NEngine::xPostType::xpUpdate);
      engine_.postCommand(pcmd);
    }

    return;
  }

  switch(cmd.type())
  {
  case xType::xEdit:
    figureColor_ = NEngine::Figure::ColorWhite;
    boardColor_ = engine_.getBoard().getColor();
    break;

  case xType::xChgColor:
    figureColor_ = NEngine::Figure::otherColor(figureColor_);
    break;

  case xType::xClearBoard:
    engine_.getBoard().initEmpty(boardColor_);
    break;

  case xType::xSetFigure:
    setFigure(cmd);
    break;

  case xType::xLeaveEdit:
    engine_.getBoard().invalidate();
    break;
  }
}

void Processor::setFigure(xCmd const& cmd)
{
  if(is_thinking())
    return;

  if(cmd.str().size() < 3)
    return;

  char str[16];

  int x = str[1] - 'a';
  if(x < 0 || x > 7)
    return;

  int y = str[2] - '1';
  if(y < 0 || y > 7)
    return;

  static std::map<char, NEngine::Figure::Type> char2type =
  {
    { 'p', NEngine::Figure::TypePawn },
    { 'b', NEngine::Figure::TypeBishop },
    { 'n', NEngine::Figure::TypeKnight },
    { 'r', NEngine::Figure::TypeRook },
    { 'q', NEngine::Figure::TypeQueen },
    { 'k', NEngine::Figure::TypeKing }
  };

  NEngine::Figure::Type ftype = NEngine::Figure::TypeNone;
  auto iter = char2type.find(str[0]);
  if(iter == char2type.end())
    return;

  bool firstStep = false;
  if(NEngine::Figure::TypeKing == ftype &&
    ((NEngine::Figure::ColorWhite == figureColor_ &&
    'e' == str[1] && '1' == str[2]) ||
    (NEngine::Figure::ColorBlack == figureColor_ && 'e' == str[1] && '8' == str[2])))
  {
  	firstStep = true;
  }

  if(NEngine::Figure::TypeRook == ftype)
  {
  	if ( 'a' == str[1] || 'h' == str[1] )
      firstStep = NEngine::Figure::ColorWhite == figureColor_ && '1' == str[2] || NEngine::Figure::ColorBlack == figureColor_ && '8' == str[2];
  }

  NEngine::Index pos(x, y);
  engine_.getBoard().addFigure(figureColor_, ftype, pos);
}

//////////////////////////////////////////////////////////////////////////
void Processor::updateTiming()
{
  if(is_thinking())
    return;

  if ( maxDepth_ > 0 )
  {
    engine_.setTimeLimit(-1);
    return;
  }

  if ( timePerMoveMS_ > 0 )
  {
    engine_.setTimeLimit(timePerMoveMS_);
    return;
  }

  if ( movesLeft_ <= 0 && movesToGo_ <= 0 && xtimeMS_ > 0 )
  {
    engine_.setTimeLimit(xtimeMS_/40);
    return;
  }

  int mcount = 2;

  if ( movesLeft_ > 0 )
    mcount = movesLeft_ - (engine_.getBoard().movesCount()-1) % movesLeft_;
  else if ( movesToGo_ > 0 )
    mcount = movesToGo_;

  if ( mcount < 2 )
    mcount = 2;

  engine_.setTimeLimit(xtimeMS_/mcount);

}

} // NShallow
