/*************************************************************
  MovesGenerator.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <MovesGenerator.h>
#include <MovesTable.h>
#include <fstream>

namespace NEngine
{

History history_[Board::NumOfFields][Board::NumOfFields] = {};

void clear_history()
{
  for (int i = 0; i < 64; ++i)
    for (int j = 0; j < 64; ++j)
      history_[i][j].clear();
}

void normalize_history(int n)
{
  for (int i = 0; i < 64; ++i)
  {
    for (int j = 0; j < 64; ++j)
    {
      History & hist = history_[i][j];
      hist.normalize(n);
    }
  }
}

void save_history(std::string const& fname)
{
  std::ofstream ofs(fname, std::ofstream::binary);
  if(!ofs)
    return;
  ofs.write(reinterpret_cast<char*>(history_), sizeof(history_));
}

void load_history(std::string const& fname)
{
  std::ifstream ifs(fname, std::ifstream::binary);
  if(!ifs)
    return;
  ifs.read(reinterpret_cast<char*>(history_), sizeof(history_));
}

//////////////////////////////////////////////////////////////////////////
MovesGenerator::MovesGenerator(const Board & board, const Move & killer) :
  MovesGeneratorBase(board), killer_(killer)
{
}

MovesGenerator::MovesGenerator(const Board & board) :
  MovesGeneratorBase(board)
{
  killer_.clear();
  generate();
  movesCount_ = moves_.size();
}
//////////////////////////////////////////////////////////////////////////
void MovesGenerator::generate()
{
  const Figure::Color & color = board_.color_;
  const Figure::Color ocolor = Figure::otherColor(color);

  if ( board_.checkingNum_ < 2 )
  {
    // pawns movements
    if ( board_.fmgr().pawn_mask(color) )
    {
      BitMask pw_mask = board_.fmgr().pawn_mask(color);
      for ( ; pw_mask; )
      {
        int pw_pos = clear_lsb(pw_mask);

        const int8 * table = movesTable().pawn(color, pw_pos);

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

          Move move(pw_pos, *table, Figure::TypeNone, capture);
          calculateSortValue(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;
            moves_.push_back(move);

            move.new_type_ = Figure::TypeRook;
            calculateSortValue(move);
            moves_.push_back(move);

            move.new_type_ = Figure::TypeBishop;
            calculateSortValue(move);
            moves_.push_back(move);

            move.new_type_ = Figure::TypeKnight;
            calculateSortValue(move);
            moves_.push_back(move);
          }
          else
          {
            moves_.push_back(move);
          }
        }

        for (; *table >= 0 && !board_.getField(*table); ++table)
        {
          bool promotion = *table > 55 || *table < 8;

          Move move(pw_pos, *table, Figure::TypeNone, false);
          calculateSortValue(move);

          if ( promotion )
          {
            move.new_type_ = Figure::TypeQueen;
            moves_.push_back(move);

            move.new_type_ = Figure::TypeRook;
            calculateSortValue(move);
            moves_.push_back(move);

            move.new_type_ = Figure::TypeBishop;
            calculateSortValue(move);
            moves_.push_back(move);

            move.new_type_ = Figure::TypeKnight;
            calculateSortValue(move);
            moves_.push_back(move);
          }
          else
          {
            moves_.push_back(move);
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

        const int8 * table = movesTable().knight(kn_pos);

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

          add(kn_pos, *table, Figure::TypeNone, capture);
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

        const uint16 * table = movesTable().move(type-Figure::TypeBishop, fg_pos);

        for (; *table; ++table)
        {
          const int8 * packed = reinterpret_cast<const int8*>(table);
          int8 count = packed[0];
          int8 const& delta = packed[1];

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

            add(fg_pos, p, Figure::TypeNone, capture);
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

    const int8 * table = movesTable().king(ki_pos);

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

      add(ki_pos, *table, Figure::TypeNone, capture);
    }

    if ( !board_.underCheck() )
    {
      // short castle
      if ( board_.castling(board_.color_, 0) && !board_.getField(ki_pos+2) && !board_.getField(ki_pos+1) )
        add(ki_pos, ki_pos+2, Figure::TypeNone, false);

      // long castle
      if ( board_.castling(board_.color_, 1) && !board_.getField(ki_pos-2) && !board_.getField(ki_pos-1) && !board_.getField(ki_pos-3) )
        add(ki_pos, ki_pos-2, Figure::TypeNone, false);
    }
  }
}

} // NEngine
