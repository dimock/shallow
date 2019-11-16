/*************************************************************
Helpers.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/

#pragma once

#undef _USE_LOG

#include <Board.h>

namespace NEngine
{
struct Move;
struct Board;

enum class eMoveNotation
{
  mnUnknown,
  mnSmith,
  mnSAN
};

eMoveNotation detectNotation(std::string const& str);

/// e2e4 - Smith notation
std::string moveToStr(Move const& move, bool wbf);
Move strToMove(std::string const& str, Board const& board);
Move strToMove(std::string const& str);

/// Rxf5+ - Standard algebraic notation
Move parseSAN(Board const& board, std::string const& str);
std::string printSAN(Board & board, const Move & move);

bool load(Board &, std::istream &);
bool save(const Board &, std::ostream &, bool = true);

std::string join(std::vector<std::string> const& vec, std::string const& delim);
std::string join(std::vector<std::string>::const_iterator from, std::vector<std::string>::const_iterator to, std::string const& delim);
void trim(std::string& str);
void to_lower(std::string& str);
bool is_any_of(std::string const& of, char c);
std::vector<std::string> split(std::string const& str, std::function<bool(char)> const& pred);

/// initialize from FEN
bool fromFEN(std::string const& i_fen, Board& board);

/// save current position to FEN
std::string toFEN(Board const& board);

// check if there is nothing between 'from' and 'to'
// inv_mask - inverted mask of all interesting figures
inline bool is_nothing_between(int from, int to, const BitMask & inv_mask)
{
  const BitMask & btw_msk = betweenMasks().between(from, to);
  return (btw_msk & inv_mask) == btw_msk;
}

struct XCounter
{
  static int count_;
  int var0_;
  int* var_;
  XCounter(int* var)
  {
    var_ = var;
    if(var_)
      var0_ = *var_;
  }
  ~XCounter()
  {
    if(var_)
      count_ += *var_ - var0_;
  }
};

#ifdef _USE_LOG
void addLog(std::string const& str);
#else
inline void addLog(std::string const&) {}
#endif

} // NEngine
