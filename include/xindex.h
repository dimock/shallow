/*************************************************************
xindex.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <xcommon.h>

namespace NEngine
{

#pragma pack (push, 1)
ALIGN_MSC(1) class ALIGN_GCC(1) Index
{
public:
  Index() : index_(-1) {}
  Index(int i) : index_(i) {}
  Index(int x, int y) { index_ = x | (y<<3); }
  Index(char x, char y) { index_ = (x-'a') | ((y-'1')<<3); }

  operator int() const
  {
    return index_;
  }

  operator int()
  {
    return index_;
  }

  int x() const
  {
    X_ASSERT(index_ < 0, "try to get x of invalid index");
    return index_&7;
  }

  int y() const
  {
    X_ASSERT(index_ < 0, "try to get y of invalid index");
    return (index_>>3)&7;
  }

  void set_x(int x)
  {
    index_ &= ~7;
    index_ |= x & 7;
  }

  void set_y(int y)
  {
    index_ &= 7;
    index_ |= (y&7)<<3;
  }

  int transp() const
  {
    return (x() << 3) | y();
  }

private:

  int8 index_;
};

#pragma pack (pop)

} // NEngine
