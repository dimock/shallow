/*************************************************************
xlist.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include "xcommon.h"

namespace NEngine
{

template <class T, int N>
struct xallocator
{
  using value_type = T;
  using pointer = T*;
  using const_pointer = T const*;
  using reference = T&;
  using const_reference = T const&;

  size_t next = 0;
  uint8 buffer[sizeof(T)*N];


  template<class U>
  struct rebind
  {
    typedef xallocator<U, N> other;
  };

  pointer allocate(size_t n)
  {
    X_ASSERT(n >= sizeof(value_type)*N, "too match memory allocated");
    auto b = buffer+next;
    next += n*sizeof(value_type);
    return reinterpret_cast<pointer>(b);
  }

  void construct(pointer p, const_reference val)
  {
    ::new(p)value_type(val);
  }

  template< class U, class... Args >
  void construct(U* p, Args&&... args)
  {
    ::new(p)U(std::forward<Args>(args)...);
  }

  void deallocate(T*, size_t)
  {}

  void destroy(pointer p)
  {
    p->~T();
  }

  template <class U>
  void destroy(U* p)
  {
    p->~U();
  }
};

} // NEngine
