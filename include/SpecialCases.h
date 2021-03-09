#pragma once

#include <xcommon.h>
#include <Figure.h>
#include <Board.h>
#include <unordered_map>

namespace NEngine
{

enum class SpecialCaseResult { SCORE, DRAW, ALMOST_DRAW, PROBABLE_DRAW, MAYBE_DRAW, NO_RESULT };

class SpecialCasesDetector
{
public:
  SpecialCasesDetector();

  inline std::pair<SpecialCaseResult, ScoreType> eval(Board const& board) const
  {
    auto const& fmgr = board.fmgr();
    auto const& hkey = fmgr.fgrsCode();
    auto iter = scases_.find(hkey);
    if (iter == scases_.end())
      return { SpecialCaseResult::NO_RESULT, 0 };
    return (iter->second)(board);
  }

private:
  void initCases();

  std::unordered_map<BitMask, std::function<std::pair<SpecialCaseResult, ScoreType>(Board const&)>> scases_;
};

}