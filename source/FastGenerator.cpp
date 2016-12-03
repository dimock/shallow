/*************************************************************
FastGenerator.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#include <MovesGenerator.h>
#include <MovesTable.h>

namespace NEngine
{

//////////////////////////////////////////////////////////////////////////
FastGenerator::FastGenerator(Board & board) :
  cg_(board), ug_(board), eg_(board), hmove_(0), killer_(0), order_(oHash),
  board_(board)
{
  if(board_.underCheck())
  {
    order_ = oEscapes;
    eg_.generate(hmove_);
  }
}

FastGenerator::FastGenerator(Board & board, const Move & hmove, const Move & killer) :
  cg_(board), ug_(board), eg_(board), hmove_(hmove), killer_(killer), order_(oHash),
  board_(board)
{
  if(board_.underCheck())
  {
    order_ = oEscapes;
    eg_.generate(hmove);
  }
}

Move* FastGenerator::move()
{
  if(order_ == oHash)
  {
    order_ = oGenCaps;
    if(hmove_)
    {
      hmove_.see_good_ = hmove_.capture_ || hmove_.new_type_ > 0;
      return &hmove_;
    }
  }

  if(order_ == oEscapes)
  {
    return eg_.move();
  }

  if(order_ == oGenCaps)
  {
    cg_.generate(hmove_, Figure::TypePawn);
    order_ = oCaps;
  }

  if(order_ == oCaps)
  {
    for(;;)
    {
      auto* move = cg_.next_move();
      if(!move)
        break;
      if(!move->see_good_)
      {
        weaks_.push_back(*move);
        continue;
      }
      return move;
    }
    order_ = oKiller;
  }

  if(order_ == oKiller)
  {
    order_ = oGenUsual;
    if(killer_)
      return &killer_;
  }

  if(order_ == oGenUsual)
  {
    ug_.generate(hmove_, killer_);
    order_ = oUsual;
  }

  if(order_ == oUsual)
  {
    if(auto* move = ug_.move())
    {
      return move;
    }
    order_ = oWeak;
  }

  if(order_ == oWeak)
  {
    for(;;)
    {
      auto iter = std::max_element(weaks_.begin(), weaks_.end(),
        [](Move const& m1, Move const& m2) { return m1 < m2; });
      if(iter == weaks_.end())
        return nullptr;
      auto* move = &*iter;
      weaks_.erase(iter);
      return move;
    }
  }
  return nullptr;
}


//////////////////////////////////////////////////////////////////////////
TacticalGenerator::TacticalGenerator(Board & board, Figure::Type thresholdType, int depth) :
  eg_(board), cg_(board), chg_(board), thresholdType_(thresholdType), board_(board),
  order_(oGenCaps), depth_(depth)
{

  if(board_.underCheck())
  {
    Move hmove(0);
    eg_.generate(hmove);
    order_ = oEscape;
  }
}

Move* TacticalGenerator::move()
{
  if(order_ == oEscape)
  {
    return eg_.move();
  }

  if(order_ == oGenCaps)
  {
    Move hmove(0);
    cg_.generate(hmove, thresholdType_);
    order_ = oCaps;
  }

  if(order_ == oCaps)
  {
    auto* cap = cg_.move();
    if(cap || depth_ < 0)
      return cap;
    order_ = oGenChecks;
  }

  if(order_ == oGenChecks)
  {
    chg_.generate();
    order_ = oChecks;
  }

  if(order_ == oChecks)
  {
    return chg_.move();
  }

  return nullptr;
}

} // NEngine
