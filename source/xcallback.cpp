/*************************************************************
xcallback.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include "xcallback.h"

namespace NEngine
{

SearchResult::SearchResult()
{
  best_ = Move{ true };
  for(auto& m : pv_)
  {
    m = Move{ true };
  }
}

SearchResult& SearchResult::operator = (SearchResult const& other)
{
  best_ = other.best_;
  dt_ = other.dt_;
  nodesCount_ = other.nodesCount_;
  totalNodes_ = other.totalNodes_;
  depthMax_ = other.depthMax_;
  numOfMoves_ = other.numOfMoves_;
  counter_ = other.counter_;
  board_ = other.board_;
  depth_ = other.depth_;
  score_ = other.score_;
  for (size_t i = 0; i < pv_.size(); ++i)
    pv_[i] = other.pv_[i];
  return *this;
}

void SearchResult::reset()
{
  for (size_t i = 0; i < pv_.size(); ++i)
    pv_[i] = Move{ true };

  best_ = Move{ true };
  prevBest_ = Move{ true };
  dt_ = NTime::duration{};
  nodesCount_ = 0;
  totalNodes_ = 0;
  depthMax_ = 0;
  numOfMoves_ = 0;
  counter_ = 0;
  board_ = Board{};
  depth_ = 0;
  score_ = -Figure::MatScore;
}
void SearchData::reset()
{
  depth_ = 0;
  nodesCount_ = 0;
  totalNodes_ = 0;
  tprev_ = tstart_ = NTime::now();
  numOfMoves_ = 0;
  best_ = SMove{ true };
  scoreBest_ = -Figure::MatScore;
  counter_ = 0;
  plyMax_ = 0;
}

void SearchData::restart()
{
  best_ = SMove{ true };
  scoreBest_ = -Figure::MatScore;
  nodesCount_ = 0;
  plyMax_ = 0;
  counter_ = 0;
}

} // NEngine
