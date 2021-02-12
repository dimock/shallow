/*************************************************************
xbitmath.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <xbitmath.h>
#include <xindex.h>
#include <Board.h>
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
  return static_cast<int>(__popcnt64(n));
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

  for(int color = 0; color < 2; ++color)
  {
    for(int i = 0; i < 64; ++i)
    {
      int x = i & 7;
      int y = i >> 3;

      pmask_row_[i] = 255ULL << (8 * y);

      BitMask forward_msk{};
      BitMask backward_msk{};
      int deltay = color ? 1 : -1;
      for(int yy = y+deltay; yy < 8 && yy >= 0; yy += deltay)
      {
        forward_msk |= set_mask_bit(Index(0, yy));
      }
      for (int yy = y; yy < 7 && yy > 0; yy -= deltay)
      {
        backward_msk |= set_mask_bit(Index(0, yy));
      }

      pmask_column_[x] = full_column_msk << x;
      pmasks_forward_[color][i] = forward_msk << x;
      pmasks_passed_[color][i] = forward_msk << x;

      BitMask neighbor_mask = set_mask_bit(Index(0, y));
      BitMask guard_mask = set_mask_bit(Index(0, y));
      if(y > 0 && y < 7)
      {
        guard_mask |= set_mask_bit(Index(0, y-deltay));
        neighbor_mask |= set_mask_bit(Index(0, y - deltay));
        neighbor_mask |= set_mask_bit(Index(0, y + deltay));
      }

      if(x > 0)
      {
        pmask_isolated_[x] |= full_column_msk << (x-1);
        pmasks_passed_[color][i] |= forward_msk << (x-1);
        pmasks_neighbor_[color][i] |= neighbor_mask << (x-1);
        pmasks_guards_[color][i] |= guard_mask << (x-1);
        pmasks_backward_[color][i] |= backward_msk << (x-1);
      }
      if(x < 7)
      {
        pmask_isolated_[x] |= full_column_msk << (x+1);
        pmasks_passed_[color][i] |= forward_msk << (x+1);
        pmasks_neighbor_[color][i] |= neighbor_mask << (x+1);
        pmasks_guards_[color][i] |= guard_mask << (x+1);
        pmasks_backward_[color][i] |= backward_msk << (x+1);
      }
    }
  }

  /// blocked knights and bishops
  pmask_blocked_knight_[Figure::ColorBlack][A1] = set_mask_bit(A2) | set_mask_bit(C2);
  pmask_blocked_knight_[Figure::ColorBlack][A2] = set_mask_bit(A3);
  pmask_blocked_knight_[Figure::ColorBlack][B1] = set_mask_bit(B2);
  pmask_blocked_knight_[Figure::ColorBlack][B2] = set_mask_bit(B3);
  pmask_blocked_knight_[Figure::ColorBlack][H1] = set_mask_bit(H2) | set_mask_bit(F2);
  pmask_blocked_knight_[Figure::ColorBlack][H2] = set_mask_bit(H3);
  pmask_blocked_knight_[Figure::ColorBlack][G1] = set_mask_bit(G2);
  pmask_blocked_knight_[Figure::ColorBlack][G2] = set_mask_bit(G3);

  pmask_blocked_knight_[Figure::ColorWhite][A8] = set_mask_bit(A7) | set_mask_bit(C7);
  pmask_blocked_knight_[Figure::ColorWhite][A7] = set_mask_bit(A6);
  pmask_blocked_knight_[Figure::ColorWhite][B8] = set_mask_bit(B7);
  pmask_blocked_knight_[Figure::ColorWhite][B7] = set_mask_bit(B6);
  pmask_blocked_knight_[Figure::ColorWhite][H8] = set_mask_bit(H7) | set_mask_bit(F7);
  pmask_blocked_knight_[Figure::ColorWhite][H7] = set_mask_bit(H6);
  pmask_blocked_knight_[Figure::ColorWhite][G8] = set_mask_bit(G7);
  pmask_blocked_knight_[Figure::ColorWhite][G7] = set_mask_bit(G6);

  pmask_blocked_bishop_[Figure::ColorBlack][A1] = set_mask_bit(B2);
  pmask_blocked_bishop_[Figure::ColorBlack][A2] = set_mask_bit(B3);
  pmask_blocked_bishop_[Figure::ColorBlack][A3] = set_mask_bit(B4);
  pmask_blocked_bishop_[Figure::ColorBlack][B1] = set_mask_bit(C2) | set_mask_bit(A2);
  pmask_blocked_bishop_[Figure::ColorBlack][B2] = set_mask_bit(C3) | set_mask_bit(A3);
  pmask_blocked_bishop_[Figure::ColorBlack][H1] = set_mask_bit(G2);
  pmask_blocked_bishop_[Figure::ColorBlack][H2] = set_mask_bit(G3);
  pmask_blocked_bishop_[Figure::ColorBlack][H3] = set_mask_bit(G4);
  pmask_blocked_bishop_[Figure::ColorBlack][G1] = set_mask_bit(F2) | set_mask_bit(H2);
  pmask_blocked_bishop_[Figure::ColorBlack][G2] = set_mask_bit(F3) | set_mask_bit(H3);

  pmask_blocked_bishop_[Figure::ColorWhite][A6] = set_mask_bit(B5);
  pmask_blocked_bishop_[Figure::ColorWhite][A7] = set_mask_bit(B6);
  pmask_blocked_bishop_[Figure::ColorWhite][A8] = set_mask_bit(B7);
  pmask_blocked_bishop_[Figure::ColorWhite][B8] = set_mask_bit(C7) | set_mask_bit(A7);
  pmask_blocked_bishop_[Figure::ColorWhite][B7] = set_mask_bit(C6) | set_mask_bit(A6);
  pmask_blocked_bishop_[Figure::ColorWhite][H6] = set_mask_bit(G5);
  pmask_blocked_bishop_[Figure::ColorWhite][H7] = set_mask_bit(G6);
  pmask_blocked_bishop_[Figure::ColorWhite][H8] = set_mask_bit(G7);
  pmask_blocked_bishop_[Figure::ColorWhite][G8] = set_mask_bit(F7) | set_mask_bit(H7);
  pmask_blocked_bishop_[Figure::ColorWhite][G7] = set_mask_bit(F6) | set_mask_bit(H6);
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
      s_between_[i][j] = 0ULL;
      s_from_[i][j] = 0ULL;
      s_tail_[i][j] = 0ULL;
      s_line_[i][j] = 0ULL;

      ///  to <- from
      FPos dp = deltaPoscounter.getDeltaPos(j, i);
      if(dp.is_zero())
        continue;
      
      {
        FPos p = FPosIndexer::get(i) + dp;
        for (; p; p += dp)
          s_line_[i][j] |= set_mask_bit(p.index());
        p = FPosIndexer::get(i);
        for (; p; p -= dp)
          s_line_[i][j] |= set_mask_bit(p.index());
      }
      
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

      {
        FPos p = FPosIndexer::get(j) + dp;

        for (; p; p += dp)
          s_tail_[i][j] |= set_mask_bit(p.index());
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

// idea from CCRL
// "color" belongs to pawn

static const BitMask cut_le = ~0x0101010101010101;
static const BitMask cut_ri = ~0x8080808080808080;

bool couldIntercept(Board const& board,
                    BitMask const& inv_mask_all,
                    BitMask const& attack_mask_c,
                    int8 color,
                    int pawn_pos,
                    int promo_pos,
                    int stepsLimit)
{
  Figure::Color ocolor = Figure::otherColor(Figure::Color(color));
  if(board.kingPos(ocolor) == promo_pos)
  {
    X_ASSERT(attack_mask_c & set_mask_bit(promo_pos), "king is on attacked field");
    return true;
  }

  BitMask target = (set_mask_bit(promo_pos) | betweenMasks().between(pawn_pos, promo_pos)) & ~attack_mask_c;

  // whole pawn track to promotion field is attacked
  if(target == 0ULL)
    return false;

  BitMask from_mask = board.fmgr().king_mask(ocolor);
  BitMask path_mask = (inv_mask_all & ~attack_mask_c) | from_mask | target;

  for(int i = 0; i < stepsLimit; ++i)
  {
    X_ASSERT(i > 64, "infinite loop in path finder");

    BitMask mask_le = ((from_mask << 1) | (from_mask << 9) | (from_mask >> 7) | (from_mask << 8)) & cut_le;
    BitMask mask_ri = ((from_mask >> 1) | (from_mask >> 9) | (from_mask << 7) | (from_mask >> 8)) & cut_ri;
    BitMask next_mask = from_mask | ((mask_le | mask_ri) & path_mask);

    if (next_mask & target) {
      int p = color ? _msb64(next_mask & target) : _lsb64(next_mask & target);
      if (distanceCounter().getDistance(p, pawn_pos) >= i)
        return true;
      break;
    }

    if(next_mask == from_mask)
      break;

    from_mask = next_mask;
  }

  return false;
}

} // NEngine
