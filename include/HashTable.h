/*************************************************************
  HashTable.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Board.h>

namespace NEngine
{

ALIGN_MSC(16) struct ALIGN_GCC(16) HItem
{
  HItem() : hkey_(0), score_(0), mask_(0), movesCount_(0)
  {}

  operator bool () const { return hkey_ != 0; }

  void clear()
  {
    move_.clear();
    mask_ = 0;
    score_ = 0;
  }

  uint64     hkey_;
  ScoreType  score_;

  union {

  struct
  {
  uint16     depth_  : 6,
             flag_   : 2,
             threat_ : 1;
  };

  uint16     mask_;
  };

  uint8      movesCount_;

  PackedMove move_;
};

ALIGN_MSC(16) struct ALIGN_GCC(16) HBucket
{
  static const int BucketSize = 4;

  const HItem * find(const uint64 & hkey) const
  {
    for (int i = 0; i < BucketSize; ++i)
    {
      if ( items_[i].hkey_ == hkey )
        return items_ + i;
    }
    return 0;
  }

  HItem * get(const uint64 & hkey)
  {
    HItem * hfar = 0;
    for (int i = 0; i < BucketSize; ++i)
    {
      if ( !items_[i].hkey_ || items_[i].hkey_ == hkey )
        return items_ + i;
      
      if ( !hfar || (int)items_[i].movesCount_ < hfar->movesCount_ )
        hfar = items_ + i;
    }

    return hfar;
  }

  HItem items_[BucketSize];
};

template <class ITEM>
class HashTable
{

public:

  HashTable(int size) : buffer_(0)
  {
    resize(size);
  }

  ~HashTable()
  {
    delete [] buffer_;
  }

  void resize(size_t sz)
  {
    delete [] buffer_;
    buffer_ = 0;
    szMask_ = 0;
    size_ = sz;
    movesCount_ = 0;

    X_ASSERT(sz > 24, "hash table size if too large");

    buffer_ = new ITEM[size()];
    szMask_ = size() - 1;
  }

  void clear()
  {
    size_t n = size();
    if ( n == 0 )
      return;

    ITEM * buf = buffer_;
    for (size_t i = 0; i < n; ++i, ++buf)
    {
      *buf = ITEM();
    }
  }

  size_t size() const
  {
    return (((size_t)1) << size_);
  }

  void inc()
  {
    movesCount_++;
  }

  bool load(const char * fname)
  {
    FILE * f = fopen(fname, "rb");
    if ( !f )
      return false;
    size_t n = 0;
    if ( fread(&size_, sizeof(size_), 1, f) == 1 && size_ > 0 && size_ <= 24 )
    {
      delete [] buffer_;
      buffer_ = new ITEM[size()];
      szMask_ = size() - 1;
      n = fread(buffer_, sizeof(ITEM), size(), f);
    }
    fclose(f);
    return n == size();
  }

  bool save(const char * fname) const
  {
    FILE * f = fopen(fname, "wb");
    if ( !f )
      return false;
    size_t n = 0;
    if ( fwrite(&size_, sizeof(size_), 1, f) == 1 )
    {
      n = fwrite(buffer_, sizeof(ITEM), size(), f);
    }
    fclose(f);
    return n == size();
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

  ITEM * buffer_;
  size_t size_;
  size_t szMask_;
  uint8 movesCount_;
};

class GHashTable : public HashTable<HBucket>
{
public:

  enum Flag { NoFlag, Alpha, AlphaBetta, Betta };

  GHashTable(int size) : HashTable<HBucket>(size)
  {}

  void push(const uint64 & hkey, ScoreType score, int depth, Flag flag, const PackedMove & move, bool threat)
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

    hitem->hkey_   = hkey;
    hitem->score_  = score;
    hitem->depth_  = depth;
    hitem->flag_   = flag;
    hitem->threat_ = threat;
    hitem->movesCount_ = movesCount_;
    hitem->move_ = move;
  }

  const HItem * find(const uint64 & hkey) const
  {
    const HBucket & hb = this->operator [] (hkey);
    const HItem * hitem = hb.find(hkey);
    return hitem;
  }
};

ALIGN_MSC(8) struct ALIGN_GCC(8) HEval
{
  HEval() : hkey_(0), mask_(0) {}

  uint32     hkey_;

  union
  {
  struct
  {
    int32  pwscore_  : 10,
           pwscore_eg_ : 10,
           score_ps_ : 10,
           initizalized_ : 1;
  };

  uint32    mask_;
  };
};

class EHashTable : public HashTable<HEval>
{
public:

  EHashTable(int size) : HashTable<HEval>(size)
  {}

  HEval * get(const uint64 & code)
  {
    HEval & heval = operator [] (code);
    return &heval;
  }
};

} // NEngine
