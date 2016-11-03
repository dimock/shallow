
/*************************************************************
  xparser.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <xparser.h>
#include <Helpers.h>
#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <boost/algorithm/string.hpp>

namespace NShallow
{

namespace
{
  int toInt(std::vector<std::string> const& params, size_t i)
  {
    if(params.size() <= i)
      return 0;
    std::istringstream iss(params[i]);
    int value{};
    if(iss >> value)
      return value;
    return 0;
  }

  xCmd parseSetFigure(std::string str)
  {
    if(str.size() == 3
      && std::isalpha(str[0], std::locale{})
      && std::isalpha(str[1], std::locale{})
      && std::isdigit(str[2], std::locale{}))
    {
      return xCmd(xType::xSetFigure, std::move(str));
    }

    return {};
  }

  xCmd parseMove(std::string str)
  {
    if(NEngine::detectNotation(str) != NEngine::eMoveNotation::mnUnknown)
    	return xCmd(xType::xMove, std::move(str));

    return {};
  }

  xCmd fromMoves(std::vector<std::string>&& params)
  {
    auto start = params.begin();
    if(start == params.end())
      return{};

    std::string fen;
    if(*start == "startpos")
      ++start;
    else if(*start == "fen")
    {
      ++start;
      if(start == params.end())
        return{};

      auto iter = std::find_if(start, params.end(), [](std::string const& str) { return str == "moves";  });
      fen = boost::algorithm::join(boost::make_iterator_range(start, iter), " ");
      start = iter;
    }

    if(start != params.end())
    {
      start++;
      if(std::find_if(start, params.end(), [](std::string const& str) { return !parseMove(str); }) != params.end())
        return{};
    }
    params.erase(params.begin(), start);

    return xCmd(xType::Position, std::move(fen), std::move(params));
  }

  xCmd fromFEN(std::vector<std::string>&& params)
  {
    if(params.size() < 2)
      return{};

    return xCmd(xType::xSetboardFEN, boost::algorithm::join(params, " "));
  }

  xCmd parseUCIGo(std::vector<std::string> const& params)
  {
    std::map<std::string, int> pmap;

    for(size_t i = 0; i < params.size(); ++i)
    {
      auto const& prm = params[i];
      if(prm == "infinite")
      {
        return xCmd{ xType::UCIgo, true };
      }

      if(i+1 >= params.size())
        break;

      int j = i;
      pmap.emplace(params[j], toInt(params, ++i));
    }

    return xCmd(xType::UCIgo, std::move(pmap));
  }

} // namespace {}

xCmd parse(std::string const& line, bool const uci)
{
	if(line.empty())
    return {};

  static std::map<std::string const, xType> const xcommands {
    { "xboard",     xType::xBoard },
    { "option",     xType::xOption },
    { "ping",       xType::xPing },
    { "new",        xType::xNew },
    { "go",         xType::xGo },
    { "undo",       xType::xUndo},
    { "remove",     xType::xRemove },
    { "force",      xType::xForce },
    { "st",         xType::xSt },
    { "sd",         xType::xSd },
    { "post",       xType::xPost },
    { "nopost",     xType::xNopost },
    { "analyze",    xType::xAnalyze },
    { "exit",       xType::xExit },
    { "time",       xType::xTime },
    { "otim",       xType::xOtime },
    { "level",      xType::xLevel },
    { "memory",     xType::xMemory },
    { "saveboard",  xType::xSaveBoard },
    { "edit",       xType::xEdit },
    { "#",          xType::xClearBoard },
    { "c",          xType::xChgColor },
    { ".",          xType::xLeaveEdit }, 
    { "?",          xType::xGoNow },
    { "protover",   xType::xProtover },
    { "setboard",   xType::xSetboardFEN },
    { "setb",       xType::xSetboardFEN },
    { "quit",       xType::xQuit },

    // uci commands
    { "uci",        xType::UCI },
    { "setoption",  xType::SetOption },
    { "isready",    xType::IsReady },
    { "ucinewgame", xType::UCInewgame },
    { "position",   xType::Position },
    { "stop",       xType::xExit },
  };

  if(auto cmd = parseSetFigure(line))
  {
    return cmd;
  }

  if(auto cmd = parseMove(line))
  {
    return cmd;
  }

  std::vector<std::string> params;
  boost::algorithm::split(params, line, boost::algorithm::is_any_of(" \t\n\r"), boost::algorithm::token_compress_on);
  if(params.empty())
  {
    return {};
  }

  auto iter = xcommands.find(params[0]);
  if(iter == xcommands.end())
  {
    return {};
  }

  params.erase(params.begin());
  auto type = iter->second;
  if(uci && type == xType::xGo)
  {
    type = xType::UCIgo;
  }

  // UCI position -> FEN/Moves list
  if(type == xType::Position)
  {
    if(auto cmd = fromMoves(std::move(params)))
      return cmd;

    return {};
  }

  if(type == xType::xSetboardFEN)
  {
    if(auto cmd = fromFEN(std::move(params)))
      return cmd;

    return {};
  }

  switch(type)
  {
  case xType::xSt:
  case xType::xSd:
  case xType::xLevel:
  case xType::xMemory:
  case xType::xPing:
  case xType::xTime:
  case xType::xOtime:
    {
      if(params.size() > 1)
      {
        return xCmd(type, toInt(params, 1));
      }
      else
      {
        return {};
      }
    }
    break;

  case xType::SetOption:
    {
      if(params.size() < 4)
        return {};

      if(params[0] == "name" && params[1] == "Hash" && params[2] == "value")
        return xCmd(type, toInt(params, 3));
    }
    break;

  case xType::UCIgo:
    return parseUCIGo(params);

  default:
    return xCmd(type, std::move(params));
  }

  return {};
}

} // NShallow