#pragma once

#include <xcommon.h>
#include <Figure.h>
#include <Board.h>
#include <unordered_map>

namespace NEngine
{

class SpecialCasesDetector
{
public:
  SpecialCasesDetector();

  inline std::pair<bool, ScoreType> eval(Board const& board) const
  {
    auto const& fmgr = board.fmgr();
    auto const& hkey = fmgr.fgrsCode();
    auto iter = scases_.find(hkey);
    if (iter == scases_.end())
      return { false, 0 };
    return (iter->second)(board);
  }

private:
  void initUsual();
  void initWinnerLoser();
  void initMatCases();

  std::unordered_map<BitMask, std::function<std::pair<bool, ScoreType>(Board const&)>> scases_;
};

}