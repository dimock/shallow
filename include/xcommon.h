
/*************************************************************
xcommon.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <ostream>
#include <istream>
#include <limits>
#include <algorithm>
#include <set>
#include <memory>
#include <random>
#include <functional>


#ifdef _MSC_VER

  #pragma warning (disable:4351)

  #define ALIGN_MSC(bytes) __declspec (align(bytes))
  #define ALIGN_GCC(bytes)

#include <intrin.h>

#elif (defined __GNUC__)

  #define ALIGN_GCC(bytes)__attribute__((aligned(bytes)))
  #define ALIGN_MSC(bytes) 


#endif

#ifndef NDEBUG
  #define X_ASSERT(v, msg) if ( v ) throw std::runtime_error(msg); else;
  #define TIMING_FLAG 0xFFF
#else
  #define X_ASSERT(v, msg)
  #define TIMING_FLAG 0x3FFF
#endif

using int8    = std::int8_t;
using uint8   = std::uint8_t;
using int16   = std::int16_t;
using uint16  = std::uint16_t;
using int32   = std::int32_t;
using uint32  = std::uint32_t;
using int64   = std::int64_t;
using uint64  = std::uint64_t;

using ScoreType     = int16;
using BitMask       = uint64;
using SortValueType = int32;

const ScoreType ScoreMax = std::numeric_limits<ScoreType>::max();

static const int NumOfFields = 64;


template <class T, size_t ALIGN_BYTES = 16>
inline T* make_aligned_array(std::vector<uint8>& arr, size_t size_)
{
  arr.resize(size_*sizeof(T) + ALIGN_BYTES*2);
  auto buffer = reinterpret_cast<size_t>(arr.data() + ALIGN_BYTES) & (~(ALIGN_BYTES-(size_t)1));
  return reinterpret_cast<T*>(buffer);
}

namespace nst
{
  enum bishop_rook_dirs
  {
    none,
    rook,
    bishop
  };

  enum dirs
  {
    no_dir,
    nw, // lsb
    no, // lsb
    ne, // lsb
    ea, // lsb
    se, // msb
    so, // msb
    sw, // msb
    we  // msb
  };

  // true - LSB
  extern bool get_bit_dir_[10];
};

#undef LOG_PV

#ifdef NDEBUG
#define USE_MINIDUMP
#endif

#define USE_IID
#define USE_FUTILITY_PRUNING
#define USE_DELTA_PRUNING
#define USE_HASH
#define USE_NULL_MOVE
#define USE_LMR
#undef USE_LMR0
#define VERIFY_LMR
#define SINGULAR_EXT
#undef LMR_REDUCE_MORE
#undef USE_PROBCUT

#define EVAL_SPECC
#define EVAL_KP_BASIC
#define EVAL_MOB
#define EVAL_KING_PR

#undef EVAL_FORKS
#define EVAL_OPEN_R

#define GEN_USUAL_AFETER_HORIZON
#define PROCESS_DANGEROUS_EVAL
#define GENERATE_MAT_CHECK_AFTER_HORIZON

#undef EVALUATE_DANGEROUS_ATTACKS

#undef SYNCHRONIZE_LAST_ITER

static const SortValueType CaptureRecentlyBonus = 10;

static const int NumUsualAfterHorizon = 16;
static const int MinimumDepthForUsualAfterHorizon = 3;
static const int ONE_PLY = 20;
static const int MaxPly = 64;
static const int LMR_DepthLimit = 3 * ONE_PLY;
static const int LMR_MinDepthLimit = 5 * ONE_PLY;
static const int NullMove_DepthMin = 2 * ONE_PLY;
static const int NullMove_PlyReduce = 4 * ONE_PLY;
static const int Probcut_Depth = 7 * ONE_PLY;
static const int Probcut_PlyReduce = 4 * ONE_PLY;
static const int Position_Gain = 130;
static const int AlphaProbCutThreshold = 200;

static const size_t N_THREADS_MAX = 8;
static const size_t N_THREADS_DEFAULT = 1;
static const size_t HASH_SIZE_DEFAULT = 256;
