// This file is mostly stolen from llvm/ADT/ArrayRef.hpp
// All "unnecessary" things are deleted (including some comments)
// Some fixes are mode to make this work with C++11 (std::enable_if_t is not
// present in C++11)

//===- ArrayRef.h - Array Reference Wrapper ---------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <vector>

namespace portal {
namespace llvm {

template <typename T> class ArrayRef {
public:
  using value_type = T;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using iterator = const_pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

private:
  const T *Data = nullptr;
  size_type Length = 0;

public:
  ArrayRef() = default;
  ArrayRef(const T &OneElt) : Data(&OneElt), Length(1) {}
  ArrayRef(const T *data, size_t length) : Data(data), Length(length) {}
  ArrayRef(const T *begin, const T *end) : Data(begin), Length(end - begin) {}

  template <typename A>
  ArrayRef(const std::vector<T, A> &Vec)
      : Data(Vec.data()), Length(Vec.size()) {}

  template <size_t N>
  constexpr ArrayRef(const std::array<T, N> &Arr)
      : Data(Arr.data()), Length(N) {}

  template <size_t N>
  constexpr ArrayRef(const T (&Arr)[N]) : Data(Arr), Length(N) {}

  /// Construct an ArrayRef from a std::initializer_list.
  ArrayRef(const std::initializer_list<T> &Vec)
      : Data(Vec.begin() == Vec.end() ? (T *)nullptr : Vec.begin()),
        Length(Vec.size()) {}

  template <typename U>
  ArrayRef(
      const ArrayRef<U *> &A,
      typename std::enable_if<
          std::is_convertible<U *const *, T const *>::value>::type * = nullptr)
      : Data(A.data()), Length(A.size()) {}

  /// Construct an ArrayRef<const T*> from std::vector<T*>. This uses SFINAE
  /// to ensure that only vectors of pointers can be converted.
  template <typename U, typename A>
  ArrayRef(
      const std::vector<U *, A> &Vec,
      typename std::enable_if<
          std::is_convertible<U *const *, T const *>::value>::type * = nullptr)
      : Data(Vec.data()), Length(Vec.size()) {}

  iterator begin() const { return Data; }
  iterator end() const { return Data + Length; }

  reverse_iterator rbegin() const { return reverse_iterator(end()); }
  reverse_iterator rend() const { return reverse_iterator(begin()); }

  bool empty() const { return Length == 0; }

  const T *data() const { return Data; }
  size_t size() const { return Length; }

  const T &front() const {
    assert(!empty());
    return Data[0];
  }

  const T &back() const {
    assert(!empty());
    return Data[Length - 1];
  }

  bool equals(ArrayRef RHS) const {
    if (Length != RHS.Length)
      return false;
    return std::equal(begin(), end(), RHS.begin());
  }

  /// slice(n, m) - Chop off the first N elements of the array, and keep M
  /// elements in the array.
  ArrayRef<T> slice(size_t N, size_t M) const {
    assert(N + M <= size() && "Invalid specifier");
    return ArrayRef<T>(data() + N, M);
  }

  /// slice(n) - Chop off the first N elements of the array.
  ArrayRef<T> slice(size_t N) const { return slice(N, size() - N); }

  /// Drop the first \p N elements of the array.
  ArrayRef<T> drop_front(size_t N = 1) const {
    assert(size() >= N && "Dropping more elements than exist");
    return slice(N, size() - N);
  }

  /// Drop the last \p N elements of the array.
  ArrayRef<T> drop_back(size_t N = 1) const {
    assert(size() >= N && "Dropping more elements than exist");
    return slice(0, size() - N);
  }

  /// Return a copy of *this with the first N elements satisfying the
  /// given predicate removed.
  template <class PredicateT> ArrayRef<T> drop_while(PredicateT Pred) const {
    return ArrayRef<T>(find_if_not(*this, Pred), end());
  }

  /// Return a copy of *this with the first N elements not satisfying
  /// the given predicate removed.
  template <class PredicateT> ArrayRef<T> drop_until(PredicateT Pred) const {
    return ArrayRef<T>(find_if(*this, Pred), end());
  }

  /// Return a copy of *this with only the first \p N elements.
  ArrayRef<T> take_front(size_t N = 1) const {
    if (N >= size())
      return *this;
    return drop_back(size() - N);
  }

  /// Return a copy of *this with only the last \p N elements.
  ArrayRef<T> take_back(size_t N = 1) const {
    if (N >= size())
      return *this;
    return drop_front(size() - N);
  }

  /// Return the first N elements of this Array that satisfy the given
  /// predicate.
  template <class PredicateT> ArrayRef<T> take_while(PredicateT Pred) const {
    return ArrayRef<T>(begin(), find_if_not(*this, Pred));
  }

  /// Return the first N elements of this Array that don't satisfy the
  /// given predicate.
  template <class PredicateT> ArrayRef<T> take_until(PredicateT Pred) const {
    return ArrayRef<T>(begin(), find_if(*this, Pred));
  }

  const T &operator[](size_t Index) const {
    assert(Index < Length && "Invalid index!");
    return Data[Index];
  }

  /// Disallow accidental assignment from a temporary.
  ///
  /// The declaration here is extra complicated so that "arrayRef = {}"
  /// continues to select the move assignment operator.
  template <typename U>
  typename std::enable_if<std::is_same<U, T>::value, ArrayRef<T>>::type &
  operator=(U &&Temporary) = delete;

  /// Disallow accidental assignment from a temporary.
  ///
  /// The declaration here is extra complicated so that "arrayRef = {}"
  /// continues to select the move assignment operator.
  template <typename U>
  typename std::enable_if<std::is_same<U, T>::value, ArrayRef<T>>::type &
  operator=(std::initializer_list<U>) = delete;

  std::vector<T> vec() const { return std::vector<T>(Data, Data + Length); }

  operator std::vector<T>() const {
    return std::vector<T>(Data, Data + Length);
  }
};

template <class T> class MutableArrayRef : public ArrayRef<T> {
public:
  using value_type = T;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using iterator = pointer;
  using const_iterator = const_pointer;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using size_type = size_t;
  using difference_type = ptrdiff_t;

  MutableArrayRef() = default;
  MutableArrayRef(T &OneElt) : ArrayRef<T>(OneElt) {}
  MutableArrayRef(T *data, size_t length) : ArrayRef<T>(data, length) {}

  MutableArrayRef(T *begin, T *end) : ArrayRef<T>(begin, end) {}
  MutableArrayRef(std::vector<T> &Vec) : ArrayRef<T>(Vec) {}

  template <size_t N>
  constexpr MutableArrayRef(std::array<T, N> &Arr) : ArrayRef<T>(Arr) {}

  template <size_t N>
  constexpr MutableArrayRef(T (&Arr)[N]) : ArrayRef<T>(Arr) {}

  T *data() const { return const_cast<T *>(ArrayRef<T>::data()); }

  iterator begin() const { return data(); }
  iterator end() const { return data() + this->size(); }

  reverse_iterator rbegin() const { return reverse_iterator(end()); }
  reverse_iterator rend() const { return reverse_iterator(begin()); }

  T &front() const {
    assert(!this->empty());
    return data()[0];
  }

  T &back() const {
    assert(!this->empty());
    return data()[this->size() - 1];
  }

  /// slice(n, m) - Chop off the first N elements of the array, and keep M
  /// elements in the array.
  MutableArrayRef<T> slice(size_t N, size_t M) const {
    assert(N + M <= this->size() && "Invalid specifier");
    return MutableArrayRef<T>(this->data() + N, M);
  }

  /// slice(n) - Chop off the first N elements of the array.
  MutableArrayRef<T> slice(size_t N) const {
    return slice(N, this->size() - N);
  }

  /// Drop the first \p N elements of the array.
  MutableArrayRef<T> drop_front(size_t N = 1) const {
    assert(this->size() >= N && "Dropping more elements than exist");
    return slice(N, this->size() - N);
  }

  MutableArrayRef<T> drop_back(size_t N = 1) const {
    assert(this->size() >= N && "Dropping more elements than exist");
    return slice(0, this->size() - N);
  }

  /// Return a copy of *this with the first N elements satisfying the
  /// given predicate removed.
  template <class PredicateT>
  MutableArrayRef<T> drop_while(PredicateT Pred) const {
    return MutableArrayRef<T>(find_if_not(*this, Pred), end());
  }

  /// Return a copy of *this with the first N elements not satisfying
  /// the given predicate removed.
  template <class PredicateT>
  MutableArrayRef<T> drop_until(PredicateT Pred) const {
    return MutableArrayRef<T>(find_if(*this, Pred), end());
  }

  /// Return a copy of *this with only the first \p N elements.
  MutableArrayRef<T> take_front(size_t N = 1) const {
    if (N >= this->size())
      return *this;
    return drop_back(this->size() - N);
  }

  /// Return a copy of *this with only the last \p N elements.
  MutableArrayRef<T> take_back(size_t N = 1) const {
    if (N >= this->size())
      return *this;
    return drop_front(this->size() - N);
  }

  /// Return the first N elements of this Array that satisfy the given
  /// predicate.
  template <class PredicateT>
  MutableArrayRef<T> take_while(PredicateT Pred) const {
    return MutableArrayRef<T>(begin(), find_if_not(*this, Pred));
  }

  /// Return the first N elements of this Array that don't satisfy the
  /// given predicate.
  template <class PredicateT>
  MutableArrayRef<T> take_until(PredicateT Pred) const {
    return MutableArrayRef<T>(begin(), find_if(*this, Pred));
  }

  T &operator[](size_t Index) const {
    assert(Index < this->size() && "Invalid index!");
    return data()[Index];
  }
};

template <typename T> ArrayRef<T> makeArrayRef(const T &OneElt) {
  return OneElt;
}

template <typename T> ArrayRef<T> makeArrayRef(const T *data, size_t length) {
  return ArrayRef<T>(data, length);
}

template <typename T> ArrayRef<T> makeArrayRef(const T *begin, const T *end) {
  return ArrayRef<T>(begin, end);
}

template <typename T> ArrayRef<T> makeArrayRef(const std::vector<T> &Vec) {
  return Vec;
}

template <typename T, std::size_t N>
ArrayRef<T> makeArrayRef(const std::array<T, N> &Arr) {
  return Arr;
}

template <typename T> ArrayRef<T> makeArrayRef(const ArrayRef<T> &Vec) {
  return Vec;
}

template <typename T> ArrayRef<T> &makeArrayRef(ArrayRef<T> &Vec) {
  return Vec;
}

template <typename T, size_t N> ArrayRef<T> makeArrayRef(const T (&Arr)[N]) {
  return ArrayRef<T>(Arr);
}

template <typename T> inline bool operator==(ArrayRef<T> LHS, ArrayRef<T> RHS) {
  return LHS.equals(RHS);
}

template <typename T> inline bool operator!=(ArrayRef<T> LHS, ArrayRef<T> RHS) {
  return !(LHS == RHS);
}

template <typename T> MutableArrayRef<T> makeMutableArrayRef(T &OneElt) {
  return OneElt;
}

template <typename T>
MutableArrayRef<T> makeMutableArrayRef(T *data, size_t length) {
  return MutableArrayRef<T>(data, length);
}

} // namespace llvm
} // namespace portal