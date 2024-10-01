// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef HELPERS_COLUMN_HPP_
#define HELPERS_COLUMN_HPP_

#include <ranges>
#include <type_traits>
#include <variant>

#include "fct/AccessIterator.hpp"
#include "helpers/Subrole.hpp"
#include "memmap/StringVector.hpp"
#include "strings/String.hpp"

namespace helpers {

template <typename T>
struct Column {
  typedef
      typename std::conditional<std::is_same<T, strings::String>::value,
                                memmap::StringVector, memmap::Vector<T>>::type
          MemmapVector;
  typedef typename std::vector<T> InMemoryVector;

  typedef std::shared_ptr<InMemoryVector> InMemoryPtr;
  typedef std::shared_ptr<MemmapVector> MemmapPtr;

  typedef std::shared_ptr<const InMemoryVector> ConstInMemoryPtr;
  typedef std::shared_ptr<const MemmapVector> ConstMemmapPtr;

  typedef std::variant<InMemoryPtr, MemmapPtr> Variant;
  typedef std::variant<ConstInMemoryPtr, ConstMemmapPtr> ConstVariant;

  Column(const ConstVariant& _ptr, const std::string& _name,
         const std::vector<Subrole>& _subroles, const std::string& _unit)
      : data_(get_data(_ptr)),
        name_(_name),
        nrows_(get_nrows(_ptr)),
        ptr_(_ptr),
        subroles_(_subroles),
        unit_(_unit) {}

  ~Column() = default;

  Column(Column&&) = default;
  Column(Column const&) = default;
  auto operator=(Column const&) -> Column& = delete;
  auto operator=(Column&&) -> Column& = delete;

  /// Iterator begin
  auto begin() const {
    if constexpr (!std::is_same<T, strings::String>()) {
      return data_;
    }
    if constexpr (std::is_same<T, strings::String>()) {
      return fct::AccessIterator<T, Column<T>>(0, this);
    }
  }

  /// Iterator end
  auto end() const { return begin() + nrows_; }

  /// Access operator
  T operator[](size_t _i) const {
    assert_msg(_i < nrows_, "_i: " + std::to_string(_i) +
                                ", nrows_: " + std::to_string(nrows_));

    if constexpr (std::is_same<T, strings::String>()) {
      if (std::holds_alternative<ConstMemmapPtr>(ptr_)) {
        const auto str = (*std::get<ConstMemmapPtr>(ptr_))[_i];
        return str.c_str()[0] != '\0' ? str : strings::String(nullptr);
      }
    }

    return *(data_ + _i);
  }

  /// Retrieves the data, when this is a mmap string vector, it returns
  /// nullptr.
  static const T* get_data(const ConstVariant _ptr) {
    assert_true(!std::holds_alternative<ConstInMemoryPtr>(_ptr) ||
                std::get<ConstInMemoryPtr>(_ptr));

    assert_true(!std::holds_alternative<ConstMemmapPtr>(_ptr) ||
                std::get<ConstMemmapPtr>(_ptr));

    if constexpr (std::is_same<T, strings::String>()) {
      return std::holds_alternative<ConstInMemoryPtr>(_ptr)
                 ? std::get<ConstInMemoryPtr>(_ptr)->data()
                 : nullptr;
    }

    if constexpr (!std::is_same<T, strings::String>()) {
      return std::holds_alternative<ConstInMemoryPtr>(_ptr)
                 ? std::get<ConstInMemoryPtr>(_ptr)->data()
                 : std::get<ConstMemmapPtr>(_ptr)->data();
    }
  }

  /// Retrieves the number of rows.
  static size_t get_nrows(const ConstVariant _ptr) {
    assert_true(!std::holds_alternative<ConstInMemoryPtr>(_ptr) ||
                std::get<ConstInMemoryPtr>(_ptr));

    assert_true(!std::holds_alternative<ConstMemmapPtr>(_ptr) ||
                std::get<ConstMemmapPtr>(_ptr));

    return std::holds_alternative<ConstInMemoryPtr>(_ptr)
               ? std::get<ConstInMemoryPtr>(_ptr)->size()
               : std::get<ConstMemmapPtr>(_ptr)->size();
  }

  /// Generates the range for the string view.
  auto make_range() const {
    const auto col = *this;
    const auto get_value = [col](const size_t _i) -> T { return col[_i]; };
    return std::views::iota(0uz, nrows_) | std::views::transform(get_value);
  }

  /// Pointer to the underlying data.
  const T* const data_;

  /// Name of the column
  const std::string name_;

  /// Number of rows
  const size_t nrows_;

  /// The pointer to take ownership of the underlying data.
  const ConstVariant ptr_;

  /// Subroles of the column
  const std::vector<Subrole> subroles_;

  /// Unit of the column
  const std::string unit_;
};

}  // namespace helpers

#endif  // HELPERS_COLUMN_HPP_
