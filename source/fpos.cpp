/*************************************************************
  fpos.cpp - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/

#include <fpos.h>

namespace NEngine
{

FPos FPosIndexer::s_fromIndex_[64] =
{
  FPos(0, 0), FPos(1, 0), FPos(2, 0), FPos(3, 0), FPos(4, 0), FPos(5, 0), FPos(6, 0), FPos(7, 0),
  FPos(0, 1), FPos(1, 1), FPos(2, 1), FPos(3, 1), FPos(4, 1), FPos(5, 1), FPos(6, 1), FPos(7, 1),
  FPos(0, 2), FPos(1, 2), FPos(2, 2), FPos(3, 2), FPos(4, 2), FPos(5, 2), FPos(6, 2), FPos(7, 2),
  FPos(0, 3), FPos(1, 3), FPos(2, 3), FPos(3, 3), FPos(4, 3), FPos(5, 3), FPos(6, 3), FPos(7, 3),
  FPos(0, 4), FPos(1, 4), FPos(2, 4), FPos(3, 4), FPos(4, 4), FPos(5, 4), FPos(6, 4), FPos(7, 4),
  FPos(0, 5), FPos(1, 5), FPos(2, 5), FPos(3, 5), FPos(4, 5), FPos(5, 5), FPos(6, 5), FPos(7, 5),
  FPos(0, 6), FPos(1, 6), FPos(2, 6), FPos(3, 6), FPos(4, 6), FPos(5, 6), FPos(6, 6), FPos(7, 6),
  FPos(0, 7), FPos(1, 7), FPos(2, 7), FPos(3, 7), FPos(4, 7), FPos(5, 7), FPos(6, 7), FPos(7, 7)
};

} // NEngine
