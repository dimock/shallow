/*************************************************************
  MovesGenerator.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <MovesGenerator.h>
#include <MovesTable.h>

namespace NEngine
{

History MovesGeneratorBase::history_[64][64];

//unsigned History::history_max_;


void MovesGeneratorBase::clear_history()
{
  for (int i = 0; i < 64; ++i)
    for (int j = 0; j < 64; ++j)
      history_[i][j].clear();
  //History::history_max_ = 0;
}

void MovesGeneratorBase::normalize_history(int n)
{
//  History::history_max_ = 0;
  for (int i = 0; i < 64; ++i)
  {
    for (int j = 0; j < 64; ++j)
    {
      History & hist = history_[i][j];
      hist.normalize(n);
      //if ( hist.score_ > History::history_max_ )
      //  History::history_max_ = hist.score_;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
bool MovesGeneratorBase::find(const Move & m) const
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    const Move & move = moves_[i];
    if ( m == move )
      return true;
  }
  return false;
}

bool MovesGeneratorBase::has_duplicates() const
{
  for (int i = 0; i < numOfMoves_; ++i)
  {
    for (int j = i+1; j < numOfMoves_; ++j)
    {
      if ( moves_[i] == moves_[j] )
        return true;
    }
  }
  return false;
}

//////////////////////////////////////////////////////////////////////////
MovesGenerator::MovesGenerator(const Board & board, const Move & killer) :
  MovesGeneratorBase(board), killer_(killer)
{
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
}

MovesGenerator::MovesGenerator(const Board & board) :
  MovesGeneratorBase(board)
{
  killer_.clear();
  numOfMoves_ = generate();
  moves_[numOfMoves_].clear();
}
//////////////////////////////////////////////////////////////////////////
int MovesGenerator::generate()
{
  int m = 0;
  const Figure::Color & color = board_.color_;
  const Figure::Color ocolor = Figure::otherColor(color);

  if ( board_.checkingNum_ < 2 )
  {
    // pawns movements
    if ( board_.fmgr().pawn_mask_o(color) )
    {
      BitMask pw_mask = board_.fmgr().pawn_mask_o(color);
      for ( ; pw_mask; )
      {
        int pw_pos = clear_lsb(pw_mask);

        const int8 * table = board_.g_movesTable->pawn(color, pw_pos);

        for (int i = 0; i < 2; ++i, ++table)
        {
          if ( *table < 0 )
            continue;

          const Field & field = board_.getField(*table);
          bool capture = false;
          if ( (field && field.color() == ocolor) ||
               (board_.en_passant_ >= 0 && *table == board_.en_passant_) )
          {
            capture = true;
          }

          if ( !capture )
            continue;

          bool promotion = *table > 55 || *table < 8;

          Move & move = moves_[m++];
          move.alreadyDone_ = 0;
          move.set(pw_pos, *table, Figure::TypeNone, capture);
          calculateSortValue(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeRook;
            calculateSortValue(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeBishop;
            calculateSortValue(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeKnight;
            calculateSortValue(moves_[m++]);
          }
        }

        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          bool promotion = *table > 55 || *table < 8;

          Move & move = moves_[m++];

          move.alreadyDone_ = 0;
          move.set(pw_pos, *table, Figure::TypeNone, false);
          calculateSortValue(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeRook;
            calculateSortValue(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeBishop;
            calculateSortValue(moves_[m++]);

            moves_[m] = move;
            moves_[m].new_type_ = Figure::TypeKnight;
            calculateSortValue(moves_[m++]);
          }
        }
      }
    }

    // knights movements
    if ( board_.fmgr().knight_mask(color) )
    {
      BitMask kn_mask = board_.fmgr().knight_mask(color);
      for ( ; kn_mask; )
      {
        int kn_pos = clear_lsb(kn_mask);

        const int8 * table = board_.g_movesTable->knight(kn_pos);

        for (; *table >= 0; ++table)
        {
          const Field & field = board_.getField(*table);
          bool capture = false;
          if ( field )
          {
            if ( field.color() == color )
              continue;

            capture = true;
          }

          add(m, kn_pos, *table, Figure::TypeNone, capture);
        }
      }
    }

    // bishops, rooks and queens movements
    for (int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, color);

      for ( ; fg_mask; )
      {
        int fg_pos = clear_lsb(fg_mask);

        const uint16 * table = board_.g_movesTable->move(type-Figure::TypeBishop, fg_pos);

        for (; *table; ++table)
        {
          const int8 * packed = reinterpret_cast<const int8*>(table);
          int8 count = packed[0];
          int8 delta = packed[1];

          int8 p = fg_pos;
          bool capture = false;
          for ( ; count && !capture; --count)
          {
            p += delta;

            const Field & field = board_.getField(p);
            if ( field )
            {
              if ( field.color() == color )
                break;

              capture = true;
            }

            add(m, fg_pos, p, Figure::TypeNone, capture);
          }
        }
      }
    }
  }

  // kings movements
  {
    BitMask ki_mask = board_.fmgr().king_mask(color);
    
    X_ASSERT( ki_mask == 0, "invalid position - no king" );

    int ki_pos = clear_lsb(ki_mask);

    const int8 * table = board_.g_movesTable->king(ki_pos);

    for (; *table >= 0; ++table)
    {
      const Field & field = board_.getField(*table);
      bool capture = false;
      if ( field )
      {
        if ( field.color() == color )
          continue;

        capture = true;
      }

      add(m, ki_pos, *table, Figure::TypeNone, capture);
    }

    if ( !board_.underCheck() )
    {
      // short castle
      if ( board_.castling(board_.color_, 0) && !board_.getField(ki_pos+2) && !board_.getField(ki_pos+1) )
        add(m, ki_pos, ki_pos+2, Figure::TypeNone, false);

      // long castle
      if ( board_.castling(board_.color_, 1) && !board_.getField(ki_pos-2) && !board_.getField(ki_pos-1) && !board_.getField(ki_pos-3) )
        add(m, ki_pos, ki_pos-2, Figure::TypeNone, false);
    }
  }

  return m;
}

//////////////////////////////////////////////////////////////////////////
void MovesGeneratorBase::save_history(const char * fname)
{
  FILE * f = fopen(fname, "wb");
  if ( !f )
    return;
  
//  fwrite(&History::history_max_, sizeof(History::history_max_), 1, f);
  fwrite((char*)history_, sizeof(History), 64*64, f);

  fclose(f);
}

void MovesGeneratorBase::load_history(const char * fname)
{
  FILE * f = fopen(fname, "rb");
  if ( !f )
    return;

  //fread(&History::history_max_, sizeof(History::history_max_), 1, f);
  fread((char*)history_, sizeof(History), 64*64, f);

  fclose(f);
}

} // NEngine
