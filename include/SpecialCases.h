#pragma once

#include <xcommon.h>
#include <Figure.h>
#include <unordered_map>

namespace NEngine
{

struct Board;

class SpecialCasesDetector
{
public:
  using Scase = uint64;

  SpecialCasesDetector();

  std::pair<bool, ScoreType> eval(Board const& board) const;

private:
  void initUsual();
  void initWinnerLoser();
  void initMatCases();

  std::pair<bool, ScoreType> evalBishopAndPawns(Board const& board) const;

  std::unordered_map<Scase, std::function<std::pair<bool, ScoreType>(Board const&)>> scases_;
  std::unordered_map<Scase, std::function<ScoreType(Board const&)>> winnerLoser_;
  std::unordered_map<Scase, std::function<ScoreType(Board const&)>> matCases_;
};

}