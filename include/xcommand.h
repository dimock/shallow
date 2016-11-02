#pragma once

/*************************************************************
  xcommand.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <vector>
#include <string>

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
    xSetboardFEN,

    // uci protocol commands
    UCI,
    SetOption,
    IsReady,
    UCInewgame,
    Position,
    PositionMoves,
    PositionFEN,
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
    xCmd(xType type, int value);
    xCmd(xType type, int mt, int mtogo, int bt, int wt, int d);
    xCmd(xType type, bool inf);

    xType type() const;    
    operator bool() const;
    std::string const& str() const;
    std::string const& fen() const;
    std::vector<std::string> const& moves() const;
    int value() const;
    int btime() const;
    int wtime() const;
    int movestogo() const;
    int movetime() const;
    int depth() const;
    bool infinite() const;

  private:
    xType type_{xType::xNone};
    std::string str_;
    std::vector<std::string> moves_;
    int value_{};
    int movetime_{};
    int movestogo_{};
    int btime_{};
    int wtime_{};
    int depth_{};
    bool infinite_{};
  };

} // NShallow
