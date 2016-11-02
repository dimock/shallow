/*************************************************************
  processor.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <engine.h>
#include <xcommand.h>
#include <Windows.h>
#include <cstdint>
#include <boost/optional.hpp>

#undef  WRITE_LOG_FILE_
#define WRITE_ERROR_PGN

namespace NShallow
{

struct ReplyStruct
{
  NEngine::Board::State state_{};
  std::string moveStr_;
  bool white_{ false };
};

class Processor
{
public:

  Processor();

  bool init();
  void enableBook(int v);
  void setMemory(int mb);
  void setDepth(int depth);
  void setTimePerMove(int ms);
  void setXtime(int ms);
  void setMovesLeft(int mleft);
  void setMovesToGo(int mtogo);
  void setPost(bool);
  bool undo();

  NEngine::Figure::Color color() const;

  void save();
  void fen2file(const char * fname);
  void pgn2file(const char * fname);
  void hash2file(const char * fname);
  std::string toFEN();

  bool fromFEN(std::string const& fen);
  bool fromFEN(xCmd const& cmd);
  void editCmd(xCmd const& cmd);

  boost::optional<ReplyStruct> move(xCmd const& moveCmd);
  bool makeMove(std::string const& moveStr);
  boost::optional<ReplyStruct> reply(bool winboardFormat);
  void analyze();
  void stop();

  void setCallback(NEngine::xCallback& xcbk);

  int giveMoreTime();

  bool is_thinking() const { return thinking_; }

#ifdef WRITE_LOG_FILE_
  void set_logfile(std::ofstream * ofslog);
#endif

private:

  void performAnalyze();
  void updateTiming();
  void setFigure(xCmd const& cmd);

  NEngine::Figure::Color boardColor_{};
  NEngine::Figure::Color figureColor_{};
  NEngine::Board board_;
  NEngine::Engine engine_;
  int movesLeft_{};
  int movesToGo_{};
  int xtimeMS_{};
  int maxDepth_{};
  int timePerMoveMS_{};
  bool post_{};
  bool thinking_{};
  unsigned int givetimeCounter_{};
};

} // NShallow