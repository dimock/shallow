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

  inline Figure::Color otherColor(Figure::Color color)
  {
    //return (Figure::Color)((static_cast<int>(color) + 1) & 1);
    return (Figure::Color)(!color);
  }

  const char * name(Type type);

  //ScoreType positionEvaluation(int stage, Figure::Color color, Figure::Type type, int pos);

  Figure::Type toFtype(char c);

  char fromFtype(Figure::Type t);
}


ALIGN_MSC(1) class ALIGN_GCC(1) FiguresCounter
{
public:

  static  int8 s_whiteColors_[64];
  static uint8 s_transposeIndex_[64];

  FiguresCounter()
  {
    clear();
  }

  void clear()
  {
    count_ = 0;
    weight_ = 0;
    //eval_[0] = 0;
    //eval_[1] = 0;
    //pmask_t_ = 0;
    for (int i = 0; i < 8; ++i)
    {
      tcount_[i][0] = tcount_[i][1] = 0;
      tmask_[i] = 0ULL;
    }
  }

  inline void incr(const Figure::Color c, const Figure::Type t, int p)
  {
    static int8 fcmask[8] = { 0/*None*/, 0/*Pawn*/, 0/*Knight*/, (int8)(255)/*bishop*/, 0/*rook*/, 0/*queen*/, 0/*king*/ };
    static int8 xincr[8]  = { 0/*None*/, 1/*Pawn*/, 1/*Knight*/, 1/*bishop*/, 1/*rook*/, 1/*queen*/, 0/*king*/ };
    static ScoreType xscore[8] = { 0/*None*/, (ScoreType)(65535)/*Pawn*/,
      (ScoreType)(65535)/*Knight*/, (ScoreType)(65535)/*bishop*/, (ScoreType)(65535)/*rook*/,
      (ScoreType)(65535)/*queen*/, 0/*king*/ };
    
    int8 field_color = s_whiteColors_[p] & fcmask[t];
    
    tcount_[t][field_color]++;
    weight_ += Figure::figureWeight_[t] & xscore[t];
    count_  += xincr[t];

    tmask_[t] |= set_mask_bit(p);

    //if ( t == Figure::Type::TypePawn )
    //  pmask_t_ |= set_mask_bit(s_transposeIndex_[p]);

    //eval_[0] += Figure::positionEvaluation(0, c, t, p);
    //eval_[1] += Figure::positionEvaluation(1, c, t, p);

    X_ASSERT(weight_ != pawns()*Figure::figureWeight_[Figure::Type::TypePawn] + bishops()*Figure::figureWeight_[Figure::TypeBishop] + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook] + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
    X_ASSERT(count_ != pawns() + bishops() + knights() + rooks() + queens(), "invalid number of figures encountered");
  }

  inline void decr(const Figure::Color c, const Figure::Type t, int p)
  {
    static int8 fcmask[8] = { 0/*None*/, 0/*Pawn*/, 0/*Knight*/, (int8)255/*bishop*/, 0/*rook*/, 0/*queen*/, 0/*king*/ };
    static int8 xdecr[8]  = { 0/*None*/, 1/*Pawn*/, 1/*Knight*/, 1/*bishop*/, 1/*rook*/, 1/*queen*/, 0/*king*/ };
    static ScoreType xscore[8] = { 0/*None*/, (ScoreType)65535/*Pawn*/, (ScoreType)65535/*Knight*/, (ScoreType)65535/*bishop*/, (ScoreType)65535/*rook*/, (ScoreType)65535/*queen*/, 0/*king*/ };

    int8 fc = s_whiteColors_[p] & fcmask[t];

    tcount_[t][fc]--;
    weight_ -= Figure::figureWeight_[t] & xscore[t];
    count_  -= xdecr[t];

    tmask_[t] ^= set_mask_bit(p);

    //if ( t == Figure::TypePawn )
    //  pmask_t_ ^= set_mask_bit(s_transposeIndex_[p]);

    X_ASSERT( tmask_[t] & set_mask_bit(p), "invalid mask" );

    //eval_[0] -= Figure::positionEvaluation(0, c, t, p);
    //eval_[1] -= Figure::positionEvaluation(1, c, t, p);

    X_ASSERT(weight_ != pawns()*Figure::figureWeight_[Figure::TypePawn] + bishops()*Figure::figureWeight_[Figure::TypeBishop] + knights()*Figure::figureWeight_[Figure::TypeKnight] + rooks()*Figure::figureWeight_[Figure::TypeRook] + queens()*Figure::figureWeight_[Figure::TypeQueen], "invalid weight" );
    X_ASSERT(count_ != pawns() + bishops() + knights() + rooks() + queens(), "invalid number of figures encountered");
  }

  inline void move(const Figure::Color c, const Figure::Type t, int from, int to)
  {
    //eval_[0] -= Figure::positionEvaluation(0, c, t, from);
    //eval_[0] += Figure::positionEvaluation(0, c, t, to);

    //eval_[1] -= Figure::positionEvaluation(1, c, t, from);
    //eval_[1] += Figure::positionEvaluation(1, c, t, to);

    tmask_[t] ^= set_mask_bit(from);
    tmask_[t] |= set_mask_bit(to);

    //if ( t == Figure::TypePawn )
    //{
    //  pmask_t_ ^= set_mask_bit(s_transposeIndex_[from]);
    //  pmask_t_ |= set_mask_bit(s_transposeIndex_[to]);
    //}
  }

  inline int tcount(Figure::Type type) const { return type == Figure::TypeBishop ? bishops() : tcount_[type][0]; }
  inline int count() const { return count_; }
  inline int pawns() const { return tcount_[Figure::TypePawn][0]; }
  inline int knights() const { return tcount_[Figure::TypeKnight][0]; }
  inline int bishops_c(Figure::Color field_c) const { return tcount_[Figure::TypeBishop][field_c]; }
  inline int bishops_w() const { return tcount_[Figure::TypeBishop][1]; }
  inline int bishops_b() const { return tcount_[Figure::TypeBishop][0]; }
  inline int bishops() const { return bishops_w() + bishops_b(); }
  inline int rooks() const { return tcount_[Figure::TypeRook][0]; }
  inline int queens() const { return tcount_[Figure::TypeQueen][0]; }
  inline ScoreType weight() const { return weight_; }
  //inline ScoreType eval(int stage) const { return eval_[stage]; }
  //inline const BitMask & pawn_mask_t() const { return pmask_t_; }
  inline const BitMask & pawn_mask() const { return tmask_[Figure::TypePawn]; }
  inline const BitMask & knight_mask() const { return tmask_[Figure::TypeKnight]; }
  inline const BitMask & bishop_mask() const { return tmask_[Figure::TypeBishop]; }
  inline const BitMask & rook_mask() const { return tmask_[Figure::TypeRook]; }
  inline const BitMask & queen_mask() const { return tmask_[Figure::TypeQueen]; }
  inline const BitMask & king_mask() const { return tmask_[Figure::TypeKing]; }
  inline const BitMask & type_mask(const Figure::Type type) const { return tmask_[type]; }

private:

  // 8 - types, 2 - field color (for bishops only!!!)
  uint8 tcount_[8][2];
  uint8 count_;
  BitMask tmask_[8];
  //BitMask pmask_t_; // transposed pawn's mask
  ScoreType weight_;// , eval_[2];
};

class FiguresManager
{
  static BitMask s_zobristCodes_[64*2*8];
  static BitMask s_zobristColor_;
  static BitMask s_zobristCastle_[2][2];

public:

  FiguresManager() : hashCode_(0ULL)
  {
    mask_[0] = mask_[1] = 0ULL;
  }

  void clear()
  {
    hashCode_ = 0ULL;
    pawnCode_ = 0ULL;
    mask_[0] = mask_[1] = 0ULL;
    fcounter_[0].clear();
    fcounter_[1].clear();
  }

  inline void incr(const Figure::Color c, const Figure::Type t, int p)
  {
    fcounter_[c].incr(c, t, p);
    const BitMask & uc = code(c, t, p);
    hashCode_ ^= uc;
    mask_[c] |= set_mask_bit(p);
    if ( t == Figure::TypePawn || t == Figure::TypeKing )
      pawnCode_ ^= uc;      
  }

  inline void decr(const Figure::Color c, const Figure::Type t, int p)
  {
    fcounter_[c].decr(c, t, p);
    const BitMask & uc = code(c, t, p);
    hashCode_ ^= uc;
    mask_[c] ^= set_mask_bit(p);
    if ( t == Figure::TypePawn || t == Figure::TypeKing )
      pawnCode_ ^= uc;      
  }

  inline void move(const Figure::Color c, const Figure::Type t, int from, int to)
  {
    const BitMask & uc0 = code(c, t, from);
    const BitMask & uc1 = code(c, t, to);
    hashCode_ ^= uc0;
    hashCode_ ^= uc1;

    if ( t == Figure::TypePawn || t == Figure::TypeKing )
    {
      pawnCode_ ^= uc0;
      pawnCode_ ^= uc1;
    }

    fcounter_[c].move(c, t, from, to);

    mask_[c] ^= set_mask_bit(from);
    mask_[c] |= set_mask_bit(to);

    X_ASSERT(mask_[c] & set_mask_bit(from), "invalid figures mask");
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
    const BitMask & enpassantCode = s_zobristCodes_[ (pos<<4) | (color<<3) ];
    hashCode_ ^= enpassantCode;
  }

  inline void hashCastling(uint8 color, uint8 index /* 0 - short, 1 - long */)
  {
    const BitMask & castleCode = s_zobristCastle_[color][index];
    hashCode_ ^= castleCode;
  }

  inline void hashColor()
  {
    hashCode_ ^= s_zobristColor_;
    pawnCode_ ^= s_zobristColor_;
  }

  inline void restoreMasks(const BitMask (& mask)[2])
  {
	  mask_[0] = mask[0];
	  mask_[1] = mask[1];
  }

  void restoreHash(const BitMask & hcode) { hashCode_ = hcode; }
  void restorePawnCode(const BitMask & pcode) { pawnCode_ = pcode; }

  inline int count(Figure::Color color) const { return fcounter_[color].count(); }
  inline int tcount(Figure::Type type, Figure::Color color) const { return fcounter_[color].tcount(type); }
  inline int count() const { return fcounter_[0].count() + fcounter_[1].count(); }
  inline int pawns(Figure::Color color) const { return fcounter_[color].pawns(); }
  inline int pawns() const { return pawns(Figure::ColorWhite) + pawns(Figure::ColorBlack); }
  inline int bishops_w(Figure::Color color) const { return fcounter_[color].bishops_w(); }
  inline int bishops_b(Figure::Color color) const { return fcounter_[color].bishops_b(); }
  inline int bishops(Figure::Color color) const { return fcounter_[color].bishops(); }
  inline int bishops_c(Figure::Color color, Figure::Color field_c) const { return fcounter_[color].bishops_c(field_c); }
  inline int knights(Figure::Color color) const { return fcounter_[color].knights(); }
  inline int rooks(Figure::Color color) const { return fcounter_[color].rooks(); }
  inline int queens(Figure::Color color) const { return fcounter_[color].queens(); }
  inline ScoreType weight(Figure::Color color) const { return fcounter_[color].weight(); }
  inline ScoreType weight() const { return weight(Figure::ColorWhite) - weight(Figure::ColorBlack); }
  //inline ScoreType eval(Figure::Color color, int stage) const { return fcounter_[color].eval(stage); }
  //inline ScoreType eval(int stage) const { return fcounter_[Figure::ColorWhite].eval(stage) - fcounter_[Figure::ColorBlack].eval(stage); }
  inline const BitMask & pawn_mask(Figure::Color color) const { return fcounter_[color].pawn_mask(); }
  //inline const BitMask & pawn_mask_t(Figure::Color color) const { return fcounter_[color].pawn_mask_t(); }
  inline const BitMask & knight_mask(Figure::Color color) const { return fcounter_[color].knight_mask(); }
  inline const BitMask & bishop_mask(Figure::Color color) const { return fcounter_[color].bishop_mask(); }
  inline const BitMask & rook_mask(Figure::Color color) const { return fcounter_[color].rook_mask(); }
  inline const BitMask & queen_mask(Figure::Color color) const { return fcounter_[color].queen_mask(); }
  inline const BitMask & king_mask(Figure::Color color) const { return fcounter_[color].king_mask(); }
  inline const BitMask & mask(Figure::Color color) const { return mask_[color]; }
  inline const BitMask & type_mask(const Figure::Type type, const Figure::Color color) const { return fcounter_[color].type_mask(type); }

  inline const BitMask & hashCode() const { return hashCode_; }
  inline const BitMask & pawnCode() const { return pawnCode_; }

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

private:

  /// all figures mask
  BitMask mask_[2];

  FiguresCounter fcounter_[2];
  BitMask hashCode_;
  BitMask pawnCode_;
};


#pragma pack (pop)
} // NEngine