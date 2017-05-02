/*************************************************************
xbitmath.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xbitmath.h>
#include <xindex.h>
#include <iostream>

namespace NEngine
{
FUNC_POP_COUNT64 g_func_pop_count64 = nullptr;

int pop_count_common(uint64 n)
{
  if(n == 0ULL)
    return 0;
  else if(one_bit_set(n))
    return 1;
  n = n - ((n >> 1)  & 0x5555555555555555ULL);
  n = (n & 0x3333333333333333ULL) + ((n >> 2) & 0x3333333333333333ULL);
  n = (n + (n >> 4)) & 0x0f0f0f0f0f0f0f0fULL;
  n = (n * 0x0101010101010101ULL) >> 56;
  return static_cast<int>(n);
}

#ifdef _MSC_VER


#ifdef _M_X64
int pop_count64_fast(uint64 n)
{
  return __popcnt64(n);
}

void init_popcount_ptr()
{
  int cpuInfo[4] = {};
  int function_id = 0x00000001;
  __cpuid(cpuInfo, function_id);
  if(cpuInfo[2] & (1<<23))
  {
    g_func_pop_count64 = pop_count64_fast;
  }
  else
  {
    g_func_pop_count64 = pop_count_common;
  }
}

#else

void init_popcount_ptr()
{
  g_func_pop_count64 = pop_count_common;
}
#endif

#elif (defined __GNUC__)

#include <cpuid.h>

int pop_count64_fast(uint64 n)
{
  return __builtin_popcountll(n);
}

void init_popcount_ptr()
{
  int level = 1;
  unsigned int eax = 0;
  unsigned int ebx = 0;
  unsigned int ecx = 0;
  unsigned int edx = 0;
  __get_cpuid(level, &eax, &ebx, &ecx, &edx);
  if(ecx & (1<<23))
  {
    g_func_pop_count64 = pop_count64_fast;
  }
  else
  {
    g_func_pop_count64 = pop_count_common;
  }
}

#endif

int8 BitsCounter::s_array_[256] =
{
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

PawnMasks::PawnMasks()
{
  BitMask full_column_msk{};
  for(int i = 0; i < 8; ++i)
  {
    full_column_msk |= set_mask_bit(Index(0, i));
  }

  for(int mask = 3; mask < 256; ++mask)
  {
    int nmax = 1;
    BitMask best_multi_mask{};
    for(int8 i = 0; i < 8; ++i)
    {
      if((mask & (1 << i)) == 0)
        continue;
      BitMask multi_mask{};
      int n = 0;
      for(int8 j = i; j < 8; ++j, ++n)
      {
        if((mask & (1 << j)) == 0)
        {
          i = j;
          break;
        }
        multi_mask |= full_column_msk << j;
      }
      if(n > nmax)
      {
        nmax = n;
        best_multi_mask = multi_mask;
      }
    }
    pmask_multi_passer_[mask] = best_multi_mask;
  }

  for(int color = 0; color < 2; ++color)
  {
    for(int i = 0; i < 64; ++i)
    {
      int x = i & 7;
      int y = i >> 3;

      BitMask pass_msk{};
      int deltay = color ? 1 : -1;
      for(int yy = y+deltay; yy < 8 && yy >= 0; yy += deltay)
      {
        pass_msk |= set_mask_bit(Index(0, yy));
      }

      pmask_doubled_[x] |= full_column_msk << x;
      pmask_column_[x] = full_column_msk << x;
      pmasks_line_blocked_[color][i] = pass_msk << x;
      pmasks_passed_[color][i] = pass_msk << x;

      BitMask back_mask{};
      for(int yy = y; yy < 8 && yy > 0; yy -= deltay)
      {
        back_mask |= set_mask_bit(Index(0, yy));
      }

      BitMask support_mask = set_mask_bit(Index(0, y));
      if(y > 0 && y < 7)
      {
        support_mask |= set_mask_bit(Index(0, y-deltay));
      }

      if(x > 0)
      {
        pmask_isolated_[x] |= full_column_msk << (x-1);
        pmasks_passed_[color][i] |= pass_msk << (x-1);
        pmasks_backward_[color][i] |= back_mask << (x-1);
        pmasks_supported_[color][i] |= support_mask << (x-1);
      }
      if(x < 7)
      {
        pmask_isolated_[x] |= full_column_msk << (x+1);
        pmasks_passed_[color][i] |= pass_msk << (x+1);
        pmasks_backward_[color][i] |= back_mask << (x+1);
        pmasks_supported_[color][i] |= support_mask << (x+1);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////
DeltaPosCounter::DeltaPosCounter()
{
  for(int i = 0; i < 4096; ++i)
  {
    FPos dp = FPosIndexer::get(i >> 6) - FPosIndexer::get(i & 63);
    s_array_[i] = deltaP(dp);
  }
}

//////////////////////////////////////////////////////////////////////////
BetweenMask::BetweenMask(DeltaPosCounter const& deltaPoscounter)
{
  for(int i = 0; i < 64; ++i)
  {
    for(int j = 0; j < 64; ++j)
    {
      // clear it first
      s_between_[i][j] = 0;
      s_from_[i][j] = 0;

      ///  to <- from
      FPos dp = deltaPoscounter.getDeltaPos(j, i);
      if(FPos(0, 0) == dp)
        continue;

      {
        FPos p = FPosIndexer::get(i) + dp;
        FPos q = FPosIndexer::get(j);

        for(; p && p != q; p += dp)
          s_between_[i][j] |= set_mask_bit(p.index());
      }

      {
        FPos p = FPosIndexer::get(i) + dp;

        for(; p; p += dp)
          s_from_[i][j] |= set_mask_bit(p.index());
      }
    }

    for(int j = 0; j < 8; ++j)
    {
      s_from_dir_[j][i] = 0;

      FPos dp;
      switch(j+1)
      {
      case nst::nw:
        dp = FPos(-1, 1);
        break;

      case nst::no:
        dp = FPos(0, 1);
        break;

      case nst::ne:
        dp = FPos(1, 1);
        break;

      case nst::ea:
        dp = FPos(1, 0);
        break;

      case nst::se:
        dp = FPos(1, -1);
        break;

      case nst::so:
        dp = FPos(0, -1);
        break;

      case nst::sw:
        dp = FPos(-1, -1);
        break;

      case nst::we:
        dp = FPos(-1, 0);
        break;
      }

      FPos p = FPosIndexer::get(i) + dp;
      for(; p; p += dp)
        s_from_dir_[j][i] |= set_mask_bit(p.index());
    }
  }
}

//////////////////////////////////////////////////////////////////////////
DistanceCounter::DistanceCounter()
{
  for(int i = 0; i < 4096; ++i)
  {
    FPos dp = FPosIndexer::get(i >> 6) - FPosIndexer::get(i & 63);
    s_array_[i] = dist_dP(dp);
  }
}

//////////////////////////////////////////////////////////////////////////
void print_bitmask(uint64 mask)
{
  for(int i = 7; i >= 0; --i)
  {
    for(int j = 0; j < 8; ++j)
    {
      std::cout << ((mask & (1ULL << Index(j, i))) ? 1 : 0);
    }
    std::cout << std::endl;
  }
}

} // NEngine
