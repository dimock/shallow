/*************************************************************
magicbb.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <xcommon.h>

namespace NEngine
{
namespace magic_details_ns
{
  struct magic_struct
  {
    uint64 magic_number;
    int start_index;
    int bits_shift;

    inline int index(uint64 const& mask) const
    {
      return start_index + ((mask * magic_number) >> bits_shift);
    }
  };
  extern std::vector<uint64> rook_masks_;
  extern std::vector<uint64> bishop_masks_;
  extern std::vector<uint64> rook_moves_;
  extern std::vector<uint64> bishop_moves_;
  extern std::vector<magic_struct> rook_magics_;
  extern std::vector<magic_struct> bishop_magics_;

  // do it only once
  void calculate();
} // magic_details_ns

namespace magic_ns
{
  void initialize();

  inline uint64 rook_moves(int pos, uint64 board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid rook position for magic bb");
    auto const& ms = magic_details_ns::rook_magics_[pos];
    auto magic_index = ms.index(magic_details_ns::rook_masks_[pos] & board);
    X_ASSERT(magic_index >= magic_details_ns::rook_moves_.size(), "invalid rook magic index");
    return magic_details_ns::rook_moves_[magic_index];
  }

  inline uint64 bishop_moves(int pos, uint64 board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid bishop position for magic bb");
    auto const& ms = magic_details_ns::bishop_magics_[pos];
    size_t magic_index = ms.index(magic_details_ns::bishop_masks_[pos] & board);
    X_ASSERT(magic_index >= magic_details_ns::bishop_moves_.size(), "invalid bishop magic index");
    return magic_details_ns::bishop_moves_[magic_index];
  }

  inline uint64 queen_moves(int pos, uint64 board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid queen position for magic bb");
    return rook_moves(pos, board) | bishop_moves(pos, board);
  }
} // magic_ns

} // NEngine
