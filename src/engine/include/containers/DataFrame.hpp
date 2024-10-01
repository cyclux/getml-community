// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef CONTAINERS_DATAFRAME_HPP_
#define CONTAINERS_DATAFRAME_HPP_

#include <Poco/DateTimeFormat.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/File.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "commands/DataFrameFromJSON.hpp"
#include "commands/DataFrameOrView.hpp"
#include "commands/Fingerprint.hpp"
#include "containers/Column.hpp"
#include "containers/DataFrameContent.hpp"
#include "containers/DataFrameIndex.hpp"
#include "containers/Encoding.hpp"
#include "containers/Float.hpp"
#include "containers/Int.hpp"
#include "containers/MonitorSummary.hpp"
#include "containers/Schema.hpp"
#include "database/Connector.hpp"
#include "fct/to.hpp"
#include "helpers/DataFrameParams.hpp"
#include "helpers/Macros.hpp"
#include "transpilation/HumanReadableSQLGenerator.hpp"

namespace containers {

class DataFrame {
 public:
  static constexpr const char *ROLE_CATEGORICAL = "categorical";
  static constexpr const char *ROLE_JOIN_KEY = "join_key";
  static constexpr const char *ROLE_NUMERICAL = "numerical";
  static constexpr const char *ROLE_TARGET = "target";
  static constexpr const char *ROLE_TEXT = "text";
  static constexpr const char *ROLE_TIME_STAMP = "time_stamp";
  static constexpr const char *ROLE_UNUSED = "unused";
  static constexpr const char *ROLE_UNUSED_FLOAT = "unused_float";
  static constexpr const char *ROLE_UNUSED_STRING = "unused_string";

  using ViewOp = typename commands::DataFrameOrView::ViewOp;

 public:
  DataFrame(const std::shared_ptr<memmap::Pool> &_pool = nullptr)
      : categories_(std::shared_ptr<Encoding>()),
        frozen_(false),
        join_keys_encoding_(std::shared_ptr<Encoding>()),
        pool_(_pool) {
    update_last_change();
  }

  DataFrame(const std::string &_name,
            const std::shared_ptr<Encoding> &_categories,
            const std::shared_ptr<Encoding> &_join_keys_encoding,
            const std::shared_ptr<memmap::Pool> &_pool)
      : categories_(_categories),
        frozen_(false),
        join_keys_encoding_(_join_keys_encoding),
        name_(_name),
        pool_(_pool) {
    update_last_change();
  }

  ~DataFrame() = default;

  /// Setter for a float_column
  void add_float_column(const Column<Float> &_col, const std::string &_role);

  /// Setter for an int_column
  void add_int_column(const Column<Int> &_col, const std::string _role);

  /// Setter for a string_column
  void add_string_column(const Column<strings::String> &_col,
                         const std::string &_role);

  /// Appends another data frame to this data frame.
  void append(const DataFrame &_other);

  /// Makes sure that the data contained in the DataFrame is plausible
  /// and consistent.
  void check_plausibility() const;

  /// Creates a deep copy of the DataFrame
  DataFrame clone(const std::string _name) const;

  /// Builds indices_, which serve the role of
  /// an index over the join keys
  void create_indices();

  /// Returns the fingerprint of the data frame (necessary to build the
  /// dependency graphs).
  commands::Fingerprint fingerprint() const;

  /// Getter for a float_column.
  const Column<Float> &float_column(const std::string &_role,
                                    const size_t _num) const;

  /// Getter for a float column.
  const Column<Float> &float_column(const std::string &_name,
                                    const std::string &_role) const;

  /// Builds a dataframe from one or several CSV files.
  void from_csv(const std::optional<std::vector<std::string>> &_colnames,
                const std::vector<std::string> &_fnames,
                const std::string &_quotechar, const std::string &_sep,
                const size_t _num_lines_read, const size_t _skip,
                const std::vector<std::string> &_time_formats,
                const Schema &_schema);

  /// Builds a dataframe from a table in the data base.
  void from_db(const rfl::Ref<database::Connector> _connector,
               const std::string &_tname, const Schema &_schema);

  /// Builds a dataframe from a JSON object.
  void from_json(const commands::DataFrameFromJSON &_obj,
                 const std::vector<std::string> _time_formats,
                 const Schema &_schema);

  /// Builds a dataframe from a query.
  void from_query(const rfl::Ref<database::Connector> _connector,
                  const std::string &_query, const Schema &_schema);

  /// Returns the content of the data frame in a format that is compatible
  /// with the DataTables.js server-side processing API.
  DataFrameContent get_content(const std::int32_t _draw,
                               const std::int32_t _start,
                               const std::int32_t _length) const;

  /// Returns the first _n rows as a html that is compatible with Jupyter
  /// notebooks.
  std::string get_html(const std::int32_t _max_rows,
                       const std::int32_t _border) const;

  /// Returns the first _n rows as a formatted string.
  std::string get_string(const std::int32_t _max_rows) const;

  /// Getter for an int_column (either join keys or categorical)
  const Column<Int> &int_column(const std::string &_role,
                                const size_t _num) const;

  /// Getter for an int_column.
  const Column<Int> &int_column(const std::string &_name,
                                const std::string &_role) const;

  /// Loads the data from the hard-disk into the engine
  void load(const std::string &_path);

  /// Returns number of bytes occupied by the data
  ULong nbytes() const;

  /// Get the number of rows or return 0, if the DataFrame contains no
  /// columns.
  size_t nrows() const;

  /// Returns the role of the column signified by _name.
  std::string role(const std::string &_name) const;

  /// Removes a column.
  bool remove_column(const std::string &_name);

  /// Saves the data on the engine
  void save(const std::string &_temp_dir, const std::string &_path,
            const std::string &_name) const;

  /// Sorts all columns by the designated key.
  void sort_by_key(const std::vector<size_t> &_key);

  /// Getter for a string_column
  const Column<strings::String> &string_column(const std::string &_role,
                                               const size_t _num) const;

  /// Getter for a string_column.
  const Column<strings::String> &string_column(const std::string &_name,
                                               const std::string &_role) const;

  /// Returns the subroles of the column signified by _name.
  std::vector<std::string> subroles(const std::string &_name) const;

  /// Transforms an immutable data frame from this.
  template <typename DataFrameType>
  DataFrameType to_immutable(
      const std::optional<Schema> &_schema = std::nullopt,
      const bool _targets = true) const;

  /// Extracts the data frame in a format the monitor can
  /// understand
  containers::MonitorSummary to_monitor() const;

  /// Expresses the schema of the DataFrame.
  Schema to_schema(const bool _separate_discrete) const;

  /// Selects all rows for which the corresponding entry in _condition is
  /// true.
  void where(const std::vector<bool> &_condition);

  /// Trivial accessor
  std::optional<commands::Fingerprint> build_history() const {
    return build_history_;
  }

  /// Trivial accessor
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  const Column<Int> &categorical(const T _i) const {
    assert_true(categoricals_.size() > 0);
    assert_true(_i >= 0);
    assert_true(_i < static_cast<T>(categoricals_.size()));

    return categoricals_[_i];
  }

  /// Trivial accessor
  const Column<Int> &categorical(const std::string &_name) const {
    for (size_t i = 0; i < num_categoricals(); ++i) {
      if (categorical(i).name() == _name) {
        return categorical(i);
      }
    }

    throw_column_does_not_exist(_name, "categorical column");
    return categorical(0);
  }

  /// Trivial (const) accessor
  const std::vector<Column<Int>> &categoricals() const { return categoricals_; }

  /// Trivial accessor
  const Encoding &categories() const {
    assert_true(categories_);
    return *categories_.get();
  }

  /// Trivial accessor
  std::string category(const size_t _i) const {
    assert_true(_i < categories().size());

    return categories()[_i].str();
  }

  /// Freezes the DataFrame, thus making it immutable.
  void freeze() { frozen_ = true; }

  /// Whether the DataFrame has any column named _name.
  bool has(const std::string &_name) const {
    return has_categorical(_name) || has_join_key(_name) ||
           has_numerical(_name) || has_target(_name) || has_time_stamp(_name) ||
           has_unused_float(_name) || has_unused_string(_name) ||
           has_text(_name);
  }

  /// Whether the DataFrame has a categorical column named _name.
  bool has_categorical(const std::string &_name) const {
    for (size_t i = 0; i < num_categoricals(); ++i) {
      if (categorical(i).name() == _name) {
        return true;
      }
    }

    return false;
  }

  /// Whether the DataFrame has a join_key named _name.
  bool has_join_key(const std::string &_name) const {
    for (size_t i = 0; i < num_join_keys(); ++i) {
      if (join_key(i).name() == _name) {
        return true;
      }
    }

    return false;
  }

  /// Whether the DataFrame has a numerical column named _name.
  bool has_numerical(const std::string &_name) const {
    for (size_t i = 0; i < num_numericals(); ++i) {
      if (numerical(i).name() == _name) {
        return true;
      }
    }

    return false;
  }

  /// Whether the DataFrame has a target column of named _name.
  bool has_target(const std::string &_name) const {
    for (size_t i = 0; i < num_targets(); ++i) {
      if (target(i).name() == _name) {
        return true;
      }
    }

    return false;
  }

  /// Whether the DataFrame has a text column named _name.
  bool has_text(const std::string &_name) const {
    for (size_t i = 0; i < num_text(); ++i) {
      if (text(i).name() == _name) {
        return true;
      }
    }

    return false;
  }

  /// Whether the DataFrame has a time_stamp column of named _name.
  bool has_time_stamp(const std::string &_name) const {
    for (size_t i = 0; i < num_time_stamps(); ++i) {
      if (time_stamp(i).name() == _name) {
        return true;
      }
    }

    return false;
  }

  /// Whether the DataFrame has an unused float column name _name.
  bool has_unused_float(const std::string &_name) const {
    for (size_t i = 0; i < num_unused_floats(); ++i) {
      if (unused_float(i).name() == _name) {
        return true;
      }
    }

    return false;
  }

  /// Whether the DataFrame has an unused string column named _name.
  bool has_unused_string(const std::string &_name) const {
    for (size_t i = 0; i < num_unused_strings(); ++i) {
      if (unused_string(i).name() == _name) {
        return true;
      }
    }

    return false;
  }

  /// Returns the exact date and and time at which the data frame was last
  /// changed.
  std::string last_change() const { return last_change_; }

  /// Returns the index signified by index _i
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  DataFrameIndex &index(T _i) {
    assert_true(indices_.size() == join_keys_.size());
    assert_true(_i >= 0);
    assert_true(static_cast<size_t>(_i) < indices_.size());

    return indices_.at(_i);
  }

  /// Returns the index signified by index _i
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  const DataFrameIndex index(T _i) const {
    assert_true(indices_.size() == join_keys_.size());
    assert_true(_i >= 0);
    assert_true(static_cast<size_t>(_i) < indices_.size());

    return indices_.at(_i);
  }

  /// Returns the index corresponding to join key _name
  const DataFrameIndex index(const std::string &_name) const {
    assert_true(indices_.size() == join_keys_.size());

    for (size_t i = 0; i < num_join_keys(); ++i) {
      if (join_key(i).name() == _name) {
        return indices_.at(i);
      }
    }

    throw_column_does_not_exist(_name, "join key");
    return indices_.at(0);
  }

  /// Trivial accessor
  std::vector<DataFrameIndex> &indices() { return indices_; }

  /// Trivial accessor
  const std::vector<DataFrameIndex> &indices() const { return indices_; }

  /// Returns the join key signified by index _i
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  const Column<Int> &join_key(const T _i) const {
    assert_true(join_keys_.size() > 0);
    assert_true(_i >= 0);
    assert_true(_i < static_cast<T>(join_keys_.size()));

    return join_keys_[_i];
  }

  /// Trivial accessor
  const Column<Int> &join_key(const std::string &_name) const {
    for (size_t i = 0; i < num_join_keys(); ++i) {
      if (join_key(i).name() == _name) {
        return join_key(i);
      }
    }

    throw_column_does_not_exist(_name, "join key");
    return categorical(0);
  }

  /// Trivial (const) accessor
  const std::vector<Column<Int>> &join_keys() const { return join_keys_; }

  /// Primitive abstraction for member join_keys_encoding_
  const Encoding &join_keys_encoding() const {
    assert_true(join_keys_encoding_);
    return *join_keys_encoding_.get();
  }

  /// Returns the maps underlying the indices.
  const std::vector<std::shared_ptr<typename DataFrameIndex::MapType>> maps()
      const {
    std::vector<std::shared_ptr<typename DataFrameIndex::MapType>> maps;
    for (const auto &ix : indices_) maps.push_back(ix.map());
    return maps;
  }

  /// Primitive abstraction for member name_
  std::string name() const { return name_; }

  /// Get the number of columns.
  size_t ncols() const {
    return unused_floats_.size() + unused_strings_.size() + join_keys_.size() +
           time_stamps_.size() + categoricals_.size() + numericals_.size() +
           targets_.size() + text_.size();
  }

  /// Returns number of categorical columns.
  size_t num_categoricals() const { return categoricals_.size(); }

  /// Returns number of join keys.
  size_t num_join_keys() const { return join_keys_.size(); }

  /// Returns number of numerical columns.
  size_t num_numericals() const { return numericals_.size(); }

  /// Returns number of target columns.
  size_t num_targets() const { return targets_.size(); }

  /// Returns number of text columns.
  size_t num_text() const { return text_.size(); }

  /// Returns number of the time stamps.
  size_t num_time_stamps() const { return time_stamps_.size(); }

  /// Returns number of unused float columns.
  size_t num_unused_floats() const { return unused_floats_.size(); }

  /// Returns number of unused string columns.
  size_t num_unused_strings() const { return unused_strings_.size(); }

  /// Trivial accessor
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  const Column<Float> &numerical(const T _i) const {
    assert_true(numericals_.size() > 0);
    assert_true(_i >= 0);
    assert_true(_i < static_cast<T>(numericals_.size()));

    return numericals_[_i];
  }

  /// Trivial accessor
  const Column<Float> &numerical(const std::string &_name) const {
    for (size_t i = 0; i < num_numericals(); ++i) {
      if (numerical(i).name() == _name) {
        return numerical(i);
      }
    }

    throw_column_does_not_exist(_name, "numerical column");
    return numerical(0);
  }

  /// Trivial (const) accessor
  const std::vector<Column<Float>> &numericals() const { return numericals_; }

  /// Trivial (const) accessor
  const std::shared_ptr<memmap::Pool> &pool() const { return pool_; }

  /// Trivial setter
  void set_build_history(const commands::Fingerprint &_build_history) {
    build_history_ = _build_history;
  }

  /// Trivial setter
  void set_categories(const std::shared_ptr<Encoding> &_categories) {
    categories_ = _categories;
  }

  /// Primitive setter
  void set_join_keys_encoding(
      const std::shared_ptr<Encoding> &_join_keys_encoding) {
    join_keys_encoding_ = _join_keys_encoding;
  }

  /// Primitive setter
  void set_name(const std::string &_name) { name_ = _name; }

  /// Trivial accessor
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  const Column<Float> &target(const T _i) const {
    assert_true(targets_.size() > 0);
    assert_true(_i >= 0);
    assert_true(_i < static_cast<T>(targets_.size()));

    return targets_[_i];
  }

  /// Trivial accessor
  const Column<Float> &target(const std::string &_name) const {
    for (size_t i = 0; i < num_targets(); ++i) {
      if (target(i).name() == _name) {
        return target(i);
      }
    }

    throw_column_does_not_exist(_name, "target column");
    return target(0);
  }

  /// Trivial (const) accessor
  const std::vector<Column<Float>> &targets() const { return targets_; }

  /// Trivial accessor
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  const Column<strings::String> &text(const T _i) const {
    assert_true(text_.size() > 0);
    assert_true(_i >= 0);
    assert_true(_i < static_cast<T>(text_.size()));

    return text_.at(_i);
  }

  /// Trivial accessor
  const Column<strings::String> &text(const std::string &_name) const {
    for (const auto &col : text_) {
      if (col.name() == _name) {
        return col;
      }
    }

    throw_column_does_not_exist(_name, "text column");
    return text(0);
  }

  /// Trivial (const) accessor
  const std::vector<Column<strings::String>> &text() const { return text_; }

  /// Trivial accessor
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  Column<Float> const &time_stamp(const T _i) const {
    assert_true(time_stamps_.size() > 0);
    assert_true(_i >= 0);
    assert_true(_i < static_cast<T>(time_stamps_.size()));

    return time_stamps_[_i];
  }

  /// Trivial accessor
  const Column<Float> &time_stamp(const std::string &_name) const {
    for (size_t i = 0; i < num_time_stamps(); ++i) {
      if (time_stamp(i).name() == _name) {
        return time_stamp(i);
      }
    }

    throw_column_does_not_exist(_name, "time stamp");
    return time_stamp(0);
  }

  /// Trivial accessor
  const std::vector<Column<Float>> &time_stamps() const { return time_stamps_; }

  /// Trivial accessor
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  const Column<Float> &unused_float(const T _i) const {
    assert_true(unused_floats_.size() > 0);
    assert_true(_i >= 0);
    assert_true(_i < static_cast<T>(unused_floats_.size()));

    return unused_floats_[_i];
  }

  /// Trivial accessor
  const Column<Float> &unused_float(const std::string &_name) const {
    for (size_t i = 0; i < num_unused_floats(); ++i) {
      if (unused_float(i).name() == _name) {
        return unused_float(i);
      }
    }

    throw_column_does_not_exist(_name, "unused float column");
    return unused_float(0);
  }

  /// Trivial accessor
  template <typename T,
            typename std::enable_if<!std::is_same<T, std::string>::value,
                                    int>::type = 0>
  const Column<strings::String> &unused_string(const T _i) const {
    assert_true(unused_strings_.size() > 0);
    assert_true(_i >= 0);
    assert_true(_i < static_cast<T>(unused_strings_.size()));

    return unused_strings_[_i];
  }

  /// Trivial accessor
  const std::vector<Column<Float>> &unused_floats() const {
    return unused_floats_;
  }

  /// Trivial accessor
  const Column<strings::String> &unused_string(const std::string &_name) const {
    for (size_t i = 0; i < num_unused_strings(); ++i) {
      if (unused_string(i).name() == _name) {
        return unused_string(i);
      }
    }

    throw_column_does_not_exist(_name, "unused string column");
    return unused_string(0);
  }

  /// Trivial (const) accessor
  const std::vector<Column<strings::String>> &unused_strings() const {
    return unused_strings_;
  }

  // -------------------------------

 private:
  /// Adds a column to _columns.
  template <class ColType>
  void add_column(const ColType &_col, std::vector<ColType> *_columns);

  /// Adds a vector of float vectors.
  void add_float_vectors(
      const std::vector<std::string> &_names,
      const std::vector<std::shared_ptr<std::vector<Float>>> &_vectors,
      const std::string &_role);

  /// Adds a vector of integer vectors.
  void add_int_vectors(
      const std::vector<std::string> &_names,
      const std::vector<std::shared_ptr<std::vector<Int>>> &_vectors,
      const std::string &_role);

  /// Adds a vector of string vectors.
  void add_string_vectors(
      const std::vector<std::string> &_names,
      const std::vector<std::shared_ptr<std::vector<strings::String>>>
          &_vectors,
      const std::string &_role);

  /// Calculate the number of bytes.
  template <class T>
  ULong calc_nbytes(const std::vector<Column<T>> &_columns) const;

  /// Makes sure that the values contained in _col are all positive and
  /// finite.
  void check_null(const Column<Float> &_col) const;

  /// Concatenate a set of colnames.
  std::vector<std::string> concat_colnames(const Schema &_schema) const;

  /// Parses int columns.
  void from_json(const commands::DataFrameFromJSON &_obj,
                 const std::vector<std::string> &_names,
                 const std::string &_role, Encoding *_encoding);

  /// Parses float columns.
  void from_json(const commands::DataFrameFromJSON &_obj,
                 const std::vector<std::string> &_names,
                 const std::string &_role);

  /// Parses time stamp columns.
  void from_json(const commands::DataFrameFromJSON &_obj,
                 const std::vector<std::string> &_names,
                 const std::vector<std::string> &_time_formats);

  /// Builds a dataframe from a reader.
  void from_reader(const std::shared_ptr<io::Reader> &_reader,
                   const std::string &_fname, const size_t _skip,
                   const std::vector<std::string> &_time_formats,
                   const Schema &_schema);

  /// Returns the colnames, roles and units of columns.
  std::tuple<std::vector<std::string>, std::vector<std::string>,
             std::vector<std::string>>
  get_headers() const;

  /// Represents the first _max rows as a set of rows.
  std::vector<std::vector<std::string>> get_rows(
      const std::int32_t _max_rows) const;

  /// Loads columns.
  template <class T>
  std::vector<Column<T>> load_columns(const std::string &_path,
                                      const std::string &_prefix) const;

  /// Loads a textfile from disc.
  std::optional<std::string> load_textfile(const std::string &_path,
                                           const std::string &_fname) const;

  /// Creates a vector of vectors of type T.
  template <class T>
  std::vector<std::shared_ptr<std::vector<T>>> make_vectors(
      const size_t _size) const;

  /// Returns the colnames of a vector of columns
  template <class T>
  bool rm_col(const std::string &_name, std::vector<Column<T>> *_columns,
              std::vector<DataFrameIndex> *_indices = nullptr) const;

  /// Saves all matrices.
  template <class T>
  void save_matrices(const std::vector<Column<T>> &_matrices,
                     const std::string &_path,
                     const std::string &_prefix) const;

  ///  Saves a string to a textfile.
  void save_text(const std::string &_tpath, const std::string &_fname,
                 const std::string &_text) const;

 private:
  /// Checks whether the data frame has been frozen.
  void check_if_frozen() const {
    if (frozen_) {
      throw std::runtime_error(
          "The DataFrame has been frozen, so in-place operations are "
          "no longer possible.");
    }
  }

  /// Generates a new pool
  std::shared_ptr<memmap::Pool> make_pool() const {
    return pool_ ? std::make_shared<memmap::Pool>(pool_->temp_dir())
                 : std::shared_ptr<memmap::Pool>();
  }

  /// Throws an error that a particular column does not exist.
  void throw_column_does_not_exist(const std::string &_colname,
                                   const std::string &_coltype) const {
    const auto [table, colname] =
        helpers::Macros::parse_table_colname(name_, _colname);
    const auto staging_table_colname =
        transpilation::HumanReadableSQLGenerator().make_staging_table_colname(
            colname);
    throw std::runtime_error("Data frame '" + table + "' contains no " +
                             _coltype + " named '" + staging_table_colname +
                             "'!");
  }

  /// Records the current time as the last time something was changed.
  void update_last_change() {
    build_history_ = std::nullopt;
    const auto now = Poco::Timestamp();
    last_change_ = Poco::DateTimeFormatter::format(
        now, Poco::DateTimeFormat::ISO8601_FRAC_FORMAT);
  }

  // -------------------------------

 private:
  /// The build history is relevant for when the data frame contains generated
  /// features. It enables us to retrieve features we have already build.
  std::optional<commands::Fingerprint> build_history_;

  /// Categorical data
  std::vector<Column<Int>> categoricals_;

  /// Maps integers to names of categories
  std::shared_ptr<Encoding> categories_;

  /// Whether the DataFrame has been frozen.
  bool frozen_;

  /// Performs the role of an "index" over the join keys
  std::vector<DataFrameIndex> indices_;

  /// Join keys - note that there might be several
  std::vector<Column<Int>> join_keys_;

  /// Maps integers to names of join keys
  std::shared_ptr<Encoding> join_keys_encoding_;

  /// The last time something was changed that is relevant to the pipeline.
  std::string last_change_;

  /// Name of the data frame
  std::string name_;

  /// The pool used for memory-mapped data.
  std::shared_ptr<memmap::Pool> pool_;

  /// Numerical data
  std::vector<Column<Float>> numericals_;

  /// "Unused" floats - unused means that
  /// no explicit role has been set yet.
  std::vector<Column<Float>> unused_floats_;

  /// "Unused" strings - unused means that
  /// no explicit role has been set yet.
  std::vector<Column<strings::String>> unused_strings_;

  /// Targets - only exists for population tables
  std::vector<Column<Float>> targets_;

  /// Text - to be interpreted as text fields.
  /// Basic text mining is to be applied.
  std::vector<Column<strings::String>> text_;

  /// Time stamps
  std::vector<Column<Float>> time_stamps_;
};

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

template <class ColType>
void DataFrame::add_column(const ColType &_col,
                           std::vector<ColType> *_columns) {
  if (_col.nrows() != nrows() && ncols() != 0) {
    throw std::runtime_error("Column '" + _col.name() + "' is of length " +
                             std::to_string(_col.nrows()) + ", expected " +
                             std::to_string(nrows()) + ".");
  }

  remove_column(_col.name());

  _columns->push_back(_col);
}

// -------------------------------------------------------------------------

template <class T>
ULong DataFrame::calc_nbytes(const std::vector<Column<T>> &_columns) const {
  return std::accumulate(_columns.begin(), _columns.end(),
                         static_cast<ULong>(0),
                         [](const ULong init, const Column<T> &col) {
                           return init + col.nbytes();
                         });
}

// ----------------------------------------------------------------------------

template <class T>
std::vector<Column<T>> DataFrame::load_columns(
    const std::string &_path, const std::string &_prefix) const {
  std::vector<Column<T>> columns;

  for (size_t i = 0; true; ++i) {
    std::string fname = _path + _prefix + std::to_string(i);

    if (!Poco::File(fname).exists()) {
      break;
    }

    Column<T> col(pool_);

    col.load(fname);

    columns.push_back(col);
  }

  return columns;
}

// ----------------------------------------------------------------------------

template <class T>
std::vector<std::shared_ptr<std::vector<T>>> DataFrame::make_vectors(
    const size_t _size) const {
  auto vectors = std::vector<std::shared_ptr<std::vector<T>>>(_size);

  for (auto &vec : vectors) vec = std::make_shared<std::vector<T>>(0);

  return vectors;
}

// ----------------------------------------------------------------------------

template <class T>
bool DataFrame::rm_col(const std::string &_name,
                       std::vector<Column<T>> *_columns,
                       std::vector<DataFrameIndex> *_indices) const {
  const auto has_name = [_name](const Column<T> &_col) {
    return _col.name() == _name;
  };

  const auto it = std::find_if(_columns->begin(), _columns->end(), has_name);

  if (it == _columns->end()) {
    return false;
  }

  if (_indices) {
    assert_true(_indices->size() == _columns->size());

    const auto dist = std::distance(_columns->begin(), it);

    _indices->erase(_indices->begin() + dist);
  }

  _columns->erase(it);

  return true;
}

// ----------------------------------------------------------------------------

template <class T>
void DataFrame::save_matrices(const std::vector<Column<T>> &_matrices,
                              const std::string &_path,
                              const std::string &_prefix) const {
  for (size_t i = 0; i < _matrices.size(); ++i) {
    _matrices.at(i).save(_path + _prefix + std::to_string(i));
  }
}

// ----------------------------------------------------------------------------

template <typename DataFrameType>
DataFrameType DataFrame::to_immutable(const std::optional<Schema> &_schema,
                                      const bool _targets) const {
  using FloatColumnType = typename DataFrameType::FloatColumnType;
  using IntColumnType = typename DataFrameType::IntColumnType;
  using StringColumnType = typename DataFrameType::StringColumnType;

  const auto schema = _schema.value_or(to_schema(true));

  const auto parse = [](const std::vector<std::string> &_vec)
      -> std::vector<helpers::Subrole> {
    return helpers::SubroleParser::parse(_vec);
  };

  const auto get_categorical = [this, parse](const std::string &_name) {
    const auto &col = categorical(_name);
    return IntColumnType(col.const_data_ptr(), _name, parse(col.subroles()),
                         col.unit());
  };

  const auto categoricals = schema.categoricals() |
                            std::views::transform(get_categorical) |
                            fct::ranges::to<std::vector>();

  const auto get_join_key = [this, parse](const std::string &_name) {
    const auto &col = join_key(_name);
    return IntColumnType(col.const_data_ptr(), _name, parse(col.subroles()),
                         col.unit());
  };

  const auto join_keys = schema.join_keys() |
                         std::views::transform(get_join_key) |
                         fct::ranges::to<std::vector>();

  const auto get_index = [this](const std::string &_name) {
    return index(_name).map();
  };

  const auto indices = schema.join_keys() | std::views::transform(get_index) |
                       fct::ranges::to<std::vector>();

  const auto get_numerical = [this, parse](const std::string &_name) {
    const auto &col = numerical(_name);
    return FloatColumnType(col.const_data_ptr(), _name, parse(col.subroles()),
                           col.unit());
  };

  const auto discretes = schema.discretes() |
                         std::views::transform(get_numerical) |
                         fct::ranges::to<std::vector>();

  const auto numericals = schema.numericals() |
                          std::views::transform(get_numerical) |
                          fct::ranges::to<std::vector>();

  const auto get_target = [this, parse](const std::string &_name) {
    const auto &col = target(_name);
    return FloatColumnType(col.const_data_ptr(), _name, parse(col.subroles()),
                           col.unit());
  };

  const auto targets = _targets ? schema.targets() |
                                      std::views::transform(get_target) |
                                      fct::ranges::to<std::vector>()
                                : std::vector<FloatColumnType>();

  const auto get_text = [this, parse](const std::string &_name) {
    const auto &col = text(_name);
    return StringColumnType(col.const_data_ptr(), _name, parse(col.subroles()),
                            col.unit());
  };

  const auto text = schema.text() | std::views::transform(get_text) |
                    fct::ranges::to<std::vector>();

  const auto get_time_stamp = [this, parse](const std::string &_name) {
    const auto &col = time_stamp(_name);
    return FloatColumnType(col.const_data_ptr(), _name, parse(col.subroles()),
                           col.unit());
  };

  const auto time_stamps = schema.time_stamps() |
                           std::views::transform(get_time_stamp) |
                           fct::ranges::to<std::vector>();

  const auto params = helpers::DataFrameParams{.categoricals_ = categoricals,
                                               .discretes_ = discretes,
                                               .indices_ = indices,
                                               .join_keys_ = join_keys,
                                               .name_ = name(),
                                               .numericals_ = numericals,
                                               .targets_ = targets,
                                               .text_ = text,
                                               .time_stamps_ = time_stamps};

  return DataFrameType(params);
}

}  // namespace containers

#endif  // CONTAINERS_DATAFRAME_HPP_
