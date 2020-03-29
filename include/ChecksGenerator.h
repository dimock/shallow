#pragma once

#include <xcommon.h>
#include <xlist.h>
#include <Board.h>

namespace NEngine
{

template <class BOARD, class MOVE>
struct ChecksGenerator
{
  using MovesList = xlist<MOVE, NumOfFields>;

  ChecksGenerator(BOARD const& board) :
    board_(board)
  {}

  inline bool add(int from, int to)
  {
    MOVE move{ from, to };
    if (board_.validateMove(move) && !find(move))
    {
      moves_.push_back(move);
      return true;
    }
    return false;
  }

  inline bool add(MOVE const& move)
  {
    X_ASSERT(!board_.validateMove(move), "ChecksGenerator. invalid move added");
    if (!find(move))
    {
      moves_.push_back(move);
      return true;
    }
    return false;
  }

  bool find(MOVE const& m)
  {
    for (auto i = moves_.begin(); i != moves_.end(); ++i)
      if (*i == m)
        return true;
    return false;
  }

  MOVE* next()
  {
    while(iter_ != moves_.end())
    {
      auto* move = &*iter_;
      ++iter_;
      //if(board_.validateMove(*move))
        return move;
    }
    return nullptr;
  }

  void generate()
  {
    BitMask visited{};
    const auto& color = board_.color();
    const auto ocolor = Figure::otherColor(color);
    auto const& fmgr = board_.fmgr();
    auto mask_all = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
    auto mask_all_inv = ~mask_all;
    auto const& oki_pos = board_.kingPos(ocolor);
    auto const pawns = fmgr.pawn_mask(color) & ~movesTable().promote(color);

    // pawns
    {
      auto pw_check_mask = movesTable().pawnCaps(ocolor, oki_pos);
      for(; pw_check_mask;)
      {
        auto to = clear_lsb(pw_check_mask);
        const auto& tfield = board_.getField(to);
        if(tfield || to == board_.enpassant())
          continue;

        // usual moves
        auto pw_from = movesTable().pawnFrom(color, to) & pawns;
        visited |= pw_from;
        for(; pw_from;)
        {
          auto from = clear_lsb(pw_from);
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
      auto kn_mask = board_.fmgr().knight_mask(color);
      for(; kn_mask;)
      {
        auto from = clear_lsb(kn_mask);
        auto kn_moves = movesTable().caps(Figure::TypeKnight, from) & mask_all_inv;
        if(!board_.discoveredCheck(from, mask_all, color, oki_pos))
          kn_moves &= knight_check_mask;
        for(; kn_moves;)
        {
          auto to = clear_lsb(kn_moves);
          X_ASSERT(board_.getField(to), "field, from that we are going to check is occupied");
          add(from, to);
        }
      }
    }

    // king
    {
      auto const& ki_pos = board_.kingPos(color);
      auto all_but_king_mask = mask_all_inv | set_mask_bit(ki_pos);
      bool castle = false;
      // short castle
      if(board_.castling(color, 0) && (movesTable().castleMasks(color, 0) & mask_all) == 0ULL)
      {
        static int rook_positions[] = { 61, 5 };
        auto const& r_pos = rook_positions[board_.color()];
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
        auto const& r_pos = rook_positions[board_.color()];
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
    auto r_king = magic_ns::rook_moves(oki_pos, mask_all);

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
            const auto* to = movesTable().pawn(color, from) + 2; // skip captures
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
            const auto* to = movesTable().pawn(color, from) + 2; // skip captures
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
    const auto* table = movesTable().move(type-Figure::TypeBishop, from);
    for(; *table; ++table)
    {
      const auto* packed = reinterpret_cast<const int8*>(table);
      auto count = packed[0];
      auto delta = packed[1];
      auto to = from;
      for(; count; --count)
      {
        to += delta;
        if(board_.getField(to))
          break;
        add(from, to);
      }
    }
  }

  int size() const
  {
    return moves_.size();
  }

  void generateOne()
  {
    BitMask visited{};
    const auto& color = board_.color();
    const auto ocolor = Figure::otherColor(color);
    auto const& fmgr = board_.fmgr();
    auto mask_all = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
    auto mask_all_inv = ~mask_all;
    auto const& oki_pos = board_.kingPos(ocolor);
    auto const pawns = fmgr.pawn_mask(color) & ~movesTable().promote(color);

    BitMask blocked_mask{};
    BitMask attack_mask{};
    BitMask multiattack_mask{};
    BitMask b_attack_mask{};
    BitMask npkqr_attack_mask{};
    {
      const BitMask & pawn_msk = fmgr.pawn_mask(color);
      if (color == Figure::ColorWhite)
        attack_mask = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
      else
        attack_mask = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
      BitMask knights = fmgr.knight_mask(color);
      for (; knights;)
      {
        int n = clear_lsb(knights);
        auto kn_attack = movesTable().caps(Figure::TypeKnight, n);
        multiattack_mask |= attack_mask & kn_attack;
        attack_mask |= kn_attack;
      }
      npkqr_attack_mask = attack_mask;
      BitMask bishops = fmgr.bishop_mask(color);
      for (; bishops;)
      {
        int n = clear_lsb(bishops);
        auto bi_attack = magic_ns::bishop_moves(n, mask_all & ~fmgr.queen_mask(color) & ~fmgr.bishop_mask(color));
        multiattack_mask |= attack_mask & bi_attack;
        attack_mask |= bi_attack;
        b_attack_mask |= bi_attack;
      }
      auto rooks = fmgr.rook_mask(color);
      for (; rooks;)
      {
        auto n = clear_lsb(rooks);
        auto r_attack = magic_ns::rook_moves(n, mask_all & ~fmgr.queen_mask(color));
        multiattack_mask |= attack_mask & r_attack;
        attack_mask |= r_attack;
      }
      auto queens = fmgr.queen_mask(color);
      for (; queens;)
      {
        auto n = clear_lsb(queens);
        auto bi_moves = magic_ns::bishop_moves(n, mask_all & ~fmgr.queen_mask(color) & ~fmgr.bishop_mask(color));
        auto qr_mask = magic_ns::rook_moves(n, mask_all & ~fmgr.queen_mask(color) & ~fmgr.rook_mask(color));
        auto q_attack = qr_mask | bi_moves;
        multiattack_mask |= attack_mask & q_attack;
        attack_mask |= q_attack;
        b_attack_mask |= bi_moves;
        npkqr_attack_mask |= qr_mask;
      }
      auto ki_att_mask = movesTable().caps(Figure::TypeKing, board_.kingPos(color));
      multiattack_mask |= attack_mask & ki_att_mask;
      attack_mask |= ki_att_mask;
      blocked_mask = attack_mask | fmgr.mask(ocolor);
      npkqr_attack_mask |= ki_att_mask;
    }

    BitMask o_attack_mask{};
    BitMask o_attack_but_king_mask{};
    BitMask o_attack_pnbr_mask{};
    BitMask o_attack_pnb_mask{};
    BitMask oqr_attack_mask{};
    BitMask oqb_attack_mask{};
    BitMask orb_attack_mask{};
    BitMask or_attack_mask{};
    BitMask ob_attack_mask{};
    {
      const BitMask & opawn_msk = fmgr.pawn_mask(ocolor);
      if (ocolor == Figure::ColorWhite)
        o_attack_but_king_mask = ((opawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((opawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
      else
        o_attack_but_king_mask = ((opawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((opawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
      BitMask knights = fmgr.knight_mask(ocolor);
      for (; knights;)
      {
        int n = clear_lsb(knights);
        o_attack_but_king_mask |= movesTable().caps(Figure::TypeKnight, n);
      }
      BitMask bishops = fmgr.bishop_mask(ocolor);
      for (; bishops;)
      {
        int n = clear_lsb(bishops);
        ob_attack_mask |= magic_ns::bishop_moves(n, mask_all);
      }
      o_attack_but_king_mask |= ob_attack_mask;
      o_attack_pnb_mask = o_attack_but_king_mask;
      auto rooks = fmgr.rook_mask(ocolor);
      for (; rooks;)
      {
        auto n = clear_lsb(rooks);
        or_attack_mask |= magic_ns::rook_moves(n, mask_all);
        orb_attack_mask |= magic_ns::bishop_moves(n, mask_all);
      }
      o_attack_but_king_mask |= or_attack_mask;
      o_attack_pnbr_mask = o_attack_but_king_mask;
      auto queens = fmgr.queen_mask(ocolor);
      for (; queens;)
      {
        auto n = clear_lsb(queens);
        auto o_qb_mask = magic_ns::bishop_moves(n, mask_all);
        auto o_qr_mask = magic_ns::rook_moves(n, mask_all);
        o_attack_but_king_mask |= o_qb_mask | o_qr_mask;
        oqr_attack_mask |= o_qr_mask;
        oqb_attack_mask |= o_qb_mask;
      }
      o_attack_mask = o_attack_but_king_mask | movesTable().caps(Figure::TypeKing, board_.kingPos(ocolor));
    }

    auto const o_kbrq_mask = fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor) | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);
    auto const o_pbrq_mask = ((fmgr.pawn_mask(ocolor) | fmgr.bishop_mask(ocolor)) & ~o_attack_but_king_mask) | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);

    auto oki_moves_all = movesTable().caps(Figure::TypeKing, oki_pos);
    auto oki_moves = oki_moves_all & ~blocked_mask;
    oki_moves_all &= mask_all_inv;

    auto bi_king = magic_ns::bishop_moves(oki_pos, mask_all);
    auto r_king = magic_ns::rook_moves(oki_pos, mask_all);

    // queen
    if (moves_.empty())
    {
      auto q_mask = fmgr.queen_mask(color);
      for (; q_mask && moves_.empty();)
      {
        auto from = clear_lsb(q_mask);
        auto q_moves_from = magic_ns::queen_moves(from, mask_all);
        auto q_moves = q_moves_from & mask_all_inv & (r_king | bi_king) &
          (~o_attack_mask | (~o_attack_but_king_mask & multiattack_mask));
        auto qki_blocked_from = q_moves_from & oki_moves_all & ~multiattack_mask;
        for (; q_moves;)
        {
          auto to = clear_lsb(q_moves);
          X_ASSERT(board_.getField(to), "queen goes to occupied field");

          auto q_att_to = magic_ns::queen_moves(to, mask_all) | betweenMasks().from(to, oki_pos);
          if ((oki_moves & ~q_att_to) == 0ULL)
          {
            // do we unblock some moves, previously blocked by this rook?
            auto qki_unblocked_to = ~q_att_to & qki_blocked_from;
            if ((qki_unblocked_to == 0ULL) && add(from, to))
              break;
          }
          // attack through king
          {
            auto through_mask = betweenMasks().from(to, oki_pos);
            auto atf_mask = (fmgr.pawn_mask(ocolor) | o_kbrq_mask) & ~o_attack_but_king_mask & magic_ns::queen_moves(oki_pos, mask_all | set_mask_bit(to)) & through_mask;
            if (atf_mask != 0ULL && (~through_mask & oki_moves_all & movesTable().caps(Figure::TypeKing, _lsb64(atf_mask))) == 0ULL)
            {
              if (add(from, to))
                break;
            }
          }
          // check with attack to unsafe figure
          {
            bool has_attack = (fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.rook_mask(ocolor)) & magic_ns::bishop_moves(to, mask_all & ~set_mask_bit(from)) &
              ~o_attack_but_king_mask;
            has_attack = has_attack || ((fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor)) & magic_ns::rook_moves(to, mask_all & ~set_mask_bit(from)) &
              ~o_attack_but_king_mask);
            if (has_attack && add(from, to))
              break;
          }
        }
      }
    }

    // pawns    
    if (moves_.empty())
    {
      auto pw_check_mask = movesTable().pawnCaps(ocolor, oki_pos) & attack_mask;
      for (; pw_check_mask && moves_.empty();)
      {
        auto to = clear_lsb(pw_check_mask);
        const auto& tfield = board_.getField(to);
        if (tfield || to == board_.enpassant())
          continue;

        // usual moves
        auto pw_from = movesTable().pawnFrom(color, to) & pawns;
        visited |= pw_from;
        for (; pw_from;)
        {
          auto from = clear_lsb(pw_from);
          if (board_.is_something_between(from, to, mask_all_inv))
            continue;
          if ((oki_moves == 0ULL) || (o_kbrq_mask & movesTable().pawnCaps(color, to)) != 0ULL)
          {
            auto pki_blocked_from = movesTable().pawnCaps(color, to) & oki_moves_all & ~multiattack_mask;
            auto pki_blocked_to = movesTable().pawnCaps(color, to) & oki_moves_all;
            pki_blocked_to = pki_blocked_from & ~pki_blocked_to;
            if (pki_blocked_to == 0ULL && add(from, to))
              break;
          }
        }
      }
    }

    // knights
    if (moves_.empty())
    {
      auto const& knight_check_mask = movesTable().caps(Figure::TypeKnight, oki_pos);
      auto kn_mask = board_.fmgr().knight_mask(color);
      for (; kn_mask && moves_.empty();)
      {
        auto from = clear_lsb(kn_mask);
        auto kn_moves = movesTable().caps(Figure::TypeKnight, from) & mask_all_inv;
        bool discovered = board_.discoveredCheck(from, mask_all, color, oki_pos);
        bool can_escape = (oki_moves & ~betweenMasks().from(from, oki_pos)) != 0ULL;
        auto nki_blocked_from = kn_moves & ~multiattack_mask;
        if (!discovered)
        {
          kn_moves &= knight_check_mask;// &(~o_attack_mask | (~o_attack_pnb_mask & multiattack_mask));
        }
        for (; kn_moves;)
        {
          auto to = clear_lsb(kn_moves);
          X_ASSERT(board_.getField(to), "field, from that we are going to check is occupied");

          bool kn_attacks = movesTable().caps(Figure::TypeKnight, to)
            & ((o_kbrq_mask & ~o_attack_but_king_mask) | o_pbrq_mask);

          if (!oki_moves ||
            (discovered && (!can_escape || kn_attacks)) ||
            (o_pbrq_mask & movesTable().caps(Figure::TypeKnight, to)) != 0ULL)
          {
            if (add(from, to))
              break;
          }
        }        
      }
    }

    // discovers bishop|queen attack
    if (moves_.empty()) 
    {
      auto pw_mask = bi_king & pawns;
      auto r_mask = bi_king & fmgr.rook_mask(color);
      if (pw_mask | r_mask)
      {
        // all checking queens and bishops if exclude pawns and rooks
        auto bq_from = magic_ns::bishop_moves(oki_pos, mask_all & ~(pw_mask | r_mask)) & (fmgr.bishop_mask(color) | fmgr.queen_mask(color));
        for (; bq_from && moves_.empty();)
        {
          auto p = clear_lsb(bq_from);
          auto const& btw_mask = betweenMasks().between(p, oki_pos);
          auto pwm = btw_mask & pw_mask;
          if (pwm)
          {
            auto from = clear_lsb(pwm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "pawn should discoved check");
            const auto* to = movesTable().pawn(color, from) + 2; // skip captures
            if ((*to >= 0 && !board_.getField(*to)) &&
                ((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL || ((movesTable().pawnCaps(color, *to) & o_kbrq_mask) != 0ULL)) )
            {
              if(add(from, *to))
                break;
            }
          }
          auto rm = btw_mask & r_mask;
          if (rm && moves_.empty())
          {
            auto from = clear_lsb(rm);
            auto rr = magic_ns::rook_moves(from, mask_all);
            auto rook_from_mask = set_mask_bit(from);
            auto rki_blocked_from = rr & oki_moves_all & ~multiattack_mask;
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "rook should discover check");
            {
              auto to_mask = magic_ns::rook_moves(from, mask_all) & ~mask_all &
                (~movesTable().caps(Figure::TypeKing, board_.kingPos(ocolor)) | multiattack_mask);
              if (to_mask != 0ULL)
              {
                int to = -1;
                bool is_mat = false;
                BitMask through_mask{};
                if (oki_moves != 0ULL)
                {
                  auto ki_move = _lsb64(oki_moves);
                  through_mask = betweenMasks().from(ki_move, oki_pos) | betweenMasks().from(oki_pos, ki_move);
                }
                auto r_mat_mask = magic_ns::rook_moves(oki_pos, mask_all) & through_mask;
                if (to_mask & r_mat_mask)
                {
                  is_mat = true;
                  to = _lsb64(to_mask & r_mat_mask);
                }
                else if (to_mask & oqr_attack_mask)
                  to = _lsb64(to_mask & oqr_attack_mask);
                else if (to_mask & or_attack_mask)
                  to = _lsb64(to_mask & or_attack_mask);
                if (to >= 0)
                {
                  X_ASSERT(board_.getField(to), "field is occupied");
                  auto r_att_to = magic_ns::rook_moves(to, mask_all & ~rook_from_mask) | betweenMasks().from(to, oki_pos) | betweenMasks().from(from, oki_pos);
                  if (!is_mat || ((oki_moves & ~r_att_to) == 0ULL && (~r_att_to & rki_blocked_from) == 0ULL))
                  {
                    if(add(from, to))
                      break;
                  }
                }
              }
            }
          }
        }
      }
    }

    // discovers rook|queen attack
    if (moves_.empty()) 
    {
      auto pw_mask = r_king & pawns & ~visited;
      auto bi_mask = r_king & fmgr.bishop_mask(color);
      if (pw_mask | bi_mask)
      {
        auto rq_from = magic_ns::rook_moves(oki_pos, mask_all & ~(pw_mask | bi_mask)) & (fmgr.rook_mask(color) | fmgr.queen_mask(color));
        for (; rq_from && moves_.empty();)
        {
          auto p = clear_lsb(rq_from);
          auto const& btw_mask = betweenMasks().between(oki_pos, p);
          auto pwm = btw_mask & pw_mask;
          if (pwm)
          {
            auto from = clear_lsb(pwm);
            // pawn on the same X as king
            if ((from & 7) == (oki_pos & 7))
              continue;
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "pawn should discover check");
            const auto* to = movesTable().pawn(color, from) + 2; // skip captures
            if ((*to >= 0 && !board_.getField(*to)) &&
              ((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL || ((movesTable().pawnCaps(color, *to) & o_kbrq_mask) != 0ULL)))
            {
              if (add(from, *to))
                break;
            }
          }
          auto bm = btw_mask & bi_mask;
          visited |= bm;
          if (bm && moves_.empty())
          {
            auto from = clear_lsb(bm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "bishop should discover check");
            {
              auto to_mask = magic_ns::bishop_moves(from, mask_all) & ~mask_all &
                (~movesTable().caps(Figure::TypeKing, board_.kingPos(ocolor)) | multiattack_mask);
              if (to_mask != 0ULL)
              {
                int to = -1;
                if (to_mask & oqb_attack_mask)
                  to = _lsb64(to_mask & oqb_attack_mask);
                else if (to_mask & orb_attack_mask)
                  to = _lsb64(to_mask & orb_attack_mask);
                else if (to_mask & ob_attack_mask)
                  to = _lsb64(to_mask & ob_attack_mask);
                else if ((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL)
                  to = _lsb64(to_mask);
                if (to >= 0)
                {
                  X_ASSERT(board_.getField(to), "field is occupied");
                  if(add(from, to))
                    break;
                }
              }
            }
          }
        }
      }
    }

    // remaining direct attacks
    // rook
    if (moves_.empty())
    {
      auto o_pnbr_mask = fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor) | fmgr.rook_mask(ocolor);
      auto o_q_mask = fmgr.queen_mask(ocolor);
      auto r_mask = fmgr.rook_mask(color);
      auto bq_mask = fmgr.bishop_mask(color) | fmgr.queen_mask(color);
      auto rq_mask = fmgr.rook_mask(color) | fmgr.queen_mask(color);
      for (; r_mask && moves_.empty();)
      {
        auto from = clear_lsb(r_mask);
        auto rook_from_mask = set_mask_bit(from);
        auto rr = magic_ns::rook_moves(from, mask_all);
        auto r_moves = rr & mask_all_inv & r_king &
          (~o_attack_mask | (~o_attack_but_king_mask & multiattack_mask) | (~o_attack_pnbr_mask & multiattack_mask));
        auto rki_blocked_from = rr & oki_moves_all & ~multiattack_mask;
        for (; r_moves && moves_.empty();)
        {
          auto to = clear_lsb(r_moves);
          X_ASSERT(board_.getField(to), "rook goes to occupied field");

          // we could block some field previously attacked by bishop|queen or lose attack of moved rook to king's move fiels
          auto suspicious_mask = b_attack_mask & oki_moves_all & ~set_mask_bit(to) & ~npkqr_attack_mask;
          bool do_lose = false;
          for (; !do_lose && suspicious_mask;)
          {
            auto x = clear_lsb(suspicious_mask);
            auto bi_susp_mask = magic_ns::bishop_moves(x, (mask_all | set_mask_bit(to)) & ~rook_from_mask) & bq_mask;
            if (bi_susp_mask)
              continue;
            auto qr_susp_mask = magic_ns::rook_moves(x, mask_all & ~rook_from_mask) & (rq_mask & ~rook_from_mask);
            if (qr_susp_mask)
              continue;
            do_lose = true;
          }
          if (!do_lose)
          {
            auto r_att_to = magic_ns::rook_moves(to, mask_all & ~rook_from_mask) | betweenMasks().from(to, oki_pos);
            if ((oki_moves & ~r_att_to) == 0ULL)
            {
              // do we unblock some moves, previously blocked by this rook?
              auto rki_unblocked_to = ~r_att_to & rki_blocked_from;
              if((rki_unblocked_to == 0ULL) && add(from, to))
                break;
            }
          }
          // attack through king
          {
            auto atk_mask = magic_ns::rook_moves(oki_pos, mask_all | set_mask_bit(to)) & betweenMasks().from(to, oki_pos);
            if (((atk_mask & o_q_mask) != 0ULL) && add(from, to))
              break;
            if (((atk_mask & (o_pnbr_mask & ~o_attack_but_king_mask)) != 0ULL))// && distanceCounter().getDistance(oki_pos, _lsb64(atk_mask)) > 2)
            {
              if(add(from, to))
                break;
            }
          }
          // check with attack to unsafe figure
          {
            bool has_attack = (((fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor)) & ~o_attack_but_king_mask) | fmgr.queen_mask(ocolor)) &
              magic_ns::rook_moves(to, mask_all & ~set_mask_bit(from));
            if (has_attack && add(from, to))
              break;
          }
        }
      }
    }

    // bishop
    if(moves_.empty())
    {
      auto o_pnb_mask = fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor);
      auto o_rq_mask = fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);
      auto bi_mask = fmgr.bishop_mask(color);
      for (; bi_mask && moves_.empty();)
      {
        auto from = clear_lsb(bi_mask);
        auto bi_moves_from = magic_ns::bishop_moves(from, mask_all);
        auto bi_moves = bi_moves_from & mask_all_inv & bi_king &
          (~o_attack_mask | (~o_attack_but_king_mask & multiattack_mask) | (~o_attack_pnb_mask & multiattack_mask));
        auto bki_blocked_from = bi_moves_from & oki_moves_all & ~multiattack_mask;
        for (; bi_moves && moves_.empty();)
        {
          auto to = clear_lsb(bi_moves);
          X_ASSERT(board_.getField(to), "bishop goes to occupied field");

          auto bi_att_to = magic_ns::bishop_moves(to, mask_all) | betweenMasks().from(to, oki_pos);
          if ((oki_moves & ~bi_att_to) == 0ULL)
          {
            // do we unblock some moves, previously blocked by this rook?
            auto bki_unblocked_to = ~bi_att_to & bki_blocked_from;
            if ((bki_unblocked_to == 0ULL) && add(from, to))
              break;
          }
          // attack through king
          {
            auto atk_mask = magic_ns::bishop_moves(oki_pos, mask_all | set_mask_bit(to)) & betweenMasks().from(to, oki_pos);
            if (((atk_mask & o_rq_mask) != 0ULL) && add(from, to))
              break;
            if (((atk_mask & (o_pnb_mask & ~o_attack_but_king_mask)) != 0ULL) && distanceCounter().getDistance(oki_pos, _lsb64(atk_mask)) > 1)
            {
              if(add(from, to))
                break;
            }
          }
          // check with attack to unsafe figure
          {
            bool has_attack = (((fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor)) & ~o_attack_but_king_mask) | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor)) &
              magic_ns::bishop_moves(to, mask_all & ~set_mask_bit(from));
            if (has_attack && add(from, to))
              break;
          }
        }
      }
    }

    // king castle with check 
    if (moves_.empty())
    {
      auto const& ki_pos = board_.kingPos(color);
      auto all_but_king_mask = mask_all_inv | set_mask_bit(ki_pos);
      // short castle
      if ((board_.castling(color, 0) && (movesTable().castleMasks(color, 0) & mask_all) == 0ULL) &&
        ((oki_moves & ~pawnMasks().mask_column(5)) == 0ULL))
      {
        static int rook_positions[] = { 61, 5 };
        auto const& r_pos = rook_positions[board_.color()];
        X_ASSERT(color && ki_pos != 4 || !color && ki_pos != 60, "invalid king position for castle");
        X_ASSERT(color && board_.getField(7).type() != Figure::TypeRook
          || !color && board_.getField(63).type() != Figure::TypeRook, "no rook for castling, but castle is possible");
        if ((oki_pos & 7) == (r_pos & 7) && board_.is_nothing_between(r_pos, oki_pos, mask_all_inv))
        {
          add(ki_pos, ki_pos + 2);
        }
      }

      // long castle
      if ((moves_.empty() && board_.castling(color, 1) && (movesTable().castleMasks(color, 1) & mask_all) == 0ULL) &&
        ((oki_moves & ~pawnMasks().mask_column(3)) == 0ULL))
      {
        static int rook_positions[] = { 59, 3 };
        auto const& r_pos = rook_positions[board_.color()];
        X_ASSERT(color && ki_pos != 4 || !color && ki_pos != 60, "invalid king position for castle");
        X_ASSERT(color && board_.getField(0).type() != Figure::TypeRook
          || !color && board_.getField(56).type() != Figure::TypeRook, "no rook for castling, but castle is possible");
        if ((oki_pos & 7) == (r_pos & 7) && board_.is_nothing_between(r_pos, oki_pos, mask_all_inv))
        {
          add(ki_pos, ki_pos - 2);
        }
      }

      if ((moves_.empty() && board_.discoveredCheck(ki_pos, mask_all, color, oki_pos)) &&
        ((oki_moves & ~betweenMasks().from(ki_pos, oki_pos)) == 0ULL))
      {
        auto exclude = ~(betweenMasks().from(oki_pos, ki_pos) | movesTable().caps(Figure::TypeKing, oki_pos));
        auto ki_mask = movesTable().caps(Figure::TypeKing, ki_pos) & mask_all_inv & exclude;
        for (; ki_mask;)
        {
          auto to = clear_lsb(ki_mask);
          X_ASSERT(board_.getField(to), "king moves to occupied field");
          add(ki_pos, to);
          break;
        }
      }
    }

    iter_ = moves_.begin();
  }

  void generateStrongest()
  {
    BitMask visited{};
    const auto& color = board_.color();
    const auto ocolor = Figure::otherColor(color);
    auto const& fmgr = board_.fmgr();
    auto mask_all = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
    auto mask_all_inv = ~mask_all;
    auto const& oki_pos = board_.kingPos(ocolor);
    auto const pawns = fmgr.pawn_mask(color) & ~movesTable().promote(color);

    BitMask blocked_mask{};
    BitMask attack_mask{};
    BitMask multiattack_mask{};
    BitMask b_attack_mask{};
    BitMask npkqr_attack_mask{};

    {
      const BitMask & pawn_msk = fmgr.pawn_mask(color);
      if (color == Figure::ColorWhite)
        attack_mask = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
      else
        attack_mask = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
      BitMask knights = fmgr.knight_mask(color);
      for (; knights;)
      {
        int n = clear_lsb(knights);
        auto kn_attack = movesTable().caps(Figure::TypeKnight, n);
        multiattack_mask |= attack_mask & kn_attack;
        attack_mask |= kn_attack;
      }
      BitMask bishops = fmgr.bishop_mask(color);
      for (; bishops;)
      {
        int n = clear_lsb(bishops);
        auto bi_attack = magic_ns::bishop_moves(n, mask_all & ~fmgr.queen_mask(color) & ~fmgr.bishop_mask(color));
        multiattack_mask |= attack_mask & bi_attack;
        attack_mask |= bi_attack;
        b_attack_mask |= bi_attack;
      }
      auto rooks = fmgr.rook_mask(color);
      for (; rooks;)
      {
        auto n = clear_lsb(rooks);
        auto r_attack = magic_ns::rook_moves(n, mask_all & ~fmgr.rook_mask(color) & ~fmgr.queen_mask(color));
        multiattack_mask |= attack_mask & r_attack;
        attack_mask |= r_attack;
      }
      auto queens = fmgr.queen_mask(color);
      for (; queens;)
      {
        auto n = clear_lsb(queens);
        auto bi_moves = magic_ns::bishop_moves(n, mask_all & ~fmgr.queen_mask(color) & ~fmgr.bishop_mask(color));
        auto qr_mask = magic_ns::rook_moves(n, mask_all & ~fmgr.queen_mask(color) & ~fmgr.rook_mask(color));
        auto q_attack = qr_mask | bi_moves;
        multiattack_mask |= attack_mask & q_attack;
        attack_mask |= q_attack;
        b_attack_mask |= bi_moves;
        npkqr_attack_mask |= qr_mask;
      }
      auto ki_att_mask = movesTable().caps(Figure::TypeKing, board_.kingPos(color));
      multiattack_mask |= attack_mask & ki_att_mask;
      attack_mask |= ki_att_mask;
      blocked_mask = attack_mask | fmgr.mask(ocolor);
      npkqr_attack_mask |= ki_att_mask;
    }

    BitMask o_attack_mask{};
    BitMask o_attack_but_king_mask{};
    BitMask o_attack_pnbr_mask{};
    BitMask o_attack_pnb_mask{};
    BitMask oqr_attack_mask{};
    BitMask oqb_attack_mask{};
    BitMask orb_attack_mask{};
    BitMask or_attack_mask{};
    BitMask ob_attack_mask{};
    {
      const BitMask & pawn_msk = fmgr.pawn_mask(ocolor);
      if (ocolor == Figure::ColorWhite)
        o_attack_but_king_mask = ((pawn_msk << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk << 7) & Figure::pawnCutoffMasks_[1]);
      else
        o_attack_but_king_mask = ((pawn_msk >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk >> 9) & Figure::pawnCutoffMasks_[1]);
      BitMask knights = fmgr.knight_mask(ocolor);
      for (; knights;)
      {
        int n = clear_lsb(knights);
        o_attack_but_king_mask |= movesTable().caps(Figure::TypeKnight, n);
      }
      BitMask bishops = fmgr.bishop_mask(ocolor);
      for (; bishops;)
      {
        int n = clear_lsb(bishops);
        ob_attack_mask |= magic_ns::bishop_moves(n, mask_all);
      }
      o_attack_but_king_mask |= ob_attack_mask;
      o_attack_pnb_mask = o_attack_but_king_mask;
      auto rooks = fmgr.rook_mask(ocolor);
      for (; rooks;)
      {
        auto n = clear_lsb(rooks);
        or_attack_mask |= magic_ns::rook_moves(n, mask_all);
        orb_attack_mask |= magic_ns::bishop_moves(n, mask_all);
      }
      o_attack_but_king_mask |= or_attack_mask;
      o_attack_pnbr_mask = o_attack_but_king_mask;
      auto queens = fmgr.queen_mask(ocolor);
      for (; queens;)
      {
        auto n = clear_lsb(queens);
        auto oqb_mask = magic_ns::bishop_moves(n, mask_all);
        auto oqr_mask = magic_ns::rook_moves(n, mask_all);
        o_attack_but_king_mask |= oqb_mask | oqr_mask;
        oqr_attack_mask |= oqr_mask;
        oqb_attack_mask |= oqb_mask;
      }
      o_attack_mask = o_attack_but_king_mask | movesTable().caps(Figure::TypeKing, board_.kingPos(ocolor));
    }

    auto const o_kbrq_mask = fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor) | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);
    auto const o_pbrq_mask = ((fmgr.pawn_mask(ocolor) | fmgr.bishop_mask(ocolor)) & ~o_attack_mask) | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);

    auto oki_moves_all = movesTable().caps(Figure::TypeKing, oki_pos);
    auto oki_moves = oki_moves_all & ~blocked_mask;
    oki_moves_all &= mask_all_inv;

    auto bi_king = magic_ns::bishop_moves(oki_pos, mask_all);
    auto r_king = magic_ns::rook_moves(oki_pos, mask_all);

    MOVE move1{true};

    // pawns    
    {
      auto pw_check_mask = movesTable().pawnCaps(ocolor, oki_pos) & attack_mask;
      for (; pw_check_mask;)
      {
        auto to = clear_lsb(pw_check_mask);
        const auto& tfield = board_.getField(to);
        if (tfield || to == board_.enpassant())
          continue;

        // usual moves
        auto pw_from = movesTable().pawnFrom(color, to) & pawns;
        visited |= pw_from;
        for (; pw_from;)
        {
          auto from = clear_lsb(pw_from);
          if (board_.is_something_between(from, to, mask_all_inv))
            continue;
          if ((oki_moves == 0ULL) || (o_kbrq_mask & movesTable().pawnCaps(color, to)) != 0ULL)
          {
            auto pki_blocked_from = movesTable().pawnCaps(color, to) & oki_moves_all & ~multiattack_mask;
            auto pki_blocked_to = movesTable().pawnCaps(color, to) & oki_moves_all;
            pki_blocked_to = pki_blocked_from & ~pki_blocked_to;
            if (pki_blocked_to == 0ULL)
              add(from, to);
          }
        }
      }
    }

    // knights
    {
      auto const& knight_check_mask = movesTable().caps(Figure::TypeKnight, oki_pos);
      auto kn_mask = board_.fmgr().knight_mask(color);
      for (; kn_mask;)
      {
        auto from = clear_lsb(kn_mask);
        auto kn_moves = movesTable().caps(Figure::TypeKnight, from) & mask_all_inv;
        bool discovered = board_.discoveredCheck(from, mask_all, color, oki_pos);
        bool can_escape = (oki_moves & ~betweenMasks().from(from, oki_pos)) != 0ULL;
        auto nki_blocked_from = kn_moves & ~multiattack_mask;
        if (!discovered)
        {
          kn_moves &= knight_check_mask;
        }
        for (; kn_moves;)
        {
          auto to = clear_lsb(kn_moves);
          X_ASSERT(board_.getField(to), "field, from that we are going to check is occupied");

          bool kn_attacks = movesTable().caps(Figure::TypeKnight, to)
            & ((o_kbrq_mask & ~o_attack_but_king_mask) | o_pbrq_mask);

          if (!oki_moves ||
            (discovered && (!can_escape || kn_attacks)) ||
            (o_pbrq_mask & movesTable().caps(Figure::TypeKnight, to)) != 0ULL)
          {
            add(from, to);
            continue;
          }
          auto attacked_mask = o_attack_mask & (o_attack_pnb_mask | ~multiattack_mask);
          if(!move1 && ((set_mask_bit(to) & attacked_mask) == 0ULL) && board_.validateMove(MOVE{ from, to }))
            move1 = MOVE{ from, to };
        }
      }
    }


    // discovers bishop|queen attack
    {
      auto pw_mask = bi_king & pawns;
      auto r_mask = bi_king & fmgr.rook_mask(color);
      if (pw_mask | r_mask)
      {
        // all checking queens and bishops if exclude pawns and rooks
        auto bq_from = magic_ns::bishop_moves(oki_pos, mask_all & ~(pw_mask | r_mask)) & (fmgr.bishop_mask(color) | fmgr.queen_mask(color));
        for (; bq_from;)
        {
          auto p = clear_lsb(bq_from);
          auto const& btw_mask = betweenMasks().between(p, oki_pos);
          auto pwm = btw_mask & pw_mask;
          if (pwm)
          {
            auto from = clear_lsb(pwm);
            const auto* to = movesTable().pawn(color, from) + 2; // skip captures
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "pawn should discoved check");
            if (*to >= 0 && !board_.getField(*to))
            {
              if (((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL) || ((movesTable().pawnCaps(color, *to) & o_kbrq_mask) != 0ULL))
                add(from, *to);
              else if (!move1 && board_.validateMove(MOVE{ from, *to }))
                move1 = MOVE{from, *to};
            }
          }
          auto rm = btw_mask & r_mask;
          if (rm)
          {
            auto from = clear_lsb(rm);
            auto rr = magic_ns::rook_moves(from, mask_all);
            auto rook_from_mask = set_mask_bit(from);
            auto rki_blocked_from = rr & oki_moves_all & ~multiattack_mask;
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "rook should discover check");
            {
              auto to_mask = magic_ns::rook_moves(from, mask_all) & ~mask_all &
                (~movesTable().caps(Figure::TypeKing, board_.kingPos(ocolor)) | multiattack_mask);
              while (to_mask != 0ULL)
              {
                bool stash_move = false;
                int to = -1;
                BitMask through_mask{};
                if (oki_moves != 0ULL)
                {
                  auto ki_move = _lsb64(oki_moves);
                  through_mask = betweenMasks().from(ki_move, oki_pos) | betweenMasks().from(oki_pos, ki_move);
                }
                auto r_mat_mask = magic_ns::rook_moves(oki_pos, mask_all) & through_mask;
                bool is_mat = false;
                if (to_mask & r_mat_mask)
                {
                  is_mat = true;
                  to = _lsb64(to_mask & r_mat_mask);
                }
                else if (to_mask & oqr_attack_mask)
                  to = _lsb64(to_mask & oqr_attack_mask);
                else if (to_mask & or_attack_mask)
                  to = _lsb64(to_mask & or_attack_mask);
                else if ((to_mask & ~o_attack_mask) != 0ULL && !move1)
                {
                  to = _lsb64(to_mask & ~o_attack_mask);
                  if (board_.validateMove(MOVE{ from, to }))
                    stash_move = true;
                  else
                    to = -1;
                }
                if (to < 0)
                  break;
                to_mask &= ~set_mask_bit(to);
                X_ASSERT(board_.getField(to), "field is occupied");
                auto r_att_to = magic_ns::rook_moves(to, mask_all & ~rook_from_mask) | betweenMasks().from(to, oki_pos) | betweenMasks().from(from, oki_pos);
                if (!stash_move && (!is_mat || ((oki_moves & ~r_att_to) == 0ULL && (~r_att_to & rki_blocked_from) == 0ULL)))
                  add(from, to);
                else if (!move1 && (stash_move || ((set_mask_bit(to) & o_attack_mask) == 0ULL) && board_.validateMove(MOVE{ from, to })))
                  move1 = MOVE{ from, to };
                if (stash_move)
                  break;
              }
            }
          }
        }
      }
    }

    // discovers rook|queen attack
    {
      auto pw_mask = r_king & pawns & ~visited;
      auto bi_mask = r_king & fmgr.bishop_mask(color);
      if (pw_mask | bi_mask)
      {
        auto rq_from = magic_ns::rook_moves(oki_pos, mask_all & ~(pw_mask | bi_mask)) & (fmgr.rook_mask(color) | fmgr.queen_mask(color));
        for (; rq_from;)
        {
          auto p = clear_lsb(rq_from);
          auto const& btw_mask = betweenMasks().between(oki_pos, p);
          auto pwm = btw_mask & pw_mask;
          if (pwm)
          {
            auto from = clear_lsb(pwm);
            // pawn on the same X as king
            if ((from & 7) == (oki_pos & 7))
              continue;
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "pawn should discover check");
            const auto* to = movesTable().pawn(color, from) + 2; // skip captures
            if ((*to >= 0 && !board_.getField(*to)))
            {
              if (((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL) || ((movesTable().pawnCaps(color, *to) & o_kbrq_mask) != 0ULL))
                add(from, *to);
              else if (!move1 && ((set_mask_bit(*to) & o_attack_mask) == 0ULL) && board_.validateMove(MOVE{ from, *to }))
                move1 = MOVE{ from, *to };
            }
          }
          auto bm = btw_mask & bi_mask;
          if (bm)
          {
            auto from = clear_lsb(bm);
            X_ASSERT(!board_.discoveredCheck(from, mask_all, color, oki_pos), "bishop should discover check");
            {
              auto to_mask = magic_ns::bishop_moves(from, mask_all) & ~mask_all &
                (~movesTable().caps(Figure::TypeKing, board_.kingPos(ocolor)) | multiattack_mask);
              while (to_mask != 0ULL)
              {
                bool stash_move = false;
                int to = -1;
                if ((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL)
                  to = _lsb64(to_mask);
                else if (to_mask & oqb_attack_mask)
                  to = _lsb64(to_mask & oqb_attack_mask);
                else if (to_mask & orb_attack_mask)
                  to = _lsb64(to_mask & orb_attack_mask);
                else if (to_mask & ob_attack_mask)
                  to = _lsb64(to_mask & ob_attack_mask);
                else if ((to_mask & ~o_attack_mask) != 0ULL && !move1)
                {
                  to = _lsb64(to_mask & ~o_attack_mask);
                  if(board_.validateMove(MOVE{ from, to }))
                    stash_move = true;
                  else
                    to = -1;
                }
                if (to < 0)
                  break;
                to_mask &= ~set_mask_bit(to);
                X_ASSERT(board_.getField(to), "field is occupied");
                if (!stash_move)
                  add(from, to);
                else
                {
                  move1 = MOVE{ from, to };
                  break;
                }
              }
            }
          }
        }
      }
    }

    // remaining direct attacks
    // rook
    {
      auto o_pnbr_mask = fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor) | fmgr.rook_mask(ocolor);
      auto o_q_mask = fmgr.queen_mask(ocolor);
      auto r_mask = fmgr.rook_mask(color);
      auto bq_mask = fmgr.bishop_mask(color) | fmgr.queen_mask(color);
      auto rq_mask = fmgr.rook_mask(color) | fmgr.queen_mask(color);
      for (; r_mask && moves_.empty();)
      {
        auto from = clear_lsb(r_mask);
        auto rook_from_mask = set_mask_bit(from);
        auto rr = magic_ns::rook_moves(from, mask_all);
        auto r_moves = rr & mask_all_inv & r_king
          & (~o_attack_mask | (~o_attack_pnb_mask & multiattack_mask));
        auto rki_blocked_from = rr & oki_moves_all & ~multiattack_mask;
        for (; r_moves && moves_.empty();)
        {
          auto to = clear_lsb(r_moves);
          X_ASSERT(board_.getField(to), "rook goes to occupied field");

          // we could block some field previously attacked by bishop|queen or lose attack of moved rook to king's move fiels
          auto suspicious_mask = b_attack_mask & oki_moves_all & ~set_mask_bit(to) & ~npkqr_attack_mask;
          bool do_lose = false;
          for (; !do_lose && suspicious_mask;)
          {
            auto x = clear_lsb(suspicious_mask);
            auto bi_susp_mask = magic_ns::bishop_moves(x, (mask_all | set_mask_bit(to)) & ~rook_from_mask) & bq_mask;
            if (bi_susp_mask)
              continue;
            auto qr_susp_mask = magic_ns::rook_moves(x, mask_all & ~rook_from_mask) & (rq_mask & ~rook_from_mask);
            if (qr_susp_mask)
              continue;
            do_lose = true;
          }
          if (!do_lose)
          {
            auto r_att_to = magic_ns::rook_moves(to, mask_all & ~rook_from_mask) | betweenMasks().from(to, oki_pos);
            if ((oki_moves & ~r_att_to) == 0ULL)
            {
              // do we unblock some moves, previously blocked by this rook?
              auto rki_unblocked_to = ~r_att_to & rki_blocked_from;
              if (rki_unblocked_to == 0ULL)
              {
                add(from, to);
                continue;
              }
            }
          }
          // attack through king
          {
            auto atk_mask = magic_ns::rook_moves(oki_pos, mask_all | set_mask_bit(to)) & betweenMasks().from(to, oki_pos);
            if ((atk_mask & o_q_mask) != 0ULL && add(from, to))
              continue;
            if (((atk_mask & (o_pnbr_mask & ~o_attack_but_king_mask)) != 0ULL) && distanceCounter().getDistance(oki_pos, _lsb64(atk_mask)) > 2)
            {
              if (add(from, to))
                continue;
            }
          }
          // check with attack to unsafe figure
          {
            bool has_attack = (((fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor)) & ~o_attack_but_king_mask) | fmgr.queen_mask(ocolor)) &
              magic_ns::rook_moves(to, mask_all & ~set_mask_bit(from));
            if (has_attack && add(from, to))
              continue;
          }
          // rook under attack
          {
            bool under_attack = (set_mask_bit(from) & o_attack_but_king_mask) != 0ULL;
            bool be_attacked = (set_mask_bit(to) & o_attack_mask) == 0ULL;
            if (under_attack && !be_attacked && add(from, to))
              continue;
          }
          auto attacked_mask = o_attack_mask & (o_attack_pnbr_mask | ~multiattack_mask);
          if (!move1 && ((set_mask_bit(to) & attacked_mask) == 0ULL) && board_.validateMove(MOVE{ from, to }))
            move1 = MOVE{from, to};
        }
      }
    }
    
    // queen
    {
      auto q_mask = fmgr.queen_mask(color);
      for (; q_mask;)
      {
        auto from = clear_lsb(q_mask);
        auto q_moves_from = magic_ns::queen_moves(from, mask_all);
        auto q_moves = q_moves_from & mask_all_inv & (r_king | bi_king)
          & (~o_attack_mask | (~o_attack_pnbr_mask & multiattack_mask));
        auto qki_blocked_from = q_moves_from & oki_moves_all & ~multiattack_mask;
        for (; q_moves;)
        {
          auto to = clear_lsb(q_moves);
          X_ASSERT(board_.getField(to), "queen goes to occupied field");

          auto q_att_to = magic_ns::queen_moves(to, mask_all) | betweenMasks().from(to, oki_pos);
          if ((oki_moves & ~q_att_to) == 0ULL)
          {
            // do we unblock some moves, previously blocked by this rook?
            auto qki_unblocked_to = ~q_att_to & qki_blocked_from;
            if (qki_unblocked_to == 0ULL)
            {
              add(from, to);
              continue;
            }
          }
          // attack through king
          {
            auto through_mask = betweenMasks().from(to, oki_pos);
            auto atf_mask = o_kbrq_mask & ~o_attack_but_king_mask & magic_ns::queen_moves(oki_pos, mask_all | set_mask_bit(to)) & through_mask;
            if (atf_mask != 0ULL)// && (~through_mask & oki_moves_all & movesTable().caps(Figure::TypeKing, _lsb64(atf_mask))) == 0ULL)
            {
              add(from, to);
              continue;
            }
          }
          // check with attack to unsafe figure
          {
            bool has_attack = (fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.rook_mask(ocolor)) & magic_ns::bishop_moves(to, mask_all & ~set_mask_bit(from)) &
              ~o_attack_but_king_mask;
            has_attack = has_attack || ((fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor)) & magic_ns::rook_moves(to, mask_all & ~set_mask_bit(from)) &
              ~o_attack_but_king_mask);
            if (has_attack && add(from, to))
              continue;
          }
          // queen under attack
          {
            bool under_attack = (set_mask_bit(from) & o_attack_but_king_mask) != 0ULL;
            bool be_attacked = (set_mask_bit(to) & o_attack_mask) == 0ULL;
            if (under_attack && !be_attacked && add(from, to))
              continue;
          }
          auto attacked_mask = o_attack_mask & (o_attack_pnbr_mask | ~multiattack_mask);
          if (!move1 && ((set_mask_bit(to) & attacked_mask) == 0ULL) && board_.validateMove(MOVE{ from, to }))
            move1 = MOVE{ from, to };
        }
      }
    }


    // bishop
    {
      auto o_pnb_mask = fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor) | fmgr.bishop_mask(ocolor);
      auto o_rq_mask = fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);
      auto bi_mask = fmgr.bishop_mask(color);
      for (; bi_mask;)
      {
        auto from = clear_lsb(bi_mask);
        auto bi_moves_from = magic_ns::bishop_moves(from, mask_all);
        auto bi_moves = bi_moves_from & mask_all_inv & bi_king
          & (~o_attack_mask | (~o_attack_pnb_mask & multiattack_mask));
        auto bki_blocked_from = bi_moves_from & oki_moves_all & ~multiattack_mask;
        for (; bi_moves;)
        {
          auto to = clear_lsb(bi_moves);
          X_ASSERT(board_.getField(to), "bishop goes to occupied field");

          auto bi_att_to = magic_ns::bishop_moves(to, mask_all) | betweenMasks().from(to, oki_pos);
          if ((oki_moves & ~bi_att_to) == 0ULL)
          {
            // do we unblock some moves, previously blocked by this rook?
            auto bki_unblocked_to = ~bi_att_to & bki_blocked_from;
            if ((bki_unblocked_to == 0ULL) && add(from, to))
              add(from, to);
          }
          // attack through king
          {
            auto atk_mask = magic_ns::bishop_moves(oki_pos, mask_all | set_mask_bit(to)) & betweenMasks().from(to, oki_pos);
            if ((atk_mask & o_rq_mask) != 0ULL && add(from, to))
              continue;
            if ((atk_mask & (o_pnb_mask & ~o_attack_but_king_mask)) != 0ULL)// && distanceCounter().getDistance(oki_pos, _lsb64(atk_mask)) > 1)
            {
              if (add(from, to))
                continue;
            }
          }
          // check with attack to unsafe figure
          {
            bool has_attack = (((fmgr.pawn_mask(ocolor) | fmgr.knight_mask(ocolor)) & ~o_attack_but_king_mask) | fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor)) &
              magic_ns::bishop_moves(to, mask_all & ~set_mask_bit(from));
            if (has_attack && add(from, to))
              continue;
          }
          auto attacked_mask = o_attack_mask & (o_attack_pnb_mask | ~multiattack_mask);
          if (!move1 && ((set_mask_bit(to) & attacked_mask) == 0ULL) && board_.validateMove(MOVE{ from, to }))
            move1 = MOVE{ from, to };
        }
      }
    }


    // king
    {
      auto const& ki_pos = board_.kingPos(color);
      auto all_but_king_mask = mask_all_inv | set_mask_bit(ki_pos);
      // short castle
      if ((board_.castling(color, 0) && (movesTable().castleMasks(color, 0) & mask_all) == 0ULL) &&
        ((oki_moves & ~pawnMasks().mask_column(5)) == 0ULL))
      {
        static int rook_positions[] = { 61, 5 };
        auto const& r_pos = rook_positions[board_.color()];
        X_ASSERT(color && ki_pos != 4 || !color && ki_pos != 60, "invalid king position for castle");
        X_ASSERT(color && board_.getField(7).type() != Figure::TypeRook
          || !color && board_.getField(63).type() != Figure::TypeRook, "no rook for castling, but castle is possible");
        if ((oki_pos & 7) == (r_pos & 7) && board_.is_nothing_between(r_pos, oki_pos, mask_all_inv))
        {
          add(ki_pos, ki_pos + 2);
        }
      }

      // long castle
      if ((board_.castling(color, 1) && (movesTable().castleMasks(color, 1) & mask_all) == 0ULL) &&
          ((oki_moves & ~pawnMasks().mask_column(3)) == 0ULL))
      {
        static int rook_positions[] = { 59, 3 };
        auto const& r_pos = rook_positions[board_.color()];
        X_ASSERT(color && ki_pos != 4 || !color && ki_pos != 60, "invalid king position for castle");
        X_ASSERT(color && board_.getField(0).type() != Figure::TypeRook
          || !color && board_.getField(56).type() != Figure::TypeRook, "no rook for castling, but castle is possible");
        if ((oki_pos & 7) == (r_pos & 7) && board_.is_nothing_between(r_pos, oki_pos, mask_all_inv))
        {
          add(ki_pos, ki_pos - 2);
        }
      }

      if ((board_.discoveredCheck(ki_pos, mask_all, color, oki_pos)) &&
        ((oki_moves & ~betweenMasks().from(ki_pos, oki_pos)) == 0ULL))
      {
        auto exclude = ~(betweenMasks().from(oki_pos, ki_pos) | movesTable().caps(Figure::TypeKing, oki_pos));
        auto ki_mask = movesTable().caps(Figure::TypeKing, ki_pos) & mask_all_inv & exclude;
        for (; ki_mask;)
        {
          auto to = clear_lsb(ki_mask);
          X_ASSERT(board_.getField(to), "king moves to occupied field");
          add(ki_pos, to);
        }
      }
    }

    if (moves_.empty() && move1)
      add(move1);

    iter_ = moves_.begin();
  }

  BOARD const& board_;
  MovesList moves_;
  typename MovesList::iterator iter_;
}; // ChecksGenerator

} // NEngine