
/*************************************************************
  xparser.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include "xparser.h"
//#include "Helpers.h"

#include <iostream>
#include <string.h>

using namespace std;

xParser::xParser()
{

}

bool xParser::split_str(char * str, std::vector<std::string> & result)
{
	if ( !str || strlen(str) == 0 )
		return false;

	const char * sepr = " \t\n\r";
	char * s = strtok(str, sepr);
	for ( ; s; )
	{
		result.push_back(s);
		s = strtok(0, sepr);
	}

	return !result.empty();
}

struct CommandLine
{
  const char * commandName_;
  int commandType_;
};

xCmd xParser::parse(char * str, bool uci)
{
	if ( !str || strlen(str) == 0 )
		return xCmd();

  static CommandLine command_lines [] = {
    { "xboard", xCmd::xBoard },
    { "option", xCmd::xOption },
    { "ping", xCmd::xPing },
    { "new", xCmd::xNew },
    { "go", xCmd::xGo },
    { "undo", xCmd::xUndo},
    { "remove", xCmd::xRemove },
    { "force", xCmd::xForce },
    { "st", xCmd::xSt },
    { "sd", xCmd::xSd },
    { "post", xCmd::xPost },
    { "nopost", xCmd::xNopost },
    { "analyze", xCmd::xAnalyze },
    { "exit", xCmd::xExit },
    { "time", xCmd::xTime },
    { "otim", xCmd::xOtime },
    { "level", xCmd::xLevel },
    { "memory", xCmd::xMemory },
    { "saveboard", xCmd::xSaveBoard },
    { "edit", xCmd::xEdit },
    { "#", xCmd::xClearBoard },
    { "c", xCmd::xChgColor },
    { ".", xCmd::xLeaveEdit }, 
    { "?", xCmd::xGoNow },
    { "protover", xCmd::xProtover },
    { "setboard", xCmd::xSetboardFEN },
    { "quit", xCmd::xQuit },

    // uci commands
    { "uci", xCmd::UCI },
    { "setoption", xCmd::SetOption },
    { "isready", xCmd::IsReady },
    { "ucinewgame", xCmd::UCInewgame },
    { "position", xCmd::Position },
    { "stop", xCmd::xExit },
  };

  //_strlwr(str);

  vector<string> params;
  split_str(str, params);

  if ( params.empty() )
    return xCmd();

  if ( strlen(str) == 3 && isalpha(str[0]) && isalpha(str[1]) && isdigit(str[2]) )
    return xCmd(xCmd::xSetFigure, str);

	xCmd moveCmd = parseMove(str);
	if ( moveCmd )
		return moveCmd;

  for (int i = 0; i < sizeof(command_lines)/sizeof(CommandLine); ++i)
  {
    const CommandLine & cmdLine = command_lines[i];

    if ( string(cmdLine.commandName_) == params[0] )
    {
      if ( params.size() )
        params.erase(params.begin());

      if ( uci )
      {
        switch ( cmdLine.commandType_ )
        {
        case xCmd::xGo:
          return xCmd(xCmd::UCIgo, params);
        }
      }

      return xCmd(cmdLine.commandType_, params);
    }
  }

  return xCmd();
}

xCmd xParser::parseMove(const char * str)
{
	if ( !str )
		return xCmd();

	//if ( detectNotation(str) != mnUnknown )
	//	return xCmd(xCmd::xMove, str);

	return xCmd();
}
