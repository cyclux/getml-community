// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef HELPERS_INTSET_HPP_
#define HELPERS_INTSET_HPP_

#include <cstddef>
#include <utility>
#include <vector>

#include "debug/assert_true.hpp"

namespace helpers {

/// We have a core advantage in that we can know the maximum possible value of
/// in advance. This enables us to implement this set, which enables a drastic
/// speed-up when compared to std::unordered_set (let alone std::set).
class IntSet {
 public:
  explicit IntSet(const size_t _maximum_value)
      : already_included_(std::vector<bool>(_maximum_value, false)),
        maximum_value_(_maximum_value) {}

  ~IntSet() = default;

  typedef std::vector<size_t>::const_iterator Iterator;

  /// Returns beginning of unique integers
  std::vector<size_t>::const_iterator begin() const {
    return unique_integers_.cbegin();
  }

  /// Deletes all entries
  void clear() {
    for (size_t i : unique_integers_) {
      already_included_[i] = false;
    }
    unique_integers_.clear();
  }

  /// Returns end of unique integers
  std::vector<size_t>::const_iterator end() const {
    return unique_integers_.cend();
  }

  /// Adds an integer to unique_integers_, if it is not already included
  void insert(const size_t _val) {
    assert_true(_val < static_cast<size_t>(already_included_.size()));

    if (!already_included_[_val]) {
      unique_integers_.push_back(_val);
      already_included_[_val] = true;
    }
  }

  /// Trivial getter
  size_t maximum_value() const { return maximum_value_; }

  /// Resizes the container.
  void resize(size_t _size) { *this = std::move(IntSet(_size)); }

  /// Whether the IntSet is empty
  size_t size() const { return unique_integers_.size(); }

  /// Trivial getter
  const std::vector<size_t>& unique_integers() const {
    return unique_integers_;
  }

 private:
  /// Denotes whether the integer is already included
  std::vector<bool> already_included_;

  /// The maximum integer that can be stored in the IntSet
  size_t maximum_value_;

  /// Contains all integers that have been included
  std::vector<size_t> unique_integers_;
};

}  // namespace helpers

#endif  // HELPERS_INTSET_HPP_
