#include <History.h>
#include <fstream>

namespace NEngine
{

History history_[2][NumOfFields][NumOfFields] = {};

void clear_history()
{
  for(int c = 0; c < 2; ++c)
    for(int i = 0; i < NumOfFields; ++i)
      for(int j = 0; j < NumOfFields; ++j)
        history_[c][i][j].clear();
}

void max_history()
{
  SortValueType score_max{ 0 };
  for(int c = 0; c < 2; ++c)
  {
    for(int i = 0; i < NumOfFields; ++i)
    {
      for(int j = 0; j < NumOfFields; ++j)
      {
        if(history_[c][i][j].score() > score_max)
          score_max = history_[c][i][j].score();
      }
    }
  }
  std::ofstream ofs("hist_score.txt", std::ios::app);
  ofs << score_max << std::endl;
}

void normalize_history(int n)
{
  for(int c = 0; c < 2; ++c)
  {
    for(int i = 0; i < NumOfFields; ++i)
    {
      for(int j = 0; j < NumOfFields; ++j)
      {
        History & hist = history_[c][i][j];
        hist.normalize(n);
      }
    }
  }
}

void save_history(std::string const& fname)
{
  std::ofstream ofs(fname, std::ofstream::binary);
  if(!ofs)
    return;
  ofs.write(reinterpret_cast<char*>(history_), sizeof(history_));
}

void load_history(std::string const& fname)
{
  std::ifstream ifs(fname, std::ifstream::binary);
  if(!ifs)
    return;
  ifs.read(reinterpret_cast<char*>(history_), sizeof(history_));
}

}
