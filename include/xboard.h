#pragma once

/*************************************************************
  xboard.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "xparser.h"
#include "Thinking.h"

class xBoardMgr
{
public:

  xBoardMgr();
  ~xBoardMgr();

  void setProtocol(bool uci);
  bool do_cmd();
  void write_error(const std::exception * e = 0);

  bool peekInput();
  //void printPV(SearchResult * sres);
  //void printStat(SearchData * sdata);
  //void printBM(SearchResult * sres);

  // uci
  //void printInfo(SearchResult * sres);
  //void printUciStat(SearchData * sdata);

private:

  void out_state(std::ostream & os, uint8_t state, bool white);

private:

  void read_cmd(xCmd & cmd);
  void process_cmd(xCmd & cmd);

  void uciSetOption(const xCmd & cmd);
  void uciPosition(const xCmd & cmd);
  void uciGo(const xCmd & cmd);

  Thinking thk_;
  xParser parser_;

  int  vNum_;
  bool stop_;
  bool force_;
  bool fenOk_;

  bool uci_protocol_;

  std::ostream & os_;

  HANDLE hinput_;
  bool   in_pipe_;

};