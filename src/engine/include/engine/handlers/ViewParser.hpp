// Copyright 2022 The SQLNet Company GmbH
// 
// This file is licensed under the Elastic License 2.0 (ELv2). 
// Refer to the LICENSE.txt file in the root of the repository 
// for details.
// 

#ifndef ENGINE_HANDLERS_VIEWPARSER_HPP_
#define ENGINE_HANDLERS_VIEWPARSER_HPP_

// ------------------------------------------------------------------------

#include <Poco/JSON/Object.h>
#include <Poco/Net/StreamSocket.h>

// ------------------------------------------------------------------------

#include <map>
#include <memory>
#include <string>

// ------------------------------------------------------------------------

#include "debug/debug.hpp"

// ------------------------------------------------------------------------

#include "engine/communication/communication.hpp"
#include "engine/config/config.hpp"
#include "engine/containers/containers.hpp"

// ------------------------------------------------------------------------

#include "engine/handlers/ArrowHandler.hpp"

// ------------------------------------------------------------------------

namespace engine {
namespace handlers {

class ViewParser {
 public:
  static constexpr const char* FLOAT_COLUMN =
      containers::Column<bool>::FLOAT_COLUMN;
  static constexpr const char* STRING_COLUMN =
      containers::Column<bool>::STRING_COLUMN;

  static constexpr const char* FLOAT_COLUMN_VIEW =
      containers::Column<bool>::FLOAT_COLUMN_VIEW;
  static constexpr const char* STRING_COLUMN_VIEW =
      containers::Column<bool>::STRING_COLUMN_VIEW;
  static constexpr const char* BOOLEAN_COLUMN_VIEW =
      containers::Column<bool>::BOOLEAN_COLUMN_VIEW;

  typedef std::variant<containers::ColumnView<strings::String>,
                       containers::ColumnView<Float>>
      ColumnViewVariant;

  // ------------------------------------------------------------------------

 public:
  ViewParser(const fct::Ref<containers::Encoding>& _categories,
             const fct::Ref<containers::Encoding>& _join_keys_encoding,
             const fct::Ref<const std::map<std::string, containers::DataFrame>>&
                 _data_frames,
             const config::Options& _options)
      : categories_(_categories),
        data_frames_(_data_frames),
        join_keys_encoding_(_join_keys_encoding),
        options_(_options) {}

  ~ViewParser() = default;

  // ------------------------------------------------------------------------

 public:
  /// Returns the content of a view.
  Poco::JSON::Object get_content(const size_t _draw, const size_t _start,
                                 const size_t _length, const bool _force_nrows,
                                 const Poco::JSON::Array::Ptr& _cols) const;

  /// Parses a view and turns it into a DataFrame.
  containers::DataFrame parse(const Poco::JSON::Object& _obj);

  /// Returns the population and peripheral data frames.
  std::tuple<containers::DataFrame, std::vector<containers::DataFrame>,
             std::optional<containers::DataFrame>>
  parse_all(const Poco::JSON::Object& _cmd);

  // ------------------------------------------------------------------------

 public:
  /// Expresses the View as a arrow::Table.
  std::shared_ptr<arrow::Table> to_table(const Poco::JSON::Object& _obj) {
    return ArrowHandler(categories_, join_keys_encoding_, options_)
        .df_to_table(parse(_obj));
  }

  // ------------------------------------------------------------------------

 private:
  /// Adds a new column to the data frame.
  void add_column(const Poco::JSON::Object& _obj, containers::DataFrame* _df);

  /// Adds a new int column to the data frame.
  void add_int_column_to_df(const std::string& _name, const std::string& _role,
                            const std::vector<std::string>& _subroles,
                            const std::string& _unit,
                            const std::vector<strings::String>& _vec,
                            containers::DataFrame* _df);

  /// Adds a new string column to the DataFrame.
  void add_string_column_to_df(const std::string& _name,
                               const std::string& _role,
                               const std::vector<std::string>& _subroles,
                               const std::string& _unit,
                               const std::vector<strings::String>& _vec,
                               containers::DataFrame* _df);

  /// Drop a set of columns.
  void drop_columns(const Poco::JSON::Object& _obj,
                    containers::DataFrame* _df) const;

  /// Generates a column view from a JSON::Object::Ptr - used by
  /// get_content(...).
  ColumnViewVariant make_column_view(Poco::JSON::Object::Ptr _ptr) const;

  /// Generates the data for get_content(...).
  Poco::JSON::Array::Ptr make_data(
      const std::vector<std::vector<std::string>>& _string_vectors) const;

  /// Generates nrows for get_content(...).
  std::optional<size_t> make_nrows(
      const std::vector<ColumnViewVariant>& _column_views,
      const size_t _force) const;

  /// Extracts a string vector from a column view - used by get_content(...).
  std::vector<std::string> make_string_vector(
      const size_t _start, const size_t _length,
      const ColumnViewVariant& _column_view) const;

  /// Applies the subselection.
  void subselection(const Poco::JSON::Object& _obj,
                    containers::DataFrame* _df) const;

  // ------------------------------------------------------------------------

 private:
  /// Encodes the categories used.
  const fct::Ref<containers::Encoding> categories_;

  /// The DataFrames this is based on.
  const fct::Ref<const std::map<std::string, containers::DataFrame>>
      data_frames_;

  /// Encodes the join keys used.
  const fct::Ref<containers::Encoding> join_keys_encoding_;

  /// Settings for the engine and the monitor
  const config::Options options_;

  // ------------------------------------------------------------------------
};

// ----------------------------------------------------------------------------
}  // namespace handlers
}  // namespace engine

#endif  // ENGINE_HANDLERS_VIEWPARSER_HPP_
