#include <xoptimize.h>
#include <MovesGenerator.h>
#include <xindex.h>
#include <Helpers.h>
#include <boost/algorithm/string/trim.hpp>
#include <algorithm>

namespace NEngine
{

int64 x_movesCounter = 0;

namespace
{

void generate(Board2 const& board, xlist<Move2, Board2::MovesMax>& moves)
{
  const Figure::Color & color = board.color();
  const Figure::Color ocolor = Figure::otherColor(color);
  auto const& fmgr = board.fmgr();

  if(!board.underCheck() || !board.doubleCheck())
  {
    // pawns movements
    if(BitMask pw_mask = fmgr.pawn_mask(color))
    {
      for(; pw_mask;)
      {
        int pw_pos = clear_lsb(pw_mask);

        const int8* table = movesTable().pawn(color, pw_pos);

        for(int i = 0; i < 2; ++i, ++table)
        {
          if(*table < 0)
            continue;

          const Field & field = board.getField(*table);
          bool capture = false;
          if((field && field.color() == ocolor) ||
             (board.enpassant() > 0 && *table == board.enpassant()))
          {
            capture = true;
          }

          if(!capture)
            continue;

          bool promotion = *table > 55 || *table < 8;

          Move2 move{ pw_pos, *table, Figure::TypeNone };

          if(promotion)
          {
            move.new_type = Figure::TypeQueen;
            moves.push_back(move);

            move.new_type = Figure::TypeRook;
            moves.push_back(move);

            move.new_type = Figure::TypeBishop;
            moves.push_back(move);

            move.new_type = Figure::TypeKnight;
            moves.push_back(move);
          }
          else
          {
            moves.push_back(move);
          }
        }

        for(; *table >= 0 && !board.getField(*table); ++table)
        {
          bool promotion = *table > 55 || *table < 8;

          Move2 move{ pw_pos, *table, Figure::TypeNone };

          if(promotion)
          {
            move.new_type = Figure::TypeQueen;
            moves.push_back(move);

            move.new_type = Figure::TypeRook;
            moves.push_back(move);

            move.new_type = Figure::TypeBishop;
            moves.push_back(move);

            move.new_type = Figure::TypeKnight;
            moves.push_back(move);
          }
          else
          {
            moves.push_back(move);
          }
        }
      }
    }

    // knights movements
    if(fmgr.knight_mask(color))
    {
      BitMask kn_mask = fmgr.knight_mask(color);
      for(; kn_mask;)
      {
        int kn_pos = clear_lsb(kn_mask);

        const int8* table = movesTable().knight(kn_pos);

        for(; *table >= 0; ++table)
        {
          const Field & field = board.getField(*table);
          bool capture = false;
          if(field)
          {
            if(field.color() == color)
              continue;

            capture = true;
          }

          moves.emplace_back(kn_pos, *table, Figure::TypeNone);
        }
      }
    }

    // bishops, rooks and queens movements
    for(int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
    {
      BitMask fg_mask = fmgr.type_mask((Figure::Type)type, color);

      for(; fg_mask;)
      {
        int fg_pos = clear_lsb(fg_mask);

        const uint16* table = movesTable().move(type-Figure::TypeBishop, fg_pos);

        for(; *table; ++table)
        {
          const int8* packed = reinterpret_cast<const int8*>(table);
          int8 count = packed[0];
          int8 const& delta = packed[1];

          int8 p = fg_pos;
          bool capture = false;
          for(; count && !capture; --count)
          {
            p += delta;

            const Field & field = board.getField(p);
            if(field)
            {
              if(field.color() == color)
                break;

              capture = true;
            }

            moves.emplace_back(fg_pos, p, Figure::TypeNone);
          }
        }
      }
    }
  }

  // kings movements
  {
    BitMask ki_mask = fmgr.king_mask(color);

    X_ASSERT(ki_mask == 0, "invalid position - no king");

    int ki_pos = clear_lsb(ki_mask);

    const int8* table = movesTable().king(ki_pos);

    for(; *table >= 0; ++table)
    {
      const Field & field = board.getField(*table);
      bool capture = false;
      if(field)
      {
        if(field.color() == color)
          continue;

        capture = true;
      }

      moves.emplace_back(ki_pos, *table, Figure::TypeNone);
    }

    if(!board.underCheck())
    {
      // short castle
      if(board.castling(board.color(), 0) && !board.getField(ki_pos+2) && !board.getField(ki_pos+1))
        moves.emplace_back(ki_pos, ki_pos+2, Figure::TypeNone);

      // long castle
      if(board.castling(board.color(), 1) && !board.getField(ki_pos-2) && !board.getField(ki_pos-1) && !board.getField(ki_pos-3))
        moves.emplace_back(ki_pos, ki_pos-2, Figure::TypeNone);
    }
  }
}

template <class BOARD, class MOVE>
struct CapsGenerator2
{
  using MovesList = xlist<MOVE, BOARD::NumOfFields>;


  CapsGenerator2(BOARD const& board) :
    board_(board)
  {}

  inline void add(int8 from, int8 to, int8 new_type)
  {
    moves_.emplace_back(from, to, new_type);
  }

  Move2* next()
  {
    if(iter_ != moves_.end())
    {
      auto* move = &*iter_;
      ++iter_;
      return move;
    }
    return nullptr;
  }

  inline void generateCaps()
  {
    const Figure::Color color = board_.color();
    const Figure::Color ocolor = Figure::otherColor(color);

    auto const& fmgr = board_.fmgr();
    BitMask opponent_mask = fmgr.mask(ocolor) ^ fmgr.king_mask(ocolor);
    const BitMask & black = fmgr.mask(Figure::ColorBlack);
    const BitMask & white = fmgr.mask(Figure::ColorWhite);
    BitMask mask_all = white | black;
    int ki_pos  = board_.kingPos(color);
    int oki_pos = board_.kingPos(ocolor);

    // generate pawn promotions
    const BitMask & pawn_msk = fmgr.pawn_mask(color);
    {
      static int pw_delta[] = { -8, +8 };
      BitMask promo_msk = movesTable().promote(color);
      promo_msk &= pawn_msk;

      for(; promo_msk;)
      {
        int from = clear_lsb(promo_msk);

        X_ASSERT((unsigned)from > 63, "invalid promoted pawn's position");

        const Field & field = board_.getField(from);

        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypePawn, "there is no pawn to promote");

        int to = from + pw_delta[color];

        X_ASSERT((unsigned)to > 63, "pawn tries to go to invalid field");

        if(board_.getField(to))
          continue;

        add(from, to, Figure::TypeQueen);

        // add promotion to checking knight
        if(figureDir().dir(Figure::TypeKnight, color, to, oki_pos) >= 0)
          add(from, to, Figure::TypeKnight);
      }
    }

    // firstly check if we have at least 1 attacking pawn
    bool pawns_eat = pawn_msk != 0ULL;
    if(pawns_eat)
    {
      BitMask pawn_eat_msk = 0;
      if(color)
        pawn_eat_msk = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
      else
        pawn_eat_msk = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
      pawns_eat = (pawn_eat_msk & opponent_mask) != 0ULL;
      if(!pawns_eat && board_.enpassant() > 0)
        pawns_eat = (pawn_eat_msk & set_mask_bit(board_.enpassant())) != 0ULL;
    }
    // generate captures

    // 1. Pawns
    if(pawns_eat)
    {
      BitMask pw_mask = fmgr.pawn_mask(color);
      BitMask promo_mask = pw_mask & movesTable().promote(color);
      pw_mask ^= promo_mask;
      for(; promo_mask;)
      {
        int pw_pos = clear_lsb(promo_mask);

        BitMask p_caps = movesTable().pawnCaps(color, pw_pos) & opponent_mask;

        for(; p_caps;)
        {
          X_ASSERT(!pawns_eat, "have pawns capture, but not detected by mask");

          int to = clear_lsb(p_caps);

          X_ASSERT(!(to > 55 || to < 8), "not a promotion"); // 1st || last line
          X_ASSERT((unsigned)to > 63, "invalid pawn's capture position");
          X_ASSERT(!board_.getField(to) || board_.getField(to).color() != ocolor, "no figure for pawns capture");
          X_ASSERT(board_.getField(to).type() == Figure::TypeKing || board_.getField(to).type() == Figure::TypePawn,
                   "captured figure should not be king|pawn");

          add(pw_pos, to, Figure::TypeQueen);

          // add promotion to checking knight
          if(figureDir().dir(Figure::TypeKnight, color, to, oki_pos) >= 0)
            add(pw_pos, to, Figure::TypeKnight);
        }
      }

      for(; pw_mask;)
      {
        int pw_pos = clear_lsb(pw_mask);

        BitMask p_caps = movesTable().pawnCaps(color, pw_pos) & opponent_mask;

        for(; p_caps;)
        {
          X_ASSERT(!pawns_eat, "have pawns capture, but not detected by mask");

          int to = clear_lsb(p_caps);

          X_ASSERT(to > 55 || to < 8, "promotion"); // 1st || last line
          X_ASSERT((unsigned)to > 63, "invalid pawn's capture position");
          X_ASSERT(!board_.getField(to) || board_.getField(to).color() != ocolor, "no figure for pawns capture");
          X_ASSERT(board_.getField(to).type() == Figure::TypeKing, "captured figure should not be king");

          add(pw_pos, to, Figure::TypeNone);
        }
      }

      if(board_.enpassant() > 0)
      {
        X_ASSERT(board_.getField(board_.enpassantPos()).type() != Figure::TypePawn || board_.getField(board_.enpassantPos()).color() != ocolor, "there is no en passant pawn");
        BitMask ep_mask = movesTable().pawnCaps(ocolor, board_.enpassant()) & fmgr.pawn_mask(color);

        for(; ep_mask;)
        {
          int from = clear_lsb(ep_mask);
          add(from, board_.enpassant(), Figure::TypeNone);
        }
      }
    }

    // 2. Knights
    BitMask kn_mask = fmgr.knight_mask(color);
    for(; kn_mask;)
    {
      int kn_pos = clear_lsb(kn_mask);

      // don't need to verify capture possibility by mask
      BitMask f_caps = movesTable().caps(Figure::TypeKnight, kn_pos) & opponent_mask;
      for(; f_caps;)
      {
        int to = clear_lsb(f_caps);

        X_ASSERT((unsigned)to > 63, "invalid field index while capture");

        const Field & field = board_.getField(to);

        X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

        add(kn_pos, to, Figure::TypeNone);
      }
    }

    {
      auto bi_mask = fmgr.type_mask(Figure::TypeBishop, color);
      for(; bi_mask;)
      {
        int from = clear_lsb(bi_mask);
        auto f_caps = magic_ns::bishop_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          int8 to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const Field & field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    {
      auto r_mask = fmgr.type_mask(Figure::TypeRook, color);
      for(; r_mask;)
      {
        int from = clear_lsb(r_mask);
        auto f_caps = magic_ns::rook_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          int8 to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const Field & field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    {
      auto q_mask = fmgr.type_mask(Figure::TypeQueen, color);
      for(; q_mask;)
      {
        int from = clear_lsb(q_mask);
        auto f_caps = magic_ns::queen_moves(from, mask_all) & opponent_mask;
        for(; f_caps;)
        {
          int8 to = clear_lsb(f_caps);

          X_ASSERT((unsigned)to > 63, "invalid field index while capture");
          const Field & field = board_.getField(to);
          X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

          add(from, to, Figure::TypeNone);
        }
      }
    }

    // 4. King
    {
      // don't need to verify capture possibility by mask
      BitMask f_caps = movesTable().caps(Figure::TypeKing, ki_pos) & opponent_mask;
      for(; f_caps;)
      {
        int to = clear_lsb(f_caps);

        X_ASSERT((unsigned)to > 63, "invalid field index while capture");

        const Field & field = board_.getField(to);

        X_ASSERT(!field || field.color() != ocolor, "there is no opponent's figure on capturing field");

        add(ki_pos, to, Figure::TypeNone);
      }
    }

    iter_ = moves_.begin();
  }

  BOARD const& board_;
  MovesList moves_;
  typename MovesList::iterator iter_;
};

template <class BOARD, class MOVE>
struct ChecksGenerator2
{
  using MovesList = xlist<MOVE, BOARD::NumOfFields>;

  ChecksGenerator2(BOARD const& board) :
    board_(board)
  {}

  inline void add(int8 from, int8 to)
  {
    moves_.emplace_back(from, to, Figure::TypeNone);
  }

  Move2* next()
  {
    if(iter_ != moves_.end())
    {
      auto* move = &*iter_;
      ++iter_;
      return move;
    }
    return nullptr;
  }

  void generate()
  {
    BitMask visited{};
    const Figure::Color & color = board_.color();
    const Figure::Color ocolor = Figure::otherColor(color);
    auto const& fmgr = board_.fmgr();
    BitMask mask_all = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
    BitMask mask_all_inv = ~mask_all;
    int oki_pos = board_.kingPos(ocolor);
    auto const pawns = fmgr.pawn_mask(color) & ~movesTable().promote(color);

    // pawns
    {
      BitMask pw_check_mask = movesTable().pawnCaps(ocolor, oki_pos);
      for(; pw_check_mask;)
      {
        int to = clear_lsb(pw_check_mask);
        const Field & tfield = board_.getField(to);
        if(tfield || to == board_.enpassant())
          continue;

        // usual moves
        BitMask pw_from = movesTable().pawnFrom(color, to) & pawns;
        visited |= pw_from;
        for(; pw_from;)
        {
          int from = clear_lsb(pw_from);
          if(board_.is_something_between(from, to, mask_all_inv))
            continue;
          add(from, to);
          break;
        }
      }
    }

    // knights
    {
      auto const& knight_check_mask = movesTable().caps(Figure::TypeKnight, oki_pos);
      BitMask kn_mask = board_.fmgr().knight_mask(color);
      for(; kn_mask;)
      {
        int from = clear_lsb(kn_mask);
        BitMask kn_moves = movesTable().caps(Figure::TypeKnight, from) & mask_all_inv;
        if(!board_.discoveredCheck(from, mask_all, color, oki_pos))
          kn_moves &= knight_check_mask;
        for(; kn_moves;)
        {
          int to = clear_lsb(kn_moves);
          X_ASSERT(board_.getField(to), "field, from that we are going to check is occupied");
          add(from, to);
        }
      }
    }
 
    // king
    {
      int ki_pos = board_.kingPos(color);
      BitMask all_but_king_mask = mask_all_inv | set_mask_bit(ki_pos);
      bool castle = false;
      // short castle
      if(board_.castling(color, 0) && (movesTable().castleMasks(color, 0) & mask_all) == 0ULL)
      {
        static int rook_positions[] = { 61, 5 };
        int const& r_pos = rook_positions[board_.color()];
        X_ASSERT(color && ki_pos != 4 ||  !color && ki_pos != 60, "invalid king position for castle");
        X_ASSERT(color && board_.getField(7).type() != Figure::TypeRook
                 || !color && board_.getField(63).type() != Figure::TypeRook, "no rook for castling, but castle is possible");
        if((oki_pos&7) == (r_pos&7) && board_.is_nothing_between(r_pos, oki_pos, mask_all_inv))
        {
          add(ki_pos, ki_pos+2);
          castle = true;
        }
      }

      // long castle
      if(!castle && board_.castling(color, 1) && (movesTable().castleMasks(color, 1) & mask_all) == 0ULL)
      {
        static int rook_positions[] = { 59, 3 };
        int const& r_pos = rook_positions[board_.color()];
        X_ASSERT(color && ki_pos != 4 ||  !color && ki_pos != 60, "invalid king position for castle");
        X_ASSERT(color && board_.getField(0).type() != Figure::TypeRook
                 || !color && board_.getField(56).type() != Figure::TypeRook, "no rook for castling, but castle is possible");
        if((oki_pos&7) == (r_pos&7) && board_.is_nothing_between(r_pos, oki_pos, mask_all_inv))
        {
          add(ki_pos, ki_pos-2);
          castle = true;
        }
      }

      if(!castle && board_.discoveredCheck(ki_pos, mask_all, color, oki_pos))
      {
        auto exclude = ~(betweenMasks().from(oki_pos, ki_pos) | movesTable().caps(Figure::TypeKing, oki_pos));
        auto ki_mask = movesTable().caps(Figure::TypeKing, ki_pos) & mask_all_inv & exclude;
        for(; ki_mask;)
        {
          auto to = clear_lsb(ki_mask);
          X_ASSERT(board_.getField(to), "king moves to occupied field");
          add(ki_pos, to);
        }
      }
    }

    auto bi_king = magic_ns::bishop_moves(oki_pos, mask_all);
    auto r_king  = magic_ns::rook_moves(oki_pos, mask_all);

    // discovers bishop|queen attack
    {
      auto pw_mask = bi_king & pawns;
      X_ASSERT(pw_mask & visited, "pawn gives check and discovers too");
      auto r_mask = bi_king & fmgr.rook_mask(color);
      if(pw_mask | r_mask)
      {
        visited |= pw_mask;
        // all checking queens and bishops if exclude pawns and rooks
        auto bq_from = magic_ns::bishop_moves(oki_pos, mask_all & ~(pw_mask | r_mask)) & (fmgr.bishop_mask(color) | fmgr.queen_mask(color));
        for(; bq_from;)
        {
          auto p = clear_lsb(bq_from);
          auto const& btw_mask = betweenMasks().between(p, oki_pos);
          auto pwm = btw_mask & pw_mask;
          if(pwm)
          {
            auto from = clear_lsb(pwm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "pawn should discoved check");
            const int8* to = movesTable().pawn(color, from) + 2; // skip captures
            for(; *to >= 0 && !board_.getField(*to); ++to)
              add(from, *to);
          }
          auto rm = btw_mask & r_mask;
          visited |= rm;
          if(rm)
          {
            auto from = clear_lsb(rm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "rook should discover check");
            generate(Figure::TypeRook, from);
          }
        }
      }
    }

    // discovers rook|queen attack
    {
      auto pw_mask = r_king & pawns & ~visited;
      auto bi_mask = r_king & fmgr.bishop_mask(color);
      if(pw_mask | bi_mask)
      {
        auto rq_from = magic_ns::rook_moves(oki_pos, mask_all & ~(pw_mask | bi_mask)) & (fmgr.rook_mask(color) | fmgr.queen_mask(color));
        for(; rq_from;)
        {
          auto p = clear_lsb(rq_from);
          auto const& btw_mask = betweenMasks().between(oki_pos, p);
          auto pwm = btw_mask & pw_mask;
          if(pwm)
          {
            auto from = clear_lsb(pwm);
            // pawn on the same X as king
            if((from&7) == (oki_pos&7))
              continue;
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "pawn should discover check");
            const int8* to = movesTable().pawn(color, from) + 2; // skip captures
            for(; *to >= 0 && !board_.getField(*to); ++to)
              add(from, *to);
          }
          auto bm = btw_mask & bi_mask;
          visited |= bm;
          if(bm)
          {
            auto from = clear_lsb(bm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "bishop should discover check");
            generate(Figure::TypeBishop, from);
          }
        }
      }
    }

    // don't change visited any more
    visited = ~visited;

    // remaining direct attacks
    // queen
    {
      auto q_mask = fmgr.queen_mask(color);
      for(; q_mask;)
      {
        auto from = clear_lsb(q_mask);
        auto q_moves = magic_ns::queen_moves(from, mask_all) & mask_all_inv & (r_king | bi_king);
        for(; q_moves;)
        {
          auto to = clear_lsb(q_moves);
          X_ASSERT(board_.getField(to), "queen goes to occupied field");
          add(from, to);
        }
      }
    }
    // bishop
    {
      auto bi_mask = fmgr.bishop_mask(color) & visited;
      for(; bi_mask;)
      {
        auto from = clear_lsb(bi_mask);
        auto bi_moves = magic_ns::bishop_moves(from, mask_all) & mask_all_inv & bi_king;
        for(; bi_moves;)
        {
          auto to = clear_lsb(bi_moves);
          X_ASSERT(board_.getField(to), "bishop goes to occupied field");
          add(from, to);
        }
      }
    }
    // rook
    {
      auto r_mask = fmgr.rook_mask(color) & visited;
      for(; r_mask;)
      {
        auto from = clear_lsb(r_mask);
        auto rr = magic_ns::rook_moves(from, mask_all);
        auto r_moves = rr & mask_all_inv & r_king;
        for(; r_moves;)
        {
          auto to = clear_lsb(r_moves);
          X_ASSERT(board_.getField(to), "rook goes to occupied field");
          add(from, to);
        }
      }
    }

    iter_ = moves_.begin();
  }

  void generate(Figure::Type type, int from)
  {
    const uint16* table = movesTable().move(type-Figure::TypeBishop, from);
    for(; *table; ++table)
    {
      const int8* packed = reinterpret_cast<const int8*>(table);
      int8 count = packed[0];
      int8 delta = packed[1];
      int8 to = from;
      for(; count; --count)
      {
        to += delta;
        if(board_.getField(to))
          break;
        add(from, to);
      }
    }
  }

  BOARD const& board_;
  MovesList moves_;
  typename MovesList::iterator iter_;
};

template <class BOARD, class MOVE>
struct UsualGenerator2
{
  using MovesList = xlist<MOVE, BOARD::MovesMax>;
  
  UsualGenerator2(BOARD const& board) :
    board_(board)
  {}

  Move2* next()
  {
    if(iter_ != moves_.end())
    {
      auto* move = &*iter_;
      ++iter_;
      return move;
    }
    return nullptr;
  }

  inline void add(int8 from, int8 to)
  {
    moves_.emplace_back(from, to, Figure::TypeNone);
  }

  inline void generate(int type, Figure::Color color, BitMask const& mask_all_inv)
  {
    BitMask fg_mask = board_.fmgr().type_mask((Figure::Type)type, color);
    for(; fg_mask;)
    {
      int fg_pos = clear_lsb(fg_mask);
      const uint16* table = movesTable().move(type-Figure::TypeBishop, fg_pos);
      for(; *table; ++table)
      {
        const int8* packed = reinterpret_cast<const int8*>(table);
        int8 count = packed[0];
        int8 delta = packed[1];
        int8 p = fg_pos;
        for(; count; --count)
        {
          p += delta;
          const Field & field = board_.getField(p);
          if(field)
            break;
          add(fg_pos, p);
        }
      }
    }
  }

  inline void generate()
  {
    const Figure::Color & color = board_.color();
    const Figure::Color ocolor = Figure::otherColor(color);
    auto const& fmgr = board_.fmgr();
    BitMask mask_all = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
    BitMask mask_all_inv = ~mask_all;

    // pawns movements except promotions cause it's already generated by caps generator
    BitMask pw_mask = fmgr.pawn_mask(color) & ~movesTable().promote(color);
    for(; pw_mask;)
    {
      int pw_pos = clear_lsb(pw_mask);
      const int8* table = movesTable().pawn(color, pw_pos) + 2; // skip captures
      for(; *table >= 0 && !board_.getField(*table); ++table)
        add(pw_pos, *table);
    }

    // knights movements
    BitMask kn_mask = fmgr.knight_mask(color);
    for(; kn_mask;)
    {
      int kn_pos = clear_lsb(kn_mask);
      BitMask kn_caps = movesTable().caps(Figure::TypeKnight, kn_pos) & mask_all_inv;

      for(; kn_caps;)
      {
        int to = clear_lsb(kn_caps);

        X_ASSERT(board_.getField(to), "try to generate capture");

        add(kn_pos, to);
      }
    }
    
    // bishops, rooks and queens movements
    generate(Figure::TypeBishop, color, mask_all_inv);
    generate(Figure::TypeRook, color, mask_all_inv);
    generate(Figure::TypeQueen, color, mask_all_inv);

    // kings movements
    int ki_pos = board_.kingPos(color);
    int oki_pos = board_.kingPos(ocolor);
    BitMask ki_mask = movesTable().caps(Figure::TypeKing, ki_pos) & mask_all_inv & ~movesTable().caps(Figure::TypeKing, oki_pos);
    if(ki_mask)
    {
      // short castle
      if(board_.castling(color, 0) && (movesTable().castleMasks(color, 0) & mask_all) == 0ULL)
      {
        X_ASSERT(color && ki_pos != 4 ||  !color && ki_pos != 60, "invalid king position for castle");
        add(ki_pos, ki_pos+2);
      }

      // long castle
      if(board_.castling(color, 1) && (movesTable().castleMasks(color, 1) & mask_all) == 0ULL)
      {
        X_ASSERT(color && ki_pos != 4 ||  !color && ki_pos != 60, "invalid king position for castle");
        add(ki_pos, ki_pos-2);
      }

      for(; ki_mask;)
      {
        int to = clear_lsb(ki_mask);
        X_ASSERT(board_.getField(to), "try to capture by king");
        add(ki_pos, to);
      }
    }

    iter_ = moves_.begin();
  }

  BOARD const& board_;
  MovesList moves_;
  typename MovesList::iterator iter_;
};

template <class BOARD, class MOVE>
struct EscapeGenerator2
{
  using MovesList = xlist<MOVE, BOARD::NumOfFields>;

  enum Order { oGenCaps, oCaps, oGenUsual, oUsual } order_{};

  EscapeGenerator2(BOARD const& board) :
    board_(board)
  {
    ocolor = Figure::otherColor(board_.color());
    king_pos_ = board_.kingPos(board_.color());

    const uint64 & black = board_.fmgr().mask(Figure::ColorBlack);
    const uint64 & white = board_.fmgr().mask(Figure::ColorWhite);
    mask_all = white | black;
  }

  inline void add_caps(int8 from, int8 to, int8 new_type)
  {
    caps_.emplace_back(from, to, new_type);
  }

  inline void add_usual(int8 from, int8 to)
  {
    usual_.emplace_back(from, to, Figure::TypeNone);
  }

  inline void generateCaps()
  {
    const Figure::Color& color = board_.color();
    protect_king_msk_ = betweenMasks().between(king_pos_, board_.checking());
    auto const& ch_pos = board_.checking();

    auto const& fmgr = board_.fmgr();

    X_ASSERT((unsigned)board_.checking() > 63, "there is no checking figure");

    // 1st - pawns
    const uint64 & pawn_msk = fmgr.pawn_mask(color);
    const uint64 & opawn_caps = movesTable().pawnCaps(ocolor, ch_pos);
    uint64 eat_msk = pawn_msk & opawn_caps;

    bool promotion = ch_pos > 55 || ch_pos < 8; // 1st || last line
    for(; eat_msk;)
    {
      int n = clear_lsb(eat_msk);

      const Field & fpawn = board_.getField(n);
      X_ASSERT(!fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != color, "no pawn on field we are going to do capture from");

      if(promotion)
      {
        add_caps(n, ch_pos, Figure::TypeQueen);

        // only checking knight
        if((movesTable().caps(Figure::TypeKnight, ch_pos) & fmgr.king_mask(ocolor)))
          add_caps(n, ch_pos, Figure::TypeKnight);
      }
      else
        add_caps(n, ch_pos, Figure::TypeNone);
    }

    if(board_.enpassant() > 0)
    {
      int8 ep_pos = board_.enpassantPos();
      if((ch_pos == ep_pos) || (protect_king_msk_ & set_mask_bit(board_.enpassant())))
      {
        X_ASSERT(!board_.getField(ep_pos) || board_.getField(ep_pos).type() != Figure::TypePawn, "en-passant pown doesnt exist");
        X_ASSERT(board_.getField(ch_pos).type() != Figure::TypePawn, "en-passant pawn is on checking position but checking figure type is not pawn");
        const uint64 & opawn_caps_ep = movesTable().pawnCaps(ocolor, board_.enpassant());
        uint64 eat_msk_ep = pawn_msk & opawn_caps_ep;
        for(; eat_msk_ep;)
        {
          int n = clear_lsb(eat_msk_ep);

          const Field & fpawn = board_.getField(n);
          X_ASSERT(!fpawn || fpawn.type() != Figure::TypePawn || fpawn.color() != color, "no pawn on field we are going to do capture from");

          add_caps(n, board_.enpassant(), Figure::TypeNone);
        }
      }
    }

    // Pawns promotions
    BitMask pw_mask = board_.fmgr().pawn_mask(color) & movesTable().promote(color);
    for(; pw_mask;)
    {
      int pw_pos = clear_msb(pw_mask);

      // +2 - skip captures
      const int8* table = movesTable().pawn(color, pw_pos) + 2;
      for(; *table >= 0 && !board_.getField(*table); ++table)
      {
        if((protect_king_msk_ & set_mask_bit(*table)) == 0)
          continue;

        X_ASSERT(*table <= 55 && *table >= 8, "not a promotion move");
        add_caps(pw_pos, *table, Figure::TypeQueen);

        // add promotion to knight only if it gives check
        if(movesTable().caps(Figure::TypeKnight, ch_pos) & fmgr.king_mask(ocolor))
          add_caps(pw_pos, *table, Figure::TypeKnight);
      }
    }

    // 2nd - knight's captures
    {
      const uint64 & knight_caps = movesTable().caps(Figure::TypeKnight, ch_pos);
      const uint64 & knight_msk = fmgr.knight_mask(color);
      uint64 eat_msk = knight_msk & knight_caps;
      for(; eat_msk;)
      {
        int n = clear_lsb(eat_msk);

        const Field & fknight = board_.getField(n);
        X_ASSERT(!fknight || fknight.type() != Figure::TypeKnight || fknight.color() != color, "no knight on field we are going to do capture from");

        add_caps(n, ch_pos, Figure::TypeNone);
      }
    }

    // 3rd - bishops, rooks and queens
    {
      auto const& bi_moves = magic_ns::bishop_moves(ch_pos, mask_all);
      auto bi_mask = bi_moves & fmgr.bishop_mask(color);
      for(; bi_mask;)
      {
        int n = clear_lsb(bi_mask);

        const Field & field = board_.getField(n);
        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypeBishop, "no bishop on field we are going to do capture from");

        add_caps(n, ch_pos, Figure::TypeNone);
      }
      auto const& r_moves = magic_ns::rook_moves(ch_pos, mask_all);
      auto r_mask = r_moves & fmgr.rook_mask(color);
      for(; r_mask;)
      {
        int n = clear_lsb(r_mask);

        const Field & field = board_.getField(n);
        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypeRook, "no rook on field we are going to do capture from");

        add_caps(n, ch_pos, Figure::TypeNone);
      }

      auto q_mask = (bi_moves | r_moves) & fmgr.queen_mask(color);
      for(; q_mask;)
      {
        int n = clear_lsb(q_mask);

        const Field & field = board_.getField(n);
        X_ASSERT(!field || field.color() != color || field.type() != Figure::TypeQueen, "no queen on field we are going to do capture from");

        add_caps(n, ch_pos, Figure::TypeNone);
      }
    }
  }

  // now try to protect king - put something between it and checking figure
  inline void generateUsual()
  {
    // checking figure position and type
    auto const& ch_type = board_.getField(board_.checking()).type();
    X_ASSERT(!ch_type, "there is no checking figure");
    X_ASSERT(ch_type == Figure::TypeKing, "king is attacking king");

    if(Figure::TypePawn == ch_type || Figure::TypeKnight == ch_type || !protect_king_msk_)
      return;

    auto const& color = board_.color();
    auto const& fmgr = board_.fmgr();

    // 1. Pawns. exclude promotions and captures
    BitMask pw_mask = fmgr.pawn_mask(color) & ~movesTable().promote(color);
    for(; pw_mask;)
    {
      int pw_pos = clear_msb(pw_mask);

      // +2 - skip captures
      const int8* table = movesTable().pawn(color, pw_pos) + 2;
      for(; *table >= 0 && !board_.getField(*table); ++table)
      {
        if((protect_king_msk_ & set_mask_bit(*table)) == 0)
          continue;

        X_ASSERT(*table > 55 || *table < 8, "pawn move should not be a promotion");
        add_usual(pw_pos, *table);
      }
    }


    // 2. Knights
    auto kn_mask = fmgr.knight_mask(color);
    for(; kn_mask;)
    {
      int kn_pos = clear_msb(kn_mask);
      const uint64 & knight_msk = movesTable().caps(Figure::TypeKnight, kn_pos);
      uint64 msk_protect = protect_king_msk_ & knight_msk;
      for(; msk_protect;)
      {
        int n = clear_lsb(msk_protect);

        const Field & field = board_.getField(n);
        X_ASSERT(field, "there is something between king and checking figure");

        add_usual(kn_pos, n);
      }
    }

    // 3. Bishops
    {
      auto bi_mask = fmgr.bishop_mask(color);
      for(; bi_mask;)
      {
        auto from = clear_lsb(bi_mask);
        auto bi_moves = magic_ns::bishop_moves(from, mask_all);
        auto bi_protect = protect_king_msk_ & bi_moves;
        for(; bi_protect;)
        {
          int to = clear_lsb(bi_protect);

          const Field & field = board_.getField(to);
          X_ASSERT(field, "there is something between king and checking figure");

          add_usual(from, to);
        }
      }
    }

    // 4. Rooks
    {
      auto r_mask = fmgr.rook_mask(color);
      for(; r_mask;)
      {
        auto from = clear_lsb(r_mask);
        auto const& r_moves = magic_ns::rook_moves(from, mask_all);
        auto r_protect = protect_king_msk_ & r_moves;
        for(; r_protect;)
        {
          int to = clear_lsb(r_protect);

          const Field & field = board_.getField(to);
          X_ASSERT(field, "there is something between king and checking figure");

          add_usual(from, to);
        }
      }
    }

    // 5. Queens
    {
      auto q_mask = fmgr.queen_mask(color);
      for(; q_mask;)
      {
        auto from = clear_lsb(q_mask);
        auto q_moves = magic_ns::queen_moves(from, mask_all);
        auto q_protect = protect_king_msk_ & q_moves;
        for(; q_protect;)
        {
          int to = clear_lsb(q_protect);

          const Field & field = board_.getField(to);
          X_ASSERT(field, "there is something between king and checking figure");

          add_usual(from, to);
        }
      }
    }
  }

  inline void generateKingCaps()
  {
    auto const& color = board_.color();
    const BitMask & o_mask = board_.fmgr().mask(ocolor);

    // captures
    auto ki_mask = movesTable().caps(Figure::TypeKing, king_pos_) & o_mask;
    for(; ki_mask;)
    {
      int to = clear_lsb(ki_mask);

      auto const& field = board_.getField(to);
      X_ASSERT(!field || field.color() == color, "escape generator: try to put king to occupied field");
      add_caps(king_pos_, to, Figure::TypeNone);
    }
  }

  inline void generateKingUsual()
  {
    auto const& color = board_.color();
    const BitMask & mask = board_.fmgr().mask(color);
    const BitMask & o_mask = board_.fmgr().mask(ocolor);

    // usual moves
    int oki_pos = board_.kingPos(ocolor);
    auto ki_mask = movesTable().caps(Figure::TypeKing, king_pos_) & ~(mask_all | movesTable().caps(Figure::TypeKing, oki_pos));
    for(; ki_mask;)
    {
      int to = clear_lsb(ki_mask);

      X_ASSERT(board_.getField(to), "escape generator: try to put king to occupied field");
      add_usual(king_pos_, to);
    }
  }

  Move2* next()
  {
    if(order_ == oGenCaps)
    {
      if(!board_.doubleCheck())
        generateCaps();
      generateKingCaps();
      order_ = oCaps;
      iter_ = caps_.begin();
    }
    if(order_ == oCaps)
    {
      if(iter_ != caps_.end())
      {
        auto* move = &*iter_;
        ++iter_;
        //caps_.erase(iter);
        return move;
      }
      order_ = oGenUsual;
    }
    if(order_ == oGenUsual)
    {
      if(!board_.doubleCheck())
        generateUsual();
      generateKingUsual();
      order_ = oUsual;
      iter_ = usual_.begin();
    }
    if(order_ == oUsual)
    {
      if(iter_ != usual_.end())
      {
        auto* move = &*iter_;
        //usual_.erase(iter);
        ++iter_;
        return move;
      }
    }
    return nullptr;
  }

  BOARD const& board_;
  MovesList caps_;
  MovesList usual_;
  typename MovesList::iterator iter_;
  int king_pos_;
  BitMask protect_king_msk_;
  BitMask mask_all;
  Figure::Color ocolor;
};


template <class BOARD, class MOVE>
struct FastGenerator2
{
  using MovesList = xlist<MOVE, BOARD::MovesMax>;

  enum Order { oEscape, oGenCaps, oCaps, oGenUsual, oUsual } order_{};

  FastGenerator2(BOARD const& board) :
    cg_(board), ug_(board), eg_(board)
  {
    if(!board.underCheck())
      order_ = oGenCaps;
  }

  Move2* next()
  {
    if(order_ == oEscape)
    {
      return eg_.next();
    }
    if(order_ == oGenCaps)
    {
      cg_.generateCaps();
      order_ = oCaps;
    }
    if(order_ == oCaps)
    {
      auto* move = cg_.next();
      if(move)
        return move;
      //auto iter = cg_.moves_.begin();
      //if(iter != cg_.moves_.end())
      //{
      //  auto* move = &*iter;
      //  cg_.moves_.erase(iter);
      //  return move;
      //}
      order_ = oGenUsual;
    }
    if(order_ == oGenUsual)
    {
      ug_.generate();
      order_ = oUsual;
    }
    if(order_ == oUsual)
    {
      auto* move = ug_.next();
      return move;
      //auto iter = ug_.moves_.begin();
      //if(iter != ug_.moves_.end())
      //{
      //  auto* move = &*iter;
      //  ug_.moves_.erase(iter);
      //  return move;
      //}
    }
    return nullptr;
  }

  CapsGenerator2<BOARD, MOVE> cg_;
  UsualGenerator2<BOARD, MOVE> ug_;
  EscapeGenerator2<BOARD, MOVE> eg_;
};

template <class BOARD, class MOVE>
struct TacticalGenerator2
{
  using MovesList = xlist<MOVE, BOARD::MovesMax>;

  enum Order { oEscape, oGenCaps, oCaps, oGenChecks, oChecks } order_{};

  TacticalGenerator2(BOARD const& board, int depth) :
    cg_(board), ckg_(board), eg_(board), depth_(depth)
  {
    if(!board.underCheck())
      order_ = oGenCaps;
  }

  Move2* next()
  {
    if(order_ == oEscape)
    {
      return eg_.next();
    }
    if(order_ == oGenCaps)
    {
      cg_.generateCaps();
      order_ = oCaps;
    }
    if(order_ == oCaps)
    {
      //auto iter = cg_.moves_.begin();
      //if(iter != cg_.moves_.end())
      //{
      //  auto* move = &*iter;
      //  cg_.moves_.erase(iter);
      //  return move;
      //}
      if(auto* move = cg_.next())
        return move;
      if(depth_ < 0)
        return nullptr;
      order_ = oGenChecks;
    }
    if(order_ == oGenChecks)
    {
      ckg_.generate();
      order_ = oChecks;
    }
    if(order_ == oChecks)
    {
      return ckg_.next();
      //auto iter = ckg_.moves_.begin();
      //if(iter != ckg_.moves_.end())
      //{
      //  auto* move = &*iter;
      //  ckg_.moves_.erase(iter);
      //  return move;
      //}
    }
    return nullptr;
  }

  CapsGenerator2<BOARD, MOVE> cg_;
  ChecksGenerator2<BOARD, MOVE> ckg_;
  EscapeGenerator2<BOARD, MOVE> eg_;
  int depth_{};
};

} // namespace {}

void xcaptures(Board2& board, int depth)
{
  if(board.drawState() || board.hasReps() || depth < -4)
    return;
  TacticalGenerator2<Board2, Move2> tg(board, depth);
  try
  {
    for(;;)
    {
      auto* pmove = tg.next();
      if(!pmove)
        break;
      auto& move = *pmove;
      if(!board.validateMove(move))
        continue;
      Board2 brd{ board };
      //std::string fen = toFEN(board);
      X_ASSERT(board.see(move) != board.see2(move), "SEE error");
      board.makeMove(move);
      x_movesCounter++;
      xcaptures(board, depth-1);
      board.unmakeMove(move);
      X_ASSERT(brd != board, "board was not correctly restored");
    }
  }
  catch(std::exception const& e)
  {
    throw std::runtime_error(toFEN(board) + "; " + e.what());
  }
}

void xsearch(Board2& board, int depth)
{
  if(board.drawState() || board.hasReps())
    return;
  if(depth <= 0)
  {
    xcaptures(board, depth);
    return;
  }
  try
  {
    FastGenerator2<Board2, Move2> fg(board);
    for(;;)
    {
      auto* pmove = fg.next();
      if(!pmove)
        break;
      auto& move = *pmove;
      if(!board.validateMove(move))
        continue;
      //std::string fen = toFEN(board);
      X_ASSERT(board.see(move) != board.see2(move), "SEE error");
      Board2 brd{ board };

      if(depth == 2 && !board.underCheck())
      {
        board.makeNullMove();
        x_movesCounter++;
        xsearch(board, depth-1);
        board.unmakeNullMove();
      }
      else
      {
        board.makeMove(move);
        x_movesCounter++;
        xsearch(board, depth-1);
        board.unmakeMove(move);
      }

      X_ASSERT(brd != board, "board was not correctly restored");
    }
  }
  catch(std::exception const& e)
  {
    throw std::runtime_error(toFEN(board) + "; " + e.what());
  }
}

bool compare(std::vector<Move2> const& moves1, std::vector<Move2> const& moves2)
{
  if(moves1.size() != moves2.size())
    return false;
  for(auto const& move1 : moves1)
  {
    if(std::find_if(moves2.begin(), moves2.end(), [&move1](Move2 const& m) { return m == move1; })
       == moves2.end())
       return false;
  }
  for(auto move2 : moves2)
  {
    if(std::find_if(moves1.begin(), moves1.end(), [&move2](Move2 const& m) { return m == move2; })
       == moves1.end())
       return false;
  }
  return true;
}

bool xverifyMoves(Board2& board)
{
  bool ok = false;
  std::vector<Move2> fmoves;
  FastGenerator2<Board2, Move2> fg(board);
  for(;;)
  {
    auto* pmove = fg.next();
    if(!pmove)
      break;
    auto& move = *pmove;
    X_ASSERT(board.validateMove(move) != board.validateMoveBruteforce(move), "validate move error");
    if(!board.validateMove(move))
      continue;
    Board2 brd{ board };
    brd.makeMove(move);
    brd.unmakeMove(move);
    X_ASSERT(board != brd, "board was not restored correctly after move");
    fmoves.push_back(move);
    ok = true;
  }
  xlist<Move2, Board2::MovesMax> moves;
  generate(board, moves);
  std::vector<Move2> etalons;
  std::vector<Move2> checks;
  auto ocolor = Figure::otherColor(board.color());
  for(auto& move : moves)
  {
    if(!board.validateMoveBruteforce(move))
      continue;
    if(move.new_type == Figure::TypeBishop || move.new_type == Figure::TypeRook)
      continue;
    if(move.new_type == Figure::TypeKnight
       && !(movesTable().caps(Figure::TypeKnight, move.to) & set_mask_bit(board.kingPos(ocolor)))
       )
       continue;
    etalons.push_back(move);
    Board2 brd{ board };
    brd.makeMove(move);
    if(brd.underCheck() && !brd.lastUndo().capture() && !move.new_type)
      checks.push_back(move);
    brd.unmakeMove(move);
    X_ASSERT(board != brd, "board was not restored correctly after move");
  }
  if(!compare(etalons, fmoves))
    return false;
  if(board.underCheck())
    return true;
  ChecksGenerator2<Board2, Move2> cg(board);
  cg.generate();
  std::vector<Move2> checks2;
  for(auto& move : cg.moves_)
  {
    X_ASSERT(board.validateMoveBruteforce(move) != board.validateMove(move), "inalid move validator");
    if(!board.validateMoveBruteforce(move))
      continue;
    if(std::find_if(etalons.begin(), etalons.end(), [&move](Move2 const& m) { return m == move; })
       == etalons.end())
       return false;
    Board2 brd{ board };
    brd.makeMove(move);
    X_ASSERT(!brd.underCheck(), "check was not detected");
    checks2.push_back(move);
  }
  if(!compare(checks, checks2))
    return false;
  return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Board2::addFigure(const Figure::Color color, const Figure::Type type, int pos)
{
  if(!type || (unsigned)pos > 63)
    return false;

  Field & field = getField(pos);
  if(field)
    return false;

  fields_[pos].set(color, type);
  fmgr_.incr(color, type, pos);

  return false;
}

// for initialization only
bool Board2::isCastlePossible(Figure::Color c, int t) const
{
  static const int king_position[] = { 60, 4 };
  static const int rook_position[2][2] = { { 63, 56 }, { 7, 0 } };
  if(kingPos(c) != king_position[c])
    return false;
  X_ASSERT((unsigned)t > 1, "invalid castle type");
  auto const& rfield = getField(rook_position[c][t]);
  return rfield && rfield.type() == Figure::TypeRook && rfield.color() == c;
}

void Board2::clear()
{
  auto* undoStack = g_undoStack;
  *this = Board2{};
  g_undoStack = undoStack;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: rewrite with std::string/std::regex/boost::trim/...
/* rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 */
bool fromFEN(std::string const& i_fen, Board2& board)
{
  static std::string const stdFEN_ = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
  board.clear();

  std::string fen = i_fen.empty() ? stdFEN_ : i_fen;
  boost::algorithm::trim(fen);

  const char* s = fen.c_str();
  int x = 0, y = 7;
  for(; s && y >= 0; ++s)
  {
    char c = *s;
    if('/' == c)
      continue;
    else if(isdigit(c))
    {
      int dx = c - '0';
      x += dx;
      if(x > 7)
      {
        --y;
        x = 0;
      }
    }
    else
    {
      Figure::Type  ftype = Figure::TypeNone;
      Figure::Color color = Figure::ColorBlack;

      if(isupper(c))
        color = Figure::ColorWhite;

      ftype = Figure::toFtype(toupper(c));

      if(Figure::TypeNone == ftype)
        return false;

      board.addFigure(color, ftype, Index(x, y));

      if(++x > 7)
      {
        --y;
        x = 0;
      }
    }
  }

  int i = 0;
  for(; *s;)
  {
    char c = *s;
    if(c <= ' ')
    {
      ++s;
      continue;
    }

    if(0 == i) // process move color
    {
      if('w' == c)
        board.setColor(Figure::ColorWhite);
      else if('b' == c)
      {
        board.setColor(Figure::ColorBlack);
        board.hashColor();
      }
      else
        return false;
      ++i;
      ++s;
      continue;
    }

    if(1 == i) // process castling possibility
    {
      if('-' == c)
      {
        ++i;
        ++s;
        continue;
      }

      for(; *s && *s > 32; ++s)
      {
        Field fk, fr;

        switch(*s)
        {
        case 'k':
          board.set_castling(Figure::ColorBlack, 0);
          break;

        case 'K':
          board.set_castling(Figure::ColorWhite, 0);
          break;

        case 'q':
          board.set_castling(Figure::ColorBlack, 1);
          break;

        case 'Q':
          board.set_castling(Figure::ColorWhite, 1);
          break;

        default:
          return false;
        }
      }

      ++i;
      continue;
    }

    if(2 == i)
    {
      ++i;
      char cx = *s++;
      if('-' == cx) // no en-passant pawn
        continue;

      if(!*s)
        return false;

      char cy = *s++;

      Index enpassant(cx, cy);

      if(board.color())
        cy--;
      else
        cy++;

      Index pawn_pos(cx, cy);

      Field & fp = board.getField(pawn_pos);
      if(fp.type() != Figure::TypePawn || fp.color() == board.color())
        return false;

      board.setEnpassant(enpassant, fp.color());
      continue;
    }

    if(3 == i) // fifty moves rule
    {
      char str[4];
      str[0] = 0;
      for(int j = 0; *s && j < 4; ++j, ++s)
      {
        if(*s > 32)
          str[j] = *s;
        else
        {
          str[j] = 0;
          break;
        }
      }

      board.setFiftyMovesCount(atoi(str));
      ++i;
      continue;
    }

    if(4 == i) // moves counter
    {
      board.setMovesCounter(atoi(s));
      break;
    }
  }

  if(!board.invalidate())
    return false;

  return true;
}

std::string toFEN(Board2 const& board)
{
  std::string fen;

  // 1 - write figures
  for(int y = 7; y >= 0; --y)
  {
    int n = 0;
    for(int x = 0; x < 8; ++x)
    {
      Index idx(x, y);
      const Field & field = board.getField(idx);
      if(!field)
      {
        ++n;
        continue;
      }

      if(n > 0)
      {
        fen += '0' + n;
        n = 0;
      }

      char c = fromFtype(field.type());
      if(field.color() == Figure::ColorBlack)
        c = tolower(c);

      fen += c;
    }

    if(n > 0)
      fen += '0' + n;

    if(y > 0)
      fen += '/';
  }

  // 2 - color to move
  {
    fen += ' ';
    if(Figure::ColorBlack == board.color())
      fen += 'b';
    else
      fen += 'w';
  }

  // 3 - castling possibility
  {
    fen += ' ';
    if(!board.castling())
    {
      fen += '-';
    }
    else
    {
      if(board.castling_K())
      {
        if(!board.verifyCastling(Figure::ColorWhite, 0))
          return "";

        fen += 'K';
      }
      if(board.castling_Q())
      {
        if(!board.verifyCastling(Figure::ColorWhite, 1))
          return "";

        fen += 'Q';
      }
      if(board.castling_k())
      {
        if(!board.verifyCastling(Figure::ColorBlack, 0))
          return "";

        fen += 'k';
      }
      if(board.castling_q())
      {
        if(!board.verifyCastling(Figure::ColorBlack, 1))
          return "";

        fen += 'q';
      }
    }
  }

  {
    // 4 - en passant
    fen += ' ';
    if(board.enpassant() > 0)
    {
      Index ep_pos(board.enpassant());

      int x = ep_pos.x();
      int y = ep_pos.y();

      if(board.color())
        y--;
      else
        y++;

      Index pawn_pos(x, y);
      const Field & ep_field = board.getField(pawn_pos);
      if(ep_field.color() == board.color() || ep_field.type() != Figure::TypePawn)
        return "";

      char cx = 'a' + ep_pos.x();
      char cy = '1' + ep_pos.y();
      fen += cx;
      fen += cy;
    }
    else
      fen += '-';

    // 5 - fifty move rule
    fen += " " + std::to_string(static_cast<int>(board.fiftyMovesCount()));

    // 6 - moves counter
    fen += " " + std::to_string(board.movesCounter());
  }

  return fen;
}

void Board2::setMovesCounter(int c)
{
  movesCounter_ = c;
  halfmovesCounter_ = (c - 1)*2 + !color();
  if(data_.fiftyMovesCount_ > halfmovesCounter_)
    data_.fiftyMovesCount_ = halfmovesCounter_;
}

bool Board2::invalidate()
{
  X_ASSERT(data_.fiftyMovesCount_ > halfmovesCounter_, "invalid moves counters");

  Figure::Color ocolor = Figure::otherColor(color());

  int ki_pos  = _lsb64(fmgr_.king_mask(color()));
  int oki_pos = _lsb64(fmgr_.king_mask(ocolor));

  data_.state_ = Ok;

  bool failed = isAttacked(color(), oki_pos);
  X_ASSERT(failed, "not moving king is under check");
  if(failed)
  {
    data_.state_ = Invalid;
    return false;
  }

  findCheckingFigures(ocolor, ki_pos);

  X_ASSERT(isAttacked(ocolor, ki_pos) && !underCheck(), "invalid check detection");

  verifyChessDraw();

  if(drawState())
    return true;

  verifyState();

  return true;
}

bool Board2::fieldAttacked(const Figure::Color c, int8 pos, const BitMask & mask_all_inv) const
{
  Figure::Color ocolor = Figure::otherColor(c);

  {
    // knights
    const BitMask & n_caps = movesTable().caps(Figure::TypeKnight, pos);
    const BitMask & knight_msk = fmgr_.knight_mask(c);
    if(n_caps & knight_msk)
      return true;

    // pawns
    const BitMask & p_caps = movesTable().pawnCaps(ocolor, pos);
    const BitMask & pawn_msk = fmgr_.pawn_mask(c);
    if(p_caps & pawn_msk)
      return true;

    // king
    const BitMask & k_caps = movesTable().caps(Figure::TypeKing, pos);
    const BitMask & king_msk = fmgr_.king_mask(c);
    if(k_caps & king_msk)
      return true;
  }

  // all long-range figures
  const BitMask & q_caps = movesTable().caps(Figure::TypeQueen, pos);
  BitMask mask_brq = fmgr_.bishop_mask(c) | fmgr_.rook_mask(c) | fmgr_.queen_mask(c);
  mask_brq &= q_caps;

  // do we have at least 1 attacking figure
  if(mask_brq)
  {
    // rooks
    const BitMask & r_caps = movesTable().caps(Figure::TypeRook, pos);
    BitMask rook_msk = fmgr_.rook_mask(c) & r_caps;
    for(; rook_msk;)
    {
      int n = clear_lsb(rook_msk);

      X_ASSERT((unsigned)n > 63, "invalid bit found in attack detector");
      X_ASSERT(!getField(n) || getField(n).type() != Figure::TypeRook, "no figure but mask bit is 1");
      X_ASSERT(getField(n).color() != c, "invalid figure color in attack detector");

      if(is_nothing_between(n, pos, mask_all_inv))
        return true;
    }

    // bishops
    const BitMask & b_caps = movesTable().caps(Figure::TypeBishop, pos);
    BitMask bishop_msk = fmgr_.bishop_mask(c) & b_caps;
    for(; bishop_msk;)
    {
      int n = clear_lsb(bishop_msk);

      X_ASSERT((unsigned)n > 63, "invalid bit found in attack detector");
      X_ASSERT(!getField(n) || getField(n).type() != Figure::TypeBishop, "no figure but mask bit is 1");
      X_ASSERT(getField(n).color() != c, "invalid figure color in attack detector");

      if(is_nothing_between(n, pos, mask_all_inv))
        return true;
    }

    // queens
    BitMask queen_msk = fmgr_.queen_mask(c) & q_caps;
    for(; queen_msk;)
    {
      int n = clear_lsb(queen_msk);

      X_ASSERT((unsigned)n > 63, "invalid bit found in attack detector");
      X_ASSERT(!getField(n) || getField(n).type() != Figure::TypeQueen, "no figure but mask bit is 1");
      X_ASSERT(getField(n).color() != c, "invalid figure color in attack detector");

      if(is_nothing_between(n, pos, mask_all_inv))
        return true;
    }
  }

  return false;
}

void Board2::findCheckingFigures(Figure::Color ocolor, int ki_pos)
{
  int count = 0;
  for(int type = Figure::TypePawn; type < Figure::TypeKing; ++type)
  {
    BitMask mask = fmgr_.type_mask((Figure::Type)type, ocolor);
    for(; mask;)
    {
      int n = clear_lsb(mask);
      const Field & field = getField(n);

      X_ASSERT(field.color() != ocolor || field.type() != type, "invalid figures mask in check detector");

      int dir = figureDir().dir(field.type(), ocolor, n, ki_pos);
      if((dir < 0) || (Figure::TypePawn == type && (2 == dir || 3 == dir)))
        continue;

      X_ASSERT(Figure::TypeKing == type, "king checks king");

      if(Figure::TypePawn == type || Figure::TypeKnight == type)
      {
        ++count;
        data_.checking_ = n;
        continue;
      }

      FPos dp = deltaPosCounter().getDeltaPos(n, ki_pos);

      X_ASSERT(FPos(0, 0) == dp, "invalid attacked position");

      FPos p = FPosIndexer::get(ki_pos) + dp;
      const FPos & figp = FPosIndexer::get(n);
      bool have_figure = false;
      for(; p != figp; p += dp)
      {
        const Field & field = getField(p.index());
        if(field)
        {
          have_figure = true;
          break;
        }
      }

      if(!have_figure)
      {
        ++count;
        data_.checking_ = n;
      }
    }
  }
  X_ASSERT(count > 2, "more than 2 checking figures");
  if(count > 0)
    data_.state_ |= State::UnderCheck;
  if(count > 1)
    data_.state_ |= State::DoubleCheck;
}

// verify is there draw or mat
void Board2::verifyState()
{
  if(matState() || drawState())
    return;

  xlist<Move2, Board2::MovesMax> moves;
  generate(*this, moves);
  bool found = false;
  for(auto it = moves.begin(); it != moves.end() && !found; ++it)
  {
    auto const& move = *it;
    if(validateMoveBruteforce(move))
      found = true;
  }

  if(!found)
  {
    if(underCheck())
      data_.state_ |= State::ChessMat;
    else
      data_.state_ |= State::Stalemat;

    // update move's state because it is last one
    if(halfmovesCounter_ > 0)
    {
      auto& undo = undoInfo(halfmovesCounter_-1);
      undo.data_.state_ = data_.state_;
    }
  }
}

/// slow, but verify all the conditions
bool Board2::validateMoveBruteforce(const Move2 & move) const
{
  if(drawState() || matState())
    return false;
  const Field & ffrom = getField(move.from);
  X_ASSERT(ffrom.type() != Figure::TypePawn && move.new_type > 0, "not a pawn promotion");
  X_ASSERT(!ffrom || ffrom.color() != color(), "no moving figure or invalid color");
  Figure::Color ocolor = Figure::otherColor(color());
  X_ASSERT(move.to == kingPos(ocolor), "try to capture king");
  X_ASSERT(getField(move.to) && getField(move.to).color() == color(), "try to capture own figure");
  BitMask mask_all = (fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack));
  X_ASSERT(underCheck() != fieldAttacked(ocolor, kingPos(color()), ~mask_all), "king under attack differs from check flag");
  X_ASSERT(fieldAttacked(color(), kingPos(ocolor), ~mask_all), "opponents king is under check");
  int king_pos = kingPos(color());
  if(ffrom.type() == Figure::TypeKing)
  {
    X_ASSERT(move.from != king_pos, "king position is invalid");
    X_ASSERT(underCheck() && std::abs(Index(move.from).x()-Index(move.to).x()) == 2, "try to caslte under check");
    X_ASSERT(std::abs(Index(move.from).x()-Index(move.to).x()) > 2 || std::abs(Index(move.from).y()-Index(move.to).y()) > 1, "invalid king move");
    X_ASSERT(std::abs(Index(move.from).x()-Index(move.to).x()) == 2 && std::abs(Index(move.from).y()-Index(move.to).y()) != 0, "invalid king move");
    auto mask_all_inv = ~(mask_all ^ set_mask_bit(move.from));
    if(fieldAttacked(ocolor, move.to, mask_all_inv))
      return false;
    if(underCheck())
      return true;
    if(std::abs(Index(move.from).x()-Index(move.to).x()) == 2)
    {
      int ctype = Index(move.from).x() > Index(move.to).x();
      X_ASSERT(!castling(color(), ctype), "castle is not allowed");
      X_ASSERT(!(move.from == 4 && color() || move.from == 60 && !color()), "kings position is wrong");
      X_ASSERT(color() && move.to != 2 && move.to != 6, "white castle final king position is wrong");
      X_ASSERT(!color() && move.to != 58 && move.to != 62, "black castle final king position is wrong");
      X_ASSERT(getField(move.to), "king position after castle is occupied");
      int king_through_pos = color()
        ? (ctype ?  3 :  5)
        : (ctype ? 59 : 61);
      X_ASSERT(getField(king_through_pos), "field, that rook is going to move to, is occupied");
      int rook_through_pos = ctype
        ? (color() ? 1 : 57)
        : 0;
      X_ASSERT(ctype == 1 && getField(rook_through_pos), "long castling impossible, next to rook field is occupied");
      return !fieldAttacked(ocolor, king_through_pos, mask_all_inv);
    }
    return true;
  }
  X_ASSERT(underCheck() && doubleCheck(), "only king movements allowed");
  auto pw_mask = fmgr_.pawn_mask(ocolor) & ~set_mask_bit(move.to);
  auto pw_caps = movesTable().pawnCaps(color(), king_pos);
  if(pw_mask & pw_caps)
    return false;
  auto kn_mask = fmgr_.knight_mask(ocolor) & ~set_mask_bit(move.to);
  if(kn_mask & movesTable().caps(Figure::TypeKnight, king_pos))
    return false;
  X_ASSERT(fmgr_.king_mask(ocolor) & movesTable().caps(Figure::TypeKing, king_pos), "king attacks king");
  int en_pos = -1;
  if(enpassant() > 0 && enpassant() == move.to && ffrom.type() == Figure::TypePawn)
  {
    en_pos = enpassantPos();
  }
  // bishops, rooks and queens movements
  for(int type = Figure::TypeBishop; type < Figure::TypeKing; ++type)
  {
    BitMask fg_mask = fmgr_.type_mask((Figure::Type)type, ocolor);
    for(; fg_mask;)
    {
      int fg_pos = clear_lsb(fg_mask);
      // recently captured figure
      if(fg_pos == move.to)
        continue;
      const uint16* table = movesTable().move(type-Figure::TypeBishop, fg_pos);
      for(; *table; ++table)
      {
        const int8* packed = reinterpret_cast<const int8*>(table);
        int8 count = packed[0];
        int8 const& delta = packed[1];
        int8 p = fg_pos;
        for(; count; --count)
        {
          p += delta;
          // skip empty field
          if(p == move.from || p == en_pos)
            continue;
          // stop on field occupied my recently moved fiure
          if(p == move.to)
            break;
          // capture my king
          if(p == king_pos)
            return false;
          X_ASSERT(getField(p) && getField(p).type() == Figure::TypeKing && getField(p).color() == color(), "king position is invalid");
          // field is occupied
          if(getField(p))
            break;
        }
      }
    }
  }
  return true;
}

bool Board2::validateMove(const Move2 & move) const
{
  if(drawState() || matState())
    return false;

  X_ASSERT((unsigned)move.from > 63 || (unsigned)move.to > 63, "invalid move positions");
  const Field & ffrom = getField(move.from);
  Figure::Color ocolor = Figure::otherColor(color());
  X_ASSERT(move.to == kingPos(ocolor), "try to capture king");
  BitMask mask_all = (fmgr_.mask(Figure::ColorWhite) | fmgr_.mask(Figure::ColorBlack));
  if(Figure::TypeKing == ffrom.type())
  {
    X_ASSERT(underCheck() && ((move.from&7)-(move.to&7) > 1 || (move.from&7)-(move.to&7) < -1),
             "try to castle under check");

    mask_all ^= set_mask_bit(move.from);
    if(isAttacked(move.to, mask_all, ocolor))
      return false;
    // no need to verify castle under check
    if(underCheck())
      return true;
    // castle
    int d = move.to - move.from;
    if(d == 2 || d == -2)
    {
      X_ASSERT(!castling(color(), d == 2 ? 0 : 1), "castle is not allowed");
      X_ASSERT(!(move.from == 4 && color() || move.from == 60 && !color()), "kings position is wrong");
      X_ASSERT(color() && move.to != 2 && move.to != 6, "white castle final king position is wrong");
      X_ASSERT(!color() && move.to != 58 && move.to != 62, "black castle final king position is wrong");
      X_ASSERT(getField(move.to), "king position after castle is occupied");
      d >>= 1;
      // verify if there is suitable rook for castling
      int rook_from = move.from + ((d>>1) ^ 3); //d < 0 ? move.from_ - 4 : move.from_ + 3
      int rook_to = move.from + d;
      X_ASSERT((rook_from != 0 && color() && d < 0) || (rook_from != 7 && color() && d > 0), "invalid castle rook position");
      X_ASSERT((rook_from != 56 && !color() && d < 0) || (rook_from != 63 && !color() && d > 0), "invalid castle rook position");
      X_ASSERT(getField(rook_to), "field, that rook is going to move to, is occupied");
      X_ASSERT(!(rook_from & 3) && getField(rook_from+1), "long castling impossible");
      return !isAttacked(rook_to, mask_all, ocolor);
    }
    return true;
  }

  X_ASSERT(underCheck() && doubleCheck(), "2 checking figures - only king moves allowed");

  // moving figure discovers check?

  int ki_pos = kingPos(color());
  if(enpassant() > 0 && move.to == enpassant() && Figure::TypePawn == ffrom.type())
  {
    int ep_pos = enpassantPos();
    BitMask through = set_mask_bit(move.from) | set_mask_bit(ep_pos);
    mask_all |= set_mask_bit(move.to);
    mask_all &= ~through;
    bool ok = !discoveredCheckEp(through, mask_all, ocolor, ki_pos);
    X_ASSERT(ok && isAttacked(ki_pos, mask_all, ~set_mask_bit(move.to), ocolor), "king is attacked from somewhere after move");
    X_ASSERT(!ok && !isAttacked(ki_pos, mask_all, ~set_mask_bit(move.to), ocolor), "invalid attack of my king detected after move");
    return ok;
  }
  bool ok = !discoveredCheckUsual(move.from, move.to, mask_all, ocolor, ki_pos);
  X_ASSERT(ok && isAttacked(ki_pos, (mask_all | set_mask_bit(move.to)) & ~set_mask_bit(move.from), ~set_mask_bit(move.to), ocolor),
           "king is attacked from somewhere after move");
  X_ASSERT(!ok && !isAttacked(ki_pos, (mask_all | set_mask_bit(move.to)) & ~set_mask_bit(move.from), ~set_mask_bit(move.to), ocolor),
           "invalid attack of my king detected after move");
  return ok;
}

void Board2::makeMove(const Move2 & move)
{
  X_ASSERT(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  halfmovesCounter_++;

  auto& undo = lastUndo();

  // save general data
  undo.data_      = data_;
  // save Zobrist keys
  undo.zcode_     = fmgr_.hashCode();
  undo.zcode_pw_  = fmgr_.pawnCode();
  // clear flags
  undo.mflags_    = 0;
  // clear eaten type
  undo.eaten_type_= 0;
  // set state
  data_.state_    = Ok;

  Figure::Color ocolor = Figure::otherColor(color());

  Field & ffrom = getField(move.from);
  Field & fto   = getField(move.to);

  // castle
  bool castle_k = castling(color(), 0);
  bool castle_q = castling(color(), 1);
  static int castle_rook_pos[2][2] = { { 63, 7 }, { 56, 0 } }; // 0 - short, 1 - long
  if(ffrom.type() == Figure::TypeKing)
  {
    int d = move.to - move.from;
    if((2 == d || -2 == d))
    {
      undo.mflags_ |= UndoInfo2::Castle | UndoInfo2::Irreversible;

      // don't do castling under check
      X_ASSERT(underCheck(), "can not castle under check");

      d >>= 1;
      int rook_to   = move.from + d;
      int rook_from = move.from + ((d>>1) ^ 3);//d < 0 ? undo.from_ - 4 : undo.from_ + 3

      Field & rf_field = getField(rook_from);
      X_ASSERT(rf_field.type() != Figure::TypeRook || rf_field.color() != color(), "no rook for castle");
      X_ASSERT(!(rook_from & 3) && getField(rook_from+1), "long castle is impossible");

      rf_field.clear();
      fmgr_.move(color(), Figure::TypeRook, rook_from, rook_to);
      Field & field_rook_to = getField(rook_to);
      X_ASSERT(field_rook_to, "field that rook is going to undo to while castling is occupied");
      field_rook_to.set(color(), Figure::TypeRook);
    }
    // hash castle and change flags
    if(castle_k)
    {
      clear_castling(color(), 0);
      fmgr_.hashCastling(color(), 0);
    }
    if(castle_q)
    {
      clear_castling(color(), 1);
      fmgr_.hashCastling(color(), 1);
    }
  }
  // is castling still possible?
  else if((castle_k || castle_q) && ffrom.type() == Figure::TypeRook)
  {
    // short castle
    if(castle_k && (move.from == castle_rook_pos[0][color()]))
    {
      clear_castling(color(), 0);
      fmgr_.hashCastling(color(), 0);
    }
    // long castle
    if(castle_q && (move.from == castle_rook_pos[1][color()]))
    {
      clear_castling(color(), 1);
      fmgr_.hashCastling(color(), 1);
    }
  }

  // remove captured figure
  if(fto)
  {
    X_ASSERT(fto.color() == color(), "invalid color of captured figure");

    // eat rook of another side
    if(castling() && fto.type() == Figure::TypeRook)
    {
      if(move.to == castle_rook_pos[0][ocolor] && castling(ocolor, 0))
      {
        clear_castling(ocolor, 0);
        fmgr_.hashCastling(ocolor, 0);
      }
      else if(move.to == castle_rook_pos[1][ocolor] && castling(ocolor, 1))
      {
        clear_castling(ocolor, 1);
        fmgr_.hashCastling(ocolor, 1);
      }
    }

    undo.eaten_type_ = fto.type();
    fmgr_.decr(fto.color(), fto.type(), move.to);
    fto.clear();
    undo.mflags_ |= UndoInfo2::Capture | UndoInfo2::Irreversible;
  }
  else if(Figure::TypePawn == ffrom.type() && move.to != 0 && move.to == enpassant())
  {
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    X_ASSERT(epfield.color() != ocolor || epfield.type() != Figure::TypePawn, "en-passant pawn is invalid");
    fmgr_.decr(epfield.color(), epfield.type(), ep_pos);
    epfield.clear();
    undo.mflags_ |= UndoInfo2::Capture | UndoInfo2::Irreversible | UndoInfo2::EnPassant;
  }

  // clear en-passant hash code
  if(data_.en_passant_ >= 0)
    fmgr_.hashEnPassant(enpassant(), ocolor);

  data_.en_passant_ = -1;

  // 'from' -> 'to'
  if(move.new_type == 0)
  {
    // en-passant
    if(Figure::TypePawn == ffrom.type() && !undo.capture())
    {
      int dy = move.from - move.to;
      if(dy == 16 || dy == -16)
      {
        data_.en_passant_ = (move.to + move.from) >> 1;
        fmgr_.hashEnPassant(data_.en_passant_, color());
      }
      undo.mflags_ |= UndoInfo2::Irreversible;
    }
    // usual movement
    fmgr_.move(ffrom.color(), ffrom.type(), move.from, move.to);
    fto.set(color(), ffrom.type());
  }
  else
  {
    X_ASSERT(Figure::TypePawn != ffrom.type(), "non-pawn move but with promotion");
    // pawn promotion
    fmgr_.decr(ffrom.color(), ffrom.type(), move.from);
    fto.set(color(), (Figure::Type)move.new_type);
    fmgr_.incr(color(), (Figure::Type)move.new_type, move.to);
    undo.mflags_ |= UndoInfo2::Irreversible;
  }

  // clear field, figure moved from
  ffrom.clear();

  if(undo.irreversible())
  {
    data_.fiftyMovesCount_ = 0;
    data_.repsCounter_ = 0;
  }
  else
  {
    data_.fiftyMovesCount_++;
  }

  movesCounter_ += ocolor;

  // add hash color key
  fmgr_.hashColor();
  setColor(ocolor);

  verifyChessDraw();

  detectCheck(move);

  X_ASSERT(isAttacked(Figure::otherColor(color()), kingPos(color())) && !underCheck(), "check isn't detected");
  X_ASSERT(!isAttacked(Figure::otherColor(color()), kingPos(color())) && underCheck(), "detected check, that doesn't exist");
  X_ASSERT(isAttacked(color(), kingPos(Figure::otherColor(color()))), "our king is under check after undo");
}

void Board2::unmakeMove(const Move2& move)
{
  X_ASSERT(halfmovesCounter_ <= 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  auto& undo = lastUndo();

  halfmovesCounter_--;

  Figure::Color ocolor = color();

  data_ = undo.data_;

  movesCounter_ -= ocolor;

  Field& ffrom = getField(move.from);
  Field& fto   = getField(move.to);

  // restore figure
  if(move.new_type != 0)
  {
    // restore old type
    fmgr_.u_decr(fto.color(), fto.type(), move.to);
    fmgr_.u_incr(fto.color(), Figure::TypePawn, move.from);
    ffrom.set(color(), Figure::TypePawn);
  }
  else
  {
    fmgr_.u_move(fto.color(), fto.type(), move.to, move.from);
    ffrom.set(fto.color(), fto.type());
  }
  fto.clear();

  // restore en-passant
  if(undo.is_enpassant())
  {
    X_ASSERT(undo.enpassant() <= 0 || undo.enpassant() != move.to || Figure::TypePawn != ffrom.type(), "en-passant error");
    int ep_pos = enpassantPos();
    Field & epfield = getField(ep_pos);
    epfield.set(ocolor, Figure::TypePawn);
    fmgr_.u_incr(epfield.color(), epfield.type(), ep_pos);
  }

  // restore eaten figure
  if(undo.eaten_type_ > 0)
  {
    fto.set(ocolor, (Figure::Type)undo.eaten_type_);
    fmgr_.u_incr(fto.color(), fto.type(), move.to);
  }

  // restore rook after castling
  if(undo.castle())
  {
    X_ASSERT(ffrom.type() != Figure::TypeKing, "castle but no king move");
    int d = move.to - move.from;
    X_ASSERT(!(2 == d || -2 == d), "invalid king move while castle");
    d >>= 1;
    int rook_to   = move.from + d;
    int rook_from = move.from + ((d>>1) ^ 3);//d < 0 ? undo.from_ - 4 : undo.from_ + 3

    Field & rfrom = getField(rook_from);
    Field & rto   = getField(rook_to);

    rto.clear();
    rfrom.set(color(), Figure::TypeRook);
    fmgr_.u_move(color(), Figure::TypeRook, rook_to, rook_from);
  }
  
  fmgr_.restoreHash(undo.zcode_);
  fmgr_.restorePawnCode(undo.zcode_pw_);
}

void Board2::makeNullMove()
{
  X_ASSERT(halfmovesCounter_ < 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");
  
  halfmovesCounter_++;

  auto& undo = lastUndo();

  // save general data
  undo.data_ = data_;
  // save Zobrist keys
  undo.zcode_ = fmgr_.hashCode();
  undo.zcode_pw_ = fmgr_.pawnCode();
  
  X_ASSERT(data_.state_ != Ok, "try null-move on invalid state");

  Figure::Color ocolor = Figure::otherColor(color());

  // clear en-passant hash code
  if(data_.en_passant_ >= 0)
    fmgr_.hashEnPassant(enpassant(), ocolor);

  data_.en_passant_ = -1;
  
  fmgr_.hashColor();
  setColor(ocolor);
}

void Board2::unmakeNullMove()
{
  X_ASSERT(halfmovesCounter_ <= 0 || halfmovesCounter_ >= GameLength, "number of halfmoves is invalid");

  auto& undo = lastUndo();

  halfmovesCounter_--;

  Figure::Color ocolor = color();

  data_ = undo.data_;

  fmgr_.restoreHash(undo.zcode_);
  fmgr_.restorePawnCode(undo.zcode_pw_);
}


bool Board2::verifyCheckingFigure(int ch_pos, Figure::Color checking_color) const
{
  int count = 0;
  int checking[2] = {};
  auto king_color = Figure::otherColor(checking_color);
  int king_pos = kingPos(king_color);
  for(int type = Figure::TypePawn; type < Figure::TypeKing; ++type)
  {
    BitMask mask = fmgr_.type_mask((Figure::Type)type, checking_color);
    for(; mask;)
    {
      int n = clear_lsb(mask);
      const Field & field = getField(n);
      X_ASSERT(field.color() != checking_color || field.type() != type, "invalid figures mask in check detector");
      int dir = figureDir().dir(field.type(), checking_color, n, king_pos);
      if((dir < 0) || (Figure::TypePawn == type && (2 == dir || 3 == dir)))
        continue;
      if(Figure::TypePawn == type || Figure::TypeKnight == type)
      {
        X_ASSERT(count > 1, "too much checking figures");
        checking[count++] = n;
        continue;
      }
      FPos dp = deltaPosCounter().getDeltaPos(n, king_pos);
      X_ASSERT(FPos(0, 0) == dp, "invalid attacked position");
      FPos p = FPosIndexer::get(king_pos) + dp;
      const FPos & figp = FPosIndexer::get(n);
      bool have_figure = false;
      for(; p != figp; p += dp)
      {
        const Field & field = getField(p.index());
        if(field)
        {
          have_figure = true;
          break;
        }
      }
      if(!have_figure)
      {
        X_ASSERT(count > 1, "too much checking figures");
        checking[count++] = n;
      }
    }
  }
  X_ASSERT(count > 2, "more than 2 checking figures");
  for(int i = 0; i < count; ++i)
  {
    if(checking[i] == ch_pos)
      return true;
  }
  return true;
}

void Board2::detectCheck(Move2 const& move)
{
  data_.checking_ = 0;
  Figure::Color ocolor = Figure::otherColor(color());
  const Field & fto = getField(move.to);
  int king_pos = kingPos(color());
  auto const& king_mask = fmgr_.king_mask(color());
  auto mask_all = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite);
  int epch{ -1 };
  // castle with check
  if(fto.type() == Figure::TypeKing)
  {
    int d = move.to - move.from;
    if(2 == d || -2 == d)
    {
      d >>= 1;
      int rook_to = move.from + d;
      if((rook_to&7) == (king_pos&7) && is_nothing_between(rook_to, king_pos, ~mask_all))
      {
        data_.checking_ = rook_to;
        data_.state_ |= UnderCheck;
      }
      X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
      // no more checking figures
      return;
    }
  }
  else if(fto.type() == Figure::TypePawn)
  {
    auto const& pw_mask = movesTable().pawnCaps(ocolor, move.to);
    if(pw_mask & king_mask)
    {
      data_.checking_ = move.to;
      data_.state_ |= UnderCheck;
      // usual pawn's move?
      if((move.from&7) == (move.to&7))
      {
        X_ASSERT(move.new_type, "invalid promotion detected");
        X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
        // no more checking figures
        return;
      }
    }
    // en-passant
    else if(move.to != 0 && lastUndo().enpassant() == move.to)
    {
      static int pw_delta[2] = { -8, 8 };
      // color == color of captured pawn
      int ep_pos = move.to + pw_delta[color()];
      // through en-passant pawn field
      epch = data_.checking_ = findDiscoveredPtEp(ep_pos, mask_all, ocolor, king_pos);
      if(data_.checking_ < 64)
      {
        data_.state_ |= UnderCheck;
        X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
      }
    }
  }
  else if(fto.type() == Figure::TypeKnight)
  {
    const BitMask & kn_mask = movesTable().caps(Figure::TypeKnight, move.to);
    if(kn_mask & king_mask)
    {
      data_.state_ |= UnderCheck;
      data_.checking_= move.to;
      X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
    }
  }
  else if(isAttackedBy(color(), ocolor, fto.type(), move.to, king_pos, mask_all))
  {
    data_.state_ |= UnderCheck;
    data_.checking_ = move.to;
    X_ASSERT(!verifyCheckingFigure(data_.checking_, ocolor), "invalid checking figure found");
  }

  // through move.from field
  int pt = findDiscoveredPt(move.from, move.to, mask_all, ocolor, king_pos);
  if(pt >= 64)
    return;
  if(pt == epch)
  {
    X_ASSERT(pt == move.to, "invalid discovered checking figure position found");
    X_ASSERT(!underCheck(), "check was not detected");
    return;
  }
  X_ASSERT(!verifyCheckingFigure(pt, ocolor), "invalid checking figure found");
  if(underCheck())
  {
    data_.state_ |= DoubleCheck;
    return;
  }
  data_.state_ |= UnderCheck;
  data_.checking_ = pt;
}

void Board2::verifyChessDraw()
{
  if((data_.fiftyMovesCount_ == 100 && !underCheck()) || (data_.fiftyMovesCount_ > 100))
  {
    data_.state_ |= Draw50Moves;
    return;
  }

  if(!fmgr_.pawns(Figure::ColorBlack) && !fmgr_.pawns(Figure::ColorWhite))
  {
    if((fmgr_.rooks(Figure::ColorBlack) + fmgr_.queens(Figure::ColorBlack)
      + fmgr_.rooks(Figure::ColorWhite) + fmgr_.queens(Figure::ColorWhite) == 0)
      && (fmgr_.knights(Figure::ColorBlack) + fmgr_.bishops(Figure::ColorBlack)) < 2
      && (fmgr_.knights(Figure::ColorWhite) + fmgr_.bishops(Figure::ColorWhite)) < 2)
    {
      data_.state_ |= DrawInsuf;
      return;
    }
  }

  int reps = countReps(2, fmgr_.hashCode());
  if(reps > data_.repsCounter_)
    data_.repsCounter_ = reps;

  if(reps >= 3)
    data_.state_ |= DrawReps;
}

bool Board2::verifyCastling(const Figure::Color c, int t) const
{
  if(c && getField(4).type() != Figure::TypeKing)
    return false;

  if(!c && getField(60).type() != Figure::TypeKing)
    return false;

  if(c && t == 0 && getField(7).type() != Figure::TypeRook)
    return false;

  if(c && t == 1 && getField(0).type() != Figure::TypeRook)
    return false;

  if(!c && t == 0 && getField(63).type() != Figure::TypeRook)
    return false;

  if(!c && t == 1 && getField(56).type() != Figure::TypeRook)
    return false;

  return true;
}

namespace
{

struct SeeCalc
{
  Board2 const& board_;
  Move2 const&  move_;
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

  SeeCalc(Board2 const& board, Move2 const& move) :
    board_(board),
    move_(move)
  {
    mask_to_ = set_mask_bit(move_.to);
    all_mask_ = board_.fmgr().mask(Figure::ColorBlack) | board_.fmgr().mask(Figure::ColorWhite) | mask_to_;
    all_mask_ &= ~set_mask_bit(move_.from);
    promotion_ = (move_.to >> 3) == 0 || (move_.to >> 3) == 7;
    color_ = board_.getField(move_.from).color();
    BitMask to_mask_inv = ~mask_to_;
    brq_masks_[0] = board_.fmgr().bishop_mask(Figure::ColorBlack) | board_.fmgr().rook_mask(Figure::ColorBlack) | board_.fmgr().queen_mask(Figure::ColorBlack);
    brq_masks_[0] &= to_mask_inv;
    brq_masks_[1] = board_.fmgr().bishop_mask(Figure::ColorWhite) | board_.fmgr().rook_mask(Figure::ColorWhite) | board_.fmgr().queen_mask(Figure::ColorWhite);
    brq_masks_[1] &= to_mask_inv;
    ki_pos_[0] = board_.kingPos(Figure::ColorBlack);
    ki_pos_[1] = board_.kingPos(Figure::ColorWhite);
    if(discovered_check(color_, Figure::otherColor(color_), ~all_mask_, move_.from))
    {
      illegal_ = true;
      return;
    }
    bq_mask_[0] = board_.fmgr().bishop_mask(Figure::ColorBlack) | board_.fmgr().queen_mask(Figure::ColorBlack);
    bq_mask_[1] = board_.fmgr().bishop_mask(Figure::ColorWhite) | board_.fmgr().queen_mask(Figure::ColorWhite);
    rq_mask_[0] = board_.fmgr().rook_mask(Figure::ColorBlack) | board_.fmgr().queen_mask(Figure::ColorBlack);
    rq_mask_[1] = board_.fmgr().rook_mask(Figure::ColorWhite) | board_.fmgr().queen_mask(Figure::ColorWhite);
    king_only_ = discovered_check(Figure::otherColor(color_), color_, ~all_mask_, move_.from);
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
        & movesTable().pawnCaps(ocolor, move_.to);
      if(do_move(color, ocolor, Figure::TypePawn, from, score))
        return true;
    }

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
      last_b_[color] = mask_b = magic_ns::bishop_moves(move_.to, all_mask_) & all_mask_;
      masks_[color][Figure::TypeBishop] = mask_b & board_.fmgr().bishop_mask(color);
    }
    if(do_move(color, ocolor, Figure::TypeBishop, from, score))
      return true;

    uint64 mask_r{};
    if(!masks_[color][Figure::TypeRook] && (rq_mask_[color] & all_mask_))
    {
      last_r_[color] = mask_r = magic_ns::rook_moves(move_.to, all_mask_) & all_mask_;
      masks_[color][Figure::TypeRook] = mask_r & board_.fmgr().rook_mask(color);
    }
    if(do_move(color, ocolor, Figure::TypeRook, from, score))
      return true;

    if(!masks_[color][Figure::TypeQueen] && (board_.fmgr().queen_mask(color) & all_mask_))
    {
      if(!mask_b)
        mask_b = magic_ns::bishop_moves(move_.to, all_mask_);
      if(!mask_r)
        mask_r = magic_ns::rook_moves(move_.to, all_mask_);
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

  inline uint64 figure_mask(Figure::Color color, Figure::Type type) const
  {
    return board_.fmgr().type_mask(type, color) & movesTable().caps(type, move_.to) & all_mask_;
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
    if(board_.fmgr().pawn_mask(ocolor) & all_mask_ & movesTable().pawnCaps(color, move_.to))
      return true;

    if(board_.fmgr().knight_mask(ocolor) & all_mask_ & movesTable().caps(Figure::TypeKnight, move_.to))
      return true;

    if((bq_mask_[ocolor] & all_mask_)
       && ((last_b_[ocolor] & all_mask_ & bq_mask_[ocolor]) || (magic_ns::bishop_moves(move_.to, all_mask_) & all_mask_ & bq_mask_[ocolor])))
    {
      return true;
    }

    if((rq_mask_[ocolor] & all_mask_)
       && ((last_r_[ocolor] & all_mask_ & rq_mask_[ocolor]) || (magic_ns::rook_moves(move_.to, all_mask_) & all_mask_ & rq_mask_[ocolor])))
    {
      return true;
    }

    return false;
  }
};

} // end of namespace {} for SeeCalc

int Board2::see(const Move2 & move) const
{
  X_ASSERT(data_.state_ == Invalid, "try to SEE invalid board");

  const Field & ffield = getField(move.from);
  const Field & tfield = getField(move.to);

  if(tfield.type())
  {
    // victim >= attacker
    if(Figure::figureWeight_[tfield.type()] >= Figure::figureWeight_[ffield.type()])
      return Figure::figureWeight_[tfield.type()] - Figure::figureWeight_[ffield.type()];
  }
  // en-passant
  else if(!tfield && ffield.type() == Figure::TypePawn && move.to > 0 && move.to == enpassant())
  {
    X_ASSERT(getField(enpassantPos()).type() != Figure::TypePawn
             || getField(enpassantPos()).color() == color(),
             "no en-passant pawn");
    return 0;
  }
  // promotion with capture
  else if(tfield && (ffield.type() == Figure::TypePawn) && ((move.to >> 3) == 0 || (move.to >> 3) == 7))
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












namespace
{

  struct SeeCalc2
  {
    Board2 const& board_;
    Move2 const&  move_;
    FiguresManager const& fmgr_;
    BitMask last_b_[2] = {};
    BitMask last_r_[2] = {};
    BitMask all_mask_;
    int ki_pos_[2];
    Figure::Color color_;
    bool promotion_;
    bool king_only_;

    SeeCalc2(Board2 const& board, Move2 const& move) :
      board_(board),
      move_(move),
      fmgr_(board.fmgr())
    {
      all_mask_ = fmgr_.mask(Figure::ColorBlack) | fmgr_.mask(Figure::ColorWhite) | set_mask_bit(move_.to);
      color_ = board_.getField(move_.from).color();
      ki_pos_[0] = board_.kingPos(Figure::ColorBlack);
      ki_pos_[1] = board_.kingPos(Figure::ColorWhite);
      X_ASSERT((all_mask_ & set_mask_bit(move_.from)) == 0ULL, "no figure on field move.from");
      X_ASSERT(discovered_check(move_.from, Figure::otherColor(color_), ki_pos_[color_]), "SEE move is illegal");
      king_only_ = discovered_check(move_.from, color_, ki_pos_[Figure::otherColor(color_)]);
      if(king_only_)
        return;
      all_mask_ ^= set_mask_bit(move_.from);
      promotion_ = (move_.to >> 3) == 0 || (move_.to >> 3) == 7;
    }

    inline bool next(Figure::Color color, int& from, int& score)
    {
      return (!king_only_ && move_figure(color, from, score)) || move_king(color, from, score);
    }

    inline bool move_figure(Figure::Color color, int& from, int& score)
    {
      auto ocolor = Figure::otherColor(color);
      {
        auto mask = fmgr_.pawn_mask(color) & all_mask_ & movesTable().pawnCaps(ocolor, move_.to);
        if(do_move(mask, color, ocolor, Figure::TypePawn, from, score))
          return true;
      }
      {
        auto mask = fmgr_.knight_mask(color) & all_mask_ & movesTable().caps(Figure::TypeKnight, move_.to);
        if(do_move(mask, color, ocolor, Figure::TypeKnight, from, score))
          return true;
      }

      uint64 mask_b{};
      if(fmgr_.bishop_mask(color) & all_mask_)
      {
        last_b_[color] = mask_b = magic_ns::bishop_moves(move_.to, all_mask_) & all_mask_;
        auto mask = mask_b & fmgr_.bishop_mask(color);
        if(do_move(mask, color, ocolor, Figure::TypeBishop, from, score))
          return true;
      }

      uint64 mask_r{};
      if(fmgr_.rook_mask(color) & all_mask_)
      {
        last_r_[color] = mask_r = magic_ns::rook_moves(move_.to, all_mask_) & all_mask_;
        auto mask= mask_r & fmgr_.rook_mask(color);
        if(do_move(mask, color, ocolor, Figure::TypeRook, from, score))
          return true;
      }

      if(fmgr_.queen_mask(color) & all_mask_)
      {
        if(!mask_b)
          mask_b = magic_ns::bishop_moves(move_.to, all_mask_);
        if(!mask_r)
          mask_r = magic_ns::rook_moves(move_.to, all_mask_);
        auto mask = (mask_b | mask_r) & all_mask_ & board_.fmgr().queen_mask(color);
        if(do_move(mask, color, ocolor, Figure::TypeQueen, from, score))
          return true;
      }

      return false;
    }

    inline bool move_king(Figure::Color color, int& from, int& score)
    {
      auto ocolor = Figure::otherColor(color);
      if((fmgr_.king_mask(color) & movesTable().caps(Figure::TypeKing, move_.to))
         && !(fmgr_.king_mask(ocolor) & movesTable().caps(Figure::TypeKing, move_.to))
         && !under_check(color, ocolor))
      {
        // score == 0 means king's attack. it should be the very last one
        score = 0;
        from = ki_pos_[color];
        return true;
      }
      return false;
    }

    inline bool do_move(BitMask mask, Figure::Color color, Figure::Color ocolor, Figure::Type const type, int& from, int& score)
    {
      while(mask)
      {
        from = clear_lsb(mask);
        if(discovered_check(from, ocolor, ki_pos_[color]))
          continue;
        X_ASSERT((all_mask_ & set_mask_bit(from)) == 0ULL, "no figure which is going to make move");
        all_mask_ ^= set_mask_bit(from);
        score = promotion_ && type == Figure::TypePawn
          ? Figure::figureWeight_[Figure::TypeQueen] - Figure::figureWeight_[Figure::TypePawn]
          : Figure::figureWeight_[type];
        if(color == color_)
          score = -score;
        return true;
      }
      return false;
    }

    inline bool discovered_check(int from, Figure::Color ocolor, int ki_pos) const
    {
      auto dir = figureDir().br_dir(ki_pos, from);
      if(!dir)
        return false;
      auto mask_all_f = all_mask_ & ~set_mask_bit(from);
      auto mask_all_t = mask_all_f & ~set_mask_bit(move_.to);
      if(dir == nst::bishop)
      {
        auto bi_moves = magic_ns::bishop_moves(ki_pos, mask_all_f) & mask_all_t;
        auto const& bi_mask = fmgr_.bishop_mask(ocolor);
        auto const& q_mask = fmgr_.queen_mask(ocolor);
        return bi_moves & (bi_mask | q_mask);
      }
      X_ASSERT(dir != nst::rook, "invalid direction from point to point");
      auto r_moves = magic_ns::rook_moves(ki_pos, mask_all_f) & mask_all_t;
      auto const& r_mask = fmgr_.rook_mask(ocolor);
      auto const& q_mask = fmgr_.queen_mask(ocolor);
      return r_moves & (r_mask | q_mask);
    }

    inline bool under_check(Figure::Color color, Figure::Color ocolor) const
    {
      if(board_.fmgr().pawn_mask(ocolor) & all_mask_ & movesTable().pawnCaps(color, move_.to))
        return true;

      if(board_.fmgr().knight_mask(ocolor) & all_mask_ & movesTable().caps(Figure::TypeKnight, move_.to))
        return true;

      auto bq_mask = (fmgr_.bishop_mask(ocolor) | fmgr_.queen_mask(ocolor)) & all_mask_;
      if((last_b_[ocolor] & bq_mask) || (magic_ns::bishop_moves(move_.to, all_mask_) & bq_mask))
      {
        return true;
      }

      auto rq_mask = (fmgr_.rook_mask(ocolor) | fmgr_.queen_mask(ocolor)) & all_mask_;
      return (last_r_[ocolor] & rq_mask) || (magic_ns::rook_moves(move_.to, all_mask_) & rq_mask);
    }
  };

} // end of namespace {} for SeeCalc

int Board2::see2(const Move2 & move) const
{
  X_ASSERT(data_.state_ == Invalid, "try to SEE invalid board");

  const Field & ffield = getField(move.from);
  const Field & tfield = getField(move.to);

  if(tfield.type())
  {
    // victim >= attacker
    if(Figure::figureWeight_[tfield.type()] >= Figure::figureWeight_[ffield.type()])
      return Figure::figureWeight_[tfield.type()] - Figure::figureWeight_[ffield.type()];
  }
  // en-passant
  else if(!tfield && ffield.type() == Figure::TypePawn && move.to > 0 && move.to == enpassant())
  {
    X_ASSERT(getField(enpassantPos()).type() != Figure::TypePawn
             || getField(enpassantPos()).color() == color(),
             "no en-passant pawn");
    return 0;
  }
  // promotion with capture
  else if(tfield && (ffield.type() == Figure::TypePawn) && ((move.to >> 3) == 0 || (move.to >> 3) == 7))
  {
    return Figure::figureWeight_[tfield.type()]-Figure::figureWeight_[Figure::TypePawn];
  }

  int score_gain = (tfield) ? Figure::figureWeight_[tfield.type()] : 0;
  int fscore = -Figure::figureWeight_[ffield.type()];

  auto color = Figure::otherColor(ffield.color());
  SeeCalc2 see_calc(*this, move);

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
    see_calc.king_only_ = see_calc.discovered_check(from, color, see_calc.ki_pos_[ocolor]);
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
