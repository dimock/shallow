#pragma once

#include <xcommon.h>
#include <xbitmath.h>
#include <Figure.h>

namespace NEngine
{

struct History
{
  void clear()
  {
    score_ = 0;
    good_count_ = 0;
    bad_count_ = 0;
  }

  unsigned score() const
  {
    //return score_;//((unsigned long long)score_ * good_count_) / (bad_count_ + good_count_ + 1);//
    return mul_div(score_, good_count_, bad_count_);
  }

  void normalize(int n)
  {
    score_ >>= n;
    good_count_ >>= n;
    bad_count_ >>= n;
  }

  unsigned good() const
  {
    return good_count_;
  }

  unsigned bad() const
  {
    return bad_count_;
  }

  void inc_gb(bool g)
  {
    good_count_ += g;
    bad_count_ += !g;
  }

  void inc_good()
  {
    good_count_++;
  }

  void inc_bad()
  {
    bad_count_++;
  }

  void inc_score(int ds)
  {
    if(ds < 1)
      ds = 1;
    score_ += ds;
  }

protected:

  unsigned score_{};
  unsigned good_count_{};
  unsigned bad_count_{};
};

extern History history_[2][NumOfFields][NumOfFields];

void normalize_history(int n);
void clear_history();
void save_history(std::string const& fname);
void load_history(std::string const& fname);

static inline History & history(Figure::Color c, int from, int to)
{
  X_ASSERT((unsigned)from > 63 || (unsigned)to > 63, "invalid history field index");
  return history_[c][from][to];
}

}