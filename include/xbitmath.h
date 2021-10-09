
/*************************************************************
xbitmath.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include "fpos.h"

namespace NEngine
{

template <class T>
inline int sign(T v)
{
  return (T{0} < v) - (v < T{0});
}

inline int change_sign(bool cond, int value)
{
  return (value ^ -(int)(cond)) + cond;
}
    
typedef int(*FUNC_POP_COUNT64)(BitMask);
extern FUNC_POP_COUNT64 g_func_pop_count64;

void init_popcount_ptr();

inline int pop_count(BitMask n)
{
  return g_func_pop_count64(n);
}

#ifdef _MSC_VER

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)

inline int _lsb32(unsigned long n)
{
  X_ASSERT(!n, "zero mask in _lsb32");
  unsigned long i;
  uint8 b = _BitScanForward(&i, n);
  X_ASSERT(!b, "no bit found in nonzero number");
  return i;
}

inline int _msb32(unsigned long n)
{
  X_ASSERT(!n, "zero mask in _msb32");
  unsigned long i;
  uint8 b = _BitScanReverse(&i, n);
  X_ASSERT(!b, "no bit found in nonzero number");
  return i;
}


#ifdef _M_X64
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

inline int _lsb64(const BitMask mask)
{
  X_ASSERT(!mask, "zero mask in _lsb64");
  unsigned long n;
  uint8 b = _BitScanForward64(&n, mask);
  X_ASSERT(!b, "no bit found in nonzero number");
  return n;
}

inline int _msb64(const BitMask mask)
{
  X_ASSERT(!mask, "zero mask in _msb64");
  unsigned long n;
  uint8 b = _BitScanReverse64(&n, mask);
  X_ASSERT(!b, "no bit found in nonzero number");
  return n;
}

inline int log2(BitMask n)
{
  unsigned long i = 0;
  if(_BitScanReverse64(&i, n))
    return i;
  return 0;
}
#else
inline int _lsb64(const BitMask mask)
{
  X_ASSERT(!mask, "zero mask in _lsb64");
  unsigned long n;
  const unsigned * pmask = reinterpret_cast<const unsigned int *>(&mask);

  if(_BitScanForward(&n, pmask[0]))
    return n;

  uint8 b = _BitScanForward(&n, pmask[1]);
  X_ASSERT(!b, "no bit found in nonzero number");
  return n+32;
}

inline int _msb64(const BitMask mask)
{
  X_ASSERT(!mask, "zero mask in _msb64");
  unsigned long n;
  const unsigned * pmask = reinterpret_cast<const unsigned int * >(&mask);

  if(_BitScanReverse(&n, pmask[1]))
    return n+32;

  uint8 b = _BitScanReverse(&n, pmask[0]);
  X_ASSERT(!b, "no bit found in nonzero number");
  return n;
}

inline int log2(BitMask n)
{
  unsigned long i = 0;
  const unsigned * pn = reinterpret_cast<const unsigned int *>(&n);

  if(_BitScanReverse(&i, pn[1]))
    return i+32;

  if(_BitScanReverse(&i, pn[0]))
    return i;

  return 0;
}
#endif // _M_X64

#elif (defined __GNUC__)

inline int _lsb32(uint32 mask)
{
  X_ASSERT(!mask, "zero mask in _lsb32");
  return __builtin_ctz(mask);
}

inline int _msb32(uint32 mask)
{
  X_ASSERT(!mask, "zero mask in _msb32");
  return 31 - __builtin_clz(mask);
}

inline int _lsb64(BitMask mask)
{
  X_ASSERT(!mask, "zero mask in _lsb64");
  return __builtin_ctzll(mask);
}

inline int _msb64(BitMask mask)
{
  X_ASSERT(!mask, "zero mask in _msb64");
  return 63 - __builtin_clzll(mask);
}

inline int log2(BitMask n)
{
  return 63 - __builtin_clzll(n);
}

#endif // __GNUC__

void print_bitmask(BitMask mask);

// got from chessprogramming.wikispaces.com
inline bool one_bit_set(BitMask n)
{
  return (n & (n-1)) == 0ULL;
}

inline BitMask set_mask_bit(int bit)
{
  return 1ULL << bit;
}

inline int set_bit(int bit)
{
  return 1 << bit;
}

inline int clear_lsb(BitMask& mask)
{
  unsigned long n = _lsb64(mask);
  mask &= mask-1;
  return n;
}

inline int clear_msb(BitMask& mask)
{
  unsigned long n = _msb64(mask);
  mask ^= set_mask_bit(n);
  return n;
}

inline unsigned mul_div(unsigned v, unsigned n, unsigned d)
{
  uint64 r = (uint64)v * n;
  r >>= log2((uint64)(d)+(uint64)(n));
  unsigned x = *((unsigned*)&r);
  return x;
}

class PawnMasks
{
public:

  PawnMasks();

  inline const BitMask mask_passed(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmasks_passed_[color][pos];
  }

  inline const BitMask mask_forward(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmasks_forward_[color][pos];
  }

  inline const BitMask mask_neighbor(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmasks_neighbor_[color][pos];
  }

  inline const BitMask mask_guards(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmasks_guards_[color][pos];
  }

  inline const BitMask mask_backward(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmasks_backward_[color][pos];
  }

  inline const BitMask mask_isolated(int x) const
  {
    X_ASSERT((unsigned)x > 7, "invalid pawn x");
    return pmask_isolated_[x];
  }

  inline const BitMask mask_column(int x) const
  {
    X_ASSERT((unsigned)x > 7, "invalid pawn x");
    return pmask_column_[x];
  }

  inline const BitMask mask_row(int n) const
  {
    X_ASSERT((unsigned)n > 63, "invalid position");
    return pmask_row_[n];
  }

  inline const BitMask blocked_knight(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmask_blocked_knight_[color][pos];
  }

  inline const BitMask blocked_bishop(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmask_blocked_bishop_[color][pos];
  }

private:
  BitMask pmask_blocked_knight_[2][64] = {};
  BitMask pmask_blocked_bishop_[2][64] = {};
  BitMask pmasks_passed_[2][64] = {};
  BitMask pmasks_forward_[2][64] = {};
  BitMask pmasks_neighbor_[2][64] = {};
  BitMask pmasks_guards_[2][64] = {};
  BitMask pmasks_backward_[2][64] = {};
  BitMask pmask_isolated_[8] = {};
  BitMask pmask_column_[8] = {};
  BitMask pmask_row_[64] = {};
};

class BitsCounter
{
  static int8 s_array_[256];

public:

  static inline int8 numBitsInByte(uint8 byte)
  {
    return s_array_[byte];
  }

  static inline int8 numBitsInWord(uint16 word)
  {
    return numBitsInByte(word&0xff) + numBitsInByte(word>>8);
  }
};

class DeltaPosCounter
{
  FPos s_array_[4096];

  FPos deltaP(FPos dp) const
  {
    int x = dp.x();
    int y = dp.y();

    if(x < 0)
      x = -x;
    if(y < 0)
      y = -y;

    if(x != 0 && y != 0 && x != y)
      return FPos();

    int xx = dp.x(), yy = dp.y();
    if(xx != 0)
      xx /= x;
    if(yy != 0)
      yy /= y;

    return FPos(xx, yy);
  }

public:

  DeltaPosCounter();

  /// returns { 0, 0 }  if (to - from) isn't horizontal, vertical or diagonal
  inline const FPos & getDeltaPos(int to, int from) const
  {
    X_ASSERT(to < 0 || to > 63 || from < 0 || from > 63, "invalid points given");
    return s_array_[(to<<6) | from];
  }
};


class BetweenMask
{
  // masks between two fields
  BitMask s_between_[64][64];

  // mask 'from' field in direction 'to'. until border
  BitMask s_from_[64][64];

  // mask from field in given direction up to the border
  BitMask s_from_dir_[8][64];

  // mask from 'to' field in direction 'from->to'. until border
  BitMask s_tail_[64][64];

  // mask through fields 'from' and 'to' from border to border
  BitMask s_line_[64][64];

public:

  BetweenMask(DeltaPosCounter const&);

  // mask contains only bits BETWEEN from & to
  inline const BitMask between(int8 from, int8 to) const
  {
    X_ASSERT((uint8)from > 63 || (uint8)to > 63, "invalid positions given");
    return s_between_[from][to];
  }

  // mask contains bits along from-to direction starting from and finishing at border
  inline const BitMask from(int8 from, int8 to) const
  {
    X_ASSERT((uint8)from > 63 || (uint8)to > 63, "invalid positions given");
    return s_from_[from][to];
  }

  // mask contains bits along from-to direction starting from 'to' and finishing at border
  inline const BitMask tail(int8 from, int8 to) const
  {
    X_ASSERT((uint8)from > 63 || (uint8)to > 63, "invalid positions given");
    return s_tail_[from][to];
  }

  // mask contains bits of whole line along from-to direction
  inline const BitMask line(int8 from, int8 to) const
  {
    X_ASSERT((uint8)from > 63 || (uint8)to > 63, "invalid positions given");
    return s_line_[from][to];
  }

  // mask contains bits from square in dir direction until the border
  inline const BitMask from_dir(int8 from, nst::dirs dir) const
  {
    X_ASSERT((uint8)from > 63 || (uint8)dir > 8 || dir == nst::dirs::no_dir, "invalid positions given");
    return s_from_dir_[(uint8)dir-1][from];
  }
};


class DistanceCounter
{
  int s_array_[4096];

  int dist_dP(FPos dp) const
  {
    int x = dp.x();
    int y = dp.y();
    if(x < 0)
      x = -x;
    if(dp.y() < 0)
      y = -y;
    return std::max(x, y);
  }

public:

  DistanceCounter();

  // returns distance between 2 points - 'a' & 'b'
  inline int getDistance(int a, int b) const
  {
    X_ASSERT(a < 0 || a > 63 || b < 0 || b > 63, "invalid points given");
    return s_array_[(a<<6) | b];
  }
};

} // NEngine