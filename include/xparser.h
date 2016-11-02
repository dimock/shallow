#pragma once

/*************************************************************
  xparser.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <string>
#include <xcommand.h>

namespace NShallow
{
	xCmd parse(std::string const& line, bool const uci);
} // NParser
