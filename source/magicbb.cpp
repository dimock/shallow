/*************************************************************
magicbb.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include "magicbb.h"
#include "xbitmath.h"
#include "xindex.h"
#include "fpos.h"
#include "iostream"
#include "random"
#include "chrono"
#include "fstream"
#include "array"
#include "functional"

namespace NEngine
{

namespace magic_details_ns
{
  magic_struct_rook*   rook_magics_p;
  magic_struct_bishop* bishop_magics_p;
 
} // magic_details_ns

namespace
{
  std::vector<uint8> rook_moves_arr;
  std::vector<uint8> bishop_moves_arr;
  std::vector<uint8> rook_magics_arr;
  std::vector<uint8> bishop_magics_arr;

  template <class mstruct>
  mstruct* initialize_magics(std::vector<uint8>& magics_arr, std::vector<mstruct> const& magic_src)
  {
    auto* magics = make_aligned_array<mstruct>(magics_arr, 64);
    for(size_t i = 0; i < magic_src.size(); ++i)
    {
      magics[i] = magic_src[i];
    }
    return magics;
  }

  template <class mstruct>
  uint64* initialize_moves(std::vector<uint8>& moves_arr)
  {
    return make_aligned_array<BitMask>(moves_arr, 64 * (1 << mstruct::bits_count));
  }

  BitMask rook_moves_from_blockers(int pos, BitMask mask_r, bool exclude_border)
  {
    Index index(pos);
    BitMask moves_x = 0;
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
    BitMask moves_y = 0;
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

  BitMask bishop_moves_from_blockers(int pos, BitMask mask_b, bool exclude_border)
  {
    std::array<FPos, 4> deltas =
    {
      FPos{1, 1},
      FPos{ 1, -1 },
      FPos{ -1, 1 },
      FPos{ -1, -1 }
    };
    BitMask mask = 0;
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
        BitMask m = 1ULL << current.index();
        mask |= m;
        if(m & mask_b)
          break;
      }
    }
    return mask;
  }

  std::vector<std::vector<BitMask>>
  calculate_rooks_blockers()
  {
    std::vector<std::vector<BitMask>> rook_all_masks_(64);
    for(int pos = 0; pos < 64; ++pos)
    {
      rook_all_masks_[pos].reserve(4096);
      Index index(pos);
      int num_x = (index.x() == 0 || index.x() == 7) ? 6 : 5;
      int num_y = (index.y() == 0 || index.y() == 7) ? 6 : 5;
      for(int bits_x = 0; bits_x < (1<<num_x); ++bits_x)
      {
        BitMask mask_x = 0;
        for(int i = 0, x = 1; i < num_x && x < 7; ++x)
        {
          if(x == index.x())
            continue;
          if((1 << i) & bits_x)
            mask_x |= 1ULL << x;
          ++i;
        }
        for(int bits_y = 0; bits_y < (1<<num_y); ++bits_y)
        {
          BitMask mask_y = 0;
          for(int i = 0, y = 1; i < num_y && y < 7; ++y)
          {
            if(y == index.y())
              continue;
            if((1 << i) & bits_y)
              mask_y |= 1ULL << y*8;
            ++i;
          }
          BitMask mask_r = (mask_x << (index.y()*8)) | (mask_y << index.x());
          rook_all_masks_[pos].push_back(mask_r);
        }
      }
    }

    return rook_all_masks_;
  }

  std::vector<std::vector<BitMask>>
  calculate_bishops_blockers()
  {
    std::vector<std::vector<BitMask>> bishop_all_masks_(64);
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
        BitMask mask = 0;
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

  template <class mstruct>
  std::vector<mstruct>
  calculate_magic_numbers(std::function<std::vector<std::vector<BitMask>>()> const& calculate_blockers,
    std::function<BitMask(int, BitMask, bool)> const& moves_from_blockers)
  {
    auto blockers = calculate_blockers();

    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<BitMask> dist;

    std::vector<mstruct> magics(64);
    int total_counter = 0;
    for(int pos = 0; pos < 64; ++pos)
    {
      auto const& xblks = blockers[pos];
      Index index(pos);
      //std::cout << "position (" << index.x() << ", " << index.y() << ") needs " << xmasks.size() << " indices" << std::endl;
      for(size_t vcounter = 0;; ++vcounter, ++total_counter)
      {
        std::vector<BitMask> moves(1 << mstruct::bits_count);
        int count = 0;
        auto mnum = dist(rgen) & dist(rgen) & dist(rgen);
        for(auto const blk : xblks)
        {
          auto xmvs = moves_from_blockers(pos, blk, false);
          X_ASSERT(xmvs == 0ULL, "no moves");
          size_t index = (blk * mnum) >> mstruct::bits_shift;
          X_ASSERT(index >= moves.size(), "magic index is too big");
          if(moves[index] && moves[index] != xmvs)
            break;
          moves[index] = xmvs;
          count++;
        }
        if(count == xblks.size())
        {
          magics[pos] = { mnum, 0ULL, nullptr };
          std::cout << "magic number for pos " << pos << " found: " << mnum << " ; variants count: " << vcounter << std::endl;
          break;
        }
      }
    }
    std::cout << "total variants count: " << total_counter << std::endl;
    return magics;
  }

  template <class mstruct>  
  void fill_magics(std::function<std::vector<std::vector<BitMask>>()> const& calculate_blockers,
    std::function<BitMask(int, BitMask, bool)> const& moves_from_blockers,
    mstruct* magics_io,
    BitMask* moves_o)
  {
    auto blockers = calculate_blockers();
    BitMask* moves_ptr = moves_o;
    for(int pos = 0; pos < 64; ++pos)
    {
      auto& ms = magics_io[pos];
      ms.mask = moves_from_blockers(pos, 0ULL, true);
      ms.moves = moves_ptr;
      moves_ptr += 1 << mstruct::bits_count;
      auto const& xblks = blockers[pos];
      for(auto const blk : xblks)
      {
        auto xmvs = moves_from_blockers(pos, blk, false);
        int index = ms.index(blk);
        X_ASSERT(ms.moves[index] && ms.moves[index] != xmvs, "magics collision detected");
        ms.moves[index] = xmvs;
        X_ASSERT(!ms.moves[index], "zero moves mask");
      }
    }
  }

  template <class mstruct>
  void verify_magics(std::function<BitMask(int, BitMask)> const& get_moves,
    std::function<std::vector<std::vector<BitMask>>()> const& calculate_blockers,
    std::function<BitMask(int, BitMask, bool)> const& moves_from_blockers,
    mstruct const* magics_i)
  {
    std::random_device rd;
    std::mt19937_64 rgen(rd());
    std::uniform_int_distribution<BitMask> dist;

    auto blockers = calculate_blockers();
    bool verified_ok = true;

    for(int pos = 0; verified_ok && pos < 64; ++pos)
    {
      for(int i = 0; verified_ok && i < 100000; ++i)
      {
        auto const& ms = magics_i[pos];
        BitMask board = dist(rgen);
        BitMask blk = board & ms.mask;
        BitMask xmvs = moves_from_blockers(pos, blk, false);
        if(xmvs != get_moves(pos, board))
        {
          verified_ok = false;
          auto const& xblks = blockers[pos];
          if(std::find_if(xblks.begin(), xblks.end(), [blk](BitMask const b) { return b == blk; })
            == xblks.end())
          {
            std::cout << "blocker not found in generated array:" << std::endl;
          }
          std::cout << "invalid magic for board:" << std::endl;
          print_bitmask(board);
          std::cout << "blocker:" << std::endl;
          print_bitmask(blk);
          std::cout << "moves:" << std::endl;
          print_bitmask(xmvs);
          std::cout << "magic moves:" << std::endl;
          print_bitmask(get_moves(pos, board));
          std::cout << "possible moves:" << std::endl;
          print_bitmask(ms.mask);
          break;
        }
      }
    }

    for(int i = 0; i < 3; ++i)
    {
      BitMask board = dist(rgen);
      int pos = dist(rgen) & 63;
      auto xmoves = get_moves(pos, board);
      std::cout << "for board:" << std::endl;
      print_bitmask(board);
      std::cout << "moves found" << std::endl;
      print_bitmask(xmoves);
      std::cout << std::endl;
    }

    if(verified_ok)
    {
      std::cout << "verified ok" << std::endl;
    }
  }

  template <class mstruct>
  void save_magic_numbers(std::vector<mstruct> const& magics, std::string const& fname)
  {
    std::ofstream ofs(fname);
    int n = 0;
    for(auto const& ms : magics)
    {
      ofs << "{" << ms.magic_number << "ULL, 0ULL, nullptr}, ";
      if(++n >= 4)
      {
        ofs << std::endl;
        n = 0;
      }
    }
  }
} // namespace {}

namespace magic_details_ns
{
  // do only once
  void calculate()
  {
    auto t_start = std::chrono::high_resolution_clock::now();

    auto rook_magics =
    calculate_magic_numbers<magic_details_ns::magic_struct_rook>(
      calculate_rooks_blockers,
      rook_moves_from_blockers);

    magic_details_ns::rook_magics_p = initialize_magics<magic_details_ns::magic_struct_rook>(rook_magics_arr, rook_magics);
    auto* rook_moves_ptr = initialize_moves<magic_details_ns::magic_struct_rook>(rook_moves_arr);

    fill_magics<magic_details_ns::magic_struct_rook>(calculate_rooks_blockers,
      rook_moves_from_blockers,
      magic_details_ns::rook_magics_p,
      rook_moves_ptr);

    verify_magics([](int pos, BitMask board) { return magic_ns::rook_moves(pos, board); },
      calculate_rooks_blockers,
      rook_moves_from_blockers,
      magic_details_ns::rook_magics_p);

    auto bishop_magics =
    calculate_magic_numbers<magic_details_ns::magic_struct_bishop>(
      calculate_bishops_blockers,
      bishop_moves_from_blockers);

    magic_details_ns::bishop_magics_p = initialize_magics<magic_details_ns::magic_struct_bishop>(bishop_magics_arr, bishop_magics);
    auto* bishop_moves_ptr = initialize_moves<magic_details_ns::magic_struct_bishop>(bishop_moves_arr);

    fill_magics<magic_details_ns::magic_struct_bishop>(calculate_bishops_blockers,
      bishop_moves_from_blockers,
      magic_details_ns::bishop_magics_p,
      bishop_moves_ptr);

    verify_magics([](int pos, BitMask board) { return magic_ns::bishop_moves(pos, board); },
      calculate_bishops_blockers,
      bishop_moves_from_blockers,
      magic_details_ns::bishop_magics_p);

    auto t_end = std::chrono::high_resolution_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start);
    std::cout << "magic numbers took: " << dur.count() << " seconds" << std::endl;

    std::cout << "rooks table size is " << rook_moves_arr.size() << " bytes" << std::endl;
    save_magic_numbers<magic_details_ns::magic_struct_rook>(rook_magics, "rook_magics.txt");

    std::cout << "bishops table size is " << bishop_moves_arr.size() << " bytes" << std::endl;
    save_magic_numbers<magic_details_ns::magic_struct_bishop>(bishop_magics, "bishop_magics.txt");
  }
} // magic_details_ns

namespace magic_ns
{
  void initialize()
  {
    std::vector<magic_details_ns::magic_struct_rook> rook_src =
    {
      {180146253911457824ULL, 0ULL, nullptr}, {1170975488756158848ULL, 0ULL, nullptr}, {36033263843860480ULL, 0ULL, nullptr}, {36033263793864834ULL, 0ULL, nullptr}, 
      {1155174403999285272ULL, 0ULL, nullptr}, {72066407311474833ULL, 0ULL, nullptr}, {1450168443058978912ULL, 0ULL, nullptr}, {144118765793092100ULL, 0ULL, nullptr}, 
      {20336575927812160ULL, 0ULL, nullptr}, {9223451202096730368ULL, 0ULL, nullptr}, {152000889900503040ULL, 0ULL, nullptr}, {1153629590229356548ULL, 0ULL, nullptr}, 
      {37510939095860484ULL, 0ULL, nullptr}, {4620904413572136974ULL, 0ULL, nullptr}, {81205532400144387ULL, 0ULL, nullptr}, {144159169631487104ULL, 0ULL, nullptr}, 
      {585469085431832704ULL, 0ULL, nullptr}, {443042171693078784ULL, 0ULL, nullptr}, {4818918190476689425ULL, 0ULL, nullptr}, {4611688217518407717ULL, 0ULL, nullptr}, 
      {4611705809653596296ULL, 0ULL, nullptr}, {144397350314491905ULL, 0ULL, nullptr}, {1134833463988288ULL, 0ULL, nullptr}, {4611686156038377600ULL, 0ULL, nullptr}, 
      {4629929115498061824ULL, 0ULL, nullptr}, {2306142780784673792ULL, 0ULL, nullptr}, {567623012059149ULL, 0ULL, nullptr}, {10377999985655687232ULL, 0ULL, nullptr}, 
      {1174318001994989632ULL, 0ULL, nullptr}, {864704324743236096ULL, 0ULL, nullptr}, {18104566247882756ULL, 0ULL, nullptr}, {4612531130956005632ULL, 0ULL, nullptr}, 
      {2344405358317930496ULL, 0ULL, nullptr}, {4613940103571046416ULL, 0ULL, nullptr}, {2310346763461984264ULL, 0ULL, nullptr}, {577024252147400960ULL, 0ULL, nullptr}, 
      {9223662445967704096ULL, 0ULL, nullptr}, {1188950851467612161ULL, 0ULL, nullptr}, {216175050393520192ULL, 0ULL, nullptr}, {3026700699483246600ULL, 0ULL, nullptr}, 
      {2310487484852109312ULL, 0ULL, nullptr}, {1172063454441246977ULL, 0ULL, nullptr}, {216181581495148546ULL, 0ULL, nullptr}, {293930811399602694ULL, 0ULL, nullptr}, 
      {2305847441720623232ULL, 0ULL, nullptr}, {1730510906670452752ULL, 0ULL, nullptr}, {598989747972734977ULL, 0ULL, nullptr}, {138648125441ULL, 0ULL, nullptr}, 
      {18594997022494752ULL, 0ULL, nullptr}, {4621537676989435972ULL, 0ULL, nullptr}, {290491543964629024ULL, 0ULL, nullptr}, {87971667775617ULL, 0ULL, nullptr}, 
      {77688468834615360ULL, 0ULL, nullptr}, {9259418563632989456ULL, 0ULL, nullptr}, {9295447227474510530ULL, 0ULL, nullptr}, {4616498590571827328ULL, 0ULL, nullptr}, 
      {1266637935231233ULL, 0ULL, nullptr}, {6938078398526786145ULL, 0ULL, nullptr}, {9024826337463553ULL, 0ULL, nullptr}, {9223381108094427138ULL, 0ULL, nullptr}, 
      {11818017305723405314ULL, 0ULL, nullptr}, {360749373587457ULL, 0ULL, nullptr}, {4611976291845996610ULL, 0ULL, nullptr}, {288869243949188290ULL, 0ULL, nullptr} 
    };

    std::vector<magic_details_ns::magic_struct_bishop> bishop_src =
    {
      {5260346202054002179ULL, 0ULL, nullptr}, {37383673901072ULL, 0ULL, nullptr}, {144396920893739012ULL, 0ULL, nullptr}, {72132955012630528ULL, 0ULL, nullptr}, 
      {20270613557936656ULL, 0ULL, nullptr}, {10139800389584896ULL, 0ULL, nullptr}, {292752154932961920ULL, 0ULL, nullptr}, {13263945365723431168ULL, 0ULL, nullptr}, 
      {72069173538209797ULL, 0ULL, nullptr}, {108161158452674578ULL, 0ULL, nullptr}, {5669377818624ULL, 0ULL, nullptr}, {4576442549600288ULL, 0ULL, nullptr}, 
      {2377936888209081152ULL, 0ULL, nullptr}, {1197994094140327938ULL, 0ULL, nullptr}, {22518292348408324ULL, 0ULL, nullptr}, {4632005027752861696ULL, 0ULL, nullptr}, 
      {4616831803754287617ULL, 0ULL, nullptr}, {584514693239144453ULL, 0ULL, nullptr}, {2306265238992848128ULL, 0ULL, nullptr}, {9296696277953777664ULL, 0ULL, nullptr}, 
      {772939101476360233ULL, 0ULL, nullptr}, {9223943821564445770ULL, 0ULL, nullptr}, {7355812280550912ULL, 0ULL, nullptr}, {4575105867976704ULL, 0ULL, nullptr}, 
      {572021998125072ULL, 0ULL, nullptr}, {45149233123657ULL, 0ULL, nullptr}, {4905554692672590848ULL, 0ULL, nullptr}, {16143443135396069633ULL, 0ULL, nullptr}, 
      {145135811715076ULL, 0ULL, nullptr}, {2252351721197600ULL, 0ULL, nullptr}, {1161359165817856ULL, 0ULL, nullptr}, {291753280267225091ULL, 0ULL, nullptr}, 
      {4612706436219175936ULL, 0ULL, nullptr}, {9259551539981517840ULL, 0ULL, nullptr}, {20409274455032962ULL, 0ULL, nullptr}, {4791673821855872ULL, 0ULL, nullptr}, 
      {54044303630338056ULL, 0ULL, nullptr}, {122169099328487440ULL, 0ULL, nullptr}, {576748963936343041ULL, 0ULL, nullptr}, {20325610605792258ULL, 0ULL, nullptr}, 
      {27039207134594560ULL, 0ULL, nullptr}, {4612249039314911768ULL, 0ULL, nullptr}, {9547638915468100097ULL, 0ULL, nullptr}, {617134711206578322ULL, 0ULL, nullptr}, 
      {4683761480745616384ULL, 0ULL, nullptr}, {9233514207406719040ULL, 0ULL, nullptr}, {9403603983316097280ULL, 0ULL, nullptr}, {308505510158663690ULL, 0ULL, nullptr}, 
      {4611840105479671810ULL, 0ULL, nullptr}, {9232520971105076362ULL, 0ULL, nullptr}, {1134767006302224ULL, 0ULL, nullptr}, {150870871052124160ULL, 0ULL, nullptr}, 
      {3538705332459569152ULL, 0ULL, nullptr}, {6940610559863734312ULL, 0ULL, nullptr}, {2305915304271970432ULL, 0ULL, nullptr}, {1143492770275362ULL, 0ULL, nullptr}, 
      {19000696955475009ULL, 0ULL, nullptr}, {1130315150182464ULL, 0ULL, nullptr}, {9225623864074059809ULL, 0ULL, nullptr}, {563059543016448ULL, 0ULL, nullptr}, 
      {144261424196485664ULL, 0ULL, nullptr}, {1207669489271054392ULL, 0ULL, nullptr}, {4613938266566297716ULL, 0ULL, nullptr}, {4612253523327850528ULL, 0ULL, nullptr} 
    };

    magic_details_ns::rook_magics_p   = initialize_magics<magic_details_ns::magic_struct_rook>(rook_magics_arr, rook_src);
    magic_details_ns::bishop_magics_p = initialize_magics<magic_details_ns::magic_struct_bishop>(bishop_magics_arr, bishop_src);

    auto* rook_moves_ptr   = initialize_moves<magic_details_ns::magic_struct_rook>(rook_moves_arr);
    auto* bishop_moves_ptr = initialize_moves<magic_details_ns::magic_struct_bishop>(bishop_moves_arr);

    fill_magics<magic_details_ns::magic_struct_rook>(calculate_rooks_blockers,
      rook_moves_from_blockers,
      magic_details_ns::rook_magics_p,
      rook_moves_ptr);

//    verify_magics([](int pos, uint64 board) { return magic_ns::rook_moves(pos, board); },
//      calculate_rooks_blockers,
//      rook_moves_from_blockers,
//      magic_details_ns::rook_magics_p);


    fill_magics<magic_details_ns::magic_struct_bishop>(calculate_bishops_blockers,
      bishop_moves_from_blockers,
      magic_details_ns::bishop_magics_p,
      bishop_moves_ptr);

//    verify_magics([](int pos, uint64 board) { return magic_ns::bishop_moves(pos, board); },
//      calculate_bishops_blockers,
//      bishop_moves_from_blockers,
//      magic_details_ns::bishop_magics_p);
  }

} // magic_ns

} // NEngine
