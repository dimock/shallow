/*************************************************************
  Evaluator.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <xoptimize.h>
#include <EvalCoefficients.h>
#include <Board.h>
#include <HashTable.h>
#include <xindex.h>
#include <xlist.h>

namespace NEngine
{

class Evaluator
{
  enum GamePhase { Opening = 0, MiddleGame, EndGame };

  struct PhaseInfo
  {
    int opening_{};
    int endGame_{};
    GamePhase phase_{Opening};
  };

  struct PasserInfo
  {
    ScoreType32 score_;
    BitMask     passers_{};
  };

  struct FieldsInfo
  {
    BitMask pawnAttacks_{};
    BitMask pawnPossibleAttacks_{};
    BitMask knightMoves_{};
    BitMask bishopMoves_{};
    BitMask rookMoves_{};
    BitMask queenMoves_{};
    BitMask knightSafeMoves_{};
    BitMask behindPawnAttacks_{};
    BitMask bishopTreatAttacks_{};
    BitMask rookTreatAttacks_{};
    BitMask kingAttacks_{};
    BitMask attack_mask_{};
    BitMask multiattack_mask_{};
    BitMask cango_mask_{};
    BitMask mask_xray_b_{};
    BitMask mask_xray_r_{};
    BitMask nb_attacked_{};
    BitMask nbr_attacked_{};
    BitMask rq_attacked_{};
    BitMask r_attacked_{};
    BitMask ki_fields_{};
    BitMask ki_fields_no_pw_{};
    BitMask brq_mask_{};
    BitMask nbrq_mask_{};
    BitMask rq_mask_{};
    BitMask pawns_fwd_{};
    BitMask discovered_attackers_{};
    BitMask discovered_mask_{};
    BitMask attackedByKnightRq_{};
    ScoreType32 score_mob_{};
#ifdef DO_KING_EVAL
    int num_attackers_{};
    int score_king_{};
    bool qkingAttack_{};
    //int score_opening_{};
    //bool matTreat_{};
#endif
    bool discoveredCheck_{};
  } finfo_[2];

  BitMask moves_masks_[NumOfFields] = {};
  BitMask attacks_masks_[NumOfFields] = {};
  BitMask qr_attacks_masks_[NumOfFields] = {};

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
  
  int closestToBackward(int x, int y, const BitMask & pmask, Figure::Color color) const;
  bool isPawnBackward(Index const& idx, Figure::Color color, BitMask const& pmask, BitMask const& opmsk, BitMask const& fwd_field) const;

  PasserInfo evaluatePawns(Figure::Color color) const;
  PasserInfo evaluatePawns() const;

  // attacked field
  // blocked knighs/bishops
  // rooks on open column
  // mobility
  // basic king pressure == distance to king
  bool blockedKnight(Figure::Color color, int n) const;
  bool blockedBishop(Figure::Color color, int n) const;
  bool blockedRook(Figure::Color color, Index rpos, BitMask rmask) const;

  void prepareAttacksMasks();

  ScoreType32 evaluateKnights();
  ScoreType32 evaluateBishops();
  ScoreType32 evaluateRook();
  ScoreType32 evaluateQueens();

  bool discoveredCheck(int pos, Figure::Color color) const
  {
    auto const& ki_pos = board_->kingPos(color);
    auto const pos_mask = set_mask_bit(pos);
    auto const& from_mask = betweenMasks().from(ki_pos, pos);
    return (from_mask & finfo_[color].discovered_attackers_ & ~pos_mask) && (from_mask & finfo_[color].discovered_mask_ & pos_mask);
  }

#ifdef DO_KING_EVAL
  ScoreType32 evaluateKingPressure(Figure::Color color);
#endif

  bool isPinned(int pos, Figure::Color color, Figure::Color ocolor, BitMask targets, BitMask attackers, nst::bishop_rook_dirs dir) const;

  PasserInfo  passerEvaluation(Figure::Color color, PasserInfo const&);
  ScoreType32 passerEvaluation(PasserInfo const&);
  bool pawnUnstoppable(Index const& pidx, Figure::Color pwcolor) const;

  ScoreType32 evaluateMaterialDiff();

  /// find knight and pawn forks and other attacks
  ScoreType32 evaluateAttacks(Figure::Color color);

  // 0 - short, 1 - long, -1 - no castle
  int getCastleType(Figure::Color color) const;
  
  bool fakeCastle(Figure::Color color, int rpos, BitMask rmask) const;

  int evaluateKingSafety(Figure::Color color) const;
  int evaluateKingSafety(Figure::Color color, Index const& kingPos) const;
  int opponentPawnsPressure(Figure::Color color, Index const& kingPos) const;
  int evaluateKingsPawn(Figure::Color color, Index const& kingPos) const;

  ScoreType32 evaluatePawnsPressure(Figure::Color color);

  // sum of weights of all figures
  static const int openingWeight_ = 2*(    Figure::figureWeight_[Figure::TypeQueen]
                                + 2*Figure::figureWeight_[Figure::TypeRook]
                                + 2*Figure::figureWeight_[Figure::TypeBishop]
                                +   Figure::figureWeight_[Figure::TypeKnight]);

  static const int endgameWeight_ = 2*(Figure::figureWeight_[Figure::TypeKnight] + Figure::figureWeight_[Figure::TypeRook]);

  static const int weightOEDiff_ = openingWeight_ - endgameWeight_;

  static const BitMask castle_mask_[2][2];
  static const BitMask blocked_rook_mask_[2][2];
  static const BitMask blocking_pw4r_mask_[2][2][2];

  static const int maximumAttackersWeight_ = 2 * Figure::figureWeight_[Figure::TypeKnight] +
    2 * Figure::figureWeight_[Figure::TypeBishop] +
    2 * Figure::figureWeight_[Figure::TypeRook] +
    Figure::figureWeight_[Figure::TypeQueen];
};

} // NEngine