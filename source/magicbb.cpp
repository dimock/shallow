/*************************************************************
magicbb.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include <magicbb.h>
#include <xbitmath.h>
#include <xindex.h>
#include <iostream>
#include <random>
#include <chrono>
#include <fstream>

namespace NEngine
{

namespace magic_details_ns
{
  std::vector<BitMask> rook_masks_;
  std::vector<BitMask> bishop_masks_;
  std::vector<uint64>  rook_magic_numbers_;
  std::vector<uint64>  bishop_magic_numbers_;
  std::vector<uint64>  rook_magic_bits_count_;
  std::vector<uint64>  bishop_magic_bits_count_;
  std::vector<size_t>  rook_start_index_;
  std::vector<size_t>  bishop_start_index_;
  std::vector<uint64>  rook_moves_;
  std::vector<uint64>  bishop_moves_;
} // magic_details_ns

namespace
{
  int const extra_bits = 2;
  uint64 border_clear_mask_ = 0;

  void build_clear_border_mask()
  {
    for(int i = 0; i < 8; ++i)
    {
      border_clear_mask_ |= 1ULL << Index(i, 0);
      border_clear_mask_ |= 1ULL << Index(i, 7);
      border_clear_mask_ |= 1ULL << Index(0, i);
      border_clear_mask_ |= 1ULL << Index(7, i);
    }
    border_clear_mask_ = ~border_clear_mask_;
  }

  uint64 rook_moves_from_mask(int pos, uint64 mask_r)
  {
    Index index(pos);
    uint64 moves_x = 0;
    for(int x = index.x()-1; x >= 0; x--)
    {
      int xx = x + index.y()*8;
      moves_x |= 1ULL << xx;
      if((1ULL << xx) & mask_r)
        break;
    }
    for(int x = index.x()+1; x < 8; x++)
    {
      int xx = x + index.y()*8;
      moves_x |= 1ULL << xx;
      if((1ULL << xx) & mask_r)
        break;
    }
    uint64 moves_y = 0;
    for(int y = index.y()-1; y >= 0; y--)
    {
      int yy = y*8 + index.x();
      moves_y |= 1ULL << yy;
      if((1ULL << yy) & mask_r)
        break;
    }
    for(int y = index.y()+1; y < 8; y++)
    {
      int yy = y*8 + index.x();
      moves_y |= 1ULL << yy;
      if((1ULL << yy) & mask_r)
        break;
    }
    return moves_x | moves_y;
  }

  std::vector<std::vector<uint64>>
  claculate_rooks_masks()
  {
    std::vector<std::vector<uint64>> rook_all_masks_(64);
    for(int pos = 0; pos < 64; ++pos)
    {
      rook_all_masks_[pos].reserve(4096);
      Index index(pos);
      int num_x = (index.x() == 0 || index.x() == 7) ? 6 : 5;
      int num_y = (index.y() == 0 || index.y() == 7) ? 6 : 5;
      for(int bits_x = 0; bits_x < (1<<num_x); ++bits_x)
      {
        uint64 mask_x = 0;
        for(int i = 0, x = 1; i < num_x && x < 7; ++x)
        {
          if(x == index.x())
            continue;
          if((1ULL << i) & bits_x)
            mask_x |= 1ULL << x;
          ++i;
        }
        for(int bits_y = 0; bits_y < (1<<num_y); ++bits_y)
        {
          uint64 mask_y = 0;
          for(int i = 0, y = 1; i < num_y && y < 7; ++y)
          {
            if(y == index.y())
              continue;
            if((1ULL << i) & bits_y)
              mask_y |= 1ULL << y*8;
            ++i;
          }
          uint64 mask_r = (mask_x << (index.y()*8)) | (mask_y << index.x());
          rook_all_masks_[pos].push_back(mask_r);
        }
      }
    }

    return rook_all_masks_;
  }

  void calculate_rooks_magic_numbers()
  {
    auto rook_all_masks = claculate_rooks_masks();

    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<uint64> dist;

    for(int pos = 0; pos < 64; ++pos)
    {
      Index index(pos);
      int num_x = (index.x() == 0 || index.x() == 7) ? 6 : 5;
      int num_y = (index.y() == 0 || index.y() == 7) ? 6 : 5;
      int bits_count = num_x + num_y + extra_bits;
      auto const& rmasks = rook_all_masks[pos];
      std::cout << "position (" << index.x() << ", " << index.y() << ") needs " << rmasks.size() << " indices" << std::endl;
      for(;;)
      {
        std::vector<uint64> moves(1 << bits_count);
        int count = 0;
        auto mnum = dist(rgen);
        for(size_t i = 0; i < rmasks.size(); ++i)
        {
          auto rmsk = rmasks[i];
          auto rmvs = rook_moves_from_mask(pos, rmsk);
          X_ASSERT(rmvs == 0ULL, "no rook moves");
          size_t index = (rmsk * mnum) >> (64- bits_count);
          X_ASSERT(index >= moves.size(), "magic index of rook is too big");
          if(moves[index] && moves[index] != rmvs)
            break;
          moves[index] = rmvs;
          count++;
        }
        if(count == rmasks.size())
        {
          magic_details_ns::rook_magic_numbers_[pos] = mnum;
          std::cout << "magic number for pos " << pos << " found: " << mnum << std::endl;
          break;
        }
      }
    }
  }

  void fill_rooks_masks()
  {
    auto rook_all_masks = claculate_rooks_masks();
    size_t start_index = 0;
    for(int pos = 0; pos < 64; ++pos)
    {
      Index index(pos);
      int num_x = (index.x() == 0 || index.x() == 7) ? 6 : 5;
      int num_y = (index.y() == 0 || index.y() == 7) ? 6 : 5;
      int bits_count = num_x + num_y + extra_bits;
      auto const& rmasks = rook_all_masks[pos];
      magic_details_ns::rook_start_index_[pos] = start_index;
      magic_details_ns::rook_magic_bits_count_[pos] = bits_count;
      magic_details_ns::rook_masks_[pos] = rook_moves_from_mask(pos, 0ULL) & border_clear_mask_;
      for(size_t i = 0; i < rmasks.size(); ++i)
      {
        auto rmsk = rmasks[i];
        auto rmvs = rook_moves_from_mask(pos, rmsk);
        auto magic_num = magic_details_ns::rook_magic_numbers_[pos];
        size_t index = (rmsk * magic_num) >> (64- bits_count);
        X_ASSERT(index >= (1 << bits_count), "index is too big");
        if(magic_details_ns::rook_moves_.size() <= index + start_index)
          magic_details_ns::rook_moves_.resize(index + start_index + 1);
        if(magic_details_ns::rook_moves_[index + start_index]
          && magic_details_ns::rook_moves_[index + start_index] != rmvs)
        {
          std::cout << index << " already exists" << std::endl;
        }
        magic_details_ns::rook_moves_[index + start_index] = rmvs;
        X_ASSERT(!magic_details_ns::rook_moves_[index + start_index], "null moves mask");
      }
      start_index += 1 << bits_count;
    }
  }

  void verify_rooks_magics()
  {
    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<uint64> dist;

    auto rook_all_masks = claculate_rooks_masks();
    bool verified_ok = true;

    for(int pos = 0; pos < 64; ++pos)
    {
      for(int i = 0; i < 10000; ++i)
      {
        uint64 board = dist(rgen);
        uint64 rmsk = board & magic_details_ns::rook_masks_[pos];
        uint64 rmvs = rook_moves_from_mask(pos, rmsk);
        int bits_count = magic_details_ns::rook_magic_bits_count_[pos];
        auto mnum = magic_details_ns::rook_magic_numbers_[pos];
        int start_index = magic_details_ns::rook_start_index_[pos];
        size_t index = (rmsk * mnum) >> (64- bits_count);
        auto rmoves_from_index = magic_details_ns::rook_moves_[start_index + index];
        if(rmvs != magic_ns::rook_moves(pos, board))
        {
          verified_ok = false;
          auto const& rmasks = rook_all_masks[pos];
          bool found = false;
          for(auto rm : rmasks)
          {
            if(rm == rmsk)
            {
              found = true;
              break;
            }
          }
          if(!found)
          {
            std::cout << "rook mask not found:" << std::endl;
          }
          std::cout << "invalid magic for board:" << std::endl;
          print_bitmask(board);
          std::cout << "rook mask:" << std::endl;
          print_bitmask(rmsk);
          std::cout << "rook moves:" << std::endl;
          print_bitmask(rmvs);
          std::cout << "magic rook moves:" << std::endl;
          print_bitmask(magic_ns::rook_moves(pos, board));
          std::cout << "rook all moves:" << std::endl;
          print_bitmask(magic_details_ns::rook_masks_[pos]);
          break;
        }
      }
    }
    if(verified_ok)
    {
      std::cout << "verified ok" << std::endl;
    }
  }

  void save_magic_numbers(std::vector<uint64> const& magics, std::string const& fname)
  {
    std::ofstream ofs(fname);
    int n = 0;
    for(auto mn : magics)
    {
      ofs << mn << ", ";
      if(++n >= 8)
      {
        ofs << std::endl;
        n = 0;
      }
    }
  }

} // namespace {}

namespace magic_details_ns
{
  void magic_details_ns::calculate()
  {
    magic_ns::initialize();

    auto t_start = std::chrono::high_resolution_clock::now();

    calculate_rooks_magic_numbers();
    fill_rooks_masks();
    verify_rooks_magics();

    auto t_end = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start);
    std::cout << "magic numbers took: " << dur.count() << " seconds" << std::endl;

    std::cout << "rooks table contains " << magic_details_ns::rook_moves_.size() << " elements" << std::endl;
    std::cout << "rooks table size is " << magic_details_ns::rook_moves_.size() * sizeof(uint64) << " bytes" << std::endl;

    save_magic_numbers(magic_details_ns::rook_magic_numbers_, "rook_magics.txt");
  }
} // magic_details_ns

namespace magic_ns
{
  void initialize()
  {
    magic_details_ns::rook_masks_.resize(64);
    magic_details_ns::bishop_masks_.resize(64);
    magic_details_ns::rook_magic_numbers_.resize(64);
    magic_details_ns::bishop_magic_numbers_.resize(64);
    magic_details_ns::rook_magic_bits_count_.resize(64);
    magic_details_ns::bishop_magic_bits_count_.resize(64);
    magic_details_ns::rook_moves_.reserve(64*1024);
    magic_details_ns::bishop_moves_.reserve(64*512);
    magic_details_ns::rook_start_index_.resize(64);
    magic_details_ns::bishop_start_index_.resize(64);

    build_clear_border_mask();

    std::cout << "border clear mask" << std::endl;
    print_bitmask(border_clear_mask_);
    std::cout << std::endl;
  }
} // magic_ns

} // NEngine
