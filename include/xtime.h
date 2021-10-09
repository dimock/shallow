/*************************************************************
xcallback.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include "chrono"
#include "ratio"

namespace NTime
{

  using point    = std::chrono::high_resolution_clock::time_point;
  using duration = std::chrono::duration<double>;
  
  inline point now()
  {
    return std::chrono::high_resolution_clock::now();
  }

  template <class T>
  T centi_seconds(duration const& dt)
  {
    return std::chrono::duration_cast<std::chrono::duration<T, std::centi>>(dt).count();
  }

  template <class T>
  T milli_seconds(duration const& dt)
  {
    return std::chrono::duration_cast<std::chrono::duration<T, std::milli>>(dt).count();
  }

  template <class T>
  T seconds(duration const& dt)
  {
    return std::chrono::duration_cast<std::chrono::duration<T>>(dt).count();
  }

  template <class T>
  duration from_milliseconds(T const& t)
  {
    return std::chrono::duration_cast<duration>(std::chrono::milliseconds(t));
  }

  template <class T>
  duration from_centiseconds(T const& t)
  {
    return std::chrono::duration_cast<duration>(std::chrono::duration<T, std::centi>(t));
  }

} // NTime
