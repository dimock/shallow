#pragma once

#include "xcommon.h"

namespace NEngine
{
  template <class T>
  T get_sign(T t)
  {
    return (T{} < t) - (t < T{});
  }
  
  template <class MOVE, class LIST>
  void insert_sorted(LIST& moves, MOVE const move)
  {
    if(moves.empty() || moves.back() >= move)
    {
      moves.push_back(move);
    }
    else if(moves.front() <= move)
    {
      moves.push_front(move);
    }
    else
    {
      X_ASSERT(moves.size() < 2, "moves size sould be > 1");
      for(auto it = moves.begin(); it != moves.end(); ++it)
      {
        if(*it <= move)
        {
          moves.insert(it, move);
          return;
        }
      }
      X_ASSERT(true, "move was not inserted");
    }
  }

  template <class MOVE>
  void bring_to_front(MOVE* b, MOVE* e, MOVE const move)
  {
    X_ASSERT(b >= e, "invalid moves array");
    auto* i = b;
    for(; i != e; ++i)
    {
      if(*i == move)
        break;
    }
    X_ASSERT(i == e, "no given move found in bring_to_front");
    if(i == b)
      return;
     std::swap(*i, *b);
  }

  template <class MOVE>
  int compact_moves(MOVE* b, MOVE* e)
  {
    X_ASSERT(b >= e, "invalid moves array");
    auto* i = b;
    int count = 0;
    for(; i != e; ++i)
    {
      if(!i->see_ok())
      {
        auto* j = i;
        j++;
        for(; j != e; ++j)
        {
          if(j->see_ok())
          {
            count++;
            std::swap(*i, *j);
            break;
          }
        }
        if(j == e)
          break;
      }
      else
        count++;
    }
    return count;
  }
}