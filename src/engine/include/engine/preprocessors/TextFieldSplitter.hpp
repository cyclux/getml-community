// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef ENGINE_PREPROCESSORS_TEXTFIELDSPLITTER_HPP_
#define ENGINE_PREPROCESSORS_TEXTFIELDSPLITTER_HPP_

#include <Poco/JSON/Object.h>

#include <memory>
#include <utility>
#include <vector>

#include "engine/commands/Preprocessor.hpp"
#include "engine/containers/containers.hpp"
#include "engine/preprocessors/FitParams.hpp"
#include "engine/preprocessors/Preprocessor.hpp"
#include "engine/preprocessors/TransformParams.hpp"
#include "fct/Field.hpp"
#include "fct/Literal.hpp"
#include "fct/Ref.hpp"
#include "fct/define_named_tuple.hpp"
#include "helpers/helpers.hpp"
#include "strings/strings.hpp"

namespace engine {
namespace preprocessors {

class TextFieldSplitter : public Preprocessor {
 public:
  using TextFieldSplitterOp =
      typename commands::Preprocessor::TextFieldSplitterOp;

  using f_cols =
      fct::Field<"cols_",
                 std::vector<std::shared_ptr<helpers::ColumnDescription>>>;

  using NamedTupleType = fct::define_named_tuple_t<TextFieldSplitterOp, f_cols>;

 public:
  TextFieldSplitter() {}

  TextFieldSplitter(const TextFieldSplitterOp& _op,
                    const std::vector<Poco::JSON::Object::Ptr>& _dependencies)
      : dependencies_(_dependencies) {}

  /// TODO: Remove this quick fix.
  TextFieldSplitter(const Poco::JSON::Object& _obj,
                    const std::vector<Poco::JSON::Object::Ptr>& _dependencies)
      : TextFieldSplitter(json::from_json<TextFieldSplitterOp>(_obj),
                          _dependencies) {}

  ~TextFieldSplitter() = default;

 public:
  /// Returns the fingerprint of the preprocessor (necessary to build
  /// the dependency graphs).
  Poco::JSON::Object::Ptr fingerprint() const final;

  /// Identifies which features should be extracted from which time stamps.
  std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
  fit_transform(const FitParams& _params) final;

  /// Stores the preprocessor.
  void save(const std::string& _fname) const final;

  /// Generates SQL code for the text field splitting.
  std::vector<std::string> to_sql(
      const helpers::StringIterator& _categories,
      const std::shared_ptr<const transpilation::SQLDialectGenerator>&
          _sql_dialect_generator) const final;

  /// Transforms the data frames by adding the desired time series
  /// transformations.
  std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
  transform(const TransformParams& _params) const final;

 public:
  /// Creates a deep copy.
  std::shared_ptr<Preprocessor> clone(
      const std::optional<std::vector<Poco::JSON::Object::Ptr>>& _dependencies =
          std::nullopt) const final {
    const auto c = std::make_shared<TextFieldSplitter>(*this);
    if (_dependencies) {
      c->dependencies_ = *_dependencies;
    }
    return c;
  }

  /// Necessary for the automated parsing to work.
  NamedTupleType named_tuple() const {
    return fct::make_field<"type_">(fct::Literal<"TextFieldSplitter">()) *
           f_cols(cols_);
  }

  /// Returns the type of the preprocessor.
  std::string type() const final { return Preprocessor::TEXT_FIELD_SPLITTER; }

 private:
  /// Adds a rowid to the data frame.
  containers::DataFrame add_rowid(const containers::DataFrame& _df) const;

  /// Fits and transforms an individual data frame.
  std::vector<std::shared_ptr<helpers::ColumnDescription>> fit_df(
      const containers::DataFrame& _df, const std::string& _marker) const;

  /// Parses a JSON object.
  TextFieldSplitter from_json_obj(const Poco::JSON::Object& _obj) const;

  /// Generates a new data frame.
  containers::DataFrame make_new_df(
      const std::shared_ptr<memmap::Pool> _pool, const std::string& _df_name,
      const containers::Column<strings::String>& _col) const;

  /// Removes the text fields from the data frame.
  containers::DataFrame remove_text_fields(
      const containers::DataFrame& _df) const;

  /// Splits the text fields on a particular column.
  std::pair<containers::Column<Int>, containers::Column<strings::String>>
  split_text_fields_on_col(
      const containers::Column<strings::String>& _col) const;

  /// Transforms a single data frame.
  void transform_df(const std::string& _marker,
                    const containers::DataFrame& _df,
                    std::vector<containers::DataFrame>* _peripheral_dfs) const;

 private:
  /// List of all columns to which the text field splitter transformation
  /// applies.
  std::vector<std::shared_ptr<helpers::ColumnDescription>> cols_;

  /// The dependencies inserted into the the preprocessor.
  std::vector<Poco::JSON::Object::Ptr> dependencies_;
};

}  // namespace preprocessors
}  // namespace engine

#endif  // ENGINE_PREPROCESSORS_TEXTFIELDSPLITTER_HPP_

