/*************************************************************
magicbb.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <xcommon.h>

namespace NEngine
{

namespace magic_details_ns
{
  template <int shift_>
  struct magic_struct
  {
    uint64  magic_number;
    uint64  mask;
    uint64* moves;

    static const int bits_count = 64 - shift_;
    static const int bits_shift = shift_;

    inline int index(uint64 const& board) const
    {
      int i = ((mask & board) * magic_number) >> shift_;
      X_ASSERT(i >= (1 << (64 - bits_shift)), "invalid magic index");
      return i;
    }

    inline uint64 const& move(uint64 const& board) const
    {
     return moves[index(board)];
    }
  };

  using magic_struct_rook   = magic_struct<52>;
  using magic_struct_bishop = magic_struct<55>;

  extern magic_struct_rook* rook_magics_p;
  extern magic_struct_bishop* bishop_magics_p;

  // do it only once
  void calculate();
} // magic_details_ns

namespace magic_ns
{
  void initialize();

  inline uint64 const& rook_moves(int pos, uint64 board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid rook position for magic bb");
    auto const& ms = magic_details_ns::rook_magics_p[pos];
    return ms.move(board);
  }

  inline uint64 const& bishop_moves(int pos, uint64 board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid bishop position for magic bb");
    auto const& ms = magic_details_ns::bishop_magics_p[pos];
    return ms.move(board);
  }

  inline uint64 queen_moves(int pos, uint64 board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid queen position for magic bb");
    return rook_moves(pos, board) | bishop_moves(pos, board);
  }
} // magic_ns

} // NEngine
