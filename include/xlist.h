/*************************************************************
xlist.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#include <xcommon.h>

namespace NEngine
{

template <class T, int N>
class xlist
{
  struct item
  {
    T x;
    int next;
  };

  item items[N];
  int first{ -1 };
  int last{ -1 };
  int heap{ 0 };

public:
  using value_type = T;

  class iterator
  {
    xlist<T, N>* ptr;
    int index;
    int prev;
    friend class xlist<T, N>;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using difference_type = int;

    iterator() = default;
    iterator(iterator const&) = default;

    iterator(xlist<T, N>* p, int i, int prv) :
      ptr(p), index(i), prev(prv)
    {}

    iterator operator ++ (int)
    {
      X_ASSERT(index < 0, "invalid xlist iterator index");
      auto iter(*this);
      iter.prev = index;
      iter.index = ptr->items[index].next;
      return iter;
    }

    iterator& operator ++ ()
    {
      X_ASSERT(index < 0, "invalid xlist iterator index");
      prev = index;
      index = ptr->items[index].next;
      return *this;
    }

    bool operator == (iterator const& other) const
    {
      return index == other.index;
    }

    bool operator != (iterator const& other) const
    {
      return index != other.index;
    }

    T& operator * ()
    {
      X_ASSERT(index < 0, "invalid xlist iterator index");
      return ptr->items[index].x;
    }

    T const& operator * () const
    {
      X_ASSERT(index < 0, "invalid xlist iterator index");
      return ptr->items[index].x;
    }

    T * operator -> ()
    {
      X_ASSERT(index < 0, "invalid xlist const_iterator index");
      return &ptr->items[index].x;
    }

    T const* operator -> () const
    {
      X_ASSERT(index < 0, "invalid xlist const_iterator index");
      return &ptr->items[index].x;
    }
  };

  class const_iterator
  {
    xlist<T, N> const* ptr;
    int index;
    int prev;
    friend class xlist<T, N>;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using difference_type = int;

    const_iterator() = default;
    const_iterator(const_iterator const&) = default;

    const_iterator(xlist<T, N> const* p, int i, int prv) :
      ptr(p), index(i), prev(prv)
    {}

    const_iterator operator ++ (int)
    {
      X_ASSERT(index < 0, "invalid xlist iterator index");
      auto iter(*this);
      iter.prev = index;
      iter.index = ptr->items[index].next;
      return iter;
    }

    const_iterator& operator ++ ()
    {
      X_ASSERT(index < 0, "invalid xlist iterator index");
      prev = index;
      index = ptr->items[index].next;
      return *this;
    }

    bool operator == (const_iterator const& other) const
    {
      return index == other.index;
    }

    bool operator != (const_iterator const& other) const
    {
      return index != other.index;
    }

    T const& operator * () const
    {
      X_ASSERT(index < 0, "invalid xlist const_iterator index");
      return ptr->items[index].x;
    }

    T const* operator -> () const
    {
      X_ASSERT(index < 0, "invalid xlist const_iterator index");
      return &ptr->items[index].x;
    }
  };

  void push_back(T const& t)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    if(last >= 0)
      items[last].next = heap;
    items[heap] = { t, -1 };
    last = heap;
    if(first < 0)
      first = heap;
    heap++;
  }

  void push_back(T&& t)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    if(last >= 0)
      items[last].next = heap;
    items[heap].x = std::move(t);
    items[heap].next = -1;
    last = heap;
    if(first < 0)
      first = heap;
    heap++;
  }

  template <class ...A>
  void emplace_back(A ...a)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    if(last >= 0)
      items[last].next = heap;
    new(&items[heap].x) T{ a... };
    items[heap].next = -1;
    last = heap;
    if(first < 0)
      first = heap;
    heap++;
  }

  T& back()
  {
    X_ASSERT(last < 0 || last >= N, "invalid last xlist index");
    return items[last].x;
  }

  T const& back() const
  {
    X_ASSERT(last < 0 || last >= N, "invalid last xlist index");
    return items[last].x;
  }

  T& front()
  {
    X_ASSERT(first < 0 || first >= N, "invalid first xlist index");
    return items[first].x;
  }

  T const& front() const
  {
    X_ASSERT(first < 0 || first >= N, "invalid first xlist index");
    return items[first].x;
  }

  iterator erase(iterator iter)
  {
    X_ASSERT(iter.index < 0, "invalid xlist erase index");
    auto const prev = iter.prev;
    auto const next = items[iter.index].next;
    if(prev < 0)
    {
      X_ASSERT(iter.index != first, "inconsistent xlist. invalid first index");
      first = next;
    }
    else
    {
      items[prev].next = next;
    }
    if(next < 0)
    {
      X_ASSERT(iter.index != last, "inconsistent xlist. invalid last index");
      last = prev;
    }
    return iterator{this, next, prev};
  }

  iterator begin()
  {
    return iterator{ this, first, -1 };
  }

  const_iterator begin() const
  {
    return iterator{ this, first, -1 };
  }

  iterator end()
  {
    return iterator{ this, -1, last };
  }

  const_iterator end() const
  {
    return iterator{ this, -1, last };
  }

  bool empty() const
  {
    X_ASSERT(first >= 0 && last < 0, "invalid xlist last index");
    return first < 0;
  }

  bool single() const
  {
    return first >= 0 && first == last;
  }
};

} // NEngine