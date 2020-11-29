/*************************************************************
  Figure.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <xbitmath.h>
#include <EvalCoefficients.h>

namespace NEngine
{

#pragma pack (push, 1)

namespace Figure
{
  enum Weights { DrawScore = 0, MatScore = 32000 };
  enum Type    { TypeNone, TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing, TypesNum };
  enum Color : int8  { ColorBlack, ColorWhite };
  
  extern const BitMask pawnCutoffMasks_[2];
  extern const BitMask pawns2ndLineMask_[2];
  
  // TypePawn, TypeKnight, TypeBishop, TypeRook, TypeQueen, TypeKing
  constexpr ScoreType figureWeight_[7] = { 0, 90, 355, 355, 545, 1090, 0 };
  extern const ScoreType32 figureWeight32_[7];

  extern const uint8     mirrorIndex_[64];
  // color, castle (K = 0, Q = 1)
  extern const BitMask   quaterBoard_[2][2];

  inline Figure::Color otherColor(Figure::Color color)
  {
    return (Figure::Color)(!color);
  }

  const char * name(Type type);

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
    weight32_.eval32_ = 0;
    figuresWeight_ = 0;
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
    weight32_ += Figure::figureWeight32_[t];
    if (t != Figure::TypePawn)
      figuresWeight_ += Figure::figureWeight_[t];

    auto mask_set = set_mask_bit(p);
    tmask_[t] |= mask_set;
    mask_all_ |= mask_set;

    X_ASSERT(weight32_ != Figure::figureWeight32_[Figure::Type::TypePawn]* pawns() + Figure::figureWeight32_[Figure::TypeBishop]* bishops()
             + Figure::figureWeight32_[Figure::TypeKnight]* knights() + Figure::figureWeight32_[Figure::TypeRook]* rooks()
             + Figure::figureWeight32_[Figure::TypeQueen]* queens(), "invalid weight" );
    X_ASSERT(figuresWeight_ != bishops()*Figure::figureWeight_[Figure::TypeBishop]
      + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook]
      + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid figures weight");
  }

  inline void decr(const Figure::Color c, const Figure::Type t, int p)
  {
    X_ASSERT(t == Figure::TypeNone, "decr for invalid figure type");

    tcount_[t]--;
    weight32_ -= Figure::figureWeight32_[t];
    if(t != Figure::TypePawn)
      figuresWeight_ -= Figure::figureWeight_[t];

    auto mask_clear = set_mask_bit(p);
    tmask_[t] ^= mask_clear;
    mask_all_ ^= mask_clear;

    X_ASSERT(tmask_[t] & set_mask_bit(p), "invalid mask");
    X_ASSERT(weight32_ != Figure::figureWeight32_[Figure::TypePawn]* pawns() + Figure::figureWeight32_[Figure::TypeBishop]* bishops()
             + Figure::figureWeight32_[Figure::TypeKnight]* knights() + Figure::figureWeight32_[Figure::TypeRook]* rooks()
             + Figure::figureWeight32_[Figure::TypeQueen]* queens(), "invalid weight" );
    X_ASSERT(figuresWeight_ != bishops()*Figure::figureWeight_[Figure::TypeBishop]
      + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook]
      + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid figures weight");
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
  inline int pawns() const { return tcount_[Figure::TypePawn]; }
  inline int knights() const { return tcount_[Figure::TypeKnight]; }
  inline int bishops() const { return tcount_[Figure::TypeBishop]; }
  inline int rooks() const { return tcount_[Figure::TypeRook]; }
  inline int queens() const { return tcount_[Figure::TypeQueen]; }
  inline int allFigures() const { return knights() + bishops() + rooks() + queens(); }
  inline ScoreType32 weight() const { return weight32_; }
  inline ScoreType figuresWeight() const { return figuresWeight_; }
  inline const BitMask & pawn_mask() const { return tmask_[Figure::TypePawn]; }
  inline const BitMask & knight_mask() const { return tmask_[Figure::TypeKnight]; }
  inline const BitMask & bishop_mask() const { return tmask_[Figure::TypeBishop]; }
  inline const BitMask & rook_mask() const { return tmask_[Figure::TypeRook]; }
  inline const BitMask & queen_mask() const { return tmask_[Figure::TypeQueen]; }
  inline const BitMask & king_mask() const { return tmask_[Figure::TypeKing]; }
  inline const BitMask & type_mask(const Figure::Type type) const { return tmask_[type]; }
  inline const BitMask & mask_all() const { return mask_all_; }

private:
  BitMask   tmask_[8];
  BitMask   mask_all_;
  uint8     tcount_[8];
  ScoreType32 weight32_;
  ScoreType figuresWeight_;
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
    fgrsCode_ = 0ULL;
    fcounter_[0].clear();
    fcounter_[1].clear();
    score_.eval32_ = 0;
  }

  inline void incr(const Figure::Color c, const Figure::Type t, int p)
  {
    if (fcounter_[c].tcount(t)) {
      fgrsCode_ ^= code(c, t, fcounter_[c].tcount(t));
    }
    fcounter_[c].incr(c, t, p);
    fgrsCode_ ^= code(c, t, fcounter_[c].tcount(t));

    const BitMask & uc = code(c, t, p);
    hashCode_ ^= uc;
    
    if(t == Figure::TypePawn || t == Figure::TypeKing)
      kpwnCode_ ^= uc;
    // + for black color. invert the sign
    score_ -= EvalCoefficients::positionEvaluations_[c][t][p];
    ScoreType32 score{ -12, -10 };
    auto s = score_.eval1();
    return;
  }

  inline void decr(const Figure::Color c, const Figure::Type t, int p)
  {
    fgrsCode_ ^= code(c, t, fcounter_[c].tcount(t));
    fcounter_[c].decr(c, t, p);
    if (fcounter_[c].tcount(t)) {
      fgrsCode_ ^= code(c, t, fcounter_[c].tcount(t));
    }

    const BitMask & uc = code(c, t, p);
    hashCode_ ^= uc;

    if(t == Figure::TypePawn || t == Figure::TypeKing)
      kpwnCode_ ^= uc;
    // + for black color. invert the sign
    score_ += EvalCoefficients::positionEvaluations_[c][t][p];
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

    // + for black color. invert the sign
    score_ += EvalCoefficients::positionEvaluations_[c][t][from];
    score_ -= EvalCoefficients::positionEvaluations_[c][t][to];

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
  }

  inline void hashNullmove()
  {
    hashCode_ ^= nullmoveCode();
  }

  void restoreHash(const BitMask & hcode) { hashCode_ = hcode; }
  void restoreKpwnCode(const BitMask & pcode) { kpwnCode_ = pcode; }
  void restoreFgrsCode(const BitMask & fcode) { fgrsCode_ = fcode; }

  inline int tcount(Figure::Type type, Figure::Color color) const { return fcounter_[color].tcount(type); }
  inline int pawns(Figure::Color color) const { return fcounter_[color].pawns(); }
  inline int pawns() const { return pawns(Figure::ColorWhite) + pawns(Figure::ColorBlack); }
  inline int bishops(Figure::Color color) const { return fcounter_[color].bishops(); }
  inline int knights(Figure::Color color) const { return fcounter_[color].knights(); }
  inline int rooks(Figure::Color color) const { return fcounter_[color].rooks(); }
  inline int queens(Figure::Color color) const { return fcounter_[color].queens(); }
  inline int allFigures(Figure::Color color) const { return fcounter_[color].allFigures(); }
  inline ScoreType32 weight(Figure::Color color) const { return fcounter_[color].weight(); }
  inline ScoreType figuresWeight(Figure::Color color) const { return fcounter_[color].figuresWeight(); }
  inline ScoreType32 weight() const { return weight(Figure::ColorWhite) - weight(Figure::ColorBlack); }
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
  inline const BitMask & fgrsCode() const { return fgrsCode_; }

  inline const ScoreType32 score() const { return score_; }

  inline void resoreEval(int32 ev32) { score_.eval32_ = ev32; }

  // HASH codes calculation
  static inline const BitMask & code(const Figure::Color c, const Figure::Type t, int p)
  {
    return s_zobristCodes_[(p<<4) | (c<<3) | t];
  }

  static inline const BitMask & enpassantCode(uint8 pos, uint8 color)
  {
    return s_zobristCodes_[ (pos<<4) | (color<<3) ];
  }

  static inline const BitMask & castleCode(uint8 color, uint8 index)
  {
    return s_zobristCastle_[color][index];
  }

  static inline const BitMask & colorCode()
  {
    return s_zobristColor_;
  }

  static inline const BitMask & nullmoveCode()
  {
    return s_zobristNullmove_;
  }

private:

  BitMask hashCode_{};
  BitMask kpwnCode_{};
  BitMask fgrsCode_{};
  FiguresCounter fcounter_[2];
  ScoreType32 score_;
};


#pragma pack (pop)
} // NEngine