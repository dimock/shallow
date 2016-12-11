/*************************************************************
  see.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <Board.h>
#include <Helpers.h>
#include <fstream>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
static const uint16 see_tp_endl = (uint16)-1;

inline uint16 see_pack_tp(const Figure::Type t, int p)
{
  return t | (p << 8);
}

inline Figure::Type see_unpack_t(uint16 v)
{
  return (Figure::Type)(v & 255);
}

inline uint8 see_unpack_p(uint16 v)
{
  return (v >> 8) & 255;
}
//////////////////////////////////////////////////////////////////////////

// static exchange evaluation
// have to be called before doing move
int Board::see_old(const Move & move) const
{
  if ( state_ == Invalid )
    return 0;

  const Field & ffield = getField(move.from_);
  const Field & tfield = getField(move.to_);

  int score_gain = 0;
  bool promotion = ((move.to_ >> 3) == 0 || (move.to_ >> 3) == 7) && (ffield.type() == Figure::TypePawn);

  if ( tfield.type() )
  {
    // victim >= attacker
    if ( Figure::figureWeight_[tfield.type()] >= Figure::figureWeight_[ffield.type()] )
      return Figure::figureWeight_[tfield.type()] - Figure::figureWeight_[ffield.type()];
  }
  // en-passant
  else if ( !tfield && ffield.type() == Figure::TypePawn && move.to_ == en_passant_ )
  {
    X_ASSERT(getField(enpassantPos()).type() != Figure::TypePawn || getField(enpassantPos()).color() == color_, "no en-passant pawn");
    return 0;
  }
  // promotion with capture
  else if ( promotion && tfield )
  {
    return Figure::figureWeight_[tfield.type()]-Figure::figureWeight_[Figure::TypePawn];
  }

  Figure::Color color = ffield.color();
  Figure::Color ocolor = Figure::otherColor(color);

  ScoreType fscore = 0;

  if ( tfield )
    fscore = Figure::figureWeight_[tfield.type()];

  // collect all attackers for each side
  // lo-byte = type, hi-byte = pos
  uint16 attackers[2][NumOfFields];
  int figsN[2] = {0, 0}; 
  bool king_found[2] = { false, false };
  uint64 brq_masks[2] = {0ULL, 0ULL};
  int ki_pos[2] = { kingPos(Figure::ColorBlack), kingPos(Figure::ColorWhite) };

  // push 1st move
  attackers[color][figsN[color]++] = see_pack_tp(ffield.type(), move.from_);

  // prepare mask of all figures 
  Figure::Color col = color;
  uint64 all_mask_inv = (~(fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite))) & (~set_mask_bit(move.to_));

  {
    BitMask to_mask_inv = ~set_mask_bit(move.to_);
    brq_masks[0] = fmgr_.bishop_mask(Figure::ColorBlack) | fmgr_.rook_mask(Figure::ColorBlack) | fmgr_.queen_mask(Figure::ColorBlack);
    brq_masks[0] &= to_mask_inv;

    brq_masks[1] = fmgr_.bishop_mask(Figure::ColorWhite) | fmgr_.rook_mask(Figure::ColorWhite) | fmgr_.queen_mask(Figure::ColorWhite);
    brq_masks[1] &= to_mask_inv;
  }


  // our move is discovered check
  //X_ASSERT(see_check(ocolor, move.from_, ki_pos[ocolor], all_mask_inv, brq_masks[color]) !=
  //         see_check_mbb(color, move.from_, move.to_, ki_pos[ocolor], all_mask_inv),
  //         "invalid see check");
  //{
  //  auto a = see_check(ocolor, move.from_, ki_pos[ocolor], all_mask_inv, brq_masks[color]);
  //  auto b = see_check_mbb(color, move.from_, move.to_, ki_pos[ocolor], all_mask_inv);
  //  auto fen = toFEN(*this);
  //  std::ofstream of("fen.txt");
  //  of << fen;
  //};

  if(see_check(ocolor, move.from_, ki_pos[ocolor], all_mask_inv, brq_masks[color]))
  //if(see_check_mbb(color, move.from_, move.to_, ki_pos[ocolor], all_mask_inv))
    return fscore;

  BitMask mask_all_inv_express = all_mask_inv | set_mask_bit(move.from_);

  int  c = ocolor;
  for (int i = 0; i < 2; ++i, c = (c+1) & 1 )
  {
    int & num = figsN[c];

    // pawns
    uint64 pmask = fmgr_.pawn_mask((Figure::Color)c) & movesTable().pawnCaps(Figure::otherColor((Figure::Color)c), move.to_);
    for ( ; pmask; )
    {
      int n = clear_lsb(pmask);
      if ( n != move.from_ )
      {
        attackers[c][num++] = see_pack_tp(Figure::TypePawn, n);

        // try 1st opponent's capture
        // if it's winning we don't need to continue, because we definitely loose 
        if ( c == ocolor && !promotion && ffield.type() > Figure::TypePawn )
        {
          ScoreType gain = fscore - Figure::figureWeight_[ffield.type()] + Figure::figureWeight_[Figure::TypePawn];
          
          // we loose and opponent's move valid
          //X_ASSERT(see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]) !=
          //         see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express),
          //         "invalid see check");
          //{
          //  auto a = see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]);
          //  auto b = see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express);
          //  auto fen = toFEN(*this);
          //  std::ofstream of("fen.txt");
          //  of << fen;

          //}

          if(gain < 0 && //!see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express))
            !see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]))
            return gain;
        }
      }
    }

    // knights
    uint64 nmask = fmgr_.knight_mask((Figure::Color)c) & movesTable().caps(Figure::TypeKnight, move.to_);
    for ( ; nmask; )
    {
      int n = clear_lsb(nmask);
      if ( n != move.from_ )
      {
        attackers[c][num++] = see_pack_tp(Figure::TypeKnight, n);

        // try 1st opponent's capture
        // if it's winning we don't need to continue, because we definitely loose 
        if ( c == ocolor && !promotion && ffield.type() > Figure::TypeBishop )
        {
          ScoreType gain = fscore - Figure::figureWeight_[ffield.type()] + Figure::figureWeight_[Figure::TypeKnight];

          // we loose and opponent's move valid
          //X_ASSERT(see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]) !=
          //         see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express),
          //         "see_check new invalid");

          if(gain < 0 && //!see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express))
             !see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]))
            return gain;
        }
      }
    }

    // bishops
    uint64 bmask = fmgr_.bishop_mask((Figure::Color)c) & movesTable().caps(Figure::TypeBishop, move.to_);
    for ( ; bmask; )
    {
      int n = clear_lsb(bmask);
      if ( n != move.from_ )
      {
        attackers[c][num++] = see_pack_tp(Figure::TypeBishop, n);

        // try 1st opponent's capture
        // if it's winning we don't need to continue, because we definitely loose 
        if ( c == ocolor && !promotion && ffield.type() > Figure::TypeBishop )
        {
          ScoreType gain = fscore - Figure::figureWeight_[ffield.type()] + Figure::figureWeight_[Figure::TypeBishop];

          // we loose and opponent's move valid
          //X_ASSERT(see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]) !=
          //         see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express),
          //         "see_check new invalid");
          if ( gain < 0 && !is_something_between(n, move.to_, all_mask_inv) &&
              //!see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express))
              !see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]))
          {
            return gain;
          }
        }
      }
    }

    // rooks
    uint64 rmask = fmgr_.rook_mask((Figure::Color)c) & movesTable().caps(Figure::TypeRook, move.to_);
    for ( ; rmask; )
    {
      int n = clear_lsb(rmask);
      if ( n != move.from_ )
      {
        attackers[c][num++] = see_pack_tp(Figure::TypeRook, n);

        // try 1st opponent's capture
        // if it's winning we don't need to continue, because we definitely loose 
        if ( c == ocolor && !promotion && ffield.type() > Figure::TypeRook )
        {
          ScoreType gain = fscore - Figure::figureWeight_[ffield.type()] + Figure::figureWeight_[Figure::TypeRook];

          // we loose and opponent's move valid
          X_ASSERT(see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]) !=
                   see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express),
                   "see_check new invalid");

          if ( gain < 0 && !is_something_between(n, move.to_, all_mask_inv) &&
              //!see_check_mbb(color, n, move.to_, ki_pos[ocolor], mask_all_inv_express))
              !see_check(ocolor, n, ki_pos[ocolor], mask_all_inv_express, brq_masks[color]))
          {
            return gain;
          }
        }
      }
    }

    // queens
    uint64 qmask = fmgr_.queen_mask((Figure::Color)c) & movesTable().caps(Figure::TypeQueen, move.to_);
    for ( ; qmask; )
    {
      int n = clear_lsb(qmask);
      if ( n != move.from_ )
        attackers[c][num++] = see_pack_tp(Figure::TypeQueen, n);
    }

    // king
    BitMask kmask = fmgr_.king_mask((Figure::Color)c) & movesTable().caps(Figure::TypeKing, move.to_);
    if ( kmask )
    {
      // save kings positions
      if ( ki_pos[c] != move.from_ )
      {
        attackers[c][num++] = see_pack_tp(Figure::TypeKing, ki_pos[c]);
        king_found[c] = true;
      }
      else
      {
        // if king's movement is 1st its is last. we can't make recapture after it
        num = 1;
      }
    }

    attackers[c][num] = see_tp_endl;
  }

  // if there are both kings they can't capture
  if ( king_found[0] && king_found[1] )
  {
    X_ASSERT( figsN[0] < 1 || figsN[1] < 1 , "see: no figures but both kings found?" );
    attackers[0][--figsN[0]] = see_tp_endl;
    attackers[1][--figsN[1]] = see_tp_endl;
  }

  if ( figsN[color] < 1 )
    return score_gain;

  // starting calculation
  for ( ;; )
  {
    // find attacker with minimal value
    uint16 attc = 0;
    for (int i = 0; !attc && i < figsN[col]; ++i)
    {
      if ( !attackers[col][i] )
        continue;

      Figure::Type t = see_unpack_t( attackers[col][i] );
      uint8 pos = see_unpack_p( attackers[col][i] );

      switch ( t )
      {
      case Figure::TypePawn:
      case Figure::TypeKnight:
        {
          auto ocol = Figure::otherColor(col);
          //if(see_check(col, see_unpack_p(attackers[col][i]), ki_pos[col], all_mask_inv, brq_masks[ocol]) !=
          //   see_check_mbb(Figure::otherColor(col), see_unpack_p(attackers[col][i]), move.to_, ki_pos[col], all_mask_inv))
          //{
          //  auto a = see_check(col, see_unpack_p(attackers[col][i]), ki_pos[col], all_mask_inv, brq_masks[ocol]);
          //  auto b = see_check_mbb(Figure::otherColor(col), see_unpack_p(attackers[col][i]), move.to_, ki_pos[col], all_mask_inv);
          //  auto fen = toFEN(*this);
          //  std::ofstream of("fen.txt");
          //  of << fen;
          //}

          bool is_checking = see_check(col, see_unpack_p(attackers[col][i]),
            ki_pos[col], all_mask_inv, brq_masks[Figure::otherColor(col)]);

          //bool is_checking = see_check_mbb(Figure::otherColor(col), see_unpack_p(attackers[col][i]), move.to_, ki_pos[col], all_mask_inv);

          if ( !is_checking )
          {
            attc = attackers[col][i];
            attackers[col][i] = 0;
          }
          // illegal move
          else if ( col == color && 0 == i )
            return -Figure::MatScore;
        }
        break;

        // bishop | rook | queen
      case Figure::TypeBishop:
      case Figure::TypeRook:
      case Figure::TypeQueen:
        {
          // can go to target field
          if ( is_something_between(pos, move.to_, all_mask_inv) )
            continue;

          auto ocol = Figure::otherColor(col);
          //X_ASSERT(see_check(col, see_unpack_p(attackers[col][i]), ki_pos[col], all_mask_inv, brq_masks[ocol]) !=
          //         see_check_mbb(Figure::otherColor(col), see_unpack_p(attackers[col][i]), move.to_, ki_pos[col], all_mask_inv),
          //         "see_check new invalid");

          bool is_checking = see_check(col, see_unpack_p(attackers[col][i]),
            ki_pos[col], all_mask_inv, brq_masks[Figure::otherColor(col)]);

          //bool is_checking = see_check_mbb(Figure::otherColor(col), see_unpack_p(attackers[col][i]), move.to_, ki_pos[col], all_mask_inv);

          if ( !is_checking )
          {
            attc = attackers[col][i];
            attackers[col][i] = 0;
          }
          // illegal move
          else if ( col == color && 0 == i )
            return -Figure::MatScore;
        }
        break;

        // only king left. need to verify check on target field
      case Figure::TypeKing:
        {
          bool check = false;
          int oc = (col+1) & 1;
          for (int j = 0; j < figsN[oc] && !check; ++j)
          {
            if ( !attackers[oc][j] )
              continue;

            Figure::Type ot = see_unpack_t(attackers[oc][j]);
            uint8 opos = see_unpack_p(attackers[oc][j]);
            if ( ot == Figure::TypePawn || ot == Figure::TypeKnight )
              check = true;
            else
            {
              if ( !is_something_between(opos, move.to_, all_mask_inv) )
                check = true;
            }
          }
          if ( !check )
          {
            attc = attackers[col][i];
            attackers[col][i] = 0;
          }
        }
        break;
      }
    }

    if ( !attc )
      break;

    Figure::Type t = see_unpack_t(attc);
    uint8 pos = see_unpack_p(attc);

    score_gain += fscore;
    if ( t == Figure::TypePawn && promotion )
    {
      int dscore = Figure::figureWeight_[Figure::TypeQueen]-Figure::figureWeight_[Figure::TypePawn];
      if ( col != color )
        dscore = -dscore;
      score_gain += dscore;
      fscore = (col != color) ? Figure::figureWeight_[Figure::TypeQueen] : -Figure::figureWeight_[Figure::TypeQueen];
    }
    else
      fscore = (col != color) ? Figure::figureWeight_[t] : -Figure::figureWeight_[t];

    // don't need to continue if we haven't won material after capture
    if ( score_gain > 0 && col != color || score_gain < 0 && col == color )
      break;

    // if we discovers check we don't need to continue
    Figure::Color ki_col = Figure::otherColor(col);

    //X_ASSERT(see_check(ki_col, pos, ki_pos[ki_col], all_mask_inv, brq_masks[col]) !=
    //         see_check_mbb(col, pos, move.to_, ki_pos[ki_col], all_mask_inv),
    //         "see_check new invalid");

    if(pos != move.from_ && //see_check_mbb(col, pos, move.to_, ki_pos[ki_col], all_mask_inv))
       see_check(ki_col, pos, ki_pos[ki_col], all_mask_inv, brq_masks[col]))
      break;

    // remove from (inverted) mask
    all_mask_inv |= set_mask_bit(pos);

    // add to move.to_ field
    all_mask_inv &= ~set_mask_bit(move.to_);

    // remove from brq mask
    brq_masks[col] &= ~set_mask_bit(pos);

    // change color
    col = Figure::otherColor(col);
  }

  return score_gain;
}

// VERSION 2
// static exchange evaluation
// have to be called before doing move
namespace
{
  struct SeeCalc
  {
    Board const& board_;
    Move const&  move_;
    uint64 all_mask_;
    uint64 mask_to_;
    uint64 brq_masks_[2];
    uint64 masks_[2][Figure::TypesNum] = {};
    int ki_pos_[2];
    uint64 bq_mask_[2];
    uint64 rq_mask_[2];
    uint64 last_b_[2] = {};
    uint64 last_r_[2] = {};
    Figure::Color color_;
    bool promotion_;
    bool king_only_;
    bool illegal_{};

    SeeCalc(Board const& board, Move const& move) :
      board_(board),
      move_(move)
    {
      mask_to_ = set_mask_bit(move_.to_);
      all_mask_ = board_.fmgr().mask(Figure::ColorBlack) | board_.fmgr().mask(Figure::ColorWhite) | mask_to_;
      all_mask_ &= ~set_mask_bit(move_.from_);
      promotion_ = (move_.to_ >> 3) == 0 || (move_.to_ >> 3) == 7;
      color_ = board_.getField(move_.from_).color();
      BitMask to_mask_inv = ~mask_to_;
      brq_masks_[0] = board_.fmgr().bishop_mask(Figure::ColorBlack) | board_.fmgr().rook_mask(Figure::ColorBlack) | board_.fmgr().queen_mask(Figure::ColorBlack);
      brq_masks_[0] &= to_mask_inv;
      brq_masks_[1] = board_.fmgr().bishop_mask(Figure::ColorWhite) | board_.fmgr().rook_mask(Figure::ColorWhite) | board_.fmgr().queen_mask(Figure::ColorWhite);
      brq_masks_[1] &= to_mask_inv;
      ki_pos_[0] = board_.kingPos(Figure::ColorBlack);
      ki_pos_[1] = board_.kingPos(Figure::ColorWhite);
      if(discovered_check(color_, Figure::otherColor(color_), ~all_mask_, move_.from_))
      {
        illegal_ = true;
        return;
      }
      bq_mask_[0] = board_.fmgr().bishop_mask(Figure::ColorBlack) | board_.fmgr().queen_mask(Figure::ColorBlack);
      bq_mask_[1] = board_.fmgr().bishop_mask(Figure::ColorWhite) | board_.fmgr().queen_mask(Figure::ColorWhite);
      rq_mask_[0] = board_.fmgr().rook_mask(Figure::ColorBlack) | board_.fmgr().queen_mask(Figure::ColorBlack);
      rq_mask_[1] = board_.fmgr().rook_mask(Figure::ColorWhite) | board_.fmgr().queen_mask(Figure::ColorWhite);
      king_only_ = discovered_check(Figure::otherColor(color_), color_, ~all_mask_, move_.from_);
    }

    inline bool next(Figure::Color color, int& from, int& score)
    {
      return (!king_only_ && move_figure(color, from, score)) || move_king(color, from, score);
    }

    inline bool move_figure(Figure::Color color, int& from, int& score)
    {
      auto ocolor = Figure::otherColor(color);
      if(board_.fmgr().pawn_mask(color) & all_mask_)
      {
        masks_[color][Figure::TypePawn] = board_.fmgr().pawn_mask(color)
          & all_mask_
          & movesTable().pawnCaps(ocolor, move_.to_);
        if(do_move(color, ocolor, Figure::TypePawn, from, score))
          return true;
      }

      //for(auto type : {Figure::TypeKnight, Figure::TypeBishop, Figure::TypeRook, Figure::TypeQueen})
      {
        Figure::Type const type = Figure::TypeKnight;
        if(!masks_[color][type]
           && (board_.fmgr().type_mask(type, color) & all_mask_))
        {
          masks_[color][type] = figure_mask(color, type);
        }
        if(do_move(color, ocolor, type, from, score))
          return true;
      }

      uint64 mask_b{};
      if(!masks_[color][Figure::TypeBishop] &&(bq_mask_[color] & all_mask_))
      {
        last_b_[color] = mask_b = magic_ns::bishop_moves(move_.to_, all_mask_) & all_mask_;
        masks_[color][Figure::TypeBishop] = mask_b & board_.fmgr().bishop_mask(color);
      }
      if(do_move(color, ocolor, Figure::TypeBishop, from, score))
        return true;

      uint64 mask_r{};
      if(!masks_[color][Figure::TypeRook] && (rq_mask_[color] & all_mask_))
      {
        last_r_[color] = mask_r = magic_ns::rook_moves(move_.to_, all_mask_) & all_mask_;
        masks_[color][Figure::TypeRook] = mask_r & board_.fmgr().rook_mask(color);
      }
      if(do_move(color, ocolor, Figure::TypeRook, from, score))
        return true;

      if(!masks_[color][Figure::TypeQueen] && (board_.fmgr().queen_mask(color) & all_mask_))
      {
        if(!mask_b)
          mask_b = magic_ns::bishop_moves(move_.to_, all_mask_);
        if(!mask_r)
          mask_r = magic_ns::rook_moves(move_.to_, all_mask_);
        masks_[color][Figure::TypeQueen] = (mask_b | mask_r) & all_mask_ & board_.fmgr().queen_mask(color);
      }
      if(do_move(color, ocolor, Figure::TypeQueen, from, score))
        return true;

      return false;
    }

    inline bool move_king(Figure::Color color, int& from, int& score)
    {
      auto ocolor = Figure::otherColor(color);
      if(figure_mask(color, Figure::TypeKing) && !figure_mask(ocolor, Figure::TypeKing)
         && !under_check(color, ocolor))
      {
        // score == 0 means king's attack. it should be the very last one
        score = 0;
        from = ki_pos_[color];
        return true;
      }
      return false;
    }
    //inline bool do_move(Figure::Color color, Figure::Color ocolor, Figure::Type const type, int& from, int& score)
    //{
    //  auto mask = masks_[color][type];
    //  while(mask)
    //  {
    //    from = clear_lsb(mask);
    //    if(move_impossible(type, from, all_mask_inv) || discovered_check(color, ocolor, ~all_mask_, from))
    //    {
    //      continue;
    //    }
    //    auto mask_from_inv = ~set_mask_bit(from);
    //    masks_[color][type] &= mask_from_inv;
    //    all_mask_ &= mask_from_inv;
    //    brq_masks_[ocolor] &= mask_from_inv;
    //    score = promotion_ && type == Figure::TypePawn
    //      ? Figure::figureWeight_[Figure::TypeQueen] - Figure::figureWeight_[Figure::TypePawn]
    //      : Figure::figureWeight_[type];
    //    if(color == color_)
    //      score = -score;
    //    return true;
    //  }
    //  return false;
    //}

    //inline bool move_impossible(Figure::Type type, int from, uint64 all_mask_inv) const
    //{
    //    if(type == Figure::TypePawn || type == Figure::TypeKnight)
    //        return false;
    //    return board_.is_something_between(from, move_.to_, all_mask_inv);
    //}

    inline uint64 figure_mask(Figure::Color color, Figure::Type type) const
    {
        return board_.fmgr().type_mask(type, color) & movesTable().caps(type, move_.to_) & all_mask_;
    }

    inline bool do_move(Figure::Color color, Figure::Color ocolor, Figure::Type const type, int& from, int& score)
    {
      while(masks_[color][type])
      {
        from = clear_lsb(masks_[color][type]);
        if(discovered_check(color, ocolor, ~all_mask_, from))
          continue;
        auto mask_from_inv = ~set_mask_bit(from);
        all_mask_ &= mask_from_inv;
        brq_masks_[ocolor] &= mask_from_inv;
        score = promotion_ && type == Figure::TypePawn
          ? Figure::figureWeight_[Figure::TypeQueen] - Figure::figureWeight_[Figure::TypePawn]
          : Figure::figureWeight_[type];
        if(color == color_)
          score = -score;
        return true;
      }
      return false;
    }

    inline bool discovered_check(Figure::Color color, Figure::Color ocolor, uint64 all_mask_inv, int from) const
    {
      return board_.see_check(color, from, ki_pos_[color], all_mask_inv, brq_masks_[ocolor]);
    }

    inline bool under_check(Figure::Color color, Figure::Color ocolor) const
    {
      if(board_.fmgr().pawn_mask(ocolor) & all_mask_ & movesTable().pawnCaps(color, move_.to_))
        return true;

      if(board_.fmgr().knight_mask(ocolor) & all_mask_ & movesTable().caps(Figure::TypeKnight, move_.to_))
        return true;

      if((bq_mask_[ocolor] & all_mask_)
         && ((last_b_[ocolor] & all_mask_ & bq_mask_[ocolor]) || (magic_ns::bishop_moves(move_.to_, all_mask_) & all_mask_ & bq_mask_[ocolor])))
      {
        return true;
      }

      if((rq_mask_[ocolor] & all_mask_)
         && ((last_r_[ocolor] & all_mask_ & rq_mask_[ocolor]) || (magic_ns::rook_moves(move_.to_, all_mask_) & all_mask_ & rq_mask_[ocolor])))
      {
        return true;
      }

      return false;
    }
  };
}

//int Board::see_failed_ = 0;
//int Board::see(const Move & move) const
//{
//  auto s1 = see_old(move);
//  auto s2 = see_new(move);
//  if((s1 < 0) && (s2 >= 0))
//  {
//    //{
//    //  auto fen = toFEN(*this);
//    //  std::ofstream of("fen.txt");
//    //  auto smv = moveToStr(move, false);
//    //  of << fen << " bm " << smv;
//    //}
//    //exit(0);
//    see_failed_++;
//  }
//  return s2;
//}

int Board::see(const Move & move) const
{
  if(state_ == Invalid)
    return 0;

  const Field & ffield = getField(move.from_);
  const Field & tfield = getField(move.to_);

  const bool promotion = ((move.to_ >> 3) == 0 || (move.to_ >> 3) == 7) && (ffield.type() == Figure::TypePawn);

  if(tfield.type())
  {
    // victim >= attacker
    if(Figure::figureWeight_[tfield.type()] >= Figure::figureWeight_[ffield.type()])
      return Figure::figureWeight_[tfield.type()] - Figure::figureWeight_[ffield.type()];
  }
  // en-passant
  else if(!tfield && ffield.type() == Figure::TypePawn && move.to_ == en_passant_)
  {
    X_ASSERT(getField(enpassantPos()).type() != Figure::TypePawn
             || getField(enpassantPos()).color() == color_,
             "no en-passant pawn");
    return 0;
  }
  // promotion with capture
  else if(promotion && tfield)
  {
    return Figure::figureWeight_[tfield.type()]-Figure::figureWeight_[Figure::TypePawn];
  }

  int score_gain = (tfield) ? Figure::figureWeight_[tfield.type()] : 0;
  int fscore = -Figure::figureWeight_[ffield.type()];

  auto color = Figure::otherColor(ffield.color());
  SeeCalc see_calc(*this, move);

  if(see_calc.illegal_)
    return -Figure::MatScore;

  // assume discovered check is always good
  if(see_calc.king_only_)
    return 0;

  for(;;)
  {
    int from{};
    int score{};
    if(!see_calc.next(color, from, score))
      break;
    auto ocolor = Figure::otherColor(color);
    see_calc.king_only_ = see_calc.discovered_check(ocolor, color, ~see_calc.all_mask_, from);
    // assume this move is good if it was mine and I discover check
    if(see_calc.king_only_ && color == see_calc.color_)
      return 0;
    score_gain += fscore;
    // king's move always last
    if(score == 0)
      break;
    // already winner even if loose last moved figure
    if(score_gain + score >= 0 && color == see_calc.color_)
      return score_gain + score;
    // could not gain something or already winner
    if((score_gain < 0 && color == see_calc.color_)
       || (score_gain >= 0 && color != see_calc.color_))
    {
      return score_gain;
    }
    fscore = score;
    color = ocolor;
  }

  return score_gain;
}


} // NEngine
