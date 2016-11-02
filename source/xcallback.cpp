/*************************************************************
xcallback.cpp - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include <xcallback.h>

namespace NEngine
{

SearchResult::SearchResult()
{
  best_.clear();
  for(auto& m : pv_)
    m.clear();
}

void SearchData::reset()
{
  depth_ = 0;
  nodesCount_ = 0;
  totalNodes_ = 0;
  tprev_ = tstart_ = clock();
  numOfMoves_ = 0;
  best_.clear();
  counter_ = 0;
  plyMax_ = 0;
}

void SearchData::restart()
{
  best_.clear();
  nodesCount_ = 0;
  plyMax_ = 0;
  counter_ = 0;
}

} // NEngine
