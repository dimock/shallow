#pragma once

#include <xcommon.h>

namespace NEngine
{
  template <class MOVE, class LIST>
  void insert_sorted(LIST& moves, MOVE const& move)
  {
    if(moves.empty() || !(moves.back() < move))
      moves.push_back(move);
    else if(!(moves.front() > move))
      moves.push_front(move);
    else
    {
      X_ASSERT(moves.size() < 2, "moves size sould be > 1");
      for(auto it = moves.begin(); it != moves.end(); ++it)
      {
        if(!(*it > move))
        {
          moves.insert(it, move);
          return;
        }
      }
      X_ASSERT(true, "move was not inserted");
    }
  }
}
