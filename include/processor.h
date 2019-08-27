/*************************************************************
  processor.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <engine.h>
#include <xcommand.h>
#include <xtime.h>

#undef  WRITE_LOG_FILE_
#undef WRITE_ERROR_PGN

namespace NShallow
{

struct ReplyStruct
{
  ReplyStruct(bool ok = false) : ok_{ ok }
  {}

  NEngine::StateType state_{};
  NEngine::Move best_;
  std::string moveStr_;
  bool white_{ false };
  ScoreType score_{};

  operator bool ()
  {
    return ok_;
  }
  bool ok_ = false;
};

class Processor
{
public:
  bool init();
  void enableBook(int v);
  void setOptions(NEngine::xOptions const& opts);
  bool setDepth(int depth);
  bool setScoreLimit(ScoreType score);
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
  void file2hash(std::string const& fname);
  std::string toFEN();

  void clear();

  void setBoard(NEngine::Board&);
  bool fromFEN(std::string const& fen);
  std::pair<bool, bool> fromFEN(xCmd const& cmd);
  void editCmd(xCmd const& cmd);

  ReplyStruct move(xCmd const& moveCmd);
  bool makeMove(std::string const& moveStr);
  ReplyStruct reply(bool winboardFormat);
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