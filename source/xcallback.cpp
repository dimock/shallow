/*************************************************************
xcallback.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include <xcallback.h>

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
  for (size_t i = 0; i < pv_.size(); ++i)
    pv_[i] = other.pv_[i];
  return *this;
}

void SearchData::reset()
{
  depth_ = 0;
  nodesCount_ = 0;
  totalNodes_ = 0;
  tprev_ = tstart_ = NTime::now();
  numOfMoves_ = 0;
  best_ = SMove{ true };
  counter_ = 0;
  plyMax_ = 0;
}

void SearchData::restart()
{
  best_ = SMove{ true };
  nodesCount_ = 0;
  plyMax_ = 0;
  counter_ = 0;
}

} // NEngine
