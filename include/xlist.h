/*************************************************************
xlist.h - Copyright (C) 2016 by Dmitry Sultanov
*************************************************************/
#pragma once

#include <xcommon.h>

namespace NEngine
{

template <class T, int N>
class xlist
{
public:
  using size_type = short;

private:
  struct item
  {
    T x;
    size_type next;
  };

  item items[N];
  size_type first{ -1 };
  size_type last{ -1 };
  size_type heap{ 0 };

public:
  using value_type = T;

  class iterator
  {
    xlist<T, N>* ptr;
    size_type index;
    size_type prev;
    friend class xlist<T, N>;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using difference_type = size_type;

    iterator() = default;
    iterator(iterator const&) = default;

    iterator(xlist<T, N>* p, size_type i, size_type prv) :
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
    size_type index;
    size_type prev;
    friend class xlist<T, N>;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using reference = T&;
    using pointer = T*;
    using difference_type = size_type;

    const_iterator() = default;
    const_iterator(const_iterator const&) = default;

    const_iterator(xlist<T, N> const* p, size_type i, size_type prv) :
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
    else
      first = heap;
    items[heap].x = t;
    items[heap].next = -1;
    last = heap;
    heap++;
  }

  void push_back(T&& t)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    if(last >= 0)
      items[last].next = heap;
    else
      first = heap;
    items[heap].x = std::move(t);
    items[heap].next = -1;
    last = heap;
    heap++;
  }

  template <class ...A>
  void emplace_back(A&& ...a)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    if(last >= 0)
      items[last].next = heap;
    else
      first = heap;
    new(&items[heap].x) T{ std::forward<A>(a)... };
    items[heap].next = -1;
    last = heap;
    heap++;
  }

  void push_front(T const& t)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    items[heap] = { t, first };
    first = heap;
    if(last < 0)
      last = heap;
    heap++;
  }

  void push_front(T&& t)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    items[heap].x = std::move(t);
    items[heap].next = first;
    first = heap;
    if(last < 0)
      last = heap;
    heap++;
  }

  template <class ...A>
  void emplace_front(A&& ...a)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    new(&items[heap].x) T{ std::forward<A>(a)... };
    items[heap].next = first;
    first = heap;
    if(last < 0)
      last = heap;
    heap++;
  }

  // inserts before iterator
  void insert(iterator iter, T const& t)
  {
    X_ASSERT(heap >= N, "invalid xlist heap index");
    items[heap] = { t, iter.index };
    if(iter.prev >= 0)
      items[iter.prev].next = heap;
    // iter == begin()
    else
      first = heap;
    // empty() or push_back()
    if(last < 0 || iter.index < 0)
      last = heap;
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

  iterator begin()
  {
    return iterator{ this, first, -1 };
  }

  const_iterator begin() const
  {
    return const_iterator{ this, first, -1 };
  }

  iterator end()
  {
    return iterator{ this, -1, last };
  }

  const_iterator end() const
  {
    return const_iterator{ this, -1, last };
  }

  bool empty() const
  {
    X_ASSERT(first >= 0 && last < 0, "invalid xlist last index");
    X_ASSERT(heap < 0, "invalid xlist size");
    X_ASSERT(heap == 0 && first >= 0, "xlist size params mismatch");
    return first < 0;
  }

  size_type size() const
  {
    X_ASSERT(heap < 0, "invalid xlist size");
    return heap;
  }

  void clear()
  {
    first = -1;
    last = -1;
    heap = 0;
  }
};

} // NEngine
