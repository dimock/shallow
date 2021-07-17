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

  inline void add(int from, int to)
  {
    MOVE move{ from, to, Figure::TypeNone, history(board_.color(), from, to).score() };
    X_ASSERT(find(move), "ChecksGenerator. move exists");
    insert_sorted(moves_, move);
  }
  
  SortValueType sortValue(BitMask mask, Figure::Type a) const
  {
    Figure::Type vt = Figure::TypeNone;
    while (mask != 0ULL && vt != Figure::TypeQueen)
    {
      auto p = clear_lsb(mask);
      vt = std::max(board_.getField(p).type(), vt);
    }
    if (vt != Figure::TypeNone)
      return Figure::figureWeight_[vt] - Figure::figureWeight_[a];
    else
      return Figure::MatScore + Figure::figureWeight_[a];
  }

  MOVE bestMove(BitMask mask, int from, int to, MOVE current) const
  {
    auto a = board_.getField(from).type();
    auto sval = sortValue(mask, a);
    if (current && sval <= current.sort_value)
      return MOVE{ false };
    auto m = MOVE{ from, to };
    m.sort_value = sval;
    if (board_.validateMove(m))
      return m;
    return MOVE{ false };
  }
 
  inline bool push_back(int from, int to)
  {
    MOVE move{ from, to };
    if (board_.validateMove(move))
    {
      X_ASSERT(find(move), "ChecksGenerator. move exists");
      moves_.push_back(move);
      return true;
    }
    return false;
  }

  bool find(MOVE const& m)
  {
    for (auto i = moves_.begin(); i != moves_.end(); ++i) {
      if (*i == m)
        return true;
    }
    return false;
  }

  MOVE* next()
  {
    while(iter_ != moves_.end())
    {
      auto* move = &*iter_;
      ++iter_;
      if(board_.validateMove(*move))
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
      auto pw_check_mask = movesTable().pawnCaps(ocolor, oki_pos) & mask_all_inv;
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
            generateRooks(from, mask_all, mask_all_inv);
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
            generateBishops(from, mask_all, mask_all_inv);
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

  inline void generateBishops(int from, BitMask const& mask_all, BitMask const& mask_all_inv)
  {
    auto bmask = magic_ns::bishop_moves(from, mask_all) & mask_all_inv;
    for (; bmask;)
    {
      auto to = clear_lsb(bmask);
      add(from, to);
    }
  }

  inline void generateRooks(int from, BitMask const& mask_all, BitMask const& mask_all_inv)
  {
    auto rmask = magic_ns::rook_moves(from, mask_all) & mask_all_inv;
    for (; rmask;)
    {
      auto to = clear_lsb(rmask);
      add(from, to);
    }
  }

  void generateMat(int threshold, bool atLeast)
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

    BitMask o_nbrq_mask = 0ULL;
    BitMask o_pbrq_mask = 0ULL;
    BitMask o_pnr_mask = 0ULL;
    BitMask o_pnb_mask = 0ULL;
    BitMask o_pw_mask = 0ULL;
    for (auto type : { Figure::TypeQueen, Figure::TypeRook, Figure::TypeBishop, Figure::TypeKnight, Figure::TypePawn })
    {
      if (threshold > Figure::figureWeight_[type])
        break;

      if(type != Figure::TypePawn)
        o_nbrq_mask |= fmgr.type_mask(type, ocolor);
      else {
        o_pw_mask |= fmgr.type_mask(type, ocolor);
      }
      if(type != Figure::TypeKnight)
        o_pbrq_mask |= fmgr.type_mask(type, ocolor);
      if (type == Figure::TypePawn || type == Figure::TypeKnight || type == Figure::TypeRook) {
        o_pnr_mask |= fmgr.type_mask(type, ocolor);
      }
      if (type == Figure::TypePawn || type == Figure::TypeKnight || type == Figure::TypeBishop) {
        o_pnb_mask |= fmgr.type_mask(type, ocolor);
      }
    }

    auto oki_moves_all = movesTable().caps(Figure::TypeKing, oki_pos);
    auto oki_moves = oki_moves_all & ~blocked_mask;
    oki_moves_all &= mask_all_inv;

    auto bi_king = magic_ns::bishop_moves(oki_pos, mask_all);
    auto r_king = magic_ns::rook_moves(oki_pos, mask_all);

    MOVE best{false};

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
            if ((qki_unblocked_to == 0ULL) && push_back(from, to))
              break;
          }
          // attack through king
          {
            auto through_mask = betweenMasks().from(to, oki_pos);
            auto atf_mask = (o_pw_mask | o_nbrq_mask) & ~o_attack_but_king_mask & magic_ns::queen_moves(oki_pos, mask_all | set_mask_bit(to)) & through_mask;
            if (atf_mask != 0ULL && (~through_mask & oki_moves_all & movesTable().caps(Figure::TypeKing, _lsb64(atf_mask))) == 0ULL)
            {
              if (auto move = bestMove(atf_mask, from, to, best)) {
                best = move;
              }
            }
          }
          // check with attack to unsafe figure
          {
            auto attack_mask = (o_pnr_mask & magic_ns::bishop_moves(to, mask_all & ~set_mask_bit(from)) & ~o_attack_but_king_mask) |
              (o_pnb_mask & magic_ns::rook_moves(to, mask_all & ~set_mask_bit(from)) & ~o_attack_but_king_mask);
            if (attack_mask != 0ULL) {
              if (auto move = bestMove(attack_mask, from, to, best)) {
                best = move;
              }
            }
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
        if (tfield || to == board_.enpassant() || (set_mask_bit(to) & o_attack_but_king_mask))
          continue;

        // usual moves
        auto pw_from = movesTable().pawnFrom(color, to) & pawns;
        visited |= pw_from;
        for (; pw_from;)
        {
          auto from = clear_lsb(pw_from);
          if (board_.is_something_between(from, to, mask_all_inv))
            continue;
          if (oki_moves == 0ULL)
          {
            auto pki_blocked_from = movesTable().pawnCaps(color, to) & oki_moves_all & ~multiattack_mask;
            auto pki_blocked_to = movesTable().pawnCaps(color, to) & oki_moves_all;
            pki_blocked_to = pki_blocked_from & ~pki_blocked_to;
            if(pki_blocked_to == 0ULL && push_back(from, to)) {
              break;
            }
          }
          auto attack_mask = o_nbrq_mask & movesTable().pawnCaps(color, to);
          if (attack_mask != 0ULL) {
            if (auto move = bestMove(attack_mask, from, to, best)) {
              best = move;
            }
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
        if (!discovered)
        {
          kn_moves &= knight_check_mask & (~o_attack_mask| multiattack_mask) & ~o_attack_pnb_mask;
        }
        for (; kn_moves;)
        {
          auto to = clear_lsb(kn_moves);
          X_ASSERT(board_.getField(to), "field, from that we are going to check is occupied");

          if ((!oki_moves || discovered && !can_escape) && push_back(from, to)) {
            break;
          }
          auto attack_mask = movesTable().caps(Figure::TypeKnight, to) & (o_pbrq_mask & ~o_attack_but_king_mask);
          if(attack_mask != 0ULL)
          {
            if (auto move = bestMove(attack_mask, from, to, best)) {
              best = move;
            }
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
                ((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL))
            {
              if(push_back(from, *to))
                break;
            }
            if ((*to >= 0 && !board_.getField(*to)) && ((movesTable().pawnCaps(color, *to) & o_nbrq_mask) != 0ULL)) {
              if (auto move = bestMove(movesTable().pawnCaps(color, *to) & o_nbrq_mask, from, *to, best)) {
                best = move;
              }
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
                BitMask r_mat_mask{};
                if (oki_moves != 0ULL) {
                  /// cross the rook move and line from oking to it's unattacked field
                  auto ki_move = _lsb64(oki_moves);
                  auto through_mask = betweenMasks().line(oki_pos, ki_move);
                  r_mat_mask = magic_ns::rook_moves(oki_pos, mask_all) & through_mask;
                }
                else if(rki_blocked_from) {
                  // move along the line going through field, attacked by this rook
                  int rb = _lsb64(rki_blocked_from);
                  r_mat_mask = betweenMasks().line(rb, from);
                }
                else {
                  // oking has no moves and out rook doesn't block it. go anywhere
                  r_mat_mask = to_mask;
                }
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
                  if (is_mat || ((oki_moves & ~r_att_to) == 0ULL && (~r_att_to & rki_blocked_from) == 0ULL))
                  {
                    if(push_back(from, to))
                      break;
                  }
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
              if((rki_unblocked_to == 0ULL) && push_back(from, to))
                break;
            }
          }
          // attack through king
          {
            auto atk_mask = magic_ns::rook_moves(oki_pos, mask_all | set_mask_bit(to)) & betweenMasks().from(to, oki_pos);
            atk_mask &= ((o_pnb_mask|o_pnr_mask) & ~o_attack_but_king_mask) | fmgr.queen_mask(ocolor);
            if ((atk_mask != 0ULL) && distanceCounter().getDistance(oki_pos, _lsb64(atk_mask)) > 2)
            {
              if (auto move = bestMove(atk_mask, from, to, best)) {
                best = move;
              }
            }
          }
          // check with attack to unsafe figure
          {
            auto attack_mask = ((o_pnb_mask & ~o_attack_but_king_mask) | fmgr.queen_mask(ocolor)) & magic_ns::rook_moves(to, mask_all & ~set_mask_bit(from));
            if (attack_mask != 0ULL) {
              if (auto move = bestMove(attack_mask, from, to, best)) {
                best = move;
              }
            }
          }
        }
      }
    }

    // bishop
    if(moves_.empty())
    {
      auto o_rq_mask = fmgr.queen_mask(ocolor);
      if (threshold <= Figure::figureWeight_[Figure::TypeRook])
        o_rq_mask |= fmgr.rook_mask(ocolor);
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
            if ((bki_unblocked_to == 0ULL) && push_back(from, to))
              break;
          }
          // attack through king
          {
            auto atk_mask = magic_ns::bishop_moves(oki_pos, mask_all | set_mask_bit(to)) & betweenMasks().from(to, oki_pos);
            atk_mask &= (o_pnb_mask & ~o_attack_but_king_mask) | o_rq_mask;
            if ((atk_mask != 0ULL) && distanceCounter().getDistance(oki_pos, _lsb64(atk_mask)) > 1)
            {
              if (auto move = bestMove(atk_mask, from, to, best)) {
                best = move;
              }
            }
          }
          // check with attack to unsafe figure
          {
            auto attack_mask = o_nbrq_mask & ~o_attack_but_king_mask & magic_ns::bishop_moves(to, mask_all & ~set_mask_bit(from));
            if (attack_mask != 0ULL) {
              if (auto move = bestMove(attack_mask, from, to, best)) {
                best = move;
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
              ((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL))
            {
              if (push_back(from, *to))
                break;
            }
            if ((*to >= 0 && !board_.getField(*to)) && ((movesTable().pawnCaps(color, *to) & o_nbrq_mask) != 0ULL)) {
              if (auto move = bestMove(movesTable().pawnCaps(color, *to) & o_nbrq_mask, from, *to, best)) {
                best = move;
              }
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
                bool is_mat = false;
                int to = -1;
                if (to_mask & oqb_attack_mask)
                  to = _lsb64(to_mask & oqb_attack_mask);
                else if (to_mask & orb_attack_mask)
                  to = _lsb64(to_mask & orb_attack_mask);
                else if (to_mask & ob_attack_mask)
                  to = _lsb64(to_mask & ob_attack_mask);
                else if ((oki_moves & ~betweenMasks().from(from, oki_pos)) == 0ULL)
                {
                  is_mat = true;
                  to = _lsb64(to_mask);
                }
                if (to >= 0)
                {
                  X_ASSERT(board_.getField(to), "field is occupied");
                  if (is_mat && push_back(from, to))
                    break;
                  auto attack_mask = magic_ns::bishop_moves(to, mask_all) & o_nbrq_mask;
                  if (auto move = bestMove(attack_mask, from, to, best)) {
                    best = move;
                  }
                }
              }
            }
          }
        }
      }
    }
    //if (moves_.empty() && best && atLeast) {
    //  moves_.push_back(best);
    //}
    iter_ = moves_.begin();
  }

  BOARD const& board_;
  MovesList moves_;
  typename MovesList::iterator iter_;
}; // ChecksGenerator

} // NEngine