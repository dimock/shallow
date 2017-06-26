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

enum {
  A1, B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
};

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

    bool operator == (FullScore const& other) const
    {
      return common_ == other.common_ && opening_ == other.opening_ && endGame_ == other.endGame_;
    }
  };

  struct PasserInfo
  {
    FullScore score;
    int       most_y{ 0 };
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
    BitMask rookAttacks_{};
    BitMask queenAttacks_{};
    BitMask kingAttacks_{};
    BitMask attack_mask_{};
    xlist<BitMask, 10> knightMasks_;
    xlist<BitMask, 10> bishopMasks_;
    xlist<BitMask, 10> rookMasks_;
    xlist<BitMask, 10> queenMasks_;
  } finfo_[2];

public:

  const int lazyThreshold0_ = 400;// Figure::figureWeight_[Figure::TypePawn] * 4;
  const int lazyThreshold1_ = 300;// Figure::figureWeight_[Figure::TypePawn] * 3;
  void initialize(Board const* board, EHashTable* ehash, GHashTable* ghash);

  ScoreType operator () (ScoreType alpha, ScoreType betta);

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

  // get from PSQ table
  // bruteforce and slow
  FullScore evaluatePsqBruteforce() const;

  // + fill attacked fileds masks
  FullScore evaluateKnights(Figure::Color color);
  FullScore evaluateFigures(Figure::Color color);

  FullScore evaluateMobility(Figure::Color color);
  FullScore evaluatePawnsPressure(Figure::Color color);

  // calculate or take from hash
  // pawns structure for middle & end game
  // + king's pawn shield???
  FullScore hashedEvaluation();
  int closestToBackward(int x, int y, const BitMask & pmask, Figure::Color color) const;
  bool couldBeSupported(Index const& idx, Figure::Color color, Figure::Color ocolor, BitMask const& pmask, BitMask const& opmsk) const;

  FullScore evaluatePawns(Figure::Color color) const;
  FullScore evaluatePawns() const
  {
    auto score = evaluatePawns(Figure::ColorWhite);
    score -= evaluatePawns(Figure::ColorBlack);
    return score;
  }

  PasserInfo passerEvaluation(Figure::Color color) const;
  FullScore passerEvaluation() const;
  // search path from opponent king to pawn's promotion of given color
  bool findRootToPawn(Figure::Color color, int promo_pos, int stepsMax) const;

  ScoreType evaluateMaterialDiff() const;

   /// find knight and pawn forks
   ScoreType evaluateForks(Figure::Color color) const;

  // 0 - short, 1 - long, -1 - no castle
  int getCastleType(Figure::Color color) const;
  
  int evaluateCastle(Figure::Color color) const;
  int evaluateCastle() const
  {
    auto score = evaluateCastle(Figure::ColorWhite);
    score -= evaluateCastle(Figure::ColorBlack);
    return score;
  }

  int evaluateKingSafety(Figure::Color color) const;
  FullScore evaluateKingSafety() const
  {
    FullScore score;
    score.opening_ = evaluateKingSafety(Figure::ColorWhite);
    score.opening_ -= evaluateKingSafety(Figure::ColorBlack);
    return score;
  }

  int evaluateBlockedKnights();
  int evaluateBlockedBishops();
  int evaluateBlockedRooks();

  // sum of weights of all figures
  const int openingWeight_ = 2*(Figure::figureWeight_[Figure::TypeQueen]
                                + 2*Figure::figureWeight_[Figure::TypeRook]
                                + 2*Figure::figureWeight_[Figure::TypeKnight]
                                + 6*Figure::figureWeight_[Figure::TypePawn]);

  const int endgameWeight_ = 2*(Figure::figureWeight_[Figure::TypeQueen]
                                + Figure::figureWeight_[Figure::TypeKnight]
                                + 3*Figure::figureWeight_[Figure::TypePawn]);

  const int weightOEDiff_ = openingWeight_ - endgameWeight_;

  Board const* board_{ nullptr };
  EHashTable*  ehash_{ nullptr };
  GHashTable*  ghash_{ nullptr };

  static const int colored_y_[2][8];

  BitMask mask_all_{};
  BitMask inv_mask_all_{};
  int alpha0_{};
  int betta0_{};
  int alpha1_{};
  int betta1_{};
};

} // NEngine