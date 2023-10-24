// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef HELPERS_STRINGITERATOR_HPP_
#define HELPERS_STRINGITERATOR_HPP_

#include <cstddef>

#include "fct/fct.hpp"
#include "strings/strings.hpp"

namespace helpers {

class StringIterator {
 public:
  StringIterator(const std::function<strings::String(size_t)> _func,
                 const size_t _size)
      : func_(_func), size_(_size) {}

  ~StringIterator() = default;

 public:
  /// Accessor with boundary checks.
  strings::String at(size_t _i) const {
    throw_unless(_i < size_, "Index out of bounds. _i: " + std::to_string(_i) +
                                 ", size_: " + std::to_string(size_));
    return func_(_i);
  }

  /// Iterator to the beginning.
  auto begin() const {
    auto iota = fct::iota<size_t>(0, size_);
    auto range = iota | VIEWS::transform(func_);
    return range.begin();
  }

  /// Iterator to the end.
  auto end() const { return begin() + size_; }

  /// Accessor without boundary checks.
  strings::String operator[](size_t _i) const {
    assert_true(_i < size_);
    return func_(_i);
  }

  /// The size of the range
  size_t size() const { return size_; }

 private:
  /// A function mapping an index to string.
  const std::function<strings::String(size_t)> func_;

  /// The total number of strings available.
  const size_t size_;
};

}  // namespace helpers

#endif  // HELPERS_STRINGITERATOR_HPP_
