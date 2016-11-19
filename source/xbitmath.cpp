/*************************************************************
xbitmath.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xbitmath.h>

namespace NEngine
{

#ifdef _MSC_VER

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
  for(int pos = 0; pos < 64; ++pos)
    clearAll(pos);

  for(int i = 0; i < 8; ++i)
    pmask_isolated_[i] = 0;


  for(int color = 0; color < 2; ++color)
  {
    for(int i = 0; i < 64; ++i)
    {
      int dy = color ? -1 : +1;
      int x = i & 7;
      int y = i >> 3;

      if(y > 0 && y < 7)
      {
        if(x < 7)
          pmasks_guarded_[color][i] |= set_mask_bit((y+dy) | ((x+1) << 3));
        if(x > 0)
          pmasks_guarded_[color][i] |= set_mask_bit((y+dy) | ((x-1) << 3));
      }

      uint8 pm = 0;
      uint8 bm = 0;
      if(color)
      {
        for(int j = y+1; j < 7; ++j)
          pm |= set_bit(j);

        if(y < 6)
          bm = set_bit(y) | set_bit(y+1);
      }
      else
      {
        for(int j = y-1; j > 0; --j)
          pm |= 1 << j;

        if(y > 1)
          bm = set_bit(y) | set_bit(y-1);
      }

      uint8 * ppmask = (uint8*)&pmasks_passed_[color][i];
      uint8 * blkmask = (uint8*)&pmasks_blocked_[color][i];

      ppmask[x] = pm;
      blkmask[x] = pm;
      if(x > 0)
      {
        ppmask[x-1] = pm;
      }
      if(x < 7)
      {
        ppmask[x+1] = pm;
      }

      uint64 & kpk_mask = pmask_kpk_[color][i];
      int x0 = x > 0 ? x-1 : 0;
      int x1 = x < 7 ? x+1 : 7;
      if(color)
      {
        for(int j = y+1; j < 8; ++j)
        {
          for(int l = x0; l <= x1; ++l)
          {
            int kp = l | (j<<3);
            kpk_mask |= set_mask_bit(kp);
          }
        }
      }
      else
      {
        for(int j = y-1; j >= 0; --j)
        {
          for(int l = x0; l <= x1; ++l)
          {
            int kp = l | (j<<3);
            kpk_mask |= set_mask_bit(kp);
          }
        }
      }
    }
  }

  for(int i = 0; i < 64; ++i)
  {
    int x = i & 7;
    int y = i >> 3;

    uint8 * dis_mask = (uint8*)&pmasks_disconnected_[i];

    if(x > 0)
    {
      dis_mask[x-1] |= set_bit(y);
      if(y > 0)
        dis_mask[x-1] |= set_bit(y-1);
      if(y < 7)
        dis_mask[x-1] |= set_bit(y+1);
    }

    if(x < 7)
    {
      dis_mask[x+1] |= set_bit(y);
      if(y > 0)
        dis_mask[x+1] |= set_bit(y-1);
      if(y < 7)
        dis_mask[x+1] |= set_bit(y+1);
    }
  }

  for(int x = 0; x < 8; ++x)
  {
    uint8 * ppmask = (uint8*)&pmask_isolated_[x];
    if(x > 0)
      ppmask[x-1] = 0xff;
    if(x < 7)
      ppmask[x+1] = 0xff;
  }

  // destination color
  for(int color = 0; color < 2; ++color)
  {
    for(int i = 0; i < 64; ++i)
    {
      int x = i & 7;
      int8 dst_color = 0;
      if(color) // white
        dst_color = (x+1) & 1;
      else
        dst_color = x & 1;
      pawn_dst_color_[color][i] = dst_color;
    }
  }
}

void PawnMasks::clearAll(int pos)
{
  for(int color = 0; color < 2; ++color)
  {
    pmasks_guarded_[color][pos] = 0;
    pmasks_passed_[color][pos] = 0;
    pmasks_blocked_[color][pos] = 0;
    pmask_kpk_[color][pos] = 0;
  }
  pmasks_disconnected_[pos] = 0;
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

} // NEngine
