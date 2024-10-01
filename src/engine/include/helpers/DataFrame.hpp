// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef HELPERS_DATAFRAME_HPP_
#define HELPERS_DATAFRAME_HPP_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "helpers/Column.hpp"
#include "helpers/CreateSubviewParams.hpp"
#include "helpers/DataFrameParams.hpp"
#include "helpers/Float.hpp"
#include "helpers/Index.hpp"
#include "helpers/Int.hpp"
#include "helpers/Schema.hpp"

namespace helpers {

// -------------------------------------------------------------------------

struct DataFrame {
 public:
  typedef DataFrameParams::FloatColumnType FloatColumnType;

  typedef DataFrameParams::IntColumnType IntColumnType;

  typedef DataFrameParams::AdditionalColumns AdditionalColumns;

  typedef DataFrameParams::RowIndices RowIndices;

  typedef DataFrameParams::StringColumnType StringColumnType;

  typedef DataFrameParams::WordIndices WordIndices;

  // ---------------------------------------------------------------------

 public:
  DataFrame(const DataFrameParams& _params);

  ~DataFrame() = default;

  DataFrame(DataFrame&&) = default;
  DataFrame(DataFrame const&) = default;
  auto operator=(DataFrame const&) -> DataFrame& = delete;
  auto operator=(DataFrame&&) -> DataFrame& = delete;

  // ---------------------------------------------------------------------

 public:
  /// Creates a subview.
  DataFrame create_subview(const CreateSubviewParams& _params) const;

  /// Find the indices associated with this join key.
  std::pair<const size_t*, const size_t*> find(
      const Int _join_key, const size_t _ix_join_key = 0) const;

  // ---------------------------------------------------------------------

 public:
  /// Getter for a categorical value.
  Int categorical(size_t _i, size_t _j) const {
    assert_true(_j < categoricals_.size());
    return categoricals_[_j][_i];
  }

  /// Getter for a categorical column.
  const Column<Int> categorical_col(size_t _j) const {
    assert_true(_j < categoricals_.size());
    return categoricals_[_j];
  }

  /// Getter for a categorical name.
  const std::string& categorical_name(size_t _j) const {
    assert_true(_j < categoricals_.size());
    return categoricals_[_j].name_;
  }

  /// Getter for a categorical name.
  const std::string& categorical_unit(size_t _j) const {
    assert_true(_j < categoricals_.size());
    return categoricals_[_j].unit_;
  }

  /// Getter for a discrete value.
  Float discrete(size_t _i, size_t _j) const {
    assert_true(_j < discretes_.size());
    return discretes_[_j][_i];
  }

  /// Getter for a discrete column.
  const Column<Float> discrete_col(size_t _j) const {
    assert_true(_j < discretes_.size());
    return discretes_[_j];
  }

  /// Getter for a discrete name.
  const std::string& discrete_name(size_t _j) const {
    assert_true(_j < discretes_.size());
    return discretes_[_j].name_;
  }

  /// Getter for a discrete name.
  const std::string& discrete_unit(size_t _j) const {
    assert_true(_j < discretes_.size());
    return discretes_[_j].unit_;
  }

  /// Whether a certain join key is included in the indices.
  bool has(const Int _join_key) const {
    const auto [begin, end] = find(_join_key);
    return begin != nullptr;
  }

  /// Getter for the indices (TODO: make this private).
  const std::vector<std::shared_ptr<Index>>& indices() const {
    return indices_;
  }

  /// Getter for a join key.
  Int join_key(size_t _i) const {
    assert_true(join_keys_.size() == 1);

    return join_keys_[0][_i];
  }

  /// Getter for a join key column.
  const Column<Int>& join_key_col(const std::string& _colname) const {
    const auto ix = find_ix_join_key(_colname, std::nullopt);
    return join_keys_.at(ix);
  }
  /// Getter for a join keys.
  const std::vector<Column<Int>>& join_keys() const { return join_keys_; }

  /// Getter for the join key name.
  const std::string& join_keys_name() const {
    assert_true(join_keys_.size() == 1);

    return join_keys_[0].name_;
  }

  /// Return the name of the data frame.
  const std::string& name() const { return name_; }

  /// Trivial getter
  size_t nrows() const {
    if (num_categoricals() > 0) return categoricals_[0].nrows_;
    if (num_discretes() > 0) return discretes_[0].nrows_;
    if (num_join_keys() > 0) return join_keys_[0].nrows_;
    if (num_numericals() > 0) return numericals_[0].nrows_;
    if (num_targets() > 0) return targets_[0].nrows_;
    if (num_text() > 0) return text_[0].nrows_;
    if (num_time_stamps() > 0) return time_stamps_[0].nrows_;
    return 0;
  }

  /// Trivial getter
  size_t num_categoricals() const { return categoricals_.size(); }

  /// Trivial getter
  size_t num_discretes() const { return discretes_.size(); }

  /// Trivial getter
  size_t num_join_keys() const { return join_keys_.size(); }

  /// Trivial getter
  size_t num_numericals() const { return numericals_.size(); }

  /// Trivial getter
  size_t num_targets() const { return targets_.size(); }

  /// Trivial getter
  size_t num_text() const { return text_.size(); }

  /// Trivial getter
  size_t num_time_stamps() const { return time_stamps_.size(); }

  /// Getter for a numerical value.
  Float numerical(size_t _i, size_t _j) const {
    assert_true(_j < numericals_.size());
    return numericals_[_j][_i];
  }

  /// Getter for a numerical column.
  const Column<Float> numerical_col(size_t _j) const {
    assert_true(_j < numericals_.size());
    return numericals_[_j];
  }

  /// Getter for a numerical name.
  const std::string& numerical_name(size_t _j) const {
    assert_true(_j < numericals_.size());
    return numericals_[_j].name_;
  }

  /// Getter for a numerical name.
  const std::string& numerical_unit(size_t _j) const {
    assert_true(_j < numericals_.size());
    return numericals_[_j].unit_;
  }

  /// Getter for a target value.
  Float target(size_t _i, size_t _j) const {
    assert_true(_j < targets_.size());
    return targets_[_j][_i];
  }

  /// Getter for a target name.
  const std::string& target_name(size_t _j) const {
    assert_true(_j < targets_.size());
    return targets_[_j].name_;
  }

  /// Getter for a target name.
  const std::string& target_unit(size_t _j) const {
    assert_true(_j < targets_.size());
    return targets_[_j].unit_;
  }

  /// Trivial getter
  Float time_stamp(size_t _i) const {
    assert_true(time_stamps_.size() <= 2);

    if (time_stamps_.size() == 0) {
      return 0.0;
    }

    assert_true(_i < time_stamps_[0].nrows_);

    return time_stamps_[0][_i];
  }
  /// Getter for the time stamps column.
  const Column<Float> time_stamp_col() const {
    assert_true(time_stamps_.size() == 1 || time_stamps_.size() == 2);
    return time_stamps_[0];
  }

  /// Getter for the time stamps column.
  const Column<Float>& time_stamp_col(const size_t _i) const {
    assert_true(_i < time_stamps_.size());
    return time_stamps_[_i];
  }

  /// Getter for the time stamps name.
  const std::string& time_stamps_name() const {
    assert_true(time_stamps_.size() == 1 || time_stamps_.size() == 2);

    return time_stamps_[0].name_;
  }

  /// Returns the schema.
  Schema to_schema() const {
    return Schema(SchemaImpl{.categoricals = get_colnames(categoricals_),
                             .discretes = get_colnames(discretes_),
                             .join_keys = get_colnames(join_keys_),
                             .name = name_,
                             .numericals = get_colnames(numericals_),
                             .targets = get_colnames(targets_),
                             .text = get_colnames(text_),
                             .time_stamps = get_colnames(time_stamps_),
                             .unused_floats = std::vector<std::string>(),
                             .unused_strings = std::vector<std::string>()});
  }

  /// Trivial getter
  Float upper_time_stamp(size_t _i) const {
    assert_true(time_stamps_.size() <= 2);

    if (time_stamps_.size() <= 1) {
      return NAN;
    }

    assert_true(_i < time_stamps_[1].nrows_);

    return time_stamps_[1][_i];
  }

  /// Getter for the time stamps name.
  const std::string& upper_time_stamps_name() const {
    assert_true(time_stamps_.size() == 2);

    return time_stamps_[1].name_;
  }

  // ---------------------------------------------------------------------

 private:
  /// Finds the index of the join key named _colname
  size_t find_ix_join_key(
      const std::string& _colname,
      const std::optional<std::function<std::string(std::string)>>&
          _make_staging_table_colname) const;

  /// Finds the index of the time stamp named _colname
  size_t find_ix_time_stamp(
      const std::string& _colname,
      const std::optional<std::function<std::string(std::string)>>&
          _make_staging_table_colname) const;

  /// Helper class that extracts the column names.
  template <typename T>
  std::vector<std::string> get_colnames(
      const std::vector<Column<T>>& _columns) const;

  /// All time stamps that are not upper time stamp are added to numerical
  /// and given the unit time stamp - this is so the end users do not have
  /// to understand the difference between time stamps as a type and
  /// time stamps as a role.
  std::vector<Column<Float>> make_numericals_and_time_stamps(
      const AdditionalColumns& _additional, const bool _allow_lagged_targets,
      const std::string& _upper_time_stamp) const;

  /// Generates the ts_index_ for the subview.
  std::shared_ptr<tsindex::Index> make_ts_index(
      const CreateSubviewParams& _params, const size_t _ix_join_key,
      const size_t _ix_time_stamp) const;

  // ---------------------------------------------------------------------

 public:
  /// Pointer to categorical columns.
  const std::vector<Column<Int>> categoricals_;

  /// Pointer to discrete columns.
  const std::vector<Column<Float>> discretes_;

  /// Indices assiciated with join keys.
  const std::vector<std::shared_ptr<Index>> indices_;

  /// Join keys of this data frame.
  const std::vector<Column<Int>> join_keys_;

  /// Name of the data frame.
  const std::string name_;

  /// Pointer to numerical columns.
  const std::vector<Column<Float>> numericals_;

  /// Index returning rows for each word.
  const RowIndices row_indices_;

  /// Pointer to target column.
  const std::vector<Column<Float>> targets_;

  /// Pointer to text column.
  const std::vector<Column<strings::String>> text_;

  /// Time stamps of this data frame.
  const std::vector<Column<Float>> time_stamps_;

  /// An index for the time stamps, which can increase performance in certain
  /// scenarios.
  const std::shared_ptr<tsindex::Index> ts_index_;

  /// Index returning words for each row.
  const WordIndices word_indices_;
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

template <typename T>
std::vector<std::string> DataFrame::get_colnames(
    const std::vector<Column<T>>& _columns) const {
  std::vector<std::string> colnames;

  for (auto& col : _columns) {
    colnames.push_back(col.name_);
  }

  return colnames;
}

// ----------------------------------------------------------------------------

}  // namespace helpers

// ----------------------------------------------------------------------------

#endif  // HELPERS_DATAFRAME_HPP_
