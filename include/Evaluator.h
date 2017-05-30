/*************************************************************
  Evaluator.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once

#include <Board.h>
#include <EvalCoefficients.h>
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
  };

  struct PasserInfo
  {
    FullScore score;
    int       most_y{ 0 };
  };

  struct FieldsInfo
  {
    int king_pos_{-1};
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
    int attackersN_[Figure::TypesNum] = {};
  } finfo_[2];

public:

	//// used to decide calculate null-move or not
	//static const ScoreType nullMoveMargin_;
	//static const ScoreType nullMoveVerifyMargin_;

 // static int score_ex_max_;

 // // position evaluation. 0 - opening, 1 - endgame; color,type,pos
 // static const ScoreType positionEvaluations_[2][8][64];

  const int lazyThreshold0_ = 400;// Figure::figureWeight_[Figure::TypePawn] * 4;
  const int lazyThreshold1_ = 300;// Figure::figureWeight_[Figure::TypePawn] * 3;

 // // evaluation constants
 // static const ScoreType bishopKnightMat_[64];
 // static const ScoreType pawnDoubled_, pawnIsolated_, pawnBackward_, pawnDisconnected_, pawnBlocked_, defendedBonus_;
 // static const ScoreType groupsPenalty_;
 // static const ScoreType assistantBishop_, rookBehindBonus_;
 // static const ScoreType semiopenRook_, openRook_, winloseBonus_;
 // static const ScoreType fakecastlePenalty_;
 // static const ScoreType castleImpossiblePenalty_;
 // static const ScoreType bishopBonus_;
 // static const ScoreType unstoppablePawn_;
 // static const ScoreType kingFarBonus_;
 // static const ScoreType pawnPassed_[8], passerCandidate_[8];//, pawnCanGo_[8];
 // static const ScoreType passersGroup_[8];
 // static const ScoreType mobilityBonus_[8][32];
 // static const ScoreType kingDistanceBonus_[8][8];
	//static const ScoreType attackerNumberBonus_[8];
 // static const ScoreType attackedByWeakBonus_;
 // static const ScoreType forkBonus_;
 // static const ScoreType fianchettoBonus_;
 // static const ScoreType rookToKingBonus_;

 // /// material difference
 // static const ScoreType figureAgainstPawnBonus_[2];
 // static const ScoreType rookAgainstFigureBonus_[2];
	//static const ScoreType rookAgainstPawnsBonus_[2];

 // // blocked bishop & knight
 // static const ScoreType bishopBlocked_;
 // static const ScoreType knightBlocked_;

 // // pinned
 // static const ScoreType pinnedKnight_;
 // static const ScoreType pinnedBishop_;
 // static const ScoreType pinnedRook_;

 // // pawns shield. penalty for absent pawn
 // static const ScoreType cf_columnOpened_;
 // static const ScoreType bg_columnOpened_;
 // static const ScoreType ah_columnOpened_;

 // static const ScoreType cf_columnSemiopened_;
 // static const ScoreType bg_columnSemiopened_;
 // static const ScoreType ah_columnSemiopened_;

 // static const ScoreType cf_columnCracked_;
 // static const ScoreType bg_columnCracked_;
 // static const ScoreType ah_columnCracked_;

 // static const ScoreType pawnBeforeKing_;

 // // pressure to opponent's king
 // static const ScoreType kingPawnPressure_;
 // //static const ScoreType kingKnightPressure_;
 // //static const ScoreType kingBishopPressure_;
 // //static const ScoreType kingRookPressure_;
 // //static const ScoreType kingQueenPressure_;
  
  //Evaluator();

  void initialize(Board const* board, EHashTable* ehash);

  ScoreType operator () (ScoreType alpha, ScoreType betta);

	///// evaluate material balance only
	//ScoreType express() const;

 // bool isSpecialCase() const;

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
    return Figure::ColorBlack  == board_->getColor() ? -score : score;
  }

  /// calculates absolute position evaluation
  ScoreType evaluate();

  // multiple coefficients for opening/endgame
  PhaseInfo detectPhase() const;

  // get from PSQ table
  // + fill attacked fileds masks
  FullScore evaluateKnights(Figure::Color color);
  FullScore evaluatePsq(Figure::Color color);

  FullScore evaluateMobility(Figure::Color color);

  FullScore evaluatePawnsPressure(Figure::Color color);

  ScoreType evaluateKingPsqEg(Figure::Color color) const;

  // calculate or take from hash
  // pawns structure for middle & end game
  // + king's pawn shield???
  FullScore hashedEvaluation();
  int closestToBackward(int x, int y, const BitMask & pmask, Figure::Color color) const;
  bool couldBeSupported(Index const& idx, Figure::Color color, Figure::Color ocolor, BitMask const& pmask, BitMask const& opmsk) const;
  FullScore evaluatePawns(Figure::Color color) const;
  PasserInfo passerEvaluation(Figure::Color color) const;
  FullScore passerEvaluation() const;
  // search path from opponent king to pawn's promotion of given color
  bool findRootToPawn(Figure::Color color, int promo_pos, int stepsMax) const;

  ScoreType evaluateMaterialDiff() const;

   /// find knight and pawn forks
   ScoreType evaluateForks(Figure::Color color) const;

  // 0 - short, 1 - long, -1 - no castle
  int getCastleType(Figure::Color color) const;
  FullScore evaluateKingSafety(Figure::Color color) const;
  FullScore evaluateKingSafety() const;
  int evaluateCastle(Figure::Color color, Figure::Color ocolor, int castleType, Index const& ki_pos) const;

  int evaluateBlockedKnights();
  int evaluateBlockedBishops();
  int evaluateBlockedRooks();

 // ScoreType evaluateFianchetto() const;

 // // special cases

 // enum SpecialCases
 // {
 //   SC_None, SC_RBR_W, SC_RNR_W, SC_RBR_B, SC_RNR_B, SC_RF_B, SC_RF_W, SC_R2F_B, SC_R2F_W, SC_RFP_B, SC_RFP_W, SC_2NP_B, SC_2NP_W, SC_RPRF_W, SC_RPRF_B
 // };

 // SpecialCases findSpecialCase() const;
 // ScoreType evaluateSpecial(SpecialCases sc) const;
  ScoreType evaluateWinnerLoser();
 // bool evaluateWinnerLoserSpecial(ScoreType & score);
 // ScoreType evaluateTrueWinnerLoser();

 // /// bishops mobility and attacks
 // ScoreType evaluateBishops();

 // /// calculate field (bit-mask) attacked by knights, and mobility
 // ScoreType evaluateKnights();

 // // evaluate rooks mobility and open column
 // void evaluateRooks(bool eval_open);

 // // queens mobility
 // void evaluateQueens();

 // /// lazy eval.
 // ScoreType evaluateExpensive(GamePhase phase, int coef_o, int coef_e);

	///// returns 1 if at least 1 field in mask is attacked from given pos (include X-Ray attacks)
	//int isAttackingFields(int from, const BitMask & mask) const;

	//ScoreType evaluateKingPressure() const;


 // inline void mobility_masks_LSB(int from, BitMask & mob_mask, const BitMask & di_mask) const
 // {
 //   BitMask mask_from = di_mask & mask_all_;
 //   mob_mask |= (mask_from) ? betweenMasks().between(from, _lsb64(mask_from)) : di_mask;
 // }

 // inline void mobility_masks_MSB(int from, BitMask & mob_mask, const BitMask & di_mask) const
 // {
 //   BitMask mask_from = di_mask & mask_all_;
 //   mob_mask |= (mask_from) ? betweenMasks().between(from, _msb64(mask_from)) : di_mask;
 // }

 // // used to find pinned figures
 // enum PinType { ptAll, ptOrtho, ptDiag };
 // // acolor - color of attacking side, ki_pos - attacked king pos
 // bool discoveredCheck(int pt, Figure::Color acolor, const BitMask & brq_mask, int ki_pos, enum PinType pinType) const;


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

  static const int colored_y_[2][8];

  BitMask mask_all_{};
  BitMask inv_mask_all_{};
  int alpha0_{};
  int betta0_{};
  int alpha1_{};
  int betta1_{};
};

} // NEngine