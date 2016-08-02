#pragma once

/*************************************************************
  Thinking.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

//#include "Player.h"
#include "xcommands.h"
#include <Windows.h>
#include <cstdint>

#undef  WRITE_LOG_FILE_
#define WRITE_ERROR_PGN

class Thinking
{
public:

	Thinking();
	~Thinking();

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

//  Figure::Color color() const;

	void save();
  void fen2file(const char * fname);
  void pgn2file(const char * fname);
  void hash2file(const char * fname);
  void toFEN(char * );

  bool fromFEN(const char * fen);
  bool fromFEN(xCmd & cmd);
	void editCmd(xCmd & cmd);

	bool move(xCmd & moveCmd, uint8_t & state, bool & white);
  bool makeMove(const char * moveStr);
  bool reply(char(&)[256], uint8_t & state, bool & white, bool wb_move);
  void analyze();
  void stop();

  //void setPlayerCallbacks(CallbackStruct cs);
  //void clearPlayerCallbacks();
  
  int giveMoreTime();

  bool is_thinking() const { return thinking_; }

#ifdef WRITE_LOG_FILE_
  void set_logfile(std::ofstream * ofslog);
#endif

private:

  void performAnalyze();
  void updateTiming();
	void setFigure(xCmd & cmd);

	//Figure::Color boardColor_, figureColor_;
 // Board board_;
	//Player player_;
  int movesLeft_;
  int movesToGo_;
  int xtimeMS_;
  int maxDepth_;
  int timePerMoveMS_;
  bool post_;
  bool thinking_;
  unsigned int givetimeCounter_;
};