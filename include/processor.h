/*************************************************************
  processor.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <engine.h>
#include <xcommand.h>
#include <xtime.h>
#include <boost/optional.hpp>

#undef  WRITE_LOG_FILE_
#undef WRITE_ERROR_PGN

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
  bool init();
  void enableBook(int v);
  void setMemory(int mb);
  bool setDepth(int depth);
  bool setTimePerMove(NTime::duration const& tm);
  bool setXtime(NTime::duration const& xtm);
  bool setMovesLeft(int mleft);
  bool setMovesToGo(int mtogo);
  bool setPost(bool);
  bool undo();

  NEngine::Figure::Color color() const;

  void save(std::string const& fname);
  void load(std::string const& fname);
  void fen2file(std::string const& fname);
  void pgn2file(std::string const& fname);
  void hash2file(std::string const& fname);
  std::string toFEN();

  bool fromFEN(std::string const& fen);
  boost::optional<bool> fromFEN(xCmd const& cmd);
  void editCmd(xCmd const& cmd);

  boost::optional<ReplyStruct> move(xCmd const& moveCmd);
  bool makeMove(std::string const& moveStr);
  boost::optional<ReplyStruct> reply(bool winboardFormat);
  bool analyze();
  void stop();

  void setCallback(NEngine::xCallback& xcbk);

  NTime::duration giveMoreTime();

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
  int maxDepth_{};
  NTime::duration xtime_{};
  NTime::duration timePerMove_{};
  bool post_{};
  bool thinking_{};
  unsigned int givetimeCounter_{};
};

} // NShallow