/*************************************************************
  HashTable.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Move.h>
#include <fstream>

namespace NEngine
{

enum Flag : uint8 { NoFlag, Alpha, AlphaBetta, Betta };

#pragma pack(push, 1)
ALIGN_MSC(2) struct ALIGN_GCC(2) HItem
{
  operator bool () const { return hkey_ != 0; }

  HKeyType   hkey_{};
  ScoreType  score_{};

  int16      depth_   : 11,
             xflag_    : 2,
             threat_  : 1,
             singular_: 1,
             pv_      : 1;

  ScoreType  eval_{ -ScoreMax };
  Move       move_;

  HItem() {}

  inline HItem(HKeyType hkey, ScoreType score, int depth, Flag xflag, const Move & move, bool threat, bool singular, bool pv)
  {
    hkey_ = hkey;
    score_ = score;
    depth_ = depth;
    xflag_ = xflag;
    move_ = move;
    threat_ = threat;
    singular_ = singular;
    pv_ = pv;
  }

  Flag flag() const
  {
    return (Flag)(((uint8)xflag_) & 3);
  }
};
#pragma pack(pop)

template <class ITEM>
class HashTable
{

public:

  HashTable(int size)
  {
    resize(size);
  }

  void resize(size_t sz)
  {
    if(sz == 0)
      return;
    X_ASSERT(sz > 24, "hash table size is too large");
    buffer_.resize((size_t)1 << sz);
    szMask_ = size() - 1;
    movesCount_ = 0;
  }

  void clear()
  {
    movesCount_ = 0;
    std::fill(buffer_.begin(), buffer_.end(), ITEM{});
  }

  inline size_t size() const
  {
    return buffer_.size();
  }

  inline bool empty() const
  {
    return buffer_.empty();
  }

  inline void inc()
  {
    movesCount_++;
  }

  inline int moveCount() const
  {
    return movesCount_;
  }

  bool load(std::string const& fname)
  {
    movesCount_ = 0;
    std::ifstream ifs(fname, std::ifstream::binary);
    if(!ifs)
      return false;
    ifs.seekg(0, ifs.end);
    size_t length = ifs.tellg() / sizeof(ITEM);
    ifs.seekg(0, ifs.beg);
    auto sz = log2(length);
    if(sz == 0)
      return false;
    X_ASSERT( (((size_t)1) << sz) != length, "invalid hash size detected");
    resize(sz);
    ifs.read(reinterpret_cast<char*>(buffer_.data()), size()*sizeof(ITEM));
    return (bool)ifs;
  }

  bool save(std::string const& fname) const
  {
    if(buffer_.empty())
      return false;
    std::ofstream ofs(fname, std::ofstream::binary);
    if(!ofs)
      return false;
    ofs.write(reinterpret_cast<char const*>(buffer_.data()), size()*sizeof(ITEM));
    return (bool)ofs;
  }

  inline void prefetch(const uint64 & code)
  {
    _mm_prefetch((char*)&buffer_[code & szMask_], _MM_HINT_T0);
  }

protected:

  inline ITEM & operator [] (const uint64 & code)
  {
    return buffer_[code & szMask_];
  }

  std::vector<ITEM> buffer_;
  size_t szMask_{0};
  uint16 movesCount_{0};
};

class GHashTable : public HashTable<HItem>
{
public:
  using ItemType = HItem;

  GHashTable(int size) : HashTable<HItem>(size)
  {}

  inline HItem* get(const uint64 & hkey)
  {
    return &(operator [] (hkey));
  }
};

ALIGN_MSC(8) struct ALIGN_GCC(8) PHEval
{
  uint32      hkey_{};
  ScoreType32 score_;
  BitMask     passers_;
};

class PHashTable : public HashTable<PHEval>
{
public:

  PHashTable(int size) : HashTable<PHEval>(size)
  {}

  inline PHEval* get(const uint64 & code)
  {
    return &(operator [] (code));
  }
};


ALIGN_MSC(8) struct ALIGN_GCC(8) FHEval
{
  uint32      hkey_{};
  ScoreType32 score_;
};

class FHashTable : public HashTable<FHEval>
{
public:

  FHashTable(int size) : HashTable<FHEval>(size)
  {}

  inline FHEval* get(const uint64 & code)
  {
    return &(operator [] (code));
  }
};

} // NEngine
