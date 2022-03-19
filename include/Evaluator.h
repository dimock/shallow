/*************************************************************
  Evaluator.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include "xoptimize.h"
#include "EvalCoefficients.h"
#include "Board.h"
#include "HashTable.h"
#include "xindex.h"
#include "xlist.h"

namespace NEngine
{

class Evaluator
{
public:
  enum GamePhase { Opening = 0, MiddleGame, EndGame };

  struct PhaseInfo
  {
    int opening_{};
    int endGame_{};
    GamePhase phase_{Opening};
  };

  struct PasserInfo
  {
    ScoreType32 pwscore_;
    int16       kscores_[2];
    BitMask     passers_{};
  };

  struct FieldsInfo
  {
    BitMask pawnAttacks_{};
    BitMask pawnAttacksForCheck_{};
    BitMask pawnPossibleAttacks_{};
    BitMask knightMoves_{};
    BitMask bishopMoves_{};
    BitMask rookMoves_{};
    BitMask queenMoves_{};
    BitMask behindPawnAttacks_{};
    BitMask behindOPawnAttacks_{};
    BitMask kingAttacks_{};
    BitMask attack_mask_{};
    BitMask multiattack_mask_{};
    BitMask cango_mask_{};
    BitMask mask_xray_b_{};
    BitMask mask_xray_r_{};
    BitMask nb_attacked_{};
    BitMask nbr_attacked_{};
    BitMask qbi_attacked_{};
    BitMask qr_attacked_{};
    BitMask ki_fields_{};
    BitMask ki_fields_no_pw_{};
    BitMask brq_mask_{};
    BitMask nbrq_mask_{};
    BitMask rq_mask_{};
    BitMask pawns_fwd_{};
    BitMask discovered_attackers_{};
    BitMask discovered_mask_{};
    BitMask attackedByKnightBrq_{};
    BitMask pinnedFigures_{};
    BitMask blockedFigures_{};
    BitMask attackedThrough_{};
    BitMask checks_mask_{};
    BitMask bishopMovesKipos_{};
    BitMask rookMovesKipos_{};
    BitMask discoveredMoves_{};
    ScoreType32 score_mob_{};
    int num_attackers_{};
    int score_king_{};
    bool qkingAttack_{};
    bool discoveredCheck_{};
    bool hasMoves_{};
  } finfo_[2];

  BitMask moves_masks_[NumOfFields] = {};

  BitMask mask_all_{};
  BitMask inv_mask_all_{};

  int alpha_{};
  int betta_{};

  Board const* board_{ nullptr };

#ifdef USE_EVAL_HASH_PW
  PHashTable ehash_{ 18 };
#else
  PHashTable ehash_{ 0 };
#endif

#ifdef USE_EVAL_HASH_MD
  FHashTable fhash_{ 18 };
#else
  FHashTable fhash_{ 0 };
#endif

#ifdef USE_EVAL_HASH_ALL
  AHashTable * ev_hash_{ nullptr };
#endif

public:
  static const int lazyThreshold_ = 1200;

  void initialize(Board const* board
#ifdef USE_EVAL_HASH_ALL
  , AHashTable* evh
#endif
    );
  void reset();

  ScoreType operator () (ScoreType alpha, ScoreType betta);
  ScoreType materialScore() const;

  inline void prefetch()
  {
#ifdef USE_EVAL_HASH_PW
    ehash_.prefetch(board_->fmgr().kpwnCode());
#endif

#ifdef USE_EVAL_HASH_MD
    fhash_.prefetch(board_->fmgr().fgrsCode());
#endif
  }

  static const int colored_y_[2][8];
  static const int promo_y_[2];
  static const int delta_y_[2];
  static const nst::dirs dir_behind_[2];

private:

  void prepare();

  // linear interpolation between opening & endgame
  ScoreType lipolScore(ScoreType32 const& score32, PhaseInfo const& phase) const
  {
    ScoreType result{};
    if (phase.phase_ == Opening)
      result += score32.eval0();
    else if (phase.phase_ == EndGame)
      result += score32.eval1();
    else // middle game
      result = result + (score32.eval0() * phase.opening_ + score32.eval1()* phase.endGame_) / weightOEDiff_;
    return result;
  }

  ScoreType considerColor(ScoreType score) const
  {
    return Figure::ColorBlack  == board_->color() ? -score : score;
  }

  /// calculates absolute position evaluation
  ScoreType evaluate(ScoreType alpha, ScoreType betta);

  // multiple coefficients for opening/endgame
  PhaseInfo detectPhase() const;

  // calculate or take from hash
  // pawns structure for middle & end game + king's pawn shield
  PasserInfo hashedEvaluation();
  
  PasserInfo evaluatePawns() const;

  bool blockedKnight(Figure::Color color, int n) const;
  bool blockedBishop(Figure::Color color, int n) const;
  bool blockedRook(Figure::Color color, Index rpos, BitMask rmask) const;

  void prepareAttacksMasks();

  ScoreType32 evaluateKnights();
  ScoreType32 evaluateBishops();
  ScoreType32 evaluateRook();
  ScoreType32 evaluateQueens();

  bool detectStalemate() const;

  bool discoveredCheck(int pos, Figure::Color color) const
  {
    auto const ki_pos = board_->kingPos(color);
    auto const pos_mask = set_mask_bit(pos);
    auto const from_mask = betweenMasks().from(ki_pos, pos);
    return (from_mask & finfo_[color].discovered_attackers_ & ~pos_mask) && (from_mask & finfo_[color].discovered_mask_ & pos_mask);
  }

  inline bool isMatTreat(Figure::Color color, Figure::Color ocolor, BitMask attacked_any_but_oking,
    BitMask q_check, BitMask r_check, BitMask bi_check) const
  {
    const auto& fmgr = board_->fmgr();
    auto mat_fields_mask = (mask_all_ | finfo_[ocolor].multiattack_mask_ | finfo_[ocolor].discoveredMoves_) & ~fmgr.king_mask(ocolor);
    if (q_check) {
      auto oking_possible_moves = finfo_[ocolor].kingAttacks_ &
        ~(finfo_[color].multiattack_mask_ | mask_all_ | (finfo_[color].attack_mask_ & ~finfo_[color].queenMoves_));
      q_check &= ~attacked_any_but_oking;
      while (q_check) {
        auto n = clear_lsb(q_check);
        const auto& qmat_attacks = magic_ns::queen_moves(n, mat_fields_mask);
        const auto attacked_ok_field = qmat_attacks & fmgr.king_mask(ocolor);
        if (attacked_ok_field && !(oking_possible_moves & ~qmat_attacks)) {
          return true;
        }
      }
    }
    if (r_check) {
      auto oking_possible_moves = finfo_[ocolor].kingAttacks_ &
        ~(finfo_[color].multiattack_mask_ | mask_all_ | (finfo_[color].attack_mask_ & ~finfo_[color].rookMoves_));
      r_check &= ~attacked_any_but_oking;
      while (r_check) {
        auto n = clear_lsb(r_check);
        const auto& rmat_attacks = magic_ns::rook_moves(n, mat_fields_mask);
        if ((rmat_attacks & fmgr.king_mask(ocolor)) && !(oking_possible_moves & ~rmat_attacks)) {
          return true;
        }
      }
    }
    if (bi_check) {
      auto oking_possible_moves = finfo_[ocolor].kingAttacks_ &
        ~(finfo_[color].multiattack_mask_ | mask_all_ | (finfo_[color].attack_mask_ & ~finfo_[color].rookMoves_));
      bi_check &= ~attacked_any_but_oking;
      while (bi_check) {
        auto n = clear_lsb(bi_check);
        const auto& bimat_attacks = magic_ns::bishop_moves(n, mat_fields_mask);
        if ((bimat_attacks & fmgr.king_mask(ocolor)) && !(oking_possible_moves & ~bimat_attacks)) {
          return true;
        }
      }
    }
    return false;
  }

  ScoreType32 evaluateKingPressure(Figure::Color color, int const kscore_o);

  bool isPinned(int pos, Figure::Color color, Figure::Color ocolor, BitMask targets, BitMask attackers, nst::bishop_rook_dirs dir) const;

  ScoreType32 passersEvaluation(PasserInfo const&);

  ScoreType32 evaluateMaterialDiff();

  /// find knight and pawn forks and other attacks
  ScoreType32 evaluateAttacks(Figure::Color color);
  ScoreType32 evaluatePawnsAttacks(Figure::Color color);

  // 0 - short, 1 - long, -1 - no castle
  int getCastleType(Figure::Color color) const;
  int getCastleType2(Figure::Color color) const;

  bool fakeCastle(Figure::Color color, int rpos, BitMask rmask) const;

  int evaluateKingSafetyW() const;
  int evaluateKingSafetyB() const;

  int evaluateKingSafety(Figure::Color color) const;
  int evaluateKingSafety(Figure::Color color, Index const kingPos) const;
  int opponentPawnsPressure(Figure::Color color, Index const kingPos) const;
  int evaluateKingsPawn(Figure::Color color, Index const kingPos) const;

  // sum of weights of all figures
  static const int openingWeight_ = 2*(    Figure::figureWeight_[Figure::TypeQueen]
                                + 2*Figure::figureWeight_[Figure::TypeRook]
                                + 2*Figure::figureWeight_[Figure::TypeBishop]
                                +   Figure::figureWeight_[Figure::TypeKnight]);

  static const int endgameWeight_ = 2*(Figure::figureWeight_[Figure::TypeKnight] + Figure::figureWeight_[Figure::TypeRook]);

  static const int weightOEDiff_ = openingWeight_ - endgameWeight_;

  static const BitMask castle_mask_[2][2];
  static const BitMask castle_mask2_[2][2];
  static const BitMask blocked_rook_mask_[2][2];

  static const int maximumAttackersWeight_ = 2 * Figure::figureWeight_[Figure::TypeKnight] +
    2 * Figure::figureWeight_[Figure::TypeBishop] +
    2 * Figure::figureWeight_[Figure::TypeRook] +
    Figure::figureWeight_[Figure::TypeQueen];
};

} // NEngine