/*************************************************************
  Evaluator.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include "Evaluator.h"
#include "Board.h"
#include "HashTable.h"
#include "xindex.h"
#include "magicbb.h"
#include "xbitmath.h"
#include "Helpers.h"
#include "xalgorithm.h"
#include "SpecialCases.h"

namespace NEngine
{

const int Evaluator::colored_y_[2][8] = {
  { 7, 6, 5, 4, 3, 2, 1, 0 },
  { 0, 1, 2, 3, 4, 5, 6, 7 } };

const int Evaluator::promo_y_[] = { 0, 7 };
const int Evaluator::delta_y_[] = { -1, +1 };
const nst::dirs Evaluator::dir_behind_[] = { nst::no, nst::so };

const BitMask Evaluator::castle_mask_[2][2] = {
  {
    set_mask_bit(F8) | set_mask_bit(G8) | set_mask_bit(H8) |
    set_mask_bit(F7) | set_mask_bit(G7) | set_mask_bit(H7) |
    set_mask_bit(F6) | set_mask_bit(G6) | set_mask_bit(H6),

    set_mask_bit(A8) | set_mask_bit(B8) | set_mask_bit(C8) |
    set_mask_bit(A7) | set_mask_bit(B7) | set_mask_bit(C7) |
    set_mask_bit(A6) | set_mask_bit(B6) | set_mask_bit(C6)
  },

  {
    set_mask_bit(F1) | set_mask_bit(G1) | set_mask_bit(H1) |
    set_mask_bit(F2) | set_mask_bit(G2) | set_mask_bit(H2) |
    set_mask_bit(F3) | set_mask_bit(G3) | set_mask_bit(H3),

    set_mask_bit(A1) | set_mask_bit(B1) | set_mask_bit(C1) |
    set_mask_bit(A2) | set_mask_bit(B2) | set_mask_bit(C2) |
    set_mask_bit(A3) | set_mask_bit(B3) | set_mask_bit(C3)
  }
};

const BitMask Evaluator::castle_mask2_[2][2] = {
  {
    set_mask_bit(H8) | set_mask_bit(F8) | set_mask_bit(G8) |
    set_mask_bit(H7) | set_mask_bit(F7) |
    set_mask_bit(H6),

    set_mask_bit(A8) | set_mask_bit(B8) | set_mask_bit(C8) |
    set_mask_bit(A7) | set_mask_bit(B7) | 
    set_mask_bit(A6)
  },

  {
    set_mask_bit(H1) | set_mask_bit(F1) | set_mask_bit(G1)  |
    set_mask_bit(H2) | set_mask_bit(G2) |
    set_mask_bit(H3),

    set_mask_bit(A1) | set_mask_bit(B1) | set_mask_bit(C1) |
    set_mask_bit(A2) | set_mask_bit(B2) |
    set_mask_bit(A3)
  }
};

const BitMask Evaluator::blocked_rook_mask_[2][2] = {
  {
    set_mask_bit(G1) | set_mask_bit(H1) | set_mask_bit(G2) | set_mask_bit(H2),
    set_mask_bit(A1) | set_mask_bit(B1) | set_mask_bit(A2) | set_mask_bit(B2)
  },
  {
    set_mask_bit(G8) | set_mask_bit(H8) | set_mask_bit(G7) | set_mask_bit(H7),
    set_mask_bit(A8) | set_mask_bit(B8) | set_mask_bit(A7) | set_mask_bit(B7)
  }
};

void Evaluator::initialize(Board const* board
#ifdef USE_EVAL_HASH_ALL
, AHashTable* evh
#endif
)
{
#ifdef USE_EVAL_HASH_ALL
  ev_hash_ = evh;
#endif
  board_ = board;
}

void Evaluator::reset()
{
  finfo_[0] = finfo_[1] = FieldsInfo{};
  mask_all_ = 0;
  inv_mask_all_ = 0;
  alpha_ = 0;
  betta_ = 0;
  ehash_.clear();
  fhash_.clear();
}

void Evaluator::prepare()
{
  auto const& fmgr = board_->fmgr();

  mask_all_ = fmgr.mask(Figure::ColorWhite) | fmgr.mask(Figure::ColorBlack);
  inv_mask_all_ = ~mask_all_;

  // pawns attacks
  {
    const BitMask pawn_msk_w = fmgr.pawn_mask(Figure::ColorWhite);
    const BitMask pawn_msk_b = fmgr.pawn_mask(Figure::ColorBlack);

    finfo_[Figure::ColorWhite].pawnAttacks_ = ((pawn_msk_w << 9) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_w << 7) & Figure::pawnCutoffMasks_[1]);
    finfo_[Figure::ColorBlack].pawnAttacks_ = ((pawn_msk_b >> 7) & Figure::pawnCutoffMasks_[0]) | ((pawn_msk_b >> 9) & Figure::pawnCutoffMasks_[1]);

    finfo_[Figure::ColorWhite].pawnPossibleAttacks_ = finfo_[Figure::ColorWhite].pawnAttacks_ |
      (finfo_[Figure::ColorWhite].pawnAttacks_ & ~0xff00000000000000) << 8;
    finfo_[Figure::ColorWhite].pawnPossibleAttacks_ = finfo_[Figure::ColorWhite].pawnPossibleAttacks_ |
      (finfo_[Figure::ColorWhite].pawnPossibleAttacks_ & ~0xff00000000000000) << 8;
    finfo_[Figure::ColorWhite].pawnPossibleAttacks_ = finfo_[Figure::ColorWhite].pawnPossibleAttacks_ |
      (finfo_[Figure::ColorWhite].pawnPossibleAttacks_ & ~0xff00000000000000) << 8;
    finfo_[Figure::ColorWhite].pawnPossibleAttacks_ = finfo_[Figure::ColorWhite].pawnPossibleAttacks_ |
      (finfo_[Figure::ColorWhite].pawnPossibleAttacks_ & ~0xff00000000000000) << 8;

    finfo_[Figure::ColorBlack].pawnPossibleAttacks_ = finfo_[Figure::ColorBlack].pawnAttacks_ |
      (finfo_[Figure::ColorBlack].pawnAttacks_ & ~0xff) >> 8;
    finfo_[Figure::ColorBlack].pawnPossibleAttacks_ = finfo_[Figure::ColorBlack].pawnPossibleAttacks_ |
      (finfo_[Figure::ColorBlack].pawnPossibleAttacks_ & ~0xff) >> 8;
    finfo_[Figure::ColorBlack].pawnPossibleAttacks_ = finfo_[Figure::ColorBlack].pawnPossibleAttacks_ |
      (finfo_[Figure::ColorBlack].pawnPossibleAttacks_ & ~0xff) >> 8;
    finfo_[Figure::ColorBlack].pawnPossibleAttacks_ = finfo_[Figure::ColorBlack].pawnPossibleAttacks_ |
      (finfo_[Figure::ColorBlack].pawnPossibleAttacks_ & ~0xff) >> 8;

    finfo_[0].attack_mask_ = finfo_[0].pawnAttacks_;
    finfo_[1].attack_mask_ = finfo_[1].pawnAttacks_;
  }

  // attacked fields near king
  {
    finfo_[0].ki_fields_ = finfo_[0].kingAttacks_ = movesTable().caps(Figure::TypeKing, board_->kingPos(Figure::ColorBlack));
    finfo_[1].ki_fields_ = finfo_[1].kingAttacks_ = movesTable().caps(Figure::TypeKing, board_->kingPos(Figure::ColorWhite));
    
    finfo_[0].multiattack_mask_ = finfo_[0].attack_mask_ & finfo_[0].kingAttacks_;
    finfo_[1].multiattack_mask_ = finfo_[1].attack_mask_ & finfo_[1].kingAttacks_;

    finfo_[0].attack_mask_ |= finfo_[0].kingAttacks_;
    finfo_[1].attack_mask_ |= finfo_[1].kingAttacks_;


    finfo_[0].ki_fields_ |= (finfo_[0].ki_fields_ >> 8);
    finfo_[1].ki_fields_ |= (finfo_[1].ki_fields_ << 8);

    finfo_[0].ki_fields_no_pw_ = finfo_[0].ki_fields_ & ~finfo_[0].pawnAttacks_;
    finfo_[1].ki_fields_no_pw_ = finfo_[1].ki_fields_ & ~finfo_[1].pawnAttacks_;
  }
  
  // other mask
  {
    finfo_[0].cango_mask_ = ~(finfo_[1].pawnAttacks_ | fmgr.king_mask(Figure::ColorBlack) | fmgr.pawn_mask(Figure::ColorBlack));
    finfo_[1].cango_mask_ = ~(finfo_[0].pawnAttacks_ | fmgr.king_mask(Figure::ColorWhite) | fmgr.pawn_mask(Figure::ColorWhite));

    finfo_[0].mask_xray_b_ = mask_all_ & ~(fmgr.bishop_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack));
    finfo_[0].mask_xray_r_ = mask_all_ & ~(fmgr.rook_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack));

    finfo_[1].mask_xray_b_ = mask_all_ & ~(fmgr.bishop_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite));
    finfo_[1].mask_xray_r_ = mask_all_ & ~(fmgr.rook_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite));

    finfo_[0].brq_mask_ = fmgr.bishop_mask(Figure::ColorBlack) | fmgr.rook_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack);
    finfo_[0].nbrq_mask_ = finfo_[0].brq_mask_ | fmgr.knight_mask(Figure::ColorBlack);
    finfo_[0].rq_mask_ = fmgr.rook_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack);

    finfo_[1].brq_mask_ = fmgr.bishop_mask(Figure::ColorWhite) | fmgr.rook_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite);
    finfo_[1].nbrq_mask_ = finfo_[1].brq_mask_ | fmgr.knight_mask(Figure::ColorWhite);
    finfo_[1].rq_mask_ = fmgr.rook_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite);

    finfo_[0].pawns_fwd_ = (fmgr.pawn_mask(Figure::ColorBlack) >> 8) & inv_mask_all_;
    finfo_[1].pawns_fwd_ = (fmgr.pawn_mask(Figure::ColorWhite) << 8) & inv_mask_all_;

    finfo_[0].pawns_fwd_ |= ((finfo_[0].pawns_fwd_ & Figure::pawns2ndLineMask_[0]) >> 8) & inv_mask_all_;
    finfo_[0].pawns_fwd_ &= ~finfo_[Figure::ColorWhite].pawnAttacks_;

    finfo_[1].pawns_fwd_ |= ((finfo_[1].pawns_fwd_ & Figure::pawns2ndLineMask_[1]) << 8) & inv_mask_all_;
    finfo_[1].pawns_fwd_ &= ~finfo_[Figure::ColorBlack].pawnAttacks_;

    finfo_[0].bishopMovesKipos_ = magic_ns::bishop_moves(board_->kingPos(Figure::ColorBlack), mask_all_);
    finfo_[0].rookMovesKipos_ = magic_ns::rook_moves(board_->kingPos(Figure::ColorBlack), mask_all_);

    finfo_[1].bishopMovesKipos_ = magic_ns::bishop_moves(board_->kingPos(Figure::ColorWhite), mask_all_);
    finfo_[1].rookMovesKipos_ = magic_ns::rook_moves(board_->kingPos(Figure::ColorWhite), mask_all_);

    auto const discovered_mask_bi_b = finfo_[0].bishopMovesKipos_ &
      (fmgr.knight_mask(Figure::ColorWhite) | fmgr.rook_mask(Figure::ColorWhite) | finfo_[0].nbrq_mask_ | fmgr.king_mask(Figure::ColorWhite));
    finfo_[0].discovered_mask_ = discovered_mask_bi_b;
    finfo_[0].discovered_attackers_ = magic_ns::bishop_moves(board_->kingPos(Figure::ColorBlack), mask_all_ & ~discovered_mask_bi_b) &
      (fmgr.bishop_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite));
    
    auto const discovered_mask_r_b = finfo_[0].rookMovesKipos_ &
      (fmgr.knight_mask(Figure::ColorWhite) | fmgr.bishop_mask(Figure::ColorWhite) | finfo_[0].nbrq_mask_ | fmgr.king_mask(Figure::ColorWhite));
    finfo_[0].discovered_mask_ |= discovered_mask_r_b;
    finfo_[0].discovered_attackers_ |= magic_ns::rook_moves(board_->kingPos(Figure::ColorBlack), mask_all_ & ~discovered_mask_r_b) &
      (fmgr.rook_mask(Figure::ColorWhite) | fmgr.queen_mask(Figure::ColorWhite));

    auto const discovered_mask_bi_w = finfo_[1].bishopMovesKipos_ &
      (fmgr.knight_mask(Figure::ColorBlack) | fmgr.rook_mask(Figure::ColorBlack) | finfo_[1].nbrq_mask_ | fmgr.king_mask(Figure::ColorBlack));
    finfo_[1].discovered_mask_ = discovered_mask_bi_w;
    finfo_[1].discovered_attackers_ = magic_ns::bishop_moves(board_->kingPos(Figure::ColorWhite), mask_all_ & ~discovered_mask_bi_w) &
      (fmgr.bishop_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack));

    auto const discovered_mask_r_w = finfo_[1].rookMovesKipos_ &
      (fmgr.knight_mask(Figure::ColorBlack) | fmgr.bishop_mask(Figure::ColorBlack) | finfo_[1].nbrq_mask_ | fmgr.king_mask(Figure::ColorBlack));
    finfo_[1].discovered_mask_ |= discovered_mask_r_w;
    finfo_[1].discovered_attackers_ |= magic_ns::rook_moves(board_->kingPos(Figure::ColorWhite), mask_all_ & ~discovered_mask_r_w) &
      (fmgr.rook_mask(Figure::ColorBlack) | fmgr.queen_mask(Figure::ColorBlack));
  }
}

//////////////////////////////////////////////////////////////////////////
ScoreType Evaluator::operator () (ScoreType alpha, ScoreType betta)
{
  X_ASSERT(!board_, "Evaluator wasn't properly initialized");

  if(!ehash_.empty())
    ehash_.prefetch(board_->fmgr().kpwnCode());

  if (!fhash_.empty())
    fhash_.prefetch(board_->fmgr().fgrsCode());

  if(board_->matState())
    return -Figure::MatScore;
  else if(board_->drawState())
    return Figure::DrawScore;

  ScoreType score = evaluate(alpha, betta);
  X_ASSERT(score <= -ScoreMax || score >= ScoreMax, "invalid score");
  return score;
}

ScoreType Evaluator::materialScore() const
{
  const FiguresManager& fmgr = board_->fmgr();
  auto result = considerColor(fmgr.weight().eval0());
  return result;
}

ScoreType Evaluator::evaluate(ScoreType alpha, ScoreType betta)
{
#ifndef NDEBUG
  std::string sfen = toFEN(*board_);
#endif

  int scoreOffset = 0;
  int scoreMultip = 1;
  auto spec = specialCases().eval(*board_);
  switch (spec.first) {
  case SpecialCaseResult::SCORE: {
    ScoreType score = spec.second;
    score = considerColor(score);
    return score;
  }
  case SpecialCaseResult::DRAW: {
    scoreOffset = 5;
    break;
  }
  case SpecialCaseResult::ALMOST_DRAW: {
    scoreOffset = 3;
    break;
  }
  case SpecialCaseResult::LIKELY_DRAW: {
    scoreOffset = 2;
    break;
  }
  case SpecialCaseResult::PROBABLE_DRAW: {
    scoreOffset = 1;
    break;
  }

  case SpecialCaseResult::MAYBE_DRAW: {
    scoreMultip = 3;
    scoreOffset = 2;
    break;
  }

  case SpecialCaseResult::POSSIBLE_WIN: {
    scoreMultip = 3;
    scoreOffset = 1;
    break;
  }

  default: {
    break;
  }
  }

#ifdef USE_LAZY_EVAL
  // prepare lazy evaluation
  alpha_ = (alpha > -Figure::MatScore) ? (int)alpha - lazyThreshold_ : -ScoreMax;
  betta_ = (betta < +Figure::MatScore) ? (int)betta + lazyThreshold_ : +ScoreMax;
#endif

  // determine game phase (opening, middle or end game)
  auto phaseInfo = detectPhase();

  const FiguresManager& fmgr = board_->fmgr();
  // evaluate figures weight

  ScoreType32 score32 = fmgr.weight();
  score32 += fmgr.score();
  auto mtdiff = evaluateMaterialDiff();
  score32 += mtdiff;

  /// use lazy evaluation
#ifdef USE_LAZY_EVAL
  {
    auto score0 = considerColor(lipolScore(score32, phaseInfo));
    if ((spec.first == SpecialCaseResult::NO_RESULT) && (score0 < alpha_ || score0 > betta_)) {
      return score0;
    }
  }
#endif

#ifdef USE_EVAL_HASH_ALL
  AHEval* heval = nullptr;
  uint32 hkey = 0;
  if (ev_hash_) {
    const BitMask code = board_->fmgr().hashCode();
    hkey = (uint32)(code >> 32);
    heval = ev_hash_->get(code);
    if (heval && heval->hkey_ == hkey) {
      return heval->score_;
    }
  }
#endif

  prepare();

  // take pawns eval from hash if possible
  auto hashedScore = hashedEvaluation();
  score32 += hashedScore.pwscore_;

  prepareAttacksMasks();

  auto score_nbrq = evaluateKnights();
  score_nbrq += evaluateBishops();
  score_nbrq += evaluateRook();
  score_nbrq += evaluateQueens();
  score32 += score_nbrq;
  
  if (detectStalemate()) {
    scoreOffset = 5;
  }

  auto score_mob = finfo_[Figure::ColorWhite].score_mob_;
  score_mob -= finfo_[Figure::ColorBlack].score_mob_;
  score32 += score_mob;

  ScoreType32 scoreKingW = ScoreType32{ hashedScore.kscores_[Figure::ColorWhite], 0 };
  ScoreType32 scoreKingB = ScoreType32{ hashedScore.kscores_[Figure::ColorBlack], 0 };
  if (phaseInfo.phase_ != GamePhase::EndGame) {
    scoreKingW -= evaluateKingPressure(Figure::ColorBlack, hashedScore.kscores_[Figure::ColorWhite]);
    scoreKingB -= evaluateKingPressure(Figure::ColorWhite, hashedScore.kscores_[Figure::ColorBlack]);
  }
  ScoreType32 scoreKing = scoreKingW - scoreKingB;
  score32 += scoreKing;

  auto scoreAttacks = evaluateAttacks(Figure::ColorWhite);
  scoreAttacks -= evaluateAttacks(Figure::ColorBlack);
  score32 += scoreAttacks;

  auto scorePP = evaluatePawnsAttacks(Figure::ColorWhite);
  scorePP -= evaluatePawnsAttacks(Figure::ColorBlack);
  score32 += scorePP;

  auto scorePassers = passersEvaluation(hashedScore);

#if 1
  score32 += scorePassers;

  auto result = considerColor(lipolScore(score32, phaseInfo));
  result *= scoreMultip;
  result >>= scoreOffset;
#else
  score32 = scoreKing;
  auto result = considerColor(lipolScore(score32, phaseInfo));
#endif

#ifdef USE_EVAL_HASH_ALL
  if (heval) {
    heval->hkey_ = hkey;
    heval->score_ = result;
  }
#endif

  result += 5;
  return result;
}

Evaluator::PhaseInfo Evaluator::detectPhase() const
{
  const FiguresManager & fmgr = board_->fmgr();

  auto wei = fmgr.figuresWeight(Figure::ColorBlack) + fmgr.figuresWeight(Figure::ColorWhite);

  PhaseInfo phaseInfo;
  phaseInfo.phase_ = MiddleGame;

  if(wei >= openingWeight_)
  {
    phaseInfo.phase_ = Opening;
  }
  else if(wei <= endgameWeight_)
  {
    phaseInfo.phase_ = EndGame;
  }

  phaseInfo.opening_ = wei - endgameWeight_;
  if(phaseInfo.opening_ > weightOEDiff_)
    phaseInfo.opening_ = weightOEDiff_;
  else if(phaseInfo.opening_ < 0)
    phaseInfo.opening_ = 0;
  phaseInfo.endGame_ = weightOEDiff_ - phaseInfo.opening_;

  return phaseInfo;
}

Evaluator::PasserInfo Evaluator::hashedEvaluation()
{
#ifdef USE_EVAL_HASH_PW
  const BitMask code = board_->fmgr().kpwnCode();
  uint32 hkey = (uint32)(code >> 32);
  auto* heval = ehash_.get(code);
  if(heval->hkey_ == hkey)
  {
    PasserInfo info;
    info.pwscore_ = heval->pwscore_;
    info.kscores_[Figure::ColorBlack] = heval->kscores_[Figure::ColorBlack];
    info.kscores_[Figure::ColorWhite] = heval->kscores_[Figure::ColorWhite];
    info.passers_ = heval->passers_;
#ifndef NDEBUG
    PasserInfo xinfo = evaluatePawns();
    auto pwscore = xinfo.pwscore_;
    auto kscorew = evaluateKingSafetyW();
    auto kscoreb = evaluateKingSafetyB();
    X_ASSERT_R(!(info.pwscore_ == pwscore && info.passers_ == xinfo.passers_ &&
      kscorew == xinfo.kscores_[Figure::ColorWhite]) && kscoreb == xinfo.kscores_[Figure::ColorBlack],
      "invalid pawns+king score in hash");
#endif
    return info;
  }
#endif

  PasserInfo info = evaluatePawns();
  auto kscorew = evaluateKingSafetyW();
  auto kscoreb = evaluateKingSafetyB();
  info.kscores_[Figure::ColorWhite] = kscorew;
  info.kscores_[Figure::ColorBlack] = kscoreb;

#ifdef USE_EVAL_HASH_PW
  heval->hkey_ = hkey;
  heval->pwscore_ = info.pwscore_;
  heval->kscores_[Figure::ColorBlack] = kscoreb;
  heval->kscores_[Figure::ColorWhite] = kscorew;
  heval->passers_ = info.passers_;
#endif

  return info;
}
template <Figure::Color>
int closestToBackward(int x, int y, const BitMask pmask);

template <>
int closestToBackward<Figure::ColorWhite>(int x, int y, const BitMask pmask)
{
  BitMask left  = (x != 0) ? (pawnMasks().mask_forward(Figure::ColorWhite, Index(x-1, y)) & pmask) : 0ULL;
  BitMask right = (x != 7) ? (pawnMasks().mask_forward(Figure::ColorWhite, Index(x+1, y)) & pmask) : 0ULL;
  int y_closest = 7;
  if(left)
    y_closest = (_lsb64(left) >> 3) + 1;
  if(right)
    y_closest = std::min(y_closest, ((_lsb64(right) >> 3) + 1));
  return y_closest;
}

template <>
int closestToBackward<Figure::ColorBlack>(int x, int y, const BitMask pmask)
{
  BitMask left = (x != 0) ? (pawnMasks().mask_forward(Figure::ColorBlack, Index(x - 1, y)) & pmask) : 0ULL;
  BitMask right = (x != 7) ? (pawnMasks().mask_forward(Figure::ColorBlack, Index(x + 1, y)) & pmask) : 0ULL;
  int y_closest = 0;
  if (left)
    y_closest = (_msb64(left) >> 3) - 1;
  if (right)
    y_closest = std::max(y_closest, ((_msb64(right) >> 3) - 1));
  return y_closest;
}

template<Figure::Color color>
bool isPawnBackward(Index const idx, BitMask const pmask, BitMask const opmsk, BitMask const fwd_field, BitMask const opawnAttacks);

template<>
bool isPawnBackward<Figure::ColorWhite>(Index const idx, BitMask const pmask, BitMask const opmsk, BitMask const fwd_field, BitMask const opawnAttacks)
{
  int closest_y = closestToBackward<Figure::ColorWhite>(idx.x(), idx.y(), pmask);
  X_ASSERT(closest_y < 0 || closest_y > 7, "backward mask calculation error - invalid next y position");
  BitMask pmask_after = betweenMasks().between(idx, Index(idx.x(), closest_y));
  auto blocked_mask = opawnAttacks | opmsk;
  bool blocked = (blocked_mask & pmask_after) != 0ULL;
  if (!blocked && (closest_y - idx.y()) < 3 && (idx.y() < 6)) {
    const auto fwd_field2 = fwd_field << 8;
    blocked = (fwd_field & pmask) != 0ULL && (fwd_field2 & blocked_mask) != 0ULL;
  }
  return blocked;
}

template <>
bool isPawnBackward<Figure::ColorBlack>(Index const idx, BitMask const pmask, BitMask const opmsk, BitMask const fwd_field, BitMask const opawnAttacks)
{
  int closest_y = closestToBackward<Figure::ColorBlack>(idx.x(), idx.y(), pmask);
  X_ASSERT(closest_y < 0 || closest_y > 7, "backward mask calculation error - invalid next y position");
  BitMask pmask_after = betweenMasks().between(idx, Index(idx.x(), closest_y));
  auto blocked_mask = opawnAttacks | opmsk;
  bool blocked = (blocked_mask & pmask_after) != 0ULL;
  if (!blocked && (idx.y() - closest_y) < 3 && (idx.y() > 3)) {
    const auto fwd_field2 = fwd_field >> 8;
    blocked = (fwd_field & pmask) != 0ULL && (fwd_field2 & blocked_mask) != 0ULL;
  }
  return blocked;
}

template <Figure::Color color>
Evaluator::PasserInfo evaluatePawn(FiguresManager const& fmgr, Evaluator::FieldsInfo const (&finfo)[2])
{
  const BitMask pmask = fmgr.pawn_mask(color);
  if(!pmask)
    return{};

  Evaluator::PasserInfo info;

  const Figure::Color ocolor = Figure::otherColor(color);
  const BitMask opmsk = fmgr.pawn_mask(ocolor);
  bool no_opawns = opmsk == 0ULL;
  auto pawns_all = opmsk | pmask;

  int dy = Evaluator::delta_y_[color];
  int py = Evaluator::promo_y_[color];

  BitMask pawn_mask = pmask;
  for (; pawn_mask;) {
    int const n = clear_lsb(pawn_mask);
    Index idx(n);

    const int x = idx.x();
    const int y = idx.y();
    int cy = Evaluator::colored_y_[color][y];
    auto n1 = n + (dy << 3);
    auto pw_field = set_mask_bit(n);
    auto fwd_field = set_mask_bit(n1);
    const auto fwd_mask = pawnMasks().mask_forward(color, n);
    const auto bkw_mask = pawnMasks().mask_forward(ocolor, n);

    auto const protectMask = movesTable().pawnCaps(ocolor, n);
    auto const attackMask = movesTable().pawnCaps(color, n);
    bool guarded = (protectMask & pmask) != 0ULL;
    bool isolated = (pmask & pawnMasks().mask_isolated(x)) == 0ULL;
    bool opened = ((opmsk| pmask) & fwd_mask) == 0ULL;
    bool backward = !isolated && ((pawnMasks().mask_backward(color, n) & pmask) == 0ULL) &&
      isPawnBackward<color>(idx, pmask, opmsk, fwd_field, finfo[ocolor].pawnAttacks_ & ~finfo[color].pawnAttacks_);
    bool neighbors = guarded || ((pawnMasks().mask_neighbor(color, n) & pmask) != 0ULL);
    bool doubled = (!guarded) && few_bits_set(pawnMasks().mask_column(x) & pmask) && ((bkw_mask & pmask) != 0ULL);
    bool unprotected = !isolated && !backward && !guarded;
    bool attack = (opmsk & attackMask) != 0ULL;

    info.pwscore_ += EvalCoefficients::isolatedPawn_[opened] * isolated;
    info.pwscore_ += EvalCoefficients::backwardPawn_[opened] * backward;
    info.pwscore_ += EvalCoefficients::unprotectedPawn_ * unprotected;
    info.pwscore_ += EvalCoefficients::hasneighborPawn_ * neighbors;
    info.pwscore_ += EvalCoefficients::doubledPawn_ * doubled;
    info.pwscore_ += EvalCoefficients::attackingPawn_[cy] * attack;

    // passer pawn - save position for further usage
    if (!(fwd_mask & (opmsk | pmask))) {
      const auto passmsk = pawnMasks().mask_passed(color, n);
      if (!(passmsk & opmsk)) {
        info.passers_ |= set_mask_bit(n);
      }
      else {
        auto attacks = movesTable().pawnCaps(color, n) & opmsk;
        auto attacks1 = movesTable().pawnCaps(color, n1) & opmsk;
        auto protects1 = movesTable().pawnCaps(ocolor, n1) & pmask;
        int attacksN = pop_count(attacks);
        auto protectsN = pop_count(protectMask & pmask);
        int attacksN1 = pop_count(attacks1);
        int protectsN1 = pop_count(protects1);
        if (protectsN >= attacksN && protectsN1 >= attacksN1) {
          info.passers_ |= set_mask_bit(n);
        }
      }
    }
  }

  return info;
}

Evaluator::PasserInfo Evaluator::evaluatePawns() const
{
  auto info_w = evaluatePawn<Figure::ColorWhite>(board_->fmgr(), finfo_);
  auto info_b = evaluatePawn<Figure::ColorBlack>(board_->fmgr(), finfo_);
  info_w.pwscore_ -= info_b.pwscore_;
  info_w.passers_ |= info_b.passers_;
  return info_w;
}

template <Figure::Color color>
bool canPromote(Board const& board, const Evaluator::FieldsInfo(&finfo)[2], BitMask const mask_all, Index const pidx)
{
  // next field is not attacked by opponent
  auto const& fmgr = board.fmgr();
  bool const bcolor = color == board.color();
  const Figure::Color ocolor = Figure::otherColor(color);
  const int py = Evaluator::promo_y_[color];
  Index pp{ pidx.x(), py };
  int oking_dist_pp = distanceCounter().getDistance(board.kingPos(ocolor), pp) - (!bcolor);
  int dist_pp = color ? py - pidx.y() : pidx.y() - py;
  bool king_far = oking_dist_pp > dist_pp;
  int king_dist_pp = distanceCounter().getDistance(board.kingPos(color), pp);
  if (fmgr.queens(ocolor) || fmgr.allFigures(ocolor) > 1 || dist_pp > 1 || king_dist_pp > 1 || oking_dist_pp < 3 || fmgr.pawns(color) == 1) {
    return false;
  }
  return true;
}

template <Figure::Color color>
bool pawnUnstoppable(Board const& board, const Evaluator::FieldsInfo(&finfo)[2], BitMask const mask_all, Index const pidx)
{
  // next field is not attacked by opponent
  auto const& fmgr = board.fmgr();
  bool const bcolor = color == board.color();
  const Figure::Color ocolor = Figure::otherColor(color);
  const int py = Evaluator::promo_y_[color];
  Index pp{ pidx.x(), py };
  int oking_dist_pp = distanceCounter().getDistance(board.kingPos(ocolor), pp) - (!bcolor);
  int dist_pp = color ? py - pidx.y() : pidx.y() - py;
  bool king_far = oking_dist_pp > dist_pp;
  if (!king_far) {
    return false;
  }
  // next move is promotion
  if (bcolor && dist_pp == 1) {
    return true;
  }
  if (fmgr.queens(ocolor) || fmgr.rooks(ocolor)) {
    return false;
  }
  bool cant_attack_pp = true;
  if (fmgr.knights(ocolor)) {
    cant_attack_pp = (movesTable().caps(Figure::TypeKnight, pp) & finfo[ocolor].knightMoves_ & ~finfo[color].attack_mask_) == 0ULL;
  }
  if (cant_attack_pp && fmgr.bishops(ocolor)) {
    cant_attack_pp = (magic_ns::bishop_moves(pp, mask_all) & finfo[ocolor].bishopMoves_ & ~finfo[color].attack_mask_) == 0ULL;
  }
  if (dist_pp == 1) {
    return cant_attack_pp;
  }
  else if (dist_pp == 2) {
    if (!cant_attack_pp) {
      return false;
    }
    if (bcolor) {
      return true;
    }
    else {
      const int dy = Evaluator::delta_y_[color];
      auto n1 = pidx + (dy << 3);
      bool cant_attack_n1 = true;
      if (fmgr.knights(ocolor)) {
        cant_attack_n1 = (movesTable().caps(Figure::TypeKnight, pp) & finfo[ocolor].knightMoves_ & ~finfo[color].attack_mask_) == 0ULL;
      }
      if (cant_attack_n1 && fmgr.bishops(ocolor)) {
        cant_attack_n1 = (magic_ns::bishop_moves(pp, mask_all) & finfo[ocolor].bishopMoves_ & ~finfo[color].attack_mask_) == 0ULL;
      }
      if (!cant_attack_n1) {
        return false;
      }
      if (fmgr.bishops(ocolor)) {
        bool bi_white = fmgr.bishop_mask(ocolor) &  FiguresCounter::s_whiteMask_;
        bool pp_white = set_mask_bit(pp) &  FiguresCounter::s_whiteMask_;
        if (bi_white != pp_white) {
          return true;
        }
      }
      if (fmgr.knights(ocolor) == 1) {
        Index n{ _lsb64(fmgr.knight_mask(ocolor)) };
        if (distanceCounter().getDistance(n, pp) > 4) {
          return true;
        }
      }
    }
  }
  return false;
}

template <Figure::Color color>
Evaluator::PasserInfo passerEvaluation(Board const& board, const Evaluator::FieldsInfo(&finfo)[2], BitMask const mask_all, Evaluator::PasserInfo const& pi)
{
  auto const& fmgr = board.fmgr();
  const bool bcolor = board.color() == color;
  const auto pmask = fmgr.pawn_mask(color);
  auto psmask = pmask & pi.passers_;
  if (!psmask)
    return{};


  const int py = Evaluator::promo_y_[color];
  const int dy = Evaluator::delta_y_[color];

  Evaluator::PasserInfo pinfo;

  Figure::Color ocolor = Figure::otherColor(color);
  const auto opmsk = fmgr.pawn_mask(ocolor);
  const auto ofgs = fmgr.mask(ocolor);
  bool no_opawns = opmsk == 0ULL;

  auto attack_mask = finfo[color].attack_mask_ | finfo[color].behindPawnAttacks_;
  auto o_attack_mask = finfo[ocolor].attack_mask_ | finfo[ocolor].behindOPawnAttacks_;
  auto multiattack_mask = finfo[color].multiattack_mask_ | (finfo[color].behindPawnAttacks_ & finfo[color].attack_mask_);
  auto o_multiattack_mask = finfo[ocolor].multiattack_mask_ | (finfo[ocolor].behindOPawnAttacks_ & finfo[ocolor].attack_mask_);
  auto blockers_mask = ((o_attack_mask & ~attack_mask) | (o_multiattack_mask & ~multiattack_mask)) & ~finfo[color].pawnAttacks_;
  blockers_mask |= mask_all;

  while (psmask) {
    const int n = clear_lsb(psmask);
    const auto fwd_fields = pawnMasks().mask_forward(color, n);

    const Index idx(n);
    const int y = idx.y();
    const int x = idx.x();
    const int cy = Evaluator::colored_y_[color][idx.y()];
    auto n1 = n + (dy << 3);
    auto pp = Index(x, py);
    auto fwd_field = set_mask_bit(n1);
    auto my_field = set_mask_bit(n);

    bool pguards  = (pawnMasks().mask_guards(color, n) & pmask & pi.passers_) != 0ULL;
    bool npguards = !pguards && ((pawnMasks().mask_guards(color, n) & pmask & ~pi.passers_) != 0ULL);

    ScoreType32 pwscore{};
    const auto passmsk = pawnMasks().mask_passed(color, n);
    BitMask attackers = passmsk & opmsk;
    int oking_dist = distanceCounter().getDistance(board.kingPos(ocolor), n1);
    int king_dist = distanceCounter().getDistance(board.kingPos(color), n1);
    if (passmsk & opmsk) {
      pwscore = EvalCoefficients::passerPawn2_[cy];
      pwscore += EvalCoefficients::passerPawnPGrds2_[cy] * pguards;
      pwscore += EvalCoefficients::passerPawnNPGrds2_[cy] * npguards;
      pwscore +=
        EvalCoefficients::okingToPasserDistanceBonus2_[cy] * oking_dist -
        EvalCoefficients::kingToPasserDistanceBonus2_[cy] * king_dist;
    }
    else {
      pwscore = EvalCoefficients::passerPawn_[cy];
      pwscore += EvalCoefficients::passerPawnPGrds_[cy] * pguards;
      pwscore += EvalCoefficients::passerPawnNPGrds_[cy] * npguards;
      pwscore +=
        EvalCoefficients::okingToPasserDistanceBonus_[cy] * oking_dist -
        EvalCoefficients::kingToPasserDistanceBonus_[cy] * king_dist;
    
      if (!(fwd_field & mask_all)) {
        if (!(fwd_field & o_attack_mask)) {
          const bool unstoppable = pawnUnstoppable<color>(board, finfo, mask_all, idx);
          pwscore += EvalCoefficients::passerUnstoppable_[cy] * unstoppable;
          if (!unstoppable && canPromote<color>(board, finfo, mask_all, idx)) {
            pwscore += EvalCoefficients::passerPawn_[cy];
          }
        }

        // all forward fields are not blocked by opponent
        auto fwd_mask = fwd_fields & blockers_mask;
        if (!fwd_mask) {
          pwscore += EvalCoefficients::passerPawnEx_[cy];
        }
        // only few fields are free
        else {
          int closest_blocker = (color == Figure::ColorWhite) ? _lsb64(fwd_mask) : _msb64(fwd_mask);
          int last_cango = Evaluator::colored_y_[color][Index(closest_blocker).y()] - 1;
          int steps = last_cango - cy;
          if (steps > 0) {
            pwscore += EvalCoefficients::passerPawnExS_[cy][steps];
          }
        }
      }
      else if (fwd_field & fmgr.mask(color)) {
        pwscore += EvalCoefficients::passerPawnMyBefore_[cy];
      }
    }

    pinfo.pwscore_ += pwscore;
  }
  return pinfo;
}

ScoreType32 Evaluator::passersEvaluation(PasserInfo const& pi)
{
  auto const& fmgr = board_->fmgr();
  auto infoW = NEngine::passerEvaluation<Figure::ColorWhite>(*board_, finfo_, mask_all_, pi);
  auto infoB = NEngine::passerEvaluation<Figure::ColorBlack>(*board_, finfo_, mask_all_, pi);
  infoW.pwscore_ -= infoB.pwscore_;
  return infoW.pwscore_;
}

bool Evaluator::fakeCastle(Figure::Color color, int rpos, BitMask rmask) const
{
  const FiguresManager & fmgr = board_->fmgr();
  Index ki_pos(board_->kingPos(color));
  Index r_pos(rpos);
  int ctype = r_pos.x() < ki_pos.x();
  if (board_->castling(color, ctype))
    return false;
  if ((ctype == 1 && ki_pos.x() > 3) || (ctype == 0 && ki_pos.x() < 4))
    return false;
  if ((color == Figure::ColorWhite && ki_pos.y() != 0) || (color == Figure::ColorBlack && ki_pos.y() != 7))
    return false;
  auto ocolor = Figure::otherColor(color);
  return ((blocked_rook_mask_[ocolor][ctype] & set_mask_bit(rpos)) != 0ULL) && ((rmask & ~blocked_rook_mask_[ocolor][ctype]) == 0ULL);
}

bool Evaluator::blockedRook(Figure::Color color, Index rpos, BitMask rmask) const
{
  const int ctype = rpos.x() < 4;
  return ((blocked_rook_mask_[color][ctype] & set_mask_bit(rpos)) != 0ULL) && ((rmask & ~blocked_rook_mask_[color][ctype]) == 0ULL);
}

ScoreType32 Evaluator::evaluateMaterialDiff()
{
#ifdef USE_EVAL_HASH_MD
  const BitMask code = board_->fmgr().fgrsCode();
  auto * heval = fhash_.get(code);
  uint32 hkey = (uint32)(code >> 32);
  if (heval->hkey_ == hkey)
  {
    return heval->score_;
  }
#endif

  const FiguresManager & fmgr = board_->fmgr();
  ScoreType32 score;

  int knightsDiff = fmgr.knights(Figure::ColorWhite) - fmgr.knights(Figure::ColorBlack);
  int bishopsDiff = fmgr.bishops(Figure::ColorWhite) - fmgr.bishops(Figure::ColorBlack);
  int figuresDiff = knightsDiff + bishopsDiff;
  int rooksDiff  = fmgr.rooks(Figure::ColorWhite)  - fmgr.rooks(Figure::ColorBlack);
  int queensDiff = fmgr.queens(Figure::ColorWhite) - fmgr.queens(Figure::ColorBlack);
  bool twoBishops = false;

  // bonus for double bishop
  if (fmgr.bishops(Figure::ColorWhite) >= 2)
  {
    const int pawnsN = fmgr.pawns(Figure::ColorWhite);
    score += EvalCoefficients::doubleBishopBonus_[pawnsN];
  }
  if (fmgr.bishops(Figure::ColorBlack) >= 2)
  {
    const int pawnsN = fmgr.pawns(Figure::ColorBlack);
    score -= EvalCoefficients::doubleBishopBonus_[pawnsN];
  }

  // bonus for double knight
  if (fmgr.knights(Figure::ColorWhite) >= 2)
  {
    const int pawnsN = fmgr.pawns(Figure::ColorWhite);
    score += EvalCoefficients::doubleKnightBonus_[pawnsN];
  }
  if (fmgr.knights(Figure::ColorBlack) >= 2)
  {
    const int pawnsN = fmgr.pawns(Figure::ColorBlack);
    score -= EvalCoefficients::doubleKnightBonus_[pawnsN];
  }

  // bonus for 2 bishops difference
  if (bishopsDiff >= 2 || bishopsDiff <= -2)
  {
    int bdiff = sign(bishopsDiff);
    Figure::Color bcolor = static_cast<Figure::Color>(bishopsDiff > 0);
    const int pawnsN = fmgr.pawns(bcolor);
    twoBishops = true;
    score += EvalCoefficients::twoBishopsBonus_[pawnsN] * bdiff;
  }

  // bonus for 2 knights difference
  if (knightsDiff >= 2 || knightsDiff <= -2)
  {
    int ndiff = sign(knightsDiff);
    Figure::Color ncolor = static_cast<Figure::Color>(knightsDiff > 0);
    const int pawnsN = fmgr.pawns(ncolor);
    score += EvalCoefficients::twoKnightsBonus_[pawnsN] * ndiff;
  }

  // Figure vs. Pawns
  if (!rooksDiff && !(twoBishops && figuresDiff*bishopsDiff > 0) && figuresDiff)
  {
    Figure::Color fcolor = static_cast<Figure::Color>(figuresDiff > 0);
    const int pawnsN = fmgr.pawns(fcolor);
    int fdiff = sign(figuresDiff);
    score += EvalCoefficients::figureAgainstPawnBonus_[pawnsN] * fdiff;
  }

  // Then evaluate 2 rooks as 1 queen
  if (queensDiff*rooksDiff < 0) {
    rooksDiff += 2 * queensDiff;
    queensDiff = 0;
  }

  // Knight|Bishop vs. Rook
  if (rooksDiff*figuresDiff == -1 && bishopsDiff != 2*figuresDiff)
  {
    Figure::Color rcolor = static_cast<Figure::Color>(rooksDiff > 0);
    Figure::Color fcolor = Figure::otherColor(rcolor);
    const int rpawnsN = fmgr.pawns(rcolor);
    const int fpawnsN = fmgr.pawns(fcolor);
    score -= EvalCoefficients::rookAgainstFigureBonus_[rpawnsN][fpawnsN] * rooksDiff;
  }
  // 2 figures vs. Rook
  if ((rooksDiff*figuresDiff <= -2) && (rooksDiff == 1 || rooksDiff == -1))
  {
    Figure::Color fcolor = static_cast<Figure::Color>(figuresDiff > 0);
    Figure::Color rcolor = Figure::otherColor(fcolor);
    const int fpawnsN = fmgr.pawns(fcolor);

    // 2 knights
    if(bishopsDiff == 0)
      score -= EvalCoefficients::knightsAgainstRookBonus_[fpawnsN] * rooksDiff;
    else
      score -= EvalCoefficients::figuresAgainstRookBonus_[fpawnsN] * rooksDiff;
    if(fmgr.bishops(fcolor) >= 2)
      score -= EvalCoefficients::doubleBishopBonus_[fpawnsN] * rooksDiff;
  }

#ifdef USE_EVAL_HASH_MD
    heval->hkey_ = hkey;
    heval->score_ = score;
#endif

  return score;
}

ScoreType32 Evaluator::evaluateAttacks(Figure::Color color)
{
  auto const& fmgr = board_->fmgr();
  ScoreType attackScore = 0;
  int attackedN = 0;
  BitMask counted_mask{};
  Figure::Color ocolor = Figure::otherColor(color);
  BitMask o_rq_mask = fmgr.rook_mask(ocolor) | fmgr.queen_mask(ocolor);
  BitMask o_brq_mask = fmgr.bishop_mask(ocolor) | o_rq_mask;
  BitMask o_mask = fmgr.knight_mask(ocolor) | o_brq_mask;

  auto pw_attacks = fmgr.pawn_mask(color) & (~finfo_[ocolor].attack_mask_ | finfo_[color].attack_mask_) &
    ~finfo_[color].pinnedFigures_;
  if (color) {
    pw_attacks = ((pw_attacks << 9) & Figure::pawnCutoffMasks_[0]) | ((pw_attacks << 7) & Figure::pawnCutoffMasks_[1]);
  }
  else {
    pw_attacks = ((pw_attacks >> 7) & Figure::pawnCutoffMasks_[0]) | ((pw_attacks >> 9) & Figure::pawnCutoffMasks_[1]);
  }
  counted_mask |= pw_attacks;
  auto pawn_fork = (o_mask & pw_attacks);
  if (pawn_fork) {
    int pawnsN = pop_count(pawn_fork);
    attackedN += pawnsN;
    attackScore += EvalCoefficients::pawnAttack_ * pawnsN;
  }
  if(pawn_fork = (o_mask & ~pw_attacks & finfo_[color].pawnAttacks_)) {
    attackedN++;
    attackScore += EvalCoefficients::pawnAttack_ >> 2;
  }

  if (auto pfwd_attacks = (finfo_[color].pawns_fwd_ &
          (finfo_[color].attack_mask_ | finfo_[color].behindPawnAttacks_ | ~finfo_[ocolor].attack_mask_) & ~finfo_[color].pinnedFigures_)) {
    if (color)
      pfwd_attacks = (((pfwd_attacks << 9) & Figure::pawnCutoffMasks_[0]) | ((pfwd_attacks << 7) & Figure::pawnCutoffMasks_[1])) & 0xffffffffffffff00;
    else
      pfwd_attacks = (((pfwd_attacks >> 7) & Figure::pawnCutoffMasks_[0]) | ((pfwd_attacks >> 9) & Figure::pawnCutoffMasks_[1])) & 0x00ffffffffffffff;
    if (auto pawn_fork = (o_mask & pfwd_attacks)) {
      int pawnsN = pop_count(pawn_fork);
      attackScore += EvalCoefficients::possiblePawnAttack_ * pawnsN;
    }
  }

  if (auto kn_fork = (o_rq_mask & finfo_[color].knightMoves_)) {
    counted_mask |= kn_fork;
    int knightsN = pop_count(kn_fork);
    attackedN += knightsN;
    attackScore += EvalCoefficients::knightAttackRQ_ * knightsN;
  }

  const auto stong_bn_attacks = ~finfo_[ocolor].attack_mask_ | finfo_[color].multiattack_mask_;
  if (auto kn_fork = (fmgr.bishop_mask(ocolor) & finfo_[color].knightMoves_ & ~counted_mask)) {
    counted_mask |= kn_fork;
    int knightsN = pop_count(kn_fork & stong_bn_attacks);
    attackedN += knightsN;
    attackScore += EvalCoefficients::knightAttack_ * knightsN;
    knightsN = pop_count(kn_fork & ~stong_bn_attacks);
    attackedN += knightsN;
    attackScore += EvalCoefficients::knightAttackWeak_ * knightsN;
  }
  if (auto bi_treat = (o_rq_mask & finfo_[color].bishopMoves_ & ~counted_mask)) {
    counted_mask |= bi_treat;
    int bishopsN = pop_count(bi_treat);
    attackedN += bishopsN;
    attackScore += EvalCoefficients::bishopsAttackRQ_ * bishopsN;
  }  
  if (auto bi_treat = (fmgr.knight_mask(ocolor) & finfo_[color].bishopMoves_ & ~counted_mask)) {
    counted_mask |= bi_treat;
    int bishopsN = pop_count(bi_treat & stong_bn_attacks);
    attackedN += bishopsN;
    attackScore += EvalCoefficients::bishopsAttack_ * bishopsN;
    bishopsN = pop_count(bi_treat & ~stong_bn_attacks);
    attackedN += bishopsN;
    attackScore += EvalCoefficients::bishopsAttackWeak_ * bishopsN;
  }

  auto qr_possible_mask = ~finfo_[ocolor].attack_mask_ & ~counted_mask;
  if (auto qr_attack = (fmgr.rook_mask(ocolor) & finfo_[color].qbi_attacked_ & qr_possible_mask)) {
    counted_mask |= qr_attack;
    ++attackedN;
    attackScore += EvalCoefficients::rookQueenAttackedBonus_;
  }

  if (auto r2q_treat = (fmgr.queen_mask(ocolor) & finfo_[color].rookMoves_ & ~counted_mask)) {
    counted_mask |= r2q_treat;
    ++attackedN;
    attackScore += EvalCoefficients::queenUnderRookAttackBonus_;
  }

  if (auto treat_mask = ~counted_mask & ~(finfo_[ocolor].pawnAttacks_ | finfo_[ocolor].multiattack_mask_ | finfo_[ocolor].nb_attacked_) &
        (((finfo_[color].qr_attacked_ | finfo_[color].rookMoves_) & fmgr.bishop_mask(ocolor)) |
         ((finfo_[color].queenMoves_ | finfo_[color].rookMoves_) & fmgr.knight_mask(ocolor)))) {
    counted_mask |= treat_mask;
    int rqtreatsN = pop_count(treat_mask & ~finfo_[ocolor].attack_mask_);
    attackedN += rqtreatsN;
    attackScore += EvalCoefficients::rookQueenAttackedBonus_ * rqtreatsN;
    rqtreatsN = pop_count(treat_mask & finfo_[ocolor].attack_mask_);
    attackScore += (EvalCoefficients::rookQueenAttackedBonus_ * rqtreatsN) >> 4;
  }

  if (auto king_attacks = (~finfo_[ocolor].attack_mask_ & finfo_[color].kingAttacks_ & fmgr.mask(ocolor) & ~fmgr.pawn_mask(ocolor) & ~counted_mask)) {
    counted_mask |= king_attacks;
    auto ktreatsN = pop_count(king_attacks);
    attackedN += ktreatsN;
    attackScore += EvalCoefficients::attackedByKingBonus_ * ktreatsN;
    counted_mask |= king_attacks;
  }

  const bool knight_protects = finfo_[color].knightMoves_ & fmgr.mask(color) & ~finfo_[color].multiattack_mask_ & finfo_[ocolor].attack_mask_;
  auto strong_nattacks = (~finfo_[ocolor].attack_mask_ | finfo_[color].multiattack_mask_) & ~finfo_[ocolor].pawnAttacks_;
  auto possible_kn_att = finfo_[ocolor].attackedByKnightBrq_ & finfo_[color].knightMoves_ &
    strong_nattacks & ~fmgr.mask(color) & ~finfo_[ocolor].nb_attacked_;
  int possibleNN = 0;
  bool with_check = false;
  while (possible_kn_att) {
    int n = clear_lsb(possible_kn_att);
    auto n_moves = movesTable().caps(Figure::TypeKnight, n);
    auto kn_fork = o_brq_mask & n_moves & ~counted_mask;
    if (!kn_fork) {
      continue;
    }
    if (n_moves & fmgr.king_mask(ocolor)) {
      with_check = true;
    }
    int knightsN = pop_count(kn_fork);
    possibleNN = std::max(possibleNN, knightsN);
  }
  possibleNN &= 3;
  attackScore += (EvalCoefficients::possibleKnightAttack_[possibleNN]) >> ((int)knight_protects);
  attackScore += EvalCoefficients::knightAttack_ * with_check;

  if (attackedN > 1) {
    attackScore += EvalCoefficients::multiattackedBonus_ * (attackedN - 1);
  }

  if (auto blocked_mask = (finfo_[ocolor].blockedFigures_ | finfo_[ocolor].pinnedFigures_)) {
    auto attacks_mask = (finfo_[color].attack_mask_ & ~finfo_[ocolor].attack_mask_) |
      finfo_[color].pawnAttacks_ |
      (finfo_[color].multiattack_mask_ & finfo_[color].nb_attacked_);
    auto blocked_attacked = blocked_mask & attacks_mask;
    int blockedN = pop_count(blocked_attacked);
    attackScore += EvalCoefficients::immobileAttackBonus_ * blockedN;
    attackedN += blockedN;
  }

  if (finfo_[color].discoveredMoves_ & finfo_[ocolor].nbrq_mask_) {
    attackScore += EvalCoefficients::discoveredAttackBonus_;
  }

  if (finfo_[ocolor].attackedThrough_) {
    attackScore += EvalCoefficients::attackedThroughBonus_;
  }
  
  return ScoreType32{ attackScore, attackScore };
}

ScoreType32 Evaluator::evaluatePawnsAttacks(Figure::Color color)
{
  auto const& fmgr = board_->fmgr();
  auto const ocolor = Figure::otherColor(color);
  auto const pw_mask = fmgr.pawn_mask(ocolor);
  auto pw_unprotected = pw_mask & ~finfo_[ocolor].pawnAttacks_;
  auto treats = finfo_[color].attack_mask_ & ~finfo_[color].pawnAttacks_;
  auto strong_attacks = ~finfo_[ocolor].attack_mask_ |
    (~finfo_[ocolor].multiattack_mask_ & finfo_[color].multiattack_mask_ & finfo_[color].nb_attacked_);
  auto medium_attacks = ~strong_attacks & (finfo_[color].multiattack_mask_ & finfo_[color].nb_attacked_);
  auto weak_attacks = ~(strong_attacks | medium_attacks);
  ScoreType32 score{};
  score += EvalCoefficients::pawnPressureStrong_ * pop_count(pw_unprotected & treats & strong_attacks);
  score += EvalCoefficients::pawnPressureMedium_ * pop_count(pw_unprotected & treats & medium_attacks);
  score += EvalCoefficients::pawnPressureWeak_ * pop_count(pw_unprotected & treats & weak_attacks);
  
  // bishop treat
  if (fmgr.bishops(color))
  {
    auto bi_mask_w = fmgr.bishop_mask(color) &  FiguresCounter::s_whiteMask_;
    auto bi_mask_b = fmgr.bishop_mask(color) & ~FiguresCounter::s_whiteMask_;
    if (bi_mask_w)
      score += EvalCoefficients::pawnBishopTreat_ * pop_count((pw_unprotected &  FiguresCounter::s_whiteMask_) & ~treats);
    if (bi_mask_b)
      score += EvalCoefficients::pawnBishopTreat_ * pop_count((pw_unprotected & ~FiguresCounter::s_whiteMask_) & ~treats);
  }
  return score;
}


} //NEngine
