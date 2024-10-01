// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "engine/preprocessors/CategoryTrimmer.hpp"

#include <rfl/replace.hpp>

#include "containers/Column.hpp"
#include "containers/DataFrame.hpp"
#include "helpers/ColumnDescription.hpp"
#include "helpers/Loader.hpp"
#include "helpers/NullChecker.hpp"
#include "helpers/Saver.hpp"
#include "transpilation/SQLGenerator.hpp"

namespace engine {
namespace preprocessors {

// ----------------------------------------------------

std::string CategoryTrimmer::column_to_sql(
    const helpers::StringIterator& _categories,
    const std::shared_ptr<const transpilation::SQLDialectGenerator>&
        _sql_dialect_generator,
    const CategoryPair& _pair) const {
  assert_true(_sql_dialect_generator);

  const auto staging_table =
      transpilation::SQLGenerator::make_staging_table_name(_pair.first.table());

  const auto colname =
      _sql_dialect_generator->make_staging_table_colname(_pair.first.name());

  std::stringstream sql;

  sql << _sql_dialect_generator->trimming()->make_header(staging_table,
                                                         colname);

  const auto vec = std::vector<Int>(_pair.second->begin(), _pair.second->end());

  constexpr size_t batch_size = 500;

  for (size_t batch_begin = 0; batch_begin < _pair.second->size();
       batch_begin += batch_size) {
    const auto batch_end =
        std::min(_pair.second->size(), batch_begin + batch_size);

    sql << _sql_dialect_generator->trimming()->make_insert_into(staging_table,
                                                                colname);

    for (size_t i = batch_begin; i < batch_end; ++i) {
      const std::string begin = (i == batch_begin) ? "" : "      ";

      const auto val = vec.at(i);

      assert_true(val >= 0);

      assert_true(static_cast<size_t>(val) < _categories.size());

      const auto end = (i == batch_end - 1) ? ";\n\n" : ",\n";

      sql << begin << "( '" << _categories.at(val).str() << "' )" << end;
    }
  }

  assert_true(_sql_dialect_generator);

  sql << _sql_dialect_generator->trimming()->join(staging_table, colname,
                                                  TRIMMED);

  return sql.str();
}

// ----------------------------------------------------

std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
CategoryTrimmer::fit_transform(const Params& _params) {
  const auto fit_peripheral = [this](const auto& _df) {
    return fit_df(_df, MarkerType::make<"[PERIPHERAL]">());
  };

  population_sets_ =
      fit_df(_params.population_df(), MarkerType::make<"[POPULATION]">());

  peripheral_sets_ = _params.peripheral_dfs() |
                     std::views::transform(fit_peripheral) |
                     fct::ranges::to<std::vector>();

  (*_params.categories())[strings::String(TRIMMED)];

  const auto logging_begin =
      (_params.logging_begin() + _params.logging_end()) / 2;

  const auto params =
      rfl::replace(_params, rfl::make_field<"logging_begin_">(logging_begin));

  return transform(params);
}

// ----------------------------------------------------

std::vector<typename CategoryTrimmer::CategoryPair> CategoryTrimmer::fit_df(
    const containers::DataFrame& _df, const MarkerType _marker) const {
  const auto include = [](const auto& _col) -> bool {
    const auto blacklist = std::vector<helpers::Subrole>(
        {helpers::Subrole::exclude_preprocessors, helpers::Subrole::email_only,
         helpers::Subrole::substring_only,
         helpers::Subrole::exclude_category_trimmer});
    return !helpers::SubroleParser::contains_any(_col.subroles(), blacklist);
  };

  const auto to_column_description = [&_df, &_marker](const auto& _col) {
    return helpers::ColumnDescription(_marker, _df.name(), _col.name());
  };

  const auto to_set = [this](const auto& _col) {
    return make_category_set(_col);
  };

  const auto to_pair = [to_column_description,
                        to_set](const auto& _col) -> CategoryPair {
    return std::make_pair(to_column_description(_col), to_set(_col));
  };

  return _df.categoricals() | std::views::filter(include) |
         std::views::transform(to_pair) | fct::ranges::to<std::vector>();
}

// ----------------------------------------------------

void CategoryTrimmer::load(const std::string& _fname) {
  const auto named_tuple = helpers::Loader::load<ReflectionType>(_fname);
  peripheral_sets_ = named_tuple.peripheral_sets();
  population_sets_ = named_tuple.population_sets();
}

// ----------------------------------------------------

rfl::Ref<const std::set<Int>> CategoryTrimmer::make_category_set(
    const containers::Column<Int>& _col) const {
  using Pair = std::pair<Int, size_t>;

  const auto counts = make_counts(_col);

  const auto count_greater_than_min_freq = [this](const Pair& p) -> bool {
    return p.second >= min_freq_;
  };

  const auto get_first = [](const Pair& p) -> Int { return p.first; };

  const auto range = counts | std::views::filter(count_greater_than_min_freq) |
                     std::views::transform(get_first) |
                     std::views::take(max_num_categories_) |
                     fct::ranges::to<std::set>();

  return rfl::Ref<const std::set<Int>>::make(range);
}

// ----------------------------------------------------

std::vector<std::pair<Int, size_t>> CategoryTrimmer::make_counts(
    const containers::Column<Int>& _col) const {
  const auto count_map = make_map(_col);

  using Pair = std::pair<Int, size_t>;

  auto count_vec = std::vector<Pair>(count_map.begin(), count_map.end());

  const auto by_count = [](const Pair& p1, const Pair& p2) -> bool {
    return p1.second > p2.second;
  };

  std::sort(count_vec.begin(), count_vec.end(), by_count);

  return count_vec;
}

// ----------------------------------------------------

std::map<Int, size_t> CategoryTrimmer::make_map(
    const containers::Column<Int>& _col) const {
  std::map<Int, size_t> count_map;

  for (const auto val : _col) {
    if (helpers::NullChecker::is_null(val)) {
      continue;
    }

    const auto it = count_map.find(val);

    if (it == count_map.end()) {
      count_map[val] = 1;
    } else {
      it->second++;
    }
  }

  return count_map;
}

// ----------------------------------------------------

void CategoryTrimmer::save(
    const std::string& _fname,
    const typename helpers::Saver::Format& _format) const {
  helpers::Saver::save(_fname, *this, _format);
}

// ----------------------------------------------------

std::vector<std::string> CategoryTrimmer::to_sql(
    const helpers::StringIterator& _categories,
    const std::shared_ptr<const transpilation::SQLDialectGenerator>&
        _sql_dialect_generator) const {
  assert_true(_sql_dialect_generator);

  std::vector<std::string> sql_code;

  for (const auto& pair : population_sets_) {
    sql_code.push_back(
        column_to_sql(_categories, _sql_dialect_generator, pair));
  }

  for (const auto& vec : peripheral_sets_) {
    for (const auto& pair : vec) {
      sql_code.push_back(
          column_to_sql(_categories, _sql_dialect_generator, pair));
    }
  }

  return sql_code;
}

// ----------------------------------------------------

std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
CategoryTrimmer::transform(const Params& _params) const {
  assert_true(peripheral_sets_.size() == _params.peripheral_dfs().size());

  const auto pool = _params.population_df().pool()
                        ? std::make_shared<memmap::Pool>(
                              _params.population_df().pool()->temp_dir())
                        : std::shared_ptr<memmap::Pool>();

  const auto population_df = transform_df(
      population_sets_, pool, _params.categories(), _params.population_df());

  const auto make_peripheral_df =
      [this, &_params, pool](const size_t _i) -> containers::DataFrame {
    return transform_df(peripheral_sets_.at(_i), pool, _params.categories(),
                        _params.peripheral_dfs().at(_i));
  };

  const auto peripheral_dfs = std::views::iota(0uz, peripheral_sets_.size()) |
                              std::views::transform(make_peripheral_df) |
                              fct::ranges::to<std::vector>();

  return std::make_pair(population_df, peripheral_dfs);
}

// ----------------------------------------------------

containers::DataFrame CategoryTrimmer::transform_df(
    const std::vector<CategoryPair>& _sets,
    const std::shared_ptr<memmap::Pool>& _pool,
    const rfl::Ref<const containers::Encoding>& _categories,
    const containers::DataFrame& _df) const {
  using Set = CategoryPair::second_type;

  using ColumnAndSet = std::pair<containers::Column<Int>, Set>;

  const auto trimmed = (*_categories)[strings::String(TRIMMED)];

  const auto get_col = [&_df](const auto& _pair) -> ColumnAndSet {
    return std::make_pair(_df.categorical(_pair.first.name()), _pair.second);
  };

  const auto trim_value = [trimmed](const Int _value, const Set& _set) -> Int {
    if (_set->find(_value) == _set->end()) {
      return trimmed;
    }
    return _value;
  };

  const auto make_trimmed_column =
      [_pool,
       trim_value](const ColumnAndSet& _pair) -> containers::Column<Int> {
    const auto& orig_col = _pair.first;

    auto new_col = containers::Column<Int>(_pool, orig_col.nrows());

    new_col.set_name(orig_col.name());
    new_col.set_subroles(orig_col.subroles());
    new_col.set_unit(orig_col.unit());

    for (size_t i = 0; i < new_col.nrows(); ++i) {
      new_col[i] = trim_value(orig_col[i], _pair.second);
    }

    return new_col;
  };

  const auto trimmed_columns = _sets | std::views::transform(get_col) |
                               std::views::transform(make_trimmed_column);

  auto df = _df;

  for (const auto col : trimmed_columns) {
    df.add_int_column(col, containers::DataFrame::ROLE_CATEGORICAL);
  }

  return df;
}

// ----------------------------------------------------
}  // namespace preprocessors
}  // namespace engine
