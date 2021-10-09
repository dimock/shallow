
/*************************************************************
  xparser.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include "xparser.h"
#include "Helpers.h"
#include "iostream"
#include "string"
#include "sstream"
#include "map"
#include "locale"

namespace NShallow
{

namespace
{
  int toInt(std::vector<std::string> const& params, size_t i)
  {
    if(params.size() <= i)
      return 0;
    auto str = params[i];
    NEngine::to_lower(str);
    std::istringstream iss(str);
    int value{};
    if(iss >> value)
      return value;
    std::istringstream issb(str);
    bool bvalue{};
    if(issb >> std::boolalpha >> bvalue)
      return (int)bvalue;
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
      fen = NEngine::join(start, iter, " ");
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

    return xCmd(xType::xSetboardFEN, NEngine::join(params, " "));
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

      auto j = i;
      pmap.emplace(params[j], toInt(params, ++i));
    }

    return xCmd(xType::UCIgo, std::move(pmap));
  }

  xCmd parseOptions(std::vector<std::string> const& params)
  {
    std::map<std::string, int> pmap;
    for(size_t i = 0; i < params.size(); i += 4)
    {
      if(i+3 >= params.size())
        break;
      if(params[i] == "name" && params[i+2] == "value")
      {
        pmap.emplace(params[i+1], toInt(params, i+3));
      }
    }

    return xCmd(xType::SetOption, std::move(pmap));
  }

  xCmd parseXOptions(std::vector<std::string> const& params)
  {
    std::map<std::string, int> pmap;
    return xCmd(xType::xOption, std::move(pmap));
  }

} // namespace {}

xCmd parse(std::string const& line, bool const uci)
{
  if(line.empty())
    return {};

  static std::map<std::string, xType> const xcommands {
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
    { "loadboard",  xType::xLoadBoard },
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
    { "stop",       xType::xExit }
  };

  auto params = NEngine::split(line, [](char c) { return NEngine::is_any_of(" \t\n\r", c); });
  if(params.empty())
  {
    return {};
  }

  auto iter = xcommands.find(params[0]);
  if(iter == xcommands.end())
  {
    if(auto cmd = parseSetFigure(line))
    {
      return cmd;
    }

    if(auto cmd = parseMove(line))
    {
      return cmd;
    }

    return{};
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
      if(!params.empty())
      {
        return xCmd(type, toInt(params, 0));
      }
      else
      {
        return {};
      }
    }
    break;

  case xType::SetOption:
    {
      return parseOptions(params);
    }
    break;

  case xType::xOption:
    {
      return parseXOptions(params);
    }
    break;

  case xType::UCIgo:
    return parseUCIGo(params);

  default:
    return xCmd(type, std::move(params));
  }

  return {};
}

std::string to_string(xCmd const& cmd)
{
  static std::vector<std::pair<std::string, xType>> const xcommands {
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
    { "loadboard",  xType::xLoadBoard },
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
    { "go",         xType::UCIgo },

    { "quit",       xType::xQuit }
  };

  auto iter = std::find_if(xcommands.begin(), xcommands.end(), [cmd] (std::pair<std::string const, xType> p) { return p.second == cmd.type(); });
  if(iter == xcommands.end())
    return "command not found: " + std::to_string((int)cmd.type());
  std::string str = cmd.params_to_str();
  return iter->first + ": " + str;
}
 
} // NShallow
