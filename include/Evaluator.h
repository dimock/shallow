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
    int       most_passer_y{ 0 };
    int       most_unstoppable_y{ 0 };
  };

  struct FieldsInfo
  {
    int knightMobility_{};
    int bishopMobility_{};
    int rookMobility_{};
    int queenMobility_{};
    int knightPressure_{};
    int bishopPressure_{};
    int rookPressure_{};
    int queenPressure_{};
    BitMask pawnAttacks_{};
    BitMask knightAttacks_{};
    BitMask bishopAttacks_{};
    BitMask bishopTreatAttacks_{};
    BitMask rookAttacks_{};
    BitMask queenAttacks_{};
    BitMask bishopDirectAttacks_{};
    BitMask rookDirectAttacks_{};
    BitMask queenDirectAttacks_{};
    BitMask kingAttacks_{};
    BitMask attack_mask_{};
    BitMask multiattack_mask_{};

#ifdef PROCESS_DANGEROUS_EVAL
    int pawnsUnderAttack_{};
    int knightsUnderAttack_{};
    int bishopsUnderAttack_{};
    int rooksUnderAttack_{};
    int queensUnderAttack_{};
    bool matThreat_{};
    bool pawnPromotion_{};
    bool forkTreat_{};
#endif // PROCESS_DANGEROUS_EVAL
  } finfo_[2];

public:
  const int lazyThreshold0_ = 1000;
  const int lazyThreshold1_ = 600;

  void initialize(Board const* board);

  ScoreType operator () (ScoreType alpha, ScoreType betta);

  ScoreType materialScore() const;

#ifdef PROCESS_DANGEROUS_EVAL
  bool dangerous() const
  {
    return dangerous_;
  }
#endif // PROCESS_DANGEROUS_EVAL

  static const int colored_y_[2][8];
  static const int promo_y_[2];
  static const int delta_y_[2];

private:

  void prepare();

#ifdef PROCESS_DANGEROUS_EVAL
  void detectDangerous();

#ifdef EVALUATE_DANGEROUS_ATTACKS
  FullScore evaluateDangerous() const;
#endif

#endif

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
  FullScore evaluateRook(Figure::Color color);
  FullScore evaluateQueens(Figure::Color color);
  FullScore evaluateMobilityAndKingPressure(Figure::Color color);
  bool isPinned(int pos, Figure::Color ocolor, BitMask targets, nst::bishop_rook_dirs dir) const;

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
  const int openingWeight_ = 2*(Figure::figureWeight_[Figure::TypeQueen]
                                + 2*Figure::figureWeight_[Figure::TypeRook]
                                + 4*Figure::figureWeight_[Figure::TypeKnight]);

  const int endgameWeight_ = 2*(Figure::figureWeight_[Figure::TypeKnight]);

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

#ifdef PROCESS_DANGEROUS_EVAL
  bool dangerous_{ false };
#endif // PROCESS_DANGEROUS_EVAL
};

} // NEngine