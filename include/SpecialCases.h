#pragma once

#include <xcommon.h>
#include <Figure.h>
#include <unordered_map>
#include <boost/optional.hpp>

namespace NEngine
{

class Board;

class SpecialCasesDetector
{
public:
  using Scase = uint64;

  SpecialCasesDetector();

  boost::optional<ScoreType> eval(Board const& board) const;

private:
  void initSimple();
  void initComplex();
  void initWinnerLoser();

  std::unordered_map<Scase, ScoreType> scases_;
  std::unordered_map<Scase, std::function<ScoreType(Board const&)>> complexScases_;
  std::unordered_map<Scase, std::function<ScoreType(Board const&)>> winnerLoser_;
};

}