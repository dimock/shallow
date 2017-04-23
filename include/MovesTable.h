/*************************************************************
  MovesTable.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <xcommon.h>

namespace NEngine
{

class MovesTable
{
  int8   s_tablePawn_[2][64][6];
  int8   s_tableKnight_[64][10];
  int8   s_tableKing_[64][10];
  uint16 s_tableOther_[4][64][10];

  // masks only for captures
  //BitMask s_pawnsCaps_t_[2][64];
  BitMask s_pawnsCaps_[2][64];
  BitMask s_pawnsMoves_[2][64];
  BitMask s_pawnsFrom_[2][64];
  BitMask s_otherCaps_[8][64];
  BitMask s_pawnPromotions_t_[2];
  BitMask s_pawnPromotions_o_[2];

  // blocked rook
  BitMask s_blockedRook_[64];

  // king pressure mask: +1 square around king
  BitMask s_kingPressure_[64];

  void resetAllTables(int);

  void initPawns(int);
  void initKnights(int);
  void initKings(int);
  void initBishops(int);
  void initRooks(int);
  void initQueens(int);

public:

  MovesTable();

  inline const int8 * pawn(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move for invalid position, color");
    return s_tablePawn_[color][pos];
  }

  inline const int8 * knight(int pos) const
  {
    X_ASSERT((unsigned)pos > 63, "try to get knight move for invalid position");
    return s_tableKnight_[pos];
  }

  inline const int8 * king(int pos) const
  {
    X_ASSERT((unsigned)pos > 63, "try to get king move from invalid position");
    return s_tableKing_[pos];
  }

  inline const uint16 * move(int type, int pos) const
  {
    X_ASSERT((unsigned)type > 2 || (unsigned)pos > 63, "try to get figure move from invalid position or type");
    return s_tableOther_[type][pos];
  }

  //// transposed captures
  //inline const BitMask & pawnCaps_t(int color, int pos) const
  //{
  //  X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move from invalid position, color");
  //  return s_pawnsCaps_t_[color][pos];
  //}

  // ordinary captures
  inline const BitMask & pawnCaps(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn cap from invalid position or color");
    return s_pawnsCaps_[color][pos];
  }

  // moves
  inline const BitMask & pawnMoves(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move from invalid position or color");
    return s_pawnsMoves_[color][pos];
  }

  // pawn can go to 'pos' from these positions
  inline const BitMask & pawnFrom(int color, int pos) const
  {
    X_ASSERT((unsigned)color > 1 || (unsigned)pos > 63, "try to get pawn move from invalid position or color");
    return s_pawnsFrom_[color][pos];
  }

  // transposed promotion mask
  inline const BitMask & promote_t(int color) const
  {
    X_ASSERT( (unsigned)color > 1, "invalid color of promotion mask" );
    return s_pawnPromotions_t_[color];
  }

  inline const BitMask & promote_o(int color) const
  {
    X_ASSERT( (unsigned)color > 1, "invalid color of promotion mask" );
    return s_pawnPromotions_o_[color];
  }

  inline const BitMask & caps(int type, int pos) const
  {
    X_ASSERT((unsigned)type > 6 || (unsigned)pos > 63, "try to get figure move for invalid position or type");
    return s_otherCaps_[type][pos];
  }

  inline const BitMask & king_pressure(int pos) const
  {
    X_ASSERT((unsigned)pos > 63, "try to get mask for invalid position");
    return s_kingPressure_[pos];
  }

  inline const BitMask & blocked_rook(int pos) const
  {
    X_ASSERT((unsigned)pos > 63, "try to get mask for invalid position");
    return s_blockedRook_[pos];
  }
};

} // NEngine
