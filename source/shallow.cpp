
/*************************************************************
  shallow.cpp - Copyright (C) 2011 - 2012 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <string>
#include <fstream>
#include "xparser.h"
#include "xboard.h"

#include <windows.h>

using namespace std;

void main_loop(xBoardMgr & xbrd)
{
  cout.setf(ios_base::unitbuf);

  for ( ; xbrd.do_cmd(); );
}

int main(int argc, char * argv[])
{
  xBoardMgr xbrd;
  main_loop(xbrd);
	return 0;
}

