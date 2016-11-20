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
  std::vector<uint64> rook_magic_numbers_;
  std::vector<uint64> bishop_magic_numbers_;
  std::vector<uint64> rook_magic_bits_count_;
  std::vector<uint64> bishop_magic_bits_count_;
  std::vector<size_t> rook_start_index_;
  std::vector<size_t> bishop_start_index_;
  std::vector<uint64> rook_moves_;
  std::vector<uint64> bishop_moves_;
} // magic_details_ns

namespace
{
  int const extra_bits = 1;

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

  std::vector<uint64>
  calculate_magic_numbers(std::function<std::vector<std::vector<uint64>>()> const& calculate_masks,
    std::function<int(int)> const& bits_counter,
    std::function<uint64(int, uint64, bool)> const& moves_from_mask)
  {
    auto all_masks = calculate_masks();

    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<uint64> dist;

    std::vector<uint64> magic_numbers(64);
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
        auto mnum = dist(rgen);
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
          magic_numbers[pos] = mnum;
          std::cout << "magic number for pos " << pos << " found: " << mnum << " ; variants count: " << vcounter << std::endl;
          break;
        }
      }
    }
    std::cout << "total variants count: " << total_counter << std::endl;
    return magic_numbers;
  }

  void fill_masks(std::function<std::vector<std::vector<uint64>>()> const& calculate_masks,
    std::function<int(int)> const& bits_counter,
    std::function<uint64(int, uint64, bool)> const& moves_from_mask,
    std::vector<uint64> const& magic_numbers_i,
    std::vector<size_t>& start_index_o,
    std::vector<uint64>& magic_bits_count_o,
    std::vector<uint64>& masks_o,
    std::vector<uint64>& moves_o)
  {
    start_index_o.resize(64);
    magic_bits_count_o.resize(64);
    masks_o.resize(64);
    moves_o.reserve(64*512);
    auto all_masks = calculate_masks();
    size_t start_index = 0;
    for(int pos = 0; pos < 64; ++pos)
    {
      int bits_count = bits_counter(pos);
      auto const& xmasks = all_masks[pos];
      start_index_o[pos] = start_index;
      magic_bits_count_o[pos] = bits_count;
      masks_o[pos] = moves_from_mask(pos, 0ULL, true);
      for(size_t i = 0; i < xmasks.size(); ++i)
      {
        auto xmsk = xmasks[i];
        auto xmvs = moves_from_mask(pos, xmsk, false);
        auto magic_num = magic_numbers_i[pos];
        size_t index = (xmsk * magic_num) >> (64- bits_count);
        X_ASSERT(index >= (1 << bits_count), "index is too big");
        if(moves_o.size() <= index + start_index)
          moves_o.resize(index + start_index + 1);
        if(moves_o[index + start_index]
          && moves_o[index + start_index] != xmvs)
        {
          X_ASSERT(true, "already exists");
        }
        moves_o[index + start_index] = xmvs;
        X_ASSERT(!moves_o[index + start_index], "null moves mask");
      }
      start_index += 1 << bits_count;
    }
  }

  void verify_magics(std::function<uint64(int, uint64)> const& get_moves,
    std::function<std::vector<std::vector<uint64>>()> const& calculate_masks,
    std::function<uint64(int, uint64, bool)> const& moves_from_mask,
    std::vector<uint64> const& magic_numbers_i,
    std::vector<size_t> const& start_index_i,
    std::vector<uint64> const& magic_bits_count_i,
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
        uint64 board = dist(rgen);
        uint64 xmsk = board & masks_i[pos];
        uint64 xmvs = moves_from_mask(pos, xmsk, false);
        int bits_count = magic_bits_count_i[pos];
        auto mnum = magic_numbers_i[pos];
        int start_index = start_index_i[pos];
        size_t index = (xmsk * mnum) >> (64- bits_count);
        auto xmoves_from_index = moves_i[start_index + index];
        if(xmvs != get_moves(pos, board))
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

  int rook_bits_counter(int pos)
  {
    Index index(pos);
    int num_x = (index.x() == 0 || index.x() == 7) ? 6 : 5;
    int num_y = (index.y() == 0 || index.y() == 7) ? 6 : 5;
    return num_x + num_y + extra_bits;
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
    return bits_count + extra_bits;
  }

} // namespace {}

namespace magic_details_ns
{
  // do only once
  void magic_details_ns::calculate()
  {
    auto t_start = std::chrono::high_resolution_clock::now();

    bool calculate_rooks_magics = true;
    bool calculate_bishops_magics = true;

    if(calculate_rooks_magics)
    {
      magic_details_ns::rook_magic_numbers_ =
        calculate_magic_numbers(calculate_rooks_masks,
        rook_bits_counter,
        rook_moves_from_mask);

      fill_masks(calculate_rooks_masks,
        rook_bits_counter,
        rook_moves_from_mask,
        magic_details_ns::rook_magic_numbers_,
        magic_details_ns::rook_start_index_,
        magic_details_ns::rook_magic_bits_count_,
        magic_details_ns::rook_masks_,
        magic_details_ns::rook_moves_);

      verify_magics([](int pos, uint64 board) { return magic_ns::rook_moves(pos, board); },
        calculate_rooks_masks,
        rook_moves_from_mask,
        magic_details_ns::rook_magic_numbers_,
        magic_details_ns::rook_start_index_,
        magic_details_ns::rook_magic_bits_count_,
        magic_details_ns::rook_masks_,
        magic_details_ns::rook_moves_);
    }

    if(calculate_bishops_magics)
    {
      magic_details_ns::bishop_magic_numbers_ =
        calculate_magic_numbers(calculate_bishops_masks,
        bishop_bits_counter,
        bishop_moves_from_mask);

      fill_masks(calculate_bishops_masks,
        bishop_bits_counter,
        bishop_moves_from_mask,
        magic_details_ns::bishop_magic_numbers_,
        magic_details_ns::bishop_start_index_,
        magic_details_ns::bishop_magic_bits_count_,
        magic_details_ns::bishop_masks_,
        magic_details_ns::bishop_moves_);

      verify_magics([](int pos, uint64 board) { return magic_ns::bishop_moves(pos, board); },
        calculate_bishops_masks,
        bishop_moves_from_mask,
        magic_details_ns::bishop_magic_numbers_,
        magic_details_ns::bishop_start_index_,
        magic_details_ns::bishop_magic_bits_count_,
        magic_details_ns::bishop_masks_,
        magic_details_ns::bishop_moves_);
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start);
    std::cout << "magic numbers took: " << dur.count() << " seconds" << std::endl;

    std::cout << "rooks table contains " << magic_details_ns::rook_moves_.size() << " elements" << std::endl;
    std::cout << "rooks table size is " << magic_details_ns::rook_moves_.size() * sizeof(uint64) << " bytes" << std::endl;
    //save_magic_numbers(magic_details_ns::rook_magic_numbers_, "rook_magics.txt");

    std::cout << "bishops table contains " << magic_details_ns::bishop_moves_.size() << " elements" << std::endl;
    std::cout << "bishops table size is " << magic_details_ns::bishop_moves_.size() * sizeof(uint64) << " bytes" << std::endl;
    //save_magic_numbers(magic_details_ns::bishop_magic_numbers_, "bishop_magics.txt");
  }
} // magic_details_ns

namespace magic_ns
{
  void initialize()
  {
    magic_details_ns::rook_magic_numbers_ =
    { 12298335813956118982, 17133281619298272970, 406918224165331307, 16078580658093089053, 15869625430609868076, 12444794983887358780, 15384301032687407616, 14876488293523228402,
      1863681016179231648, 10205668513559442246, 11100514073586510297, 11582926713541646373, 9248153944044062731, 16007082469306885417, 2020678184101771067, 18201161217504019122,
      747876285534796673, 17781923417723562566, 10260636896267176122, 16025250024678567015, 6744782562619648000, 12420736528101221641, 12198322096721598541, 5504200517354886043,
      1902151856038026751, 10671900124591431785, 12101672227528102042, 13589029379661960470, 5155328549292616768, 6597511772478063616, 12333491930144115671, 15854238111759944582,
      14017016541534486874, 12955776913735092149, 17405682645676067461, 16933055878283987695, 1261971093995284594, 13920945796643100063, 6870858016549340280, 17714354749186242137,
      14956603799383773140, 2980085031808873314, 7000511532611421858, 5282939074697828469, 5514799299884482820, 1310998687551357013, 13256686622421593109, 1681704969678578243,
      10783011539402410432, 3531785210179820147, 14978884886117597744, 17663801457073004542, 38243703810999888, 7354201995498997759, 12594131037466338249, 4832501146654103040,
      7365878415102275130, 12632680743851620454, 14411095916545306874, 681732225953875706, 14131244670764736522, 11949035695658697734, 12024143727801091532, 14352409193325837554
    };

    magic_details_ns::bishop_magic_numbers_ =
    {
      2154904554928305641, 9139149320830582737, 10064234309773979648, 1498586696984861395, 7500781995101000517, 8500139862115875231, 902305784613995291, 13560264734996091381,
      6420299324013233963, 7001961858572390754, 2255447180714362711, 9034069165027648810, 9493695190374944889, 6927715205595334291, 5405505419139247776, 13202981585276948540,
      14517510535077439478, 16210755394945900528, 7318731131146897249, 8770676352629404976, 11322133324513812068, 11165849670032120276, 15982349871200018847, 12671871205763920150,
      13790576234250695592, 1573456091092649572, 7391111300290485556, 1499619519040357422, 10074444565315980279, 15970974917864473537, 14501454556013022507, 10365422700508056212,
      16268491803628788031, 766103300021418328, 11749384417675279608, 10814830456632770502, 3865101125924158976, 17635496042619338785, 7897404579933970904, 14258283400907538858,
      6994374454075115273, 16968108490905897226, 798249424787078411, 12962417393591401734, 17974967369325951934, 5443726398088012336, 3912929548201020342, 2205584053949318405,
      5144212957512788900, 18048658682516825980, 8842071446678315297, 17978149283268878754, 2493718505378496860, 9521293632353888045, 17417159455762441478, 13024039268031204209,
      3205396220667670549, 14789965425224635437, 14287796388602237031, 2375481232804480695, 7991808352830994755, 7809742222275009873, 11649712510646626714, 4296685157770897698
    };

    fill_masks(calculate_rooks_masks,
      rook_bits_counter,
      rook_moves_from_mask,
      magic_details_ns::rook_magic_numbers_,
      magic_details_ns::rook_start_index_,
      magic_details_ns::rook_magic_bits_count_,
      magic_details_ns::rook_masks_,
      magic_details_ns::rook_moves_);
    
    fill_masks(calculate_bishops_masks,
      bishop_bits_counter,
      bishop_moves_from_mask,
      magic_details_ns::bishop_magic_numbers_,
      magic_details_ns::bishop_start_index_,
      magic_details_ns::bishop_magic_bits_count_,
      magic_details_ns::bishop_masks_,
      magic_details_ns::bishop_moves_);
  }
} // magic_ns

} // NEngine
