
/*************************************************************
xcommon.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#include <cstdint>
//#include <stdexcept>
#include <vector>
#include <string>
#include <ostream>
#include <istream>
#include <limits>
#include <algorithm>


#ifdef _MSC_VER

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

using ScoreType = int16;
using BitMask   = uint64;

const ScoreType ScoreMax = std::numeric_limits<ScoreType>::max();

namespace nst
{
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

//  //#define LOG_PV
#define USE_IID
#define USE_FUTILITY_PRUNING
#define USE_DELTA_PRUNING
#define USE_HASH
#define USE_NULL_MOVE
#define USE_LMR
#define VERIFY_LMR
#define SINGULAR_EXT

  //#define VERIFY_ESCAPE_GENERATOR
  //#define VERIFY_CHECKS_GENERATOR
  //#define VERIFY_CAPS_GENERATOR
  //#define VERIFY_FAST_GENERATOR
  //#define VERIFY_TACTICAL_GENERATOR
  //#define VALIDATE_VALIDATOR

static const int ONE_PLY = 20;
static const int MaxPly = 64;
static const int LMR_DepthLimit = 3 * ONE_PLY;
static const int LMR_MinDepthLimit = 5 * ONE_PLY;
static const int NullMove_DepthMin = 2 * ONE_PLY;
static const int NullMove_PlyReduce = 4 * ONE_PLY;
static const int NullMove_PlyVerify = 5 * ONE_PLY;
