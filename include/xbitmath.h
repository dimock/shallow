
/*************************************************************
xbitmath.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <fpos.h>

namespace NEngine
{
    

#ifdef _MSC_VER

typedef int (*FUNC_POP_COUNT64)(uint64);
extern FUNC_POP_COUNT64 g_func_pop_count64;

void init_popcount_ptr();

inline int pop_count(uint64 n)
{
  return g_func_pop_count64(n);
}

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)

inline int _lsb32(unsigned long n)
{
  unsigned long i;
  uint8 b = _BitScanForward(&i, n);
  X_ASSERT(!b, "no bit found in nonzero number");
  return i;
}

inline int _msb32(unsigned long n)
{
  unsigned long i;
  uint8 b = _BitScanReverse(&i, n);
  X_ASSERT(!b, "no bit found in nonzero number");
  return i;
}


#ifdef _M_X64
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

inline int _lsb64(const uint64 & mask)
{
  unsigned long n;
  uint8 b = _BitScanForward64(&n, mask);
  X_ASSERT(!b, "no bit found in nonzero number");
  return n;
}

inline int _msb64(const uint64 & mask)
{
  unsigned long n;
  uint8 b = _BitScanReverse64(&n, mask);
  X_ASSERT(!b, "no bit found in nonzero number");
  return n;
}

inline int log2(uint64 n)
{
  unsigned long i = 0;
  if(_BitScanReverse64(&i, n))
    return i;
  return 0;
}
#else
inline int _lsb64(const uint64 & mask)
{
  unsigned long n;
  const unsigned * pmask = reinterpret_cast<const unsigned int *>(&mask);

  if(_BitScanForward(&n, pmask[0]))
    return n;

  uint8 b = _BitScanForward(&n, pmask[1]);
  X_ASSERT(!b, "no bit found in nonzero number");
  return n+32;
}

inline int _msb64(const uint64 & mask)
{
  unsigned long n;
  const unsigned * pmask = reinterpret_cast<const unsigned int * >(&mask);

  if(_BitScanReverse(&n, pmask[1]))
    return n+32;

  uint8 b = _BitScanReverse(&n, pmask[0]);
  X_ASSERT(!b, "no bit found in nonzero number");
  return n;
}

inline int log2(uint64 n)
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
  return __builtin_ctz(mask);
}

inline int _msb32(uint32 mask)
{
  return 31 - __builtin_clz(mask);
}

inline int _lsb64(uint64 mask)
{
  return __builtin_ctzll(mask);
}

inline int _msb64(uint64 mask)
{
  return 63 - __builtin_clzll(mask);
}

inline int log2(uint64 n)
{
  return 63 - __builtin_clzll(n);
}

inline int pop_count(uint64 n)
{
  return __builtin_popcountll(n);
}

#endif // __GNUC__

void print_bitmask(uint64 mask);

// got from chessprogramming.wikispaces.com
inline bool one_bit_set(uint64 n)
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

inline int clear_lsb(uint64 & mask)
{
  unsigned long n = _lsb64(mask);
  mask &= mask-1;
  return n;
}

inline int clear_msb(uint64 & mask)
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

  //inline const BitMask & mask_guarded(int color, int pos) const
  //{
  //  X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
  //  return pmasks_guarded_[color][pos];
  //}

  //inline const BitMask & mask_disconnected(int pos) const
  //{
  //  X_ASSERT((unsigned)pos > 63, "invalid pawn pos");
  //  return pmasks_disconnected_[pos];
  //}

  inline const BitMask & mask_passed(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmasks_passed_[color][pos];
  }

  inline const BitMask & mask_line_blocked(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmasks_line_blocked_[color][pos];
  }

  inline const BitMask & mask_backward(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
    return pmasks_backward_[color][pos];
  }

  inline const BitMask & mask_isolated(int x) const
  {
    X_ASSERT((unsigned)x > 7, "invalid pawn x or color");
    return pmask_isolated_[x];
  }

  inline const BitMask & mask_doubled(int x) const
  {
    X_ASSERT((unsigned)x > 7, "invalid pawn x or color");
    return pmask_doubled_[x];
  }

  //inline int8 pawn_dst_color(int color, int pos) const
  //{
  //  X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "invalid pawn pos or color");
  //  return pawn_dst_color_[color][pos];
  //}

  //inline const BitMask & mask_kpk(int color, int p) const
  //{
  //  X_ASSERT((unsigned)color > 1 || (unsigned)p > 63, "invalid pawn pos or color");
  //  return pmask_kpk_[color][p];
  //}

private:

  //void clearAll(int);

  //BitMask pmasks_guarded_[2][64];
  BitMask pmasks_passed_[2][64] = {};
  BitMask pmasks_line_blocked_[2][64] = {};
  BitMask pmasks_backward_[2][64] = {};
  //BitMask pmasks_disconnected_[64];
  BitMask pmask_isolated_[8] = {};
  BitMask pmask_doubled_[8] = {};
  //BitMask pmask_kpk_[2][64];
  //int8    pawn_dst_color_[2][64];
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

public:

  BetweenMask(DeltaPosCounter const&);

  // mask contains only bits BETWEEN from & to
  inline const BitMask & between(int8 from, int8 to) const
  {
    X_ASSERT((uint8)from > 63 || (uint8)to > 63, "invalid positions given");
    return s_between_[from][to];
  }

  // mask contains bits along from-to direction starting from and finishing at border
  inline const BitMask & from(int8 from, int8 to) const
  {
    X_ASSERT((uint8)from > 63 || (uint8)to > 63, "invalid positions given");
    return s_from_[from][to];
  }

  // mask contains bits from square in dir direction  up to the border
  inline const BitMask & from_dir(int8 from, nst::dirs dir) const
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