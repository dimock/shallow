#pragma once

#include <xcommon.h>
#include <Figure.h>
#include <unordered_map>
#include <boost/optional.hpp>

namespace NEngine
{

struct Board;

class SpecialCasesDetector
{
public:
  using Scase = uint64;

  SpecialCasesDetector();

  boost::optional<ScoreType> eval(Board const& board) const;

private:
  void initUsual();
  void initWinnerLoser();

  std::unordered_map<Scase, std::function<ScoreType(Board const&)>> scases_;
  std::unordered_map<Scase, std::function<ScoreType(Board const&)>> winnerLoser_;
};

}