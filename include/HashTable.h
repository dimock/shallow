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
             singular_: 1,
             pv_      : 1;
  };
  uint16     mask_{};
  };

  uint16     movesCount_{};
  Move       move_;

  HItem() {}

  inline HItem(uint64 hkey, ScoreType score, int depth, uint16 flag, const Move & move, bool threat, bool singular, bool pv, uint16 movesCount)
  {
    hkey_ = hkey;
    score_ = score;
    depth_ = depth;
    flag_ = flag;
    movesCount_ = movesCount_;
    move_ = move;
    threat_ = threat;
    singular_ = singular;
    pv_ = pv;
  }
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
    _mm_prefetch((char*)&buffer_[code & szMask_], _MM_HINT_NTA);
  }

protected:

  inline ITEM & operator [] (const uint64 & code)
  {
    return buffer_[code & szMask_];
  }

  inline const ITEM & operator [] (const uint64 & code) const
  {
    return buffer_[code & szMask_];
  }

  std::vector<ITEM> buffer_;
  size_t szMask_{0};
  uint16 movesCount_{0};
};

#if 0
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
  using ItemType = HBucket;
  enum Flag : uint16 { NoFlag, Alpha, AlphaBetta, Betta };

  GHashTable(int size) : HashTable<HBucket>(size)
  {}

  void push(const uint64 & hkey, ScoreType score, int depth, Flag flag, const Move & move, bool threat, bool singular, bool pv)
  {
    HBucket & hb = (*this)[hkey];
    HItem * hitem = hb.get(hkey);
    if( !hitem )
      return;

    //if (hitem->hkey_ == hkey && hitem->flag_ != NoFlag && flag == NoFlag && hitem->depth_ < depth) {
    //  return;
    //}

    if ((hitem->hkey_ != hkey) || (depth >= hitem->depth_) ||
        (((AlphaBetta == flag && hitem->flag_ < AlphaBetta) || (pv && !hitem->pv_ && flag != NoFlag)) && depth >= hitem->depth_-2 && depth > 0))
    {
      X_ASSERT(score > 32760, "write wrong value to the hash");

      HItem newItem;
      newItem.hkey_ = hkey;
      newItem.score_ = score;
      newItem.depth_ = depth;
      newItem.flag_ = flag;
      newItem.movesCount_ = movesCount_;
      newItem.move_ = move;
      newItem.threat_ = threat;
      newItem.singular_ = singular;
      newItem.pv_ = pv;

      *hitem = newItem;
    }
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
  using ItemType = HItem;
  enum Flag { NoFlag, Alpha, AlphaBetta, Betta };

  GHashTable(int size) : HashTable<HItem>(size)
  {}

  inline void push(const uint64 & hkey, ScoreType score, int depth, Flag flag, const Move & move, bool threat, bool singular, bool pv)
  {
    auto& hitem = (*this)[hkey];
    if ((hitem.hkey_ != hkey) || (depth >= hitem.depth_) ||
      (((AlphaBetta == flag && hitem.flag_ < AlphaBetta) || (pv && !hitem.pv_ && flag != NoFlag)) && depth >= hitem.depth_ - 2 && depth > 0))
    {
      X_ASSERT(score > 32760, "write wrong value to the hash");
      hitem = HItem{ hkey, score, depth, (uint16)flag, move, threat, singular, pv, movesCount_ };// newItem;
    }
  }

  inline HItem* find(const uint64 & hkey)
  {
    return &(operator [] (hkey));
  }
};
#endif

ALIGN_MSC(8) struct ALIGN_GCC(8) HEval
{
  uint32      hkey_{};
  ScoreType32 score_;
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
