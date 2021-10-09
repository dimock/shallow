/*************************************************************
  FigureDirs.h - Copyright (C) 2016 by Dmitry Sultanov
 *************************************************************/
#pragma once


#include "Figure.h"

namespace NEngine
{

class FigureDir
{
	int s_dirs_[8*2*4096];
  nst::dirs s_ddirs_[4096];
  nst::bishop_rook_dirs s_br_dirs_[4096];

public:

  // from 'figure' with 'pos' to 'p'
  inline int dir(const Figure::Type type, const Figure::Color color, int pos, int p) const
	{
    X_ASSERT( p < 0 || p > 63, "invalid points to get direction" );
    unsigned u = (p << 6) | pos;
		return *(s_dirs_ + ((type<<13) | (color<<12) | u));
	}

  // get direction 'from' one square 'to' another
  inline nst::dirs dir(int from, int to) const
  {
    X_ASSERT( from < 0 || from > 63 || to < 0 || to > 63, "invaid from or to point to get direction" );
    return *(s_ddirs_ + ((to << 6) | from));
  }

  // used for discovered check detection
  inline nst::bishop_rook_dirs br_dir(int from, int to) const
  {
    X_ASSERT(from < 0 || from > 63 || to < 0 || to > 63, "invaid from or to point to get direction");
    return *(s_br_dirs_ + ((to << 6) | from));
  }

	FigureDir();

private:

	inline int & dir_ref(Figure::Type type, Figure::Color color, int idp)
	{
		X_ASSERT( (unsigned)type > Figure::TypeKing || (unsigned)color > 1, "try to get invalid direction" );
		return *(s_dirs_ + ((type<<13) | (color<<12) | idp));
	}

  inline nst::dirs & dir_ref(int from, int to)
  {
    X_ASSERT( from < 0 || from > 63 || to < 0 || to > 63, "invaid from or to point to get direction" );
    return *(s_ddirs_ + ((to << 6) | from));
  }


	void calcPawnDir(int idp);
	void calcKnightDir(int idp);
	void calcBishopDir(int idp);
	void calcRookDir(int idp);
	void calcQueenDir(int idp);
	void calcKingDir(int idp);

  void calcDDir(int i);
};

} // NEngine
