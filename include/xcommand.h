#pragma once

/*************************************************************
  xcommand.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <vector>
#include <string>
#include <map>

namespace NShallow
{

  enum class xType {
    // x-board protocol commands
    xNone,
    xOption,
    xPing,
    xBoard,
    xMemory,
    xNew,
    xMove,
    xProtover,
    xGo,
    xSt,
    xSd,
    xUndo,
    xRemove,
    xForce,
    xEdit,
    xPost,
    xNopost,
    xAnalyze,
    xExit,
    xTime,
    xOtime,
    xLevel,
    xLeaveEdit,
    xGoNow,
    xChgColor,
    xClearBoard,
    xSetFigure,
    xSaveBoard,
    xLoadBoard,
    xSetboardFEN,

    // uci protocol commands
    UCI,
    SetOption,
    IsReady,
    UCInewgame,
    Position,
    UCIgo,

    // common
    xQuit,
  };

  class xCmd
  {
  public:
    xCmd(xType type = xType::xNone);
    xCmd(xType type, std::string&& str);
    xCmd(xType type, std::vector<std::string>&& moves);
    xCmd(xType type, std::string&& fen, std::vector<std::string>&& moves);
    xCmd(xType type, int value);
    xCmd(xType type, std::map<std::string, int>&& params);
    xCmd(xType type, bool inf);

    xType type() const;    
    operator bool() const;
    std::string const& str() const;
    std::string const& fen() const;
    std::vector<std::string> const& moves() const;
    int value() const;
    bool infinite() const;
    int param(std::string const& name) const;
    std::string param(size_t i) const;

  private:
    xType type_{xType::xNone};
    std::string str_;
    std::vector<std::string> moves_;
    std::map<std::string, int> params_;
    int value_{0};
    bool infinite_{false};
  };

} // NShallow
