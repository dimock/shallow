#pragma once

/*************************************************************
  xcommands.h - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <vector>
#include <string>
#include <stdio.h>

class xCmd
{
public:

	enum {
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
    UCIgo,

    // common
    xQuit,
	};

	xCmd(int type = xNone) :
		type_(type)
	{}

	xCmd(int type, const char * str) :
		type_(type), str_(str)
	{}
	
	xCmd(int type, const std::vector<std::string> & params) :
		type_(type), params_(params)
	{}

	int type() const { return type_; }
	size_t paramsNum() const { return params_.size(); }
	const std::string & param(size_t i) const { return params_.at(i); }

	operator bool () const
	{
		return type_ != xNone;
	}

	int asInt(size_t i) const
	{
		if ( params_.size() <= i )
			return 0;

		int n;
		if ( sscanf(params_.at(i).c_str(), "%d", &n) == 1 )
			return n;

		return 0;
	}

  int getOption(const char * oname) const
  {
    if ( !oname )
      return -1;

    for (size_t i = 0; i < params_.size(); ++i)
    {
      std::string option = params_[i];
      size_t n = option.find(oname);
      if ( n == std::string::npos )
        continue;
      n = option.find('=');
      if ( n == std::string::npos )
        continue;
      option.erase(0, n+1);
      int v;
      if ( sscanf(option.c_str(), "%d", &v) == 1 )
        return v;
    }
    return -1;
  }

	const char * str() const
	{
		return str_.c_str();
	}

  std::string packParams(size_t from = 0) const
  {
    std::string res;
    for (size_t i = from; i < params_.size(); ++i)
    {
      res += params_.at(i);
      res += " ";
    }
    return res;
  }

private:

	int type_;
	std::string str_;
	std::vector<std::string> params_;
};