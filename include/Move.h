#pragma once

#include "xcommon.h"
#include "Figure.h"

namespace NEngine
{

struct Move
{
  uint16 mask;

  Move() = default;

  explicit Move(bool) : mask{0} {}

  Move(int f, int t, Figure::Type n)
  {
    mask = f | (t << 6) | (n << 12);
  }

  Move(int f, int t)
  {
    mask = f | (t << 6);
  }

  operator bool() const
  {
    return mask != 0;
  }

  int from() const { return mask & 63; }
  int to() const { return (mask >> 6) & 63; }
  int new_type() const { return (mask >> 12) & 7; }
  bool see_ok() const { return (mask >> 15) & 1; }
  void set_ok() { mask |= 1 << 15; }
  void clear_ok() { mask &= ~(1 << 15); }

  inline bool operator == (Move const m) const
  {
    return (mask & (~(1<<15))) == (m.mask & (~(1<<15)));
  }

  inline bool operator != (Move const m) const
  {
    return (mask & (~(1<<15))) != (m.mask & (~(1<<15)));
  }
};

struct SMove : public Move
{
  SortValueType  sort_value;

  SMove() = default;

  explicit SMove(bool) : Move(true), sort_value{0} {}

  SMove(int f, int t, Figure::Type n) :
    Move(f, t, n)
  {
  }

  SMove(int f, int t, Figure::Type n, SortValueType s) :
    Move(f, t, n),
    sort_value(s)
  {
  }

  SMove(int f, int t) :
    Move(f, t)
  {
  }

  SMove& operator = (Move const m)
  {
    (Move&)(*this) = m;
    return *this;
  }

  inline bool operator < (SMove const m) const
  {
    return sort_value < m.sort_value;
  }

  inline bool operator <= (SMove const m) const
  {
    return sort_value <= m.sort_value;
  }

  inline bool operator > (SMove const m) const
  {
    return sort_value > m.sort_value;
  }

  inline bool operator >= (SMove const m) const
  {
    return sort_value >= m.sort_value;
  }
};

} // NEngine