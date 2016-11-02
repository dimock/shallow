/*************************************************************
  shallow.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <iostream>
#include <xprotocol.h>

int main(int argc, char * argv[])
{
  std::cout.setf(std::ios_base::unitbuf);
  NShallow::xProtocolMgr xpr;
  for(; xpr.doCmd(););
	return 0;
}

