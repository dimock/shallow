/*************************************************************
xcmdqueue.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include "queue"
#include "xcommand.h"
#include "xinput.h"

namespace NShallow
{

class xCmdQueue
{
  std::queue<xCmd> commands_;
  xInput input_;
  bool uci_{false};

public:
  void setUci(bool uci);
  bool isUci() const;

  void push(xCmd const& cmd);
  xCmd peek();
  xCmd next();
};

} // NShallow
