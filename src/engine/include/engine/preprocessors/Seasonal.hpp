// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef ENGINE_PREPROCESSORS_SEASONAL_HPP_
#define ENGINE_PREPROCESSORS_SEASONAL_HPP_

#include <memory>
#include <utility>
#include <vector>

#include "commands/Fingerprint.hpp"
#include "commands/Preprocessor.hpp"
#include "containers/containers.hpp"
#include "engine/Float.hpp"
#include "engine/Int.hpp"
#include "engine/preprocessors/Params.hpp"
#include "engine/preprocessors/Preprocessor.hpp"
#include "helpers/ColumnDescription.hpp"
#include "helpers/StringIterator.hpp"
#include "rfl/Field.hpp"
#include "rfl/Literal.hpp"
#include "rfl/NamedTuple.hpp"
#include "rfl/Ref.hpp"
#include "strings/strings.hpp"

namespace engine {
namespace preprocessors {

class Seasonal : public Preprocessor {
  using MarkerType = typename helpers::ColumnDescription::MarkerType;

 public:
  using SeasonalOp = typename commands::Preprocessor::SeasonalOp;

  using f_hour =
      rfl::Field<"hour_", std::vector<rfl::Ref<helpers::ColumnDescription>>>;

  using f_minute =
      rfl::Field<"minute_", std::vector<rfl::Ref<helpers::ColumnDescription>>>;

  using f_month =
      rfl::Field<"month_", std::vector<rfl::Ref<helpers::ColumnDescription>>>;

  using f_weekday =
      rfl::Field<"weekday_", std::vector<rfl::Ref<helpers::ColumnDescription>>>;

  using f_year =
      rfl::Field<"year_", std::vector<rfl::Ref<helpers::ColumnDescription>>>;

  using NamedTupleType =
      rfl::NamedTuple<f_hour, f_minute, f_month, f_weekday, f_year>;

 private:
  static constexpr bool ADD_ZERO = true;
  static constexpr bool DONT_ADD_ZERO = false;

 public:
  Seasonal(const SeasonalOp& _op,
           const std::vector<commands::Fingerprint>& _dependencies)
      : dependencies_(_dependencies), op_(_op) {}

  ~Seasonal() = default;

 public:
  /// Returns the fingerprint of the preprocessor (necessary to build
  /// the dependency graphs).
  commands::Fingerprint fingerprint() const final;

  /// Identifies which features should be extracted from which time stamps.
  std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
  fit_transform(const Params& _params) final;

  /// Loads the predictor
  void load(const std::string& _fname) final;

  /// Necessary for the automated parsing to work.
  NamedTupleType named_tuple() const;

  /// Stores the preprocessor.
  void save(const std::string& _fname,
            const typename helpers::Saver::Format& _format) const final;

  /// Transforms the data frames by adding the desired time series
  /// transformations.
  std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
  transform(const Params& _params) const final;

 public:
  /// Creates a deep copy.
  rfl::Ref<Preprocessor> clone(
      const std::optional<std::vector<commands::Fingerprint>>& _dependencies =
          std::nullopt) const final {
    const auto c = rfl::Ref<Seasonal>::make(*this);
    if (_dependencies) {
      c->dependencies_ = *_dependencies;
    }
    return c;
  }

  /// The preprocessor does not generate any SQL scripts.
  std::vector<std::string> to_sql(
      const helpers::StringIterator& _categories,
      const std::shared_ptr<const transpilation::SQLDialectGenerator>&
          _sql_dialect_generator) const final {
    return {};
  }

  /// Returns the type of the preprocessor.
  std::string type() const final { return Preprocessor::SEASONAL; }

 private:
  /// Extracts the hour of a time stamp. Returns std::nullopt, if the column
  /// generates warning.
  std::optional<containers::Column<Int>> extract_hour(
      const containers::Column<Float>& _col,
      containers::Encoding* _categories) const;

  /// Extracts the hour of a time stamp.
  containers::Column<Int> extract_hour(
      const containers::Encoding& _categories,
      const containers::Column<Float>& _col) const;

  /// Extracts the minute of a time stamp. Returns std::nullopt, if the column
  /// generates warning.
  std::optional<containers::Column<Int>> extract_minute(
      const containers::Column<Float>& _col,
      containers::Encoding* _categories) const;

  /// Extracts the minute of a time stamp.
  containers::Column<Int> extract_minute(
      const containers::Encoding& _categories,
      const containers::Column<Float>& _col) const;

  /// Extracts the month of a time stamp. Returns std::nullopt, if the column
  /// generates warning.
  std::optional<containers::Column<Int>> extract_month(
      const containers::Column<Float>& _col,
      containers::Encoding* _categories) const;

  /// Extracts the month of a time stamp.
  containers::Column<Int> extract_month(
      const containers::Encoding& _categories,
      const containers::Column<Float>& _col) const;

  /// Extracts the weekday of a time stamp. Returns std::nullopt, if the
  /// column generates warnings.
  std::optional<containers::Column<Int>> extract_weekday(
      const containers::Column<Float>& _col,
      containers::Encoding* _categories) const;

  /// Extracts the weekday of a time stamp.
  containers::Column<Int> extract_weekday(
      const containers::Encoding& _categories,
      const containers::Column<Float>& _col) const;

  /// Extracts the year of a time stamp. Returns std::nullopt, if the column
  /// generates warning.
  std::optional<containers::Column<Float>> extract_year(
      const containers::Column<Float>& _col);

  /// Extracts the year of a time stamp.
  containers::Column<Float> extract_year(
      const containers::Column<Float>& _col) const;

  /// Fits and transforms an individual data frame.
  containers::DataFrame fit_transform_df(const containers::DataFrame& _df,
                                         const MarkerType _marker,
                                         const size_t _table,
                                         containers::Encoding* _categories);

  // Transforms a float column to a categorical column.
  containers::Column<Int> to_int(const containers::Column<Float>& _col,
                                 const bool _add_zero,
                                 containers::Encoding* _categories) const;

  /// Transforms a float column to a categorical column.
  containers::Column<Int> to_int(const containers::Encoding& _categories,
                                 const bool _add_zero,
                                 const containers::Column<Float>& _col) const;

  /// Transforms a single data frame.
  containers::DataFrame transform_df(const containers::Encoding& _categories,
                                     const containers::DataFrame& _df,
                                     const MarkerType _marker,
                                     const size_t _table) const;

 private:
  /// Whether a particular feature is enabled.
  template <rfl::internal::StringLiteral _field_name>
  bool is_disabled() const {
    return op_.get<_field_name>() && *op_.get<_field_name>();
  }

  /// Undertakes a transformation based on the template class
  /// Operator.
  template <class Operator>
  containers::Column<Int> to_categorical(
      const containers::Column<Float>& _col, const bool _add_zero,
      const Operator& _op, containers::Encoding* _categories) const {
    auto result = containers::Column<Float>(_col.pool(), _col.nrows());
    std::transform(_col.begin(), _col.end(), result.begin(), _op);
    return to_int(result, _add_zero, _categories);
  }

  /// Undertakes a transformation based on the template class
  /// Operator.
  template <class Operator>
  containers::Column<Int> to_categorical(
      const containers::Encoding& _categories,
      const containers::Column<Float>& _col, const bool _add_zero,
      const Operator& _op) const {
    auto result = containers::Column<Float>(_col.pool(), _col.nrows());
    std::transform(_col.begin(), _col.end(), result.begin(), _op);
    return to_int(_categories, _add_zero, result);
  }

  /// Undertakes a transformation based on the template class
  /// Operator.
  template <class Operator>
  containers::Column<Float> to_numerical(const containers::Column<Float>& _col,
                                         const Operator& _op) const {
    auto result = containers::Column<Float>(_col.pool(), _col.nrows());
    std::transform(_col.begin(), _col.end(), result.begin(), _op);
    return result;
  }

 private:
  /// The dependencies inserted into the the preprocessor.
  std::vector<commands::Fingerprint> dependencies_;

  /// List of all columns to which the hour transformation applies.
  std::vector<rfl::Ref<helpers::ColumnDescription>> hour_;

  /// List of all columns to which the minute transformation applies.
  std::vector<rfl::Ref<helpers::ColumnDescription>> minute_;

  /// List of all columns to which the month transformation applies.
  std::vector<rfl::Ref<helpers::ColumnDescription>> month_;

  /// The underlying hyperparameters.
  SeasonalOp op_;

  /// List of all columns to which the weekday transformation applies.
  std::vector<rfl::Ref<helpers::ColumnDescription>> weekday_;

  /// List of all columns to which the year transformation applies.
  std::vector<rfl::Ref<helpers::ColumnDescription>> year_;
};

}  // namespace preprocessors
}  // namespace engine

#endif  // ENGINE_PREPROCESSORS_SEASONAL_HPP_

