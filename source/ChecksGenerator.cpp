/*************************************************************
  ChecksGenerator.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <MovesGenerator.h>
#include <MovesTable.h>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
ChecksGenerator::ChecksGenerator(Board & board) :
  MovesGeneratorBase(board), hmove_(0)
{
  moves_[0].clear();
}

int ChecksGenerator::generate()
{
  numOfMoves_ = genChecks();
  moves_[numOfMoves_].clear();
  return numOfMoves_;
}

int ChecksGenerator::generate(const Move & hmove)
{
  hmove_ = hmove;
  numOfMoves_ = genChecks();
  moves_[numOfMoves_].clear();
  return numOfMoves_;
}

int ChecksGenerator::genChecks()
{
  int m = 0;

  Figure::Color color  = board_.getColor();
  Figure::Color ocolor = Figure::otherColor(color);

  int oki_pos = board_.kingPos(ocolor);

  BitMask brq_mask = board_.fmgr_.bishop_mask(board_.color_) | board_.fmgr_.rook_mask(board_.color_) | board_.fmgr_.queen_mask(board_.color_);
  const BitMask & knight_check_mask = movesTable().caps(Figure::TypeKnight, oki_pos);
  const BitMask & black = board_.fmgr_.mask(Figure::ColorBlack);
  const BitMask & white = board_.fmgr_.mask(Figure::ColorWhite);
  BitMask mask_all = white | black;


  // 1. King
  {
    int ki_pos = board_.kingPos(color);

    // figure opens line between attacker and king
    bool discovered = board_.discoveredCheck(ki_pos, color, mask_all, brq_mask, oki_pos);

    const BitMask & from_mask = betweenMasks().from(oki_pos, ki_pos);

    if ( discovered )
    {
      const int8 * table = movesTable().king(ki_pos);

      for (; *table >= 0; ++table)
      {
        const Field & field = board_.getField(*table);
        if ( field )
          continue;

        // if king moves along line of attack, it covers opponent from check
        if ( from_mask & set_mask_bit(*table) )
          continue;

        add(m, ki_pos, *table, Figure::TypeNone, discovered);
      }
    }

    // castling also could be with check
    BitMask all_but_king_mask = ~mask_all | set_mask_bit(ki_pos);
    if ( board_.castling(board_.color_, 0) && !board_.getField(ki_pos+1) && !board_.getField(ki_pos+2) ) // short
    {
      static int rook_positions[] = { 63, 7 };
      int & r_pos = rook_positions[board_.color_];
      const Field & rfield = board_.getField(r_pos);
      X_ASSERT( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
      int r_pos_to = r_pos - 2;
      BitMask rk_mask = betweenMasks().between(r_pos_to, oki_pos);
      if ( figureDir().dir(Figure::TypeRook, board_.color_, r_pos_to, oki_pos) >= 0 && (rk_mask & all_but_king_mask) == rk_mask )
      {
        add(m, ki_pos, ki_pos+2, Figure::TypeNone, discovered);
      }
    }

    if ( board_.castling(board_.color_, 1) && !board_.getField(ki_pos-1) &&
        !board_.getField(ki_pos-2) && !board_.getField(ki_pos-3) ) // long
    {
      static int rook_positions[] = { 56, 0 };
      int & r_pos = rook_positions[board_.color_];
      const Field & rfield = board_.getField(r_pos);
      X_ASSERT( rfield.type() != Figure::TypeRook || rfield.color() != board_.color_, "no rook for castling, but castle is possible" );
      int r_pos_to = r_pos + 3;
      BitMask rk_mask = betweenMasks().between(r_pos_to, oki_pos);
      if ( figureDir().dir(Figure::TypeRook, board_.color_, r_pos_to, oki_pos) >= 0 && (rk_mask & all_but_king_mask) == rk_mask )
        add(m, ki_pos, ki_pos-2, Figure::TypeNone, discovered);
    }
  }

  // 2. Bishop + Rook + Queen
  {
    for (int t = Figure::TypeBishop; t < Figure::TypeKing; ++t)
    {
      Figure::Type type = (Figure::Type)t;
      BitMask fg_mask = board_.fmgr().type_mask(type, color);

      for ( ; fg_mask; )
      {
        int fg_pos = clear_lsb(fg_mask);

        bool discovered = false;
        if ( type != Figure::TypeQueen )
          discovered = board_.discoveredCheck(fg_pos, color, mask_all, brq_mask, oki_pos);

        if ( discovered )
        {
          const uint16 * table = movesTable().move(type-Figure::TypeBishop, fg_pos);

          for (; *table; ++table)
          {
            const int8 * packed = reinterpret_cast<const int8*>(table);
            int8 count = packed[0];
            int8 delta = packed[1];

            int8 p = fg_pos;
            for ( ; count; --count)
            {
              p += delta;

              const Field & field = board_.getField(p);
              if ( field )
                break;

              add(m, fg_pos, p, Figure::TypeNone, discovered);

              if ( field )
                break;
            }
          }
        }
        else
        {
          BitMask mask_all_inv_ex = ~(mask_all & ~set_mask_bit(fg_pos));
          BitMask f_msk = movesTable().caps(type, fg_pos) & movesTable().caps(type, oki_pos);
          f_msk &= ~mask_all;

          for ( ; f_msk; )
          {
            int to = clear_lsb(f_msk);

            X_ASSERT( board_.getField(to), "field, from that we are going to check is occupied" );

            // can we check from this position?
            if ( board_.is_something_between(to, oki_pos, mask_all_inv_ex) )
              continue;

            // can we go to this position?
            if ( board_.is_something_between(to, fg_pos, mask_all_inv_ex) )
              continue;

            add(m, fg_pos, to, Figure::TypeNone, discovered);
          }
        }
      }
    }
  }

  // 3. Knight
  {
    BitMask kn_mask = board_.fmgr().knight_mask(color);
    for ( ; kn_mask; )
    {
      int kn_pos = clear_lsb(kn_mask);

      bool discovered = board_.discoveredCheck(kn_pos, color, mask_all, brq_mask, oki_pos);

      BitMask kn_msk = movesTable().caps(Figure::TypeKnight, kn_pos) & ~mask_all;
      if ( !discovered )
        kn_msk &= knight_check_mask;

      for ( ; kn_msk; )
      {
        int to = clear_lsb(kn_msk);
        X_ASSERT( board_.getField(to), "field, from that we are going to check is occupied" );
        add(m, kn_pos, to, Figure::TypeNone, discovered);
      }
    }
  }

  // 4. Pawn
  {
    // 1st find immediate checks with or without capture
    BitMask pw_check_mask = movesTable().pawnCaps(ocolor, oki_pos);
    BitMask looked_up = 0;

    for ( ; pw_check_mask; )
    {
      int to = clear_lsb(pw_check_mask);
      const Field & tfield = board_.getField(to);
      if ( tfield || to == board_.en_passant_ )
        continue;

      // usual moves
      {
        BitMask inv_mask_all = ~mask_all;
        BitMask pw_from = movesTable().pawnFrom(color, to) & board_.fmgr().pawn_mask(color);
        looked_up |= pw_from;

        for ( ; pw_from; )
        {
          int from = clear_lsb(pw_from);
          if ( board_.is_something_between(from, to, inv_mask_all) )
            continue;

          add(m, from, to, Figure::TypeNone, false);
        }
      }
    }

    // discovered checks
    {
      BitMask disc_mask = movesTable().caps(Figure::TypeQueen, oki_pos) & board_.fmgr().pawn_mask(color);
      disc_mask &= ~looked_up;

      for ( ; disc_mask; )
      {
        int from = clear_lsb(disc_mask);
        bool discovered = board_.discoveredCheck(from, color, mask_all, brq_mask, oki_pos);
        if ( !discovered )
          continue;

        // usual moves
        const BitMask & from_oki_mask = betweenMasks().from(oki_pos, from);
        const int8 * table = movesTable().pawn(board_.color_, from) + 2;

        for ( ; *table >= 0 && !board_.getField(*table); ++table)
        {
          // don't generate promotions twice
          int y = (*table) >> 3;
          if ( y == 0 || y == 7 )
            break;

          // pawn shouldn't cover opponents king in its new position - i.e it shouldn't go to the same line
          if ( from_oki_mask & set_mask_bit(*table) )
            break;

          add(m, from, *table, Figure::TypeNone, true /* discovered */);
        }
      }
    }
  }

  return m;
}

} // NEngine
