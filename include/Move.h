/*************************************************************
  Move.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#pragma once

#include <xcommon.h>
#include <Field.h>

namespace NEngine
{

#pragma pack (push, 1)

ALIGN_MSC(1) struct ALIGN_GCC(1) PackedMove
{
  PackedMove() : mask_(0)
  {}

  union
  {
    struct
    {
      uint16 from_ : 6,
             to_ : 6,
             new_type_ : 4;
    };
    uint16 mask_;
  };

  operator bool() const
  {
    return from_ || to_;
  }

  void clear()
  {
    mask_ = 0;
  }

  bool operator == (const PackedMove & other) const
  {
    return *reinterpret_cast<const uint16*>(this) == *reinterpret_cast<const uint16*>(&other);
  }

  bool operator != (const PackedMove & other) const
  {
    return *reinterpret_cast<const uint16*>(this) != *reinterpret_cast<const uint16*>(&other);
  }
};

/*! basic move structure.
    contains minimal necessary information it could be stored in hash etc.
    */
ALIGN_MSC(1) struct ALIGN_GCC(1) Move
{
#ifndef NDEBUG
  // make all values invalid
  Move() : from_(-1), to_(-1), new_type_(10), vsort_(0), flags_(-1)
  {}
#else
  Move() {}
#endif

  Move(int) { clear(); }

  Move(int8 from, int8 to, Figure::Type new_type, bool capture)
  {
    set(from, to, new_type, capture);
  }

  /// index of field go from
  int8 from_;

  /// index of field go to
  int8 to_;

  /// new type while pawn promotion
  int8 new_type_;

  /// flags
  union
  {
    struct
    {
      uint8
      checkVerified_ : 1,
      capture_ : 1,
      checkFlag_ : 1,
      threat_ : 1,
      see_good_ : 1,
      discoveredCheck_ : 1,
      seen_ : 1;
    };

    uint8  flags_;
  };

  unsigned vsort_;

  inline void clear()
  {
    from_ = -1;
    to_ = -1;
    new_type_ = 0;
    vsort_ = 0;
    flags_ = 0;
  }

  inline void clearFlags()
  {
    flags_ = 0;
  }

  inline void set(int from, int to, Figure::Type new_type, bool capture)
  {
    clearFlags();

    from_ = from;
    to_ = to;
    new_type_ = new_type;
    vsort_ = 0;
    capture_ = capture;
    checkVerified_ = false;
  }

  inline operator bool() const
  {
    return to_ >= 0;
  }

  // compare only first 3 bytes
  inline bool operator == (const Move & other) const
  {
    return *reinterpret_cast<const uint16*>(this) == *reinterpret_cast<const uint16*>(&other) &&
      this->new_type_ == other.new_type_;
  }

  // compare only first 3 bytes
  inline bool operator != (const Move & other) const
  {
    return *reinterpret_cast<const uint16*>(this) != *reinterpret_cast<const uint16*>(&other) ||
      this->new_type_ != other.new_type_;
  }

  inline bool operator < (const Move & other) const
  {
    return vsort_ > other.vsort_;
  }
};

/// complete move structure with all information, required for undo
ALIGN_MSC(1) struct ALIGN_GCC(1) UndoInfo : public Move
{
  UndoInfo() {}

  UndoInfo(const Move & move) : Move(move)
  {}

  UndoInfo & operator = (const Move & move)
  {
    *((Move*)this) = move;
    return *this;
  }


  /// Zobrist key - used in fifty-move-rule detector
  uint64 zcode_;
  uint64 zcode_old_;
  uint64 zcode_pawn_;

  /// figures masks
  BitMask mask_[2];

  /// if this move is irreversible, we don't need to enumerate any move in fifty-move-rule detector
  bool  irreversible_;

  /// state of board after move
  uint8 state_;

  /// en-passant position
  int8 en_passant_;

  /// type of eaten figure to restore it in undo
  int8 eaten_type_;

  /// restore old board state in undo
  int8 old_state_;

  /// number of checking figures
  uint8 checkingNum_;

  union
  {
    /// checking figures positions
    uint8  checking_[2];
    uint16 checking_figs_;
  };

  uint8 castling_;

  /// fifty moves rule
  uint8 fifty_moves_;

  /// repetitions counter
  uint8 reps_counter_;

  bool can_win_[2];

  /// reduced || extended flags
  bool reduced_;
  bool extended_;
  bool castle_;

  /// performs clear operation before doing movement. it clears only undo info
  void clearUndo()
  {
    en_passant_ = -1;
    eaten_type_ = 0;
    old_state_ = 0;
    state_ = 0;
    reduced_ = false;
    extended_ = false;
    castle_ = false;
    checkingNum_ = 0;
  }
};

#pragma pack (pop)

} // NEngine
