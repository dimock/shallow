/*************************************************************
  Figure.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <xbitmath.h>

namespace NEngine
{

#pragma pack (push, 1)

namespace Figure
{
  enum Weights { DrawScore = 0, MatScore = 32000 };
  enum Type    { TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing, TypesNum };
  enum Color : int8   { ColorBlack, ColorWhite };
  
  extern const BitMask pawnCutoffMasks_[2];
  extern const ScoreType figureWeight_[7]; // TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing
  extern const uint8     mirrorIndex_[64];
  // color, castle (K = 0, Q = 1)
  extern const BitMask   quaterBoard_[2][2];

  inline Figure::Color otherColor(Figure::Color color)
  {
    //return (Figure::Color)((static_cast<int>(color) + 1) & 1);
    return (Figure::Color)(!color);
  }

  const char * name(Type type);

  ScoreType positionEvaluation(int stage, Figure::Color color, Figure::Type type, int pos);

  Figure::Type toFtype(char c);

  char fromFtype(Figure::Type t);
}


ALIGN_MSC(1) class ALIGN_GCC(1) FiguresCounter
{
public:

  static  int8 s_whiteColors_[64];
  static uint8 s_transposeIndex_[64];
  static uint64 s_whiteMask_;

  FiguresCounter()
  {
    clear();
  }

  void clear()
  {
    //count_ = 0;
    weight_ = 0;
    mask_all_ = 0ULL;
    for (int i = 0; i < 8; ++i)
    {
      tcount_[i] = 0;
      tmask_[i]  = 0ULL;
    }
  }

  inline void incr(const Figure::Color c, const Figure::Type t, int p)
  {
    X_ASSERT(t == Figure::TypeNone, "incr for invalid figure type");
    
    tcount_[t]++;
    //count_++;
    weight_ += Figure::figureWeight_[t];

    auto mask_set = set_mask_bit(p);
    tmask_[t] |= mask_set;
    mask_all_ |= mask_set;

    X_ASSERT(weight_ != pawns()*Figure::figureWeight_[Figure::Type::TypePawn] + bishops()*Figure::figureWeight_[Figure::TypeBishop]
             + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook]
             + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
  }

  inline void decr(const Figure::Color c, const Figure::Type t, int p)
  {
    X_ASSERT(t == Figure::TypeNone, "decr for invalid figure type");

    tcount_[t]--;
    //count_--;
    weight_ -= Figure::figureWeight_[t];

    auto mask_clear = set_mask_bit(p);
    tmask_[t] ^= mask_clear;
    mask_all_ ^= mask_clear;

    X_ASSERT(tmask_[t] & set_mask_bit(p), "invalid mask");
    X_ASSERT(weight_ != pawns()*Figure::figureWeight_[Figure::TypePawn] + bishops()*Figure::figureWeight_[Figure::TypeBishop]
             + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook]
             + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
  }

  inline void move(const Figure::Color c, const Figure::Type t, int from, int to)
  {
    auto from_mask = set_mask_bit(from);
    auto to_mask = set_mask_bit(to);
    tmask_[t] ^= from_mask;
    tmask_[t] |= to_mask;
    mask_all_ ^= from_mask;
    mask_all_ |= to_mask;
  }

  inline int tcount(Figure::Type type) const { return tcount_[type]; }
  //inline int count() const { return count_; }
  inline int pawns() const { return tcount_[Figure::TypePawn]; }
  inline int knights() const { return tcount_[Figure::TypeKnight]; }
  inline int bishops() const { return tcount_[Figure::TypeBishop]; }
  inline int rooks() const { return tcount_[Figure::TypeRook]; }
  inline int queens() const { return tcount_[Figure::TypeQueen]; }
  inline ScoreType weight() const { return weight_; }
  inline const BitMask & pawn_mask() const { return tmask_[Figure::TypePawn]; }
  inline const BitMask & knight_mask() const { return tmask_[Figure::TypeKnight]; }
  inline const BitMask & bishop_mask() const { return tmask_[Figure::TypeBishop]; }
  inline const BitMask & rook_mask() const { return tmask_[Figure::TypeRook]; }
  inline const BitMask & queen_mask() const { return tmask_[Figure::TypeQueen]; }
  inline const BitMask & king_mask() const { return tmask_[Figure::TypeKing]; }
  inline const BitMask & type_mask(const Figure::Type type) const { return tmask_[type]; }
  inline const BitMask & mask_all() const { return mask_all_; }

  //// temp. should be removed
  //inline void set_mask_all(BitMask const& m) { mask_all_ = m; }

private:

  BitMask   tmask_[8];
  BitMask   mask_all_;
  uint8     tcount_[8];
  ScoreType weight_;
};

class FiguresManager
{
  static BitMask s_zobristCodes_[64*2*8];
  static BitMask s_zobristColor_;
  static BitMask s_zobristNullmove_;
  static BitMask s_zobristCastle_[2][2];

public:
  void clear()
  {
    hashCode_ = 0ULL;
    kpwnCode_ = 0ULL;
    fcounter_[0].clear();
    fcounter_[1].clear();
    eval32_ = 0;
  }

  inline void incr(const Figure::Color c, const Figure::Type t, int p)
  {
    fcounter_[c].incr(c, t, p);
    const BitMask & uc = code(c, t, p);
    hashCode_ ^= uc;
    if(t == Figure::TypePawn || t == Figure::TypeKing)
      kpwnCode_ ^= uc;
    eval_[0] += Figure::positionEvaluation(0, c, t, p);
    eval_[1] += Figure::positionEvaluation(1, c, t, p);
  }

  inline void decr(const Figure::Color c, const Figure::Type t, int p)
  {
    fcounter_[c].decr(c, t, p);
    const BitMask & uc = code(c, t, p);
    hashCode_ ^= uc;
    if(t == Figure::TypePawn || t == Figure::TypeKing)
      kpwnCode_ ^= uc;
    eval_[0] -= Figure::positionEvaluation(0, c, t, p);
    eval_[1] -= Figure::positionEvaluation(1, c, t, p);
  }

  inline void move(const Figure::Color c, const Figure::Type t, int from, int to)
  {
    fcounter_[c].move(c, t, from, to);

    const BitMask & uc0 = code(c, t, from);
    const BitMask & uc1 = code(c, t, to);
    hashCode_ ^= uc0;
    hashCode_ ^= uc1;

    if(t == Figure::TypePawn || t == Figure::TypeKing)
    {
      kpwnCode_ ^= uc0;
      kpwnCode_ ^= uc1;
    }
    eval_[0] -= Figure::positionEvaluation(0, c, t, from);
    eval_[0] += Figure::positionEvaluation(0, c, t, to);

    eval_[1] -= Figure::positionEvaluation(1, c, t, from);
    eval_[1] += Figure::positionEvaluation(1, c, t, to);

    X_ASSERT(fcounter_[c].mask_all() & set_mask_bit(from), "invalid figures mask");
  }

  // the same as original versions, except of skipped mask & zcode
  inline void u_incr(const Figure::Color c, const Figure::Type t, int p)
  {
	  fcounter_[c].incr(c, t, p);
  }

  inline void u_decr(const Figure::Color c, const Figure::Type t, int p)
  {
	  fcounter_[c].decr(c, t, p);
  }

  inline void u_move(const Figure::Color c, const Figure::Type t, int from, int to)
  {
	  fcounter_[c].move(c, t, from, to);
  }

  inline void hashEnPassant(uint8 pos, uint8 color)
  {
    hashCode_ ^= enpassantCode(pos, color);
  }

  inline void hashCastling(uint8 color, uint8 index /* 0 - short, 1 - long */)
  {
    hashCode_ ^= castleCode(color, index);
    kpwnCode_ ^= castleCode(color, index);
  }

  inline void hashColor()
  {
    hashCode_ ^= colorCode();
    kpwnCode_ ^= colorCode();
  }

  inline void hashNullmove()
  {
    hashCode_ ^= nullmoveCode();
  }

  void restoreHash(const BitMask & hcode) { hashCode_ = hcode; }
  void restoreKpwnCode(const BitMask & pcode) { kpwnCode_ = pcode; }

  inline int tcount(Figure::Type type, Figure::Color color) const { return fcounter_[color].tcount(type); }
  inline int pawns(Figure::Color color) const { return fcounter_[color].pawns(); }
  inline int pawns() const { return pawns(Figure::ColorWhite) + pawns(Figure::ColorBlack); }
  inline int bishops(Figure::Color color) const { return fcounter_[color].bishops(); }
  inline int knights(Figure::Color color) const { return fcounter_[color].knights(); }
  inline int rooks(Figure::Color color) const { return fcounter_[color].rooks(); }
  inline int queens(Figure::Color color) const { return fcounter_[color].queens(); }
  inline ScoreType weight(Figure::Color color) const { return fcounter_[color].weight(); }
  inline ScoreType weight() const { return weight(Figure::ColorWhite) - weight(Figure::ColorBlack); }
  inline const BitMask & pawn_mask(Figure::Color color) const { return fcounter_[color].pawn_mask(); }
  inline const BitMask & knight_mask(Figure::Color color) const { return fcounter_[color].knight_mask(); }
  inline const BitMask & bishop_mask(Figure::Color color) const { return fcounter_[color].bishop_mask(); }
  inline const BitMask & rook_mask(Figure::Color color) const { return fcounter_[color].rook_mask(); }
  inline const BitMask & queen_mask(Figure::Color color) const { return fcounter_[color].queen_mask(); }
  inline const BitMask & king_mask(Figure::Color color) const { return fcounter_[color].king_mask(); }
  inline const BitMask & mask(Figure::Color color) const { return fcounter_[color].mask_all(); }
  inline const BitMask & type_mask(const Figure::Type type, const Figure::Color color) const { return fcounter_[color].type_mask(type); }

  inline const BitMask & hashCode() const { return hashCode_; }
  inline const BitMask & kpwnCode() const { return kpwnCode_; }

  inline const ScoreType eval(int stage) const { X_ASSERT(stage < 0 || stage > 1, "invalid stage"); return eval_[stage]; }

  inline int32 eval32() const { return eval32_; }
  inline void resoreEval(int32 ev32) { eval32_ = ev32; }

  inline const BitMask & code(const Figure::Color c, const Figure::Type t, int p) const
  {
    return s_zobristCodes_[(p<<4) | (c<<3) | t];
  }

  inline const BitMask & enpassantCode(uint8 pos, uint8 color) const
  {
    return s_zobristCodes_[ (pos<<4) | (color<<3) ];
  }

  inline const BitMask & castleCode(uint8 color, uint8 index) const
  {
    return s_zobristCastle_[color][index];
  }

  inline const BitMask & colorCode() const
  {
    return s_zobristColor_;
  }

  inline const BitMask & nullmoveCode() const
  {
    return s_zobristNullmove_;
  }

private:

  BitMask hashCode_{};
  BitMask kpwnCode_{};
  FiguresCounter fcounter_[2];

  union
  {
    struct
    {
      ScoreType eval_[2];
    };
    int32 eval32_{};
  };
};


#pragma pack (pop)
} // NEngine