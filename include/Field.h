/*************************************************************
  Field.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Figure.h>

namespace NEngine
{

class Board;

#pragma pack (push, 1)

ALIGN_MSC(1) class ALIGN_GCC(1) Field
{
public:
  Field() : color_(0), type_(0)
  {
  }

  inline operator bool () const { return type_ != 0; }
  inline Figure::Type  type() const { return (Figure::Type)type_; }
  inline Figure::Color color() const { return (Figure::Color)color_; }

  inline void set(const Figure::Color color, const Figure::Type type)
  {
    X_ASSERT( *this, "try to put figure to occupied field");

    color_ = color;
    type_  = type;
  }

  inline void clear()
  {
    val_ = 0;
  }

private:

  union
  {
    struct
    {
      uint8 color_ : 1,
            type_ : 3;
    };
    uint8 val_{};
  };
};

#pragma pack (pop)

} // NEngine