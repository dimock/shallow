/*************************************************************
  HashTable.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Move.h>
#include <fstream>

namespace NEngine
{

ALIGN_MSC(16) struct ALIGN_GCC(16) HItem
{
  operator bool () const { return hkey_ != 0; }

  uint64     hkey_{};
  ScoreType  score_{};

  union {

  struct
  {
  uint16     depth_   : 6,
             flag_    : 2,
             threat_  : 1,
             singular_: 1;
  };
  uint16     mask_{};
  };

  uint16     movesCount_{};
  Move       move_;
};


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

  size_t size() const
  {
    return buffer_.size();
  }

  bool empty() const
  {
    return buffer_.empty();
  }

  void inc()
  {
    movesCount_++;
  }

  int moveCount() const
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

  void prefetch(const uint64 & code)
  {
    _mm_prefetch((char*)&buffer_[code & szMask_], _MM_HINT_NTA);
  }

protected:

  ITEM & operator [] (const uint64 & code)
  {
    return buffer_[code & szMask_];
  }

  const ITEM & operator [] (const uint64 & code) const
  {
    return buffer_[code & szMask_];
  }

  std::vector<ITEM> buffer_;
  size_t szMask_{0};
  uint16 movesCount_{0};
};

#if 1
ALIGN_MSC(16) struct ALIGN_GCC(16) HBucket
{
  static const int BucketSize = 4;

  const HItem * find(const uint64 & hkey) const
  {
    for (int i = 0; i < BucketSize; ++i)
    {
      if (items_[i].hkey_ == hkey)
        return items_ + i;
    }
    return 0;
  }

  HItem* get(const uint64 & hkey)
  {
    HItem * hfar = 0;
    for (int i = 0; i < BucketSize; ++i)
    {
      if (!items_[i].hkey_ || items_[i].hkey_ == hkey)
        return items_ + i;
      
      if (!hfar || items_[i].movesCount_ < hfar->movesCount_)
        hfar = items_ + i;
    }

    return hfar;
  }

  HItem items_[BucketSize];
};

class GHashTable : public HashTable<HBucket>
{
public:

  enum Flag { NoFlag, Alpha, AlphaBetta, Betta };

  GHashTable(int size) : HashTable<HBucket>(size)
  {}

  void push(const uint64 & hkey, ScoreType score, int depth, Flag flag, const Move & move, bool threat, bool singular)
  {
    HBucket & hb = (*this)[hkey];
    HItem * hitem = hb.get(hkey);
    if( !hitem )
      return;

    if ( (hitem->hkey_ == hkey) &&
          ( (hitem->depth_ > depth) || 
            (depth == hitem->depth_ && Alpha == flag && hitem->flag_ > Alpha) ||
            (depth == hitem->depth_ && Alpha == flag && hitem->flag_ == Alpha && score >= hitem->score_) ) )
    {
      return;
    }

    X_ASSERT(score > 32760, "wrong value to hash");

    HItem newItem;
    newItem.hkey_   = hkey;
    newItem.score_  = score;
    newItem.depth_  = depth;
    newItem.flag_   = flag;
    newItem.movesCount_ = movesCount_;
    newItem.move_   = move;
    newItem.threat_ = threat;
    newItem.singular_ = singular;

    *hitem = newItem;
  }

  const HItem * find(const uint64 & hkey) const
  {
    const HBucket & hb = this->operator [] (hkey);
    const HItem * hitem = hb.find(hkey);
    return hitem;
  }
};

#else

class GHashTable : public HashTable<HItem>
{
public:

  enum Flag { NoFlag, Alpha, AlphaBetta, Betta };

  GHashTable(int size) : HashTable<HItem>(size)
  {}

  void push(const uint64 & hkey, ScoreType score, int depth, Flag flag, const Move & move)
  {
    auto& hitem = (*this)[hkey];
    if((hitem.hkey_ == hkey) &&
       ((hitem.depth_ > depth) ||
       (depth == hitem.depth_ && Alpha == flag && hitem.flag_ > Alpha) ||
       (depth == hitem.depth_ && Alpha == flag && hitem.flag_ == Alpha && score >= hitem.score_)))
    {
      return;
    }

    X_ASSERT(score > 32760, "wrong value to hash");

    hitem.hkey_   = hkey;
    hitem.score_  = score;
    hitem.depth_  = depth;
    hitem.flag_   = flag;
    hitem.move_   = move;
  }

  HItem const* find(const uint64 & hkey) const
  {
    auto const& hitem = operator [] (hkey);
    return hitem.hkey_ == hkey ? &hitem : nullptr;
  }
};
#endif

ALIGN_MSC(8) struct ALIGN_GCC(8) HEval
{
  HEval() : hkey_(0), mask_(0) {}

  uint32     hkey_;

  union
  {
  struct
  {
    int32  common_  : 10,
           opening_ : 10,
           endGame_ : 10,
           initizalized_ : 1;
  };

  uint32   mask_;
  };
};

class EHashTable : public HashTable<HEval>
{
public:

  EHashTable(int size) : HashTable<HEval>(size)
  {}

  HEval* get(const uint64 & code)
  {
    return &(operator [] (code));
  }
};

} // NEngine
