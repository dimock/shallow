/*************************************************************
  MovesTable.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <MovesTable.h>
#include <Figure.h>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
MovesTable::MovesTable()
{
  initCastle();

  for (int i = 0; i < 64; ++i)
  {
    resetAllTables(i);

    initPawns(i);
    initKnights(i);
    initKings(i);
    initBishops(i);
    initRooks(i);
    initQueens(i);
  }

  // pawn promotions
  for (int color = 0; color < 2; ++color)
  {
    s_pawnPromotions_[color] = 0;

    int y = color ? 6 : 1;
    for (int x = 0; x < 8; ++x)
    {
      int pp = x | (y << 3);

      // fill ordinary promotion mask
      s_pawnPromotions_[color] |= set_mask_bit(pp);
    }
  }
}

void MovesTable::initCastle()
{
  s_castleMasks_[0][0] = set_mask_bit(61) | set_mask_bit(62);
  s_castleMasks_[0][1] = set_mask_bit(57) | set_mask_bit(58) | set_mask_bit(59);

  s_castleMasks_[1][0] = set_mask_bit(5) | set_mask_bit(6);
  s_castleMasks_[1][1] = set_mask_bit(1) | set_mask_bit(2) | set_mask_bit(3);
}

void MovesTable::resetAllTables(int pos)
{
  s_blockedRook_[pos] = 0;
  for (int color = 0; color < 2; ++color)
  {
    for (int i = 0; i < 6; ++i)
      s_tablePawn_[color][pos][i] = -1;
  }

  for (int i = 0; i < 10; ++i)
  {
    s_tableKnight_[pos][i] = -1;
    s_tableKing_[pos][i] = -1;

    for(int j = 0; j < 4; ++j)
      s_tableOther_[j][pos][i] = 0;
  }

  for (int color = 0; color < 2; ++color)
  {
    //s_pawnsCaps_t_[color][pos] = 0;
    s_pawnsCaps_[color][pos] = 0;
    s_pawnsMoves_[color][pos] = 0;
    s_pawnsFrom_[color][pos] = 0;
  }

  for (int type = 0; type < 8; ++type)
    s_otherCaps_[type][pos] = 0;

  s_kingPressure_[pos] = 0;
}

//////////////////////////////////////////////////////////////////////////
void MovesTable::initPawns(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 1), FPos(1, 1), FPos(0, 1), FPos(0, 2) };

  for (int color = 0; color < 2; ++color)
  {
    //if ( p.y() == 0 || p.y() == 7 )
    //  continue;

    bool first = color ? p.y() == 1 : p.y() == 6;
    int n = first ? 4 : 3;
    s_tablePawn_[color][pos][3] = -1;
    for (int i = 0; i < n; ++i)
    {
      FPos d = color ? dpos[i] : FPos(dpos[i].x(), -dpos[i].y());
      FPos q = p + d;
      if ( q )
        s_tablePawn_[color][pos][i] = q.index();
      else
        s_tablePawn_[color][pos][i] = -1;
    }

    // fill captures masks
    for (int i = 0; i < 2; ++i)
    {
      if ( s_tablePawn_[color][pos][i] >= 0 )
      {
        int8 & pp = s_tablePawn_[color][pos][i];
        s_pawnsCaps_[color][pos] |= set_mask_bit(pp);
        //s_pawnsCaps_t_[color][pos] |= set_mask_bit(FiguresCounter::s_transposeIndex_[pp]);
      }
    }

    // fill moves mask
    int8 * ptable  = s_tablePawn_[color][pos] + 2;
    for ( ; *ptable >= 0; ++ptable)
    {
      s_pawnsMoves_[color][pos] |= set_mask_bit(*ptable);
    }

    // fill 'from' mask
    if ( p.y() == 0 || p.y() == 7 )
      continue;

    {
      int y_from[2] = { -1, -1 };

      if ( color )
      {
        if ( p.y() == 1 )
          continue;

        y_from[0] = p.y() - 1;
        if ( p.y() == 3 )
          y_from[1] = 1;
      }
      else
      {
        if ( p.y() == 6 )
          continue;

        y_from[0] = p.y() + 1;
        if ( p.y() == 4 )
          y_from[1] = 6;
      }

      for (int i = 0; i < 2; ++i)
      {
        if ( y_from[i] >= 0 )
        {
          int index = p.x() | (y_from[i] << 3);
          s_pawnsFrom_[color][pos] |= set_mask_bit(index);
        }
      }
    }
  }
}

void MovesTable::initKnights(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-2, 1), FPos(-1, 2), FPos(1, 2), FPos(2, 1), FPos(2, -1), FPos(1, -2), FPos(-1, -2), FPos(-2, -1) };

  int j = 0;
  for (int i = 0; i < 8; ++i)
  {
    const FPos & d = dpos[i];
    FPos q = p + d;
    if ( !q )
      continue;
    s_tableKnight_[pos][j++] = q.index();
  }

  // fill captures masks
  for (int i = 0; i < 8 && s_tableKnight_[pos][i] >= 0; ++i)
    s_otherCaps_[Figure::TypeKnight][pos] |= set_mask_bit(s_tableKnight_[pos][i]);
}

void MovesTable::initKings(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 0), FPos(-1, 1), FPos(0, 1), FPos(1, 1), FPos(1, 0), FPos(1, -1), FPos(0, -1), FPos(-1, -1) };

  int j = 0;
  for (int i = 0; i < 8; ++i)
  {
    const FPos & d = dpos[i];
    FPos q = p + d;
    if ( !q )
      continue;
    s_tableKing_[pos][j++] = q.index();
  }

  // fill captures masks
  for (int i = 0; i < 8 && s_tableKing_[pos][i] >= 0; ++i)
    s_otherCaps_[Figure::TypeKing][pos] |= set_mask_bit(s_tableKing_[pos][i]);

  // fill pressure mask
  int xp = pos & 7;
  int yp = pos >>3;
  for (int x = xp-2; x < xp+2; ++x)
  {
    if ( x < 0 || x > 7 )
      continue;

    int y = yp-2;
    if ( y >= 0 )
    {
      int p = x | (y<<3);
      s_kingPressure_[pos] |= set_mask_bit(p);
    }

    y = yp+2;
    if ( y < 8 )
    {
      int p = x | (y<<3);
      s_kingPressure_[pos] |= set_mask_bit(p);
    }
  }

  for (int y = yp-2; y < yp+2; ++y)
  {
    if ( y < 0 || y > 7 )
      continue;

    int x = xp-2;
    if ( x >= 0 )
    {
      int p = x | (y<<3);
      s_kingPressure_[pos] |= set_mask_bit(p);
    }

    x = xp+2;
    if ( x < 8 )
    {
      int p = x | (y<<3);
      s_kingPressure_[pos] |= set_mask_bit(p);
    }
  }
}

void MovesTable::initBishops(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, -1), FPos(-1, 1), FPos(1, 1), FPos(1, -1) };

  int j = 0;
  for (int i = 0; i < 4; ++i)
  {
    const FPos & d = dpos[i];
    int n = 0;
    for (FPos q = p + d; q; ++n, q += d)
    {
      // fill captures mask
      s_otherCaps_[Figure::TypeBishop][pos] |= set_mask_bit(q.index());
    }

    if ( !n )
      continue;

    s_tableOther_[Figure::TypeBishop-Figure::TypeBishop][pos][j++] = (d.delta() << 8) | (n);
  }
}

void MovesTable::initRooks(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 0), FPos(0, 1), FPos(1, 0), FPos(0, -1) };

  int j = 0;
  for (int i = 0; i < 4; ++i)
  {
    const FPos & d = dpos[i];
    int n = 0;
    FPos q = p + d;
    if(q)
      s_blockedRook_[pos] |= set_mask_bit(q.index());
    for (; q; ++n, q += d)
    {
      // fill captures masks
      s_otherCaps_[Figure::TypeRook][pos] |= set_mask_bit(q.index());
    }
    
    if ( !n )
      continue;

    s_tableOther_[Figure::TypeRook-Figure::TypeBishop][pos][j++] = (d.delta() << 8) | (n);
  }
}

void MovesTable::initQueens(int pos)
{
  FPos p(pos);
  static FPos dpos[] = { FPos(-1, 0), FPos(-1, 1), FPos(0, 1), FPos(1, 1), FPos(1, 0), FPos(1, -1), FPos(0, -1), FPos(-1, -1) };

  int j = 0;
  for (int i = 0; i < 8; ++i)
  {
    const FPos & d = dpos[i];
    int n = 0;
    for (FPos q = p + d; q; ++n, q += d)
    {
      // fill captures masks
      s_otherCaps_[Figure::TypeQueen][pos] |= set_mask_bit(q.index());
    }

    if ( !n )
      continue;
    s_tableOther_[Figure::TypeQueen-Figure::TypeBishop][pos][j++] = (d.delta() << 8) | (n);
  }
}

} // NEngine
