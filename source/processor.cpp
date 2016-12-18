
/*************************************************************
  processor.cpp - Copyright (C) 2016 by Dmitry Sultanov
  *************************************************************/

#define NOMINMAX

#include <iostream>
#include <fstream>
#include <map>
#include <processor.h>
#include <Helpers.h>
#include <xindex.h>
#include <algorithm>
#include <boost/algorithm/string.hpp>

namespace NShallow
{

static const NTime::duration minimalXtimeToGive_ = NTime::duration(0.5);
static const NTime::duration minimalTimePerMove_ = NTime::duration(0.1);
static const int DepthMaximum = 32;

void Processor::setCallback(NEngine::xCallback& xcbk)
{
  xcbk.giveTime_ = [this]()
  {
    return this->giveMoreTime();
  };

  engine_.setCallbacks(xcbk);
}

bool Processor::setPost(bool p)
{
  if(is_thinking())
    return false;

  post_ = p;
  return true;
}

bool Processor::setDepth(int depth)
{
  if(is_thinking())
    return false;

  xtime_ = NTime::duration(0);
  movesLeft_ = 0;
  movesToGo_ = 0;
  timePerMove_ = NTime::duration(0);
  if(depth < 2)
    depth = 2;
  maxDepth_ = depth;
  engine_.setMaxDepth(depth);
  engine_.setTimeLimit(timePerMove_);
  return true;
}

bool Processor::setTimePerMove(NTime::duration const& tm)
{
  if(is_thinking())
    return false;

  maxDepth_ = -1;
  xtime_ = NTime::duration(0);
  movesLeft_ = 0;
  movesToGo_ = 0;
  timePerMove_ = std::max(tm, minimalTimePerMove_);
  engine_.setTimeLimit(timePerMove_);
  engine_.setMaxDepth(DepthMaximum);
  return true;
}

bool Processor::setXtime(NTime::duration const& xtm)
{
  if(is_thinking())
    return false;

  maxDepth_ = -1;
  xtime_ = std::max(xtm, minimalTimePerMove_);
  timePerMove_ = NTime::duration(0);
  engine_.setMaxDepth(DepthMaximum);
  return true;
}

bool Processor::setMovesLeft(int mleft)
{
  if(is_thinking())
    return false;

  maxDepth_ = -1;
  movesLeft_ = mleft;
  movesToGo_ = 0;
  timePerMove_ = NTime::duration(0);
  engine_.setMaxDepth(DepthMaximum);
  return true;
}

bool Processor::setMovesToGo(int mtogo)
{
  if(is_thinking())
    return false;

  maxDepth_ = -1;
  movesLeft_ = 0;
  movesToGo_ = mtogo;
  timePerMove_ = NTime::duration(0);
  engine_.setMaxDepth(DepthMaximum);
  return true;
}


NTime::duration Processor::giveMoreTime()
{
  if(timePerMove_ > NTime::duration(0) || xtime_ <= minimalXtimeToGive_ || givetimeCounter_ > 0)
    return NTime::duration(0);

  NTime::duration add_t(0);
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
    return NTime::duration(0);

  add_t = xtime_ / mcount;
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

void Processor::enableBook(int)
{
}

bool Processor::undo()
{
  if(is_thinking())
  {
    engine_.pleaseStop();
    return false;
  }

  auto& board = engine_.getBoard();
  if(board.halfmovesCount() > 0)
  {
    board.unmakeMove();
    return true;
  }

  return false;
}

void Processor::setOptions(NEngine::xOptions const& opts)
{
  if(is_thinking())
    return;

  engine_.setOptions(opts);
}

NEngine::Figure::Color Processor::color() const
{
  return engine_.getBoard().getColor();
}

bool Processor::init()
{
  if(is_thinking())
  {
    engine_.pleaseStop();
    return true;
  }

  engine_.clearHash();
  return engine_.fromFEN("");
}

bool Processor::analyze()
{
  if(is_thinking())
    return false;

  thinking_ = true;
  engine_.setTimeLimit(NTime::duration(0));
  engine_.setMaxDepth(DepthMaximum);

  NEngine::SearchResult sres;

  engine_.setAnalyzeMode(true);
  engine_.search(sres);
  engine_.setAnalyzeMode(false);

  engine_.setTimeLimit(timePerMove_);
  engine_.setMaxDepth(maxDepth_ < 0 ? DepthMaximum : maxDepth_);
  thinking_ = false;
  return true;
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
  if(engine_.search(sres))
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
  rep.best_ = sres.best_;
  rep.moveStr_ = moveToStr(sres.best_, winboardFormat);
  return rep;
}

boost::optional<ReplyStruct> Processor::move(xCmd const& moveCmd)
{
  if(is_thinking())
  {
    return boost::none;
  }

  if(moveCmd.type() != xType::xMove)
  {
    return boost::none;
  }

  ReplyStruct rep;
  auto& board = engine_.getBoard();
  auto color = board.getColor();
  rep.state_ = static_cast<NEngine::Board::State>(board.getState());
  rep.white_ = NEngine::Figure::ColorWhite == color;

  if(board.drawState() || board.matState())
    return rep;

  auto move = strToMove(moveCmd.str(), board);
  if(!move)
  {
    return boost::none;
  }

  if(!board.validateMove(move))
  {
    return boost::none;
  }

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

  if(!board.validateMove(move))
    return false;

  board.makeMove(move);
  board.verifyState();
  updateTiming();

  return true;
}

void Processor::adjustEval(std::set<std::string> const& exclude,
                           std::vector<NEngine::details::Which> const& which, 
                           double percent)
{
  engine_.adjustEval(exclude, which, percent);
}

void Processor::saveEval(std::string const& fname)
{
  engine_.saveEval(fname);
}

void Processor::save(std::string const& fname)
{
  if(is_thinking())
    return;

  if(fname.empty())
    pgn2file("game_001.pgn");
  else
    pgn2file(fname);
}

void Processor::load(std::string const& fname)
{
  if(is_thinking())
    return;

  try
  {
    std::ifstream ifs(fname.empty() ? "game_001.pgn" : fname, std::ios::in);
    if(!ifs)
      return;
    NEngine::load(engine_.getBoard(), ifs);
  }
  catch(...)
  {
  }
}

void Processor::fen2file(std::string const& fname)
{
  if(is_thinking())
    return;

  try
  {
    std::ofstream ofs(fname);
    if(!ofs)
      return;
    auto fen = NEngine::toFEN(engine_.getBoard());
    ofs << fen << std::endl;
  }
  catch(...)
  {
  }
}

void Processor::pgn2file(std::string const& fname)
{
  try
  {
    std::ofstream ofs(fname);
    if(!ofs)
      return;
    NEngine::save(engine_.getBoard(), ofs);
  }
  catch(...)
  {
  }
}

void Processor::hash2file(std::string const& fname)
{
  if(is_thinking())
    return;

  engine_.saveHash(fname);
}

//////////////////////////////////////////////////////////////////////////
void Processor::clear()
{
  engine_.clearHash();
}

boost::optional<bool> Processor::fromFEN(xCmd const& cmd)
{
  if(is_thinking())
  {
    engine_.pleaseStop();
    return false;
  }
  if(fromFEN(cmd.fen()))
    return true;
  return boost::none;
}

bool Processor::fromFEN(std::string const& fen)
{
  return engine_.fromFEN(fen);
}

void Processor::setBoard(NEngine::Board& board)
{
  engine_.setBoard(board);
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
      engine_.needUpdate();
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

  std::string str = cmd.str();
  boost::algorithm::to_lower(str);

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
    if('a' == str[1] || 'h' == str[1])
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

  if(maxDepth_ > 0)
  {
    engine_.setTimeLimit(NTime::duration(0));
    return;
  }

  if(timePerMove_ > NTime::duration(0))
  {
    engine_.setTimeLimit(timePerMove_);
    return;
  }

  if(movesLeft_ <= 0 && movesToGo_ <= 0 && xtime_ > NTime::duration(0))
  {
    engine_.setTimeLimit(xtime_ / 40);
    return;
  }

  int mcount = 2;

  if(movesLeft_ > 0)
    mcount = movesLeft_ - (engine_.getBoard().movesCount()-1) % movesLeft_;
  else if(movesToGo_ > 0)
    mcount = movesToGo_;

  if(mcount < 2)
    mcount = 2;

  engine_.setTimeLimit(xtime_ / mcount);
}

} // NShallow
