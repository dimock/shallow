/*************************************************************
xprotocol.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <xparser.h>
#include <processor.h>
#include <xinput.h>

namespace NShallow
{

class xProtocolMgr
{
public:

  xProtocolMgr();

  bool doCmd();

  void printPV(NEngine::SearchResult const& sres);
  void printStat(NEngine::SearchData const& sdata);
  void printBM(NEngine::SearchResult const& sres);

  // uci
  void printInfo(NEngine::SearchResult const& sres);
  void printUciStat(NEngine::SearchData const& sdata);

private:

  void processCmd(xCmd const& cmd);
  void uciSetOption(const xCmd & cmd);
  void uciPositionFEN(const xCmd & cmd);
  void uciPositionMoves(const xCmd & cmd);
  void uciGo(const xCmd & cmd);
  void outState(NEngine::Board::State state, bool white);

  Processor proc_;
  xInput    input_;
 
  bool stop_  = false;
  bool force_ = false;
  bool fenOk_ = false;
  bool isUci_ = false;

  std::ostream& os_;
};

} // NShallow
