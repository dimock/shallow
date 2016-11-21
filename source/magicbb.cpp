/*************************************************************
magicbb.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include <magicbb.h>
#include <xbitmath.h>
#include <xindex.h>
#include <fpos.h>

#include <iostream>
#include <random>
#include <chrono>
#include <fstream>
#include <array>
#include <functional>

namespace NEngine
{

namespace magic_details_ns
{
  std::vector<uint64> rook_masks_;
  std::vector<uint64> bishop_masks_;
  std::vector<uint64> rook_moves_;
  std::vector<uint64> bishop_moves_;
  std::vector<magic_struct> rook_magics_;
  std::vector<magic_struct> bishop_magics_;
} // magic_details_ns

namespace
{
  std::vector<magic_details_ns::magic_struct> predefined_rook_magics()
  {
    return std::vector<magic_details_ns::magic_struct>
    {
      { 72075465399107584, 0, 52}, { 9241386572837814336, 0, 53 }, { 144150925458542664, 0, 53 }, { 72092847134213376, 0, 53 },
      { 180148384249151872, 0, 53 }, { 72061992219836672, 0, 53 }, { 108105082779730432, 0, 53 }, { 1008814022834913572, 0, 52 },
      { 1548113445687340, 0, 53 }, { 12141775032860803273, 0, 54 }, { 9224357268001136640, 0, 54 }, { 5066619374559744, 0, 54 },
      { 1153484497520428049, 0, 54 }, { 2306406268489695744, 0, 54 }, { 2468535708961474561, 0, 54 }, { 432486320028256512, 0, 53 },
      { 4611828405319516160, 0, 53 }, { 9148487573233664, 0, 54 }, { 11678186777541120256, 0, 54 }, { 2612792021206708224, 0, 54 },
      { 282574773815296, 0, 54 }, { 1225261673301147672, 0, 54 }, { 36173932587452464, 0, 54 }, { 14375015088673921, 0, 53 },
      { 146507873554415620, 0, 53 }, { 3463268389399830528, 0, 54 }, { 18150534021696, 0, 54 }, { 571748194975872, 0, 54 },
      { 74773233664128, 0, 54 }, { 1157988097137856548, 0, 54 }, { 563302141004040, 0, 54 }, { 4785078899081282, 0, 53 },
      { 31595568895623320, 0, 53 }, { 2810600347666485248, 0, 54 }, { 3747628346125595392, 0, 54 }, { 1769493574971305984, 0, 54 },
      { 864831883131766784, 0, 54 }, { 604133312625188928, 0, 54 }, { 140746086711552, 0, 54 }, { 703690704945408, 0, 53 },
      { 234504410129006593, 0, 53 }, { 1153625333816754242, 0, 54 }, { 2306124553446817856, 0, 54 }, { 19149094777815168, 0, 54 },
      { 45620936593932416, 0, 54 }, { 9232942237741350929, 0, 54 }, { 580965520298475538, 0, 54 }, { 564051633635364, 0, 53 },
      { 576532231297704192, 0, 53 }, { 4614289666794848320, 0, 54 }, { 140806209929600, 0, 54 }, { 14267456406807904384, 0, 54 },
      { 5497831293184, 0, 54 }, { 1297601843807125632, 0, 54 }, { 562968259994112, 0, 54 }, { 141085389357568, 0, 53 },
      { 7353810792022043, 0, 52 }, { 18006667313161, 0, 53 }, { 1738389526495691841, 0, 53 }, { 2018175721128607878, 0, 53 },
      { 9890467870314598402, 0, 53 }, { 4612249175546397058, 0, 53 }, { 9370302009393381634, 0, 53 }, { 15132376242307858465, 0, 52 }
    };
  }

  std::vector<magic_details_ns::magic_struct> predefined_bishop_magics()
  {
    return std::vector<magic_details_ns::magic_struct>
    {
      { 9044651520565380, 0, 58}, { 43911204502372352, 0, 59 }, { 10177086623058432, 0, 59 }, { 9242516769826930688, 0, 59 },
      { 297536917457608704, 0, 59 }, { 11830762630291718, 0, 59 }, { 585750814348345344, 0, 59 }, { 1153221155902464000, 0, 58 },
      { 289426653694788099, 0, 59 }, { 720593876179550754, 0, 59 }, { 2305895802960617484, 0, 59 }, { 2882312652103811104, 0, 59 },
      { 866432884679168, 0, 59 }, { 22635663598092332, 0, 59 }, { 9583945884388823040, 0, 59 }, { 13835130661905760993, 0, 59 },
      { 18025462412870274, 0, 59 }, { 4503616874660864, 0, 59 }, { 13863205587323464192, 0, 57 }, { 282678171598912, 0, 57 },
      { 1127001576464385, 0, 57 }, { 1157566541812728064, 0, 57 }, { 591185483026141186, 0, 59 }, { 2305913382274994176, 0, 59 },
      { 2310356505528369408, 0, 59 }, { 2256335848669760, 0, 59 }, { 291766407727719936, 0, 57 }, { 4621256717525614602, 0, 55 },
      { 9268553168671760404, 0, 55 }, { 4613938368148930688, 0, 57 }, { 164400079405351424, 0, 59 }, { 576813699835176960, 0, 59 },
      { 2256268861677574, 0, 59 }, { 121810701370527745, 0, 59 }, { 2360097517177803776, 0, 57 }, { 4613977417844261152, 0, 55 },
      { 29295542429303040, 0, 55 }, { 1153071630911603456, 0, 57 }, { 9227877837653025928, 0, 59 }, { 74311628509684993, 0, 59 },
      { 198441027081617473, 0, 59 }, { 147495121783390720, 0, 59 }, { 18084907108573248, 0, 57 }, { 576460890145030404, 0, 57 },
      { 1442394475094606852, 0, 57 }, { 295579546265059620, 0, 57 }, { 73187909238331456, 0, 59 }, { 298416818811568404, 0, 59 },
      { 9799987339666128900, 0, 59 }, { 4611827308088559104, 0, 59 }, { 1134979874816040, 0, 59 }, { 1134978793521, 0, 59 },
      { 36032371241583888, 0, 59 }, { 4701830682677544960, 0, 59 }, { 54329377822893072, 0, 59 }, { 72638140509618178, 0, 59 },
      { 598323573033088, 0, 58 }, { 22520223097771008, 0, 59 }, { 36067419584202752, 0, 59 }, { 21990241207312, 0, 59 },
      { 9012149910250500, 0, 59 }, { 9483824791702, 0, 59 }, { 35331206414468, 0, 59 }, { 6922635194671448196, 0, 58 }
    };
  }

  uint64 rook_moves_from_mask(int pos, uint64 mask_r, bool exclude_border)
  {
    Index index(pos);
    uint64 moves_x = 0;
    int x0 = exclude_border ? 1 : 0;
    int y0 = exclude_border ? 1 : 0;
    int x8 = exclude_border ? 7 : 8;
    int y8 = exclude_border ? 7 : 8;
    for(int x = index.x()-1; x >= x0; x--)
    {
      int xx = x + index.y()*8;
      moves_x |= 1ULL << xx;
      if((1ULL << xx) & mask_r)
        break;
    }
    for(int x = index.x()+1; x < x8; x++)
    {
      int xx = x + index.y()*8;
      moves_x |= 1ULL << xx;
      if((1ULL << xx) & mask_r)
        break;
    }
    uint64 moves_y = 0;
    for(int y = index.y()-1; y >= y0; y--)
    {
      int yy = y*8 + index.x();
      moves_y |= 1ULL << yy;
      if((1ULL << yy) & mask_r)
        break;
    }
    for(int y = index.y()+1; y < y8; y++)
    {
      int yy = y*8 + index.x();
      moves_y |= 1ULL << yy;
      if((1ULL << yy) & mask_r)
        break;
    }
    return moves_x | moves_y;
  }

  uint64 bishop_moves_from_mask(int pos, uint64 mask_b, bool exclude_border)
  {
    std::array<FPos, 4> deltas =
    {
      FPos{1, 1},
      FPos{ 1, -1 },
      FPos{ -1, 1 },
      FPos{ -1, -1 }
    };
    uint64 mask = 0;
    for(auto const& d : deltas)
    {
      FPos current(pos);
      for(current += d; current; current += d)
      {
        if(exclude_border &&
          (current.x() < 1 || current.x() > 6 || current.y() < 1 || current.y() > 6))
        {
          break;
        }
        uint64 m = 1ULL << current.index();
        mask |= m;
        if(m & mask_b)
          break;
      }
    }
    return mask;
  }

  std::vector<std::vector<uint64>>
  calculate_rooks_masks()
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

  std::vector<std::vector<uint64>>
  calculate_bishops_masks()
  {
    std::vector<std::vector<uint64>> bishop_all_masks_(64);
    std::array<FPos, 4> deltas =
    {
      FPos{ 1, 1 },
      FPos{ 1, -1 },
      FPos{ -1, 1 },
      FPos{ -1, -1 }
    };
    for(int pos = 0; pos < 64; ++pos)
    {
      bishop_all_masks_[pos].reserve(512);
      FPos start(pos);
      for(int bits = 0; bits < 512; ++bits)
      {
        uint64 mask = 0;
        int i = 0;
        bool zero_found = false;
        for(auto const& d : deltas)
        {
          FPos current(pos);
          for(current += d;; current += d)
          {
            if(current.x() < 1 || current.x() > 6 || current.y() < 1 || current.y() > 6)
              break;
            if((bits & (1 << i++)))
              mask |= 1ULL << current.index();
            else
              zero_found = true;
          }
        }
        bishop_all_masks_[pos].push_back(mask);
        if(!zero_found)
          break;
      }
    }
    return bishop_all_masks_;
  }

  std::vector<magic_details_ns::magic_struct>
  calculate_magic_numbers(std::function<std::vector<std::vector<uint64>>()> const& calculate_masks,
    std::function<int(int)> const& bits_counter,
    std::function<uint64(int, uint64, bool)> const& moves_from_mask)
  {
    auto all_masks = calculate_masks();

    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<uint64> dist;

    std::vector<magic_details_ns::magic_struct> magics(64);
    int total_counter = 0;
    for(int pos = 0; pos < 64; ++pos)
    {
      auto const& xmasks = all_masks[pos];
      Index index(pos);
      int bits_count = bits_counter(pos);
      //std::cout << "position (" << index.x() << ", " << index.y() << ") needs " << xmasks.size() << " indices" << std::endl;
      for(size_t vcounter = 0;; ++vcounter, ++total_counter)
      {
        std::vector<uint64> moves(1 << bits_count);
        int count = 0;
        auto mnum = dist(rgen) & dist(rgen) & dist(rgen);
        for(size_t i = 0; i < xmasks.size(); ++i)
        {
          auto xmsk = xmasks[i];
          auto xmvs = moves_from_mask(pos, xmsk, false);
          X_ASSERT(xmvs == 0ULL, "no moves");
          size_t index = (xmsk * mnum) >> (64- bits_count);
          X_ASSERT(index >= moves.size(), "magic index is too big");
          if(moves[index] && moves[index] != xmvs)
            break;
          moves[index] = xmvs;
          count++;
        }
        if(count == xmasks.size())
        {
          magics[pos] = magic_details_ns::magic_struct{ mnum, 0, 64-bits_count };
          std::cout << "magic number for pos " << pos << " found: " << mnum << " ; variants count: " << vcounter << std::endl;
          break;
        }
      }
    }
    std::cout << "total variants count: " << total_counter << std::endl;
    return magics;
  }

  void fill_masks(std::function<std::vector<std::vector<uint64>>()> const& calculate_masks,
    std::function<int(int)> const& bits_counter,
    std::function<uint64(int, uint64, bool)> const& moves_from_mask,
    std::vector<magic_details_ns::magic_struct> & magics_io,
    std::vector<uint64>& masks_o,
    std::vector<uint64>& moves_o)
  {
    masks_o.resize(64);
    moves_o.reserve(64*512);
    auto all_masks = calculate_masks();
    int start_index = 0;
    for(int pos = 0; pos < 64; ++pos)
    {
      auto& ms = magics_io[pos];
      ms.start_index = start_index;
      auto const& xmasks = all_masks[pos];
      masks_o[pos] = moves_from_mask(pos, 0ULL, true);
      for(size_t i = 0; i < xmasks.size(); ++i)
      {
        auto xmsk = xmasks[i];
        auto xmvs = moves_from_mask(pos, xmsk, false);
        int index = ms.index(xmsk);
        if(moves_o.size() <= index)
          moves_o.resize(index + 1);
        if(moves_o[index] && moves_o[index] != xmvs)
        {
          X_ASSERT(true, "already exists");
        }
        moves_o[index] = xmvs;
        X_ASSERT(!moves_o[index], "zero moves mask");
      }
      start_index += 1 << (64 - ms.bits_shift);
    }
  }

  void verify_magics(std::function<uint64(int, uint64)> const& get_moves,
    std::function<std::vector<std::vector<uint64>>()> const& calculate_masks,
    std::function<uint64(int, uint64, bool)> const& moves_from_mask,
    std::vector<magic_details_ns::magic_struct> const& magics_i,
    std::vector<uint64> const& masks_i,
    std::vector<uint64> const& moves_i)
  {
    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<uint64> dist;

    auto all_masks = calculate_masks();
    bool verified_ok = true;

    for(int pos = 0; pos < 64; ++pos)
    {
      for(int i = 0; i < 1000000; ++i)
      {
        auto const& ms = magics_i[pos];
        uint64 board = dist(rgen);
        uint64 xmsk = board & masks_i[pos];
        uint64 xmvs = moves_from_mask(pos, xmsk, false);
        int index = ms.index(xmsk);
        auto xmoves_from_index = moves_i[index];
        if(xmvs != get_moves(pos, board)
          || xmoves_from_index != xmvs)
        {
          verified_ok = false;
          auto const& rmasks = all_masks[pos];
          bool found = false;
          for(auto rm : rmasks)
          {
            if(rm == xmsk)
            {
              found = true;
              break;
            }
          }
          if(!found)
          {
            std::cout << "mask not found:" << std::endl;
          }
          std::cout << "invalid magic for board:" << std::endl;
          print_bitmask(board);
          std::cout << "mask:" << std::endl;
          print_bitmask(xmsk);
          std::cout << "moves:" << std::endl;
          print_bitmask(xmvs);
          std::cout << "magic moves:" << std::endl;
          print_bitmask(get_moves(pos, board));
          std::cout << "all moves:" << std::endl;
          print_bitmask(masks_i[pos]);
          break;
        }
      }
    }

    //for(int i = 0; i < 3; ++i)
    //{
    //  uint64 board = dist(rgen);
    //  int pos = dist(rgen) & 63;
    //  auto xmoves = get_moves(pos, board);
    //  std::cout << "for board:" << std::endl;
    //  print_bitmask(board);
    //  std::cout << "moves found" << std::endl;
    //  print_bitmask(xmoves);
    //  std::cout << std::endl;
    //}

    if(verified_ok)
    {
      std::cout << "verified ok" << std::endl;
    }
  }

  void save_magic_numbers(std::vector<magic_details_ns::magic_struct> const& magics, std::string const& fname)
  {
    std::ofstream ofs(fname);
    int n = 0;
    for(auto const& ms : magics)
    {
      ofs << "{" << ms.magic_number << ", 0, " << ms.bits_shift << "}, ";
      if(++n >= 4)
      {
        ofs << std::endl;
        n = 0;
      }
    }
  }

  int rook_bits_counter(int pos)
  {
    Index index(pos);
    int num_x = (index.x() == 0 || index.x() == 7) ? 6 : 5;
    int num_y = (index.y() == 0 || index.y() == 7) ? 6 : 5;
    return num_x + num_y;
  }

  int bishop_bits_counter(int pos)
  {
    std::array<FPos, 4> deltas =
    {
      FPos{ 1, 1 },
      FPos{ 1, -1 },
      FPos{ -1, 1 },
      FPos{ -1, -1 }
    };
    int bits_count = 0;
    for(auto const& d : deltas)
    {
      FPos current(pos);
      for(current += d;; current += d)
      {
        if(current.x() < 1 || current.x() > 6 || current.y() < 1 || current.y() > 6)
          break;
        bits_count++;
      }
    }
    return bits_count;
  }

} // namespace {}

namespace magic_details_ns
{
  // do only once
  void calculate()
  {
    auto t_start = std::chrono::high_resolution_clock::now();

    bool calculate_rooks_magics = true;
    bool calculate_bishops_magics = true;

    if(calculate_rooks_magics)
    {
      magic_details_ns::rook_magics_ =
        calculate_magic_numbers(calculate_rooks_masks,
        rook_bits_counter,
        rook_moves_from_mask);

      fill_masks(calculate_rooks_masks,
        rook_bits_counter,
        rook_moves_from_mask,
        magic_details_ns::rook_magics_,
        magic_details_ns::rook_masks_,
        magic_details_ns::rook_moves_);

      verify_magics([](int pos, uint64 board) { return magic_ns::rook_moves(pos, board); },
        calculate_rooks_masks,
        rook_moves_from_mask,
        magic_details_ns::rook_magics_,
        magic_details_ns::rook_masks_,
        magic_details_ns::rook_moves_);
    }

    if(calculate_bishops_magics)
    {
      magic_details_ns::bishop_magics_ =
        calculate_magic_numbers(calculate_bishops_masks,
        bishop_bits_counter,
        bishop_moves_from_mask);

      fill_masks(calculate_bishops_masks,
        bishop_bits_counter,
        bishop_moves_from_mask,
        magic_details_ns::bishop_magics_,
        magic_details_ns::bishop_masks_,
        magic_details_ns::bishop_moves_);

      verify_magics([](int pos, uint64 board) { return magic_ns::bishop_moves(pos, board); },
        calculate_bishops_masks,
        bishop_moves_from_mask,
        magic_details_ns::bishop_magics_,
        magic_details_ns::bishop_masks_,
        magic_details_ns::bishop_moves_);
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start);
    std::cout << "magic numbers took: " << dur.count() << " seconds" << std::endl;

    std::cout << "rooks table contains " << magic_details_ns::rook_moves_.size() << " elements" << std::endl;
    std::cout << "rooks table size is " << magic_details_ns::rook_moves_.size() * sizeof(uint64) << " bytes" << std::endl;
    save_magic_numbers(magic_details_ns::rook_magics_, "rook_magics.txt");

    std::cout << "bishops table contains " << magic_details_ns::bishop_moves_.size() << " elements" << std::endl;
    std::cout << "bishops table size is " << magic_details_ns::bishop_moves_.size() * sizeof(uint64) << " bytes" << std::endl;
    save_magic_numbers(magic_details_ns::bishop_magics_, "bishop_magics.txt");
  }
} // magic_details_ns

namespace magic_ns
{
  void initialize()
  {
    magic_details_ns::rook_magics_   = predefined_rook_magics();
    magic_details_ns::bishop_magics_ = predefined_bishop_magics();

    fill_masks(calculate_rooks_masks,
      rook_bits_counter,
      rook_moves_from_mask,
      magic_details_ns::rook_magics_,
      magic_details_ns::rook_masks_,
      magic_details_ns::rook_moves_);

    fill_masks(calculate_bishops_masks,
      bishop_bits_counter,
      bishop_moves_from_mask,
      magic_details_ns::bishop_magics_,
      magic_details_ns::bishop_masks_,
      magic_details_ns::bishop_moves_);
  }
} // magic_ns

} // NEngine
