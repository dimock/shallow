/*************************************************************
xprotocol.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <xparser.h>
#include <processor.h>
#include <xcmdqueue.h>

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
  bool swallowCmd(xCmd const& cmd);

  void uciOutputOptions();
  void setOption(const xCmd & cmd);
  void uciPosition(const xCmd & cmd);
  bool uciGo(const xCmd & cmd);
  void outState(NEngine::Board::State state, bool white);

  void printCmdDbg(xCmd const& cmd) const;

  Processor proc_;
  xCmdQueue cmds_;
 
  bool stop_  = false;
  bool force_ = false;
  bool fenOk_ = true;

  std::ostream& os_;
  std::unique_ptr<std::ofstream> ofs_log_;
};

} // NShallow
