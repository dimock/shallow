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

  struct FullScore
  {
    int common_{};
    int opening_{};
    int endGame_{};

    FullScore& operator -= (FullScore const& other)
    {
      common_  -= other.common_;
      opening_ -= other.opening_;
      endGame_ -= other.endGame_;
      return *this;
    }

    FullScore& operator += (FullScore const& other)
    {
      common_  += other.common_;
      opening_ += other.opening_;
      endGame_ += other.endGame_;
      return *this;
    }

    FullScore operator + (FullScore const& other) const
    {
      FullScore result{*this};
      result.common_  += other.common_;
      result.opening_ += other.opening_;
      result.endGame_ += other.endGame_;
      return result;
    }

    FullScore operator - (FullScore const& other) const
    {
      FullScore result{ *this };
      result.common_ -= other.common_;
      result.opening_ -= other.opening_;
      result.endGame_ -= other.endGame_;
      return result;
    }

    FullScore& operator >>= (int shift)
    {
      common_ >>= shift;
      opening_ >>= shift;
      endGame_ >>= shift;
      return *this;
    }

    FullScore& operator /= (int n)
    {
      common_  /= n;
      opening_ /= n;
      endGame_ /= n;
      return *this;
    }

    bool operator == (FullScore const& other) const
    {
      return common_ == other.common_ && opening_ == other.opening_ && endGame_ == other.endGame_;
    }
  };

  struct PasserInfo
  {
    FullScore score;
    BitMask   passers_[2] = {};
    bool      passers_observed_[2] = {};
  };

  struct FieldsInfo
  {
    int num_attackers_{};
    int score_mob_{};
    int score_king_{};
    int score_opening_{};
    BitMask pawnAttacks_{};
    BitMask knightMoves_{};
    BitMask bishopMoves_{};
    BitMask rookMoves_{};
    BitMask queenMoves_{};
    BitMask knightAttacks_{};
    BitMask bishopAttacks_{};
    BitMask rookAttacks_{};
    BitMask queenAttacks_{};
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
    BitMask ki_fields_{};
    //BitMask ki_fields_prot_{};
    BitMask brq_mask_{};
    BitMask nbrq_mask_{};
    BitMask rq_mask_{};
  } finfo_[2];

public:
  const int lazyThreshold0_ = 1000;
  const int lazyThreshold1_ = 600;

  void initialize(Board const* board);

  ScoreType operator () (ScoreType alpha, ScoreType betta);
  ScoreType materialScore() const;

  static const int colored_y_[2][8];
  static const int promo_y_[2];
  static const int delta_y_[2];
  static const nst::dirs dir_behind_[2];

private:

  void prepare();

  // linear interpolation between opening & endgame
  ScoreType lipolScore(FullScore const& score, PhaseInfo const& phase) const
  {
    ScoreType result = score.common_;
    if(phase.phase_ == Opening)
      result += score.opening_;
    else if(phase.phase_ == EndGame)
      result += score.endGame_;
    else // middle game
      result = result + (score.opening_ * phase.opening_ + score.endGame_ * phase.endGame_) / weightOEDiff_;
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
  // pawns structure for middle & end game
  // + king's pawn shield???
  PasserInfo hashedEvaluation();
  int closestToBackward(int x, int y, const BitMask & pmask, Figure::Color color) const;
  bool isPawnBackward(Index const& idx, Figure::Color color, BitMask const& pmask, BitMask const& opmsk, BitMask const& fwd_field) const;
  bool isPawnBlocked(Index const& idx, Figure::Color color, BitMask const& pmask, BitMask const& opmsk, BitMask const& fwd_field) const;
  bool couldBeGuarded(Index const& idx, Figure::Color color, Figure::Color ocolor, BitMask const& pmask, BitMask const& opmsk, BitMask const& fwd_field, int n1) const;

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

  FullScore evaluateKnights();
  FullScore evaluateBishops();
  FullScore evaluateRook();
  FullScore evaluateQueens();
  FullScore evaluateKingPressure(Figure::Color color);
  bool isPinned(int pos, Figure::Color color, Figure::Color ocolor, BitMask targets, BitMask attackers, nst::bishop_rook_dirs dir) const;

  PasserInfo passerEvaluation(Figure::Color color, PasserInfo const&);
  FullScore passerEvaluation(PasserInfo const&);

  // search path from opponent king to pawn's promotion path of given color
  // idea from CCRL
  inline bool couldIntercept(Figure::Color color, int pawn_pos, int promo_pos, int stepsMax) const
  {
    auto ocolor = Figure::otherColor(color);
    BitMask inv_mask_all = ~(board_->fmgr().pawn_mask(ocolor) | board_->fmgr().king_mask(ocolor) | board_->fmgr().king_mask(color));
    return NEngine::couldIntercept(*board_, inv_mask_all, finfo_[color].attack_mask_, color, pawn_pos, promo_pos, stepsMax);
  }

  FullScore evaluateMaterialDiff() const;

  /// find knight and pawn forks
  ScoreType evaluateForks(Figure::Color color);

  // 0 - short, 1 - long, -1 - no castle
  int getCastleType(Figure::Color color) const;
  
  bool fakeCastle(Figure::Color color, int rpos, BitMask rmask) const;

  int evaluateKingSafety(Figure::Color color) const;
  int evaluateKingSafety(Figure::Color color, Index const& kingPos) const;

  FullScore evaluatePawnsPressure(Figure::Color color);

  // sum of weights of all figures
  const int openingWeight_ = 2*(    Figure::figureWeight_[Figure::TypeQueen]
                                + 2*Figure::figureWeight_[Figure::TypeRook]
                                + 2*Figure::figureWeight_[Figure::TypeBishop]
                                +   Figure::figureWeight_[Figure::TypeKnight]);

  const int endgameWeight_ = 2*(Figure::figureWeight_[Figure::TypeKnight] + Figure::figureWeight_[Figure::TypeRook]);

  const int weightOEDiff_ = openingWeight_ - endgameWeight_;

  Board const* board_{ nullptr };

#ifdef USE_HASH
  EHashTable ehash_{18};
#else
  EHashTable ehash_{0};
#endif

  static const BitMask castle_mask_[2][2];
  static const BitMask blocked_rook_mask_[2][2];

  BitMask mask_all_{};
  BitMask inv_mask_all_{};
  int alpha0_{};
  int betta0_{};
  int alpha1_{};
  int betta1_{};

  static const int maximumAttackersWeight_ = 2 * Figure::figureWeight_[Figure::TypeKnight] +
    2 * Figure::figureWeight_[Figure::TypeBishop] +
    2 * Figure::figureWeight_[Figure::TypeRook] +
    Figure::figureWeight_[Figure::TypeQueen];
};

} // NEngine