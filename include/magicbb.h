/*************************************************************
magicbb.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <xcommon.h>

namespace NEngine
{
namespace magic_details_ns
{
  extern std::vector<BitMask> rook_masks_;
  extern std::vector<BitMask> bishop_masks_;
  extern std::vector<uint64>  rook_magic_numbers_;
  extern std::vector<uint64>  bishop_magic_numbers_;
  extern std::vector<uint64>  rook_magic_bits_count_;
  extern std::vector<uint64>  bishop_magic_bits_count_;
  extern std::vector<size_t>  rook_start_index_;
  extern std::vector<size_t>  bishop_start_index_;
  extern std::vector<uint64>  rook_moves_;
  extern std::vector<uint64>  bishop_moves_;

  // do it only once
  void calculate();
} // magic_details_ns

namespace magic_ns
{
  void initialize();

  inline BitMask rook_moves(int pos, BitMask board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid rook position for magic bb");
    auto rmask = magic_details_ns::rook_masks_[pos] & board;
    size_t magic_index = (rmask * magic_details_ns::rook_magic_numbers_[pos])
      >> (64 - magic_details_ns::rook_magic_bits_count_[pos]);
    X_ASSERT(magic_index >= (1 << magic_details_ns::rook_magic_bits_count_[pos]), "invalid rook magic index");
    return magic_details_ns::rook_moves_[magic_details_ns::rook_start_index_[pos] + magic_index];
  }

  inline BitMask bishop_moves(int pos, BitMask board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid bishop position for magic bb");
    size_t magic_index = ((magic_details_ns::bishop_masks_[pos] & board) * magic_details_ns::bishop_magic_numbers_[pos])
      >> (64 - magic_details_ns::bishop_magic_bits_count_[pos]);
    X_ASSERT(magic_index >= (1 << magic_details_ns::bishop_magic_bits_count_[pos]), "invalid bishop magic index");
    return magic_details_ns::bishop_moves_[magic_details_ns::bishop_start_index_[pos] + magic_index];
  }

  inline BitMask queen_moves(int pos, BitMask board)
  {
    X_ASSERT(pos < 0 || pos > 63, "invalid queen position for magic bb");
    return rook_moves(pos, board) | bishop_moves(pos, board);
  }
} // magic_ns

} // NEngine
