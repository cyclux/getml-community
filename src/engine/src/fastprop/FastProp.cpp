// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "fastprop/algorithm/FastProp.hpp"

#include <random>

#include "fastprop/algorithm/Aggregator.hpp"
#include "fastprop/algorithm/ConditionParser.hpp"
#include "fastprop/algorithm/RSquared.hpp"
#include "fastprop/algorithm/TableHolderParams.hpp"
#include "fct/to.hpp"
#include "helpers/Matchmaker.hpp"
#include "transpilation/HumanReadableSQLGenerator.hpp"

namespace fastprop {
namespace algorithm {

FastProp::FastProp(
    const std::shared_ptr<const Hyperparameters> &_hyperparameters,
    const std::shared_ptr<const std::vector<std::string>> &_peripheral,
    const std::shared_ptr<const containers::Placeholder> &_placeholder)
    : allow_http_(false),
      comm_(nullptr),
      hyperparameters_(_hyperparameters),
      peripheral_(_peripheral),
      placeholder_(_placeholder) {
  if (placeholder_) {
    placeholder().check_data_model(peripheral(), true);
  }
}

// ---------------------------------------------------------------------------

FastProp::FastProp(const ReflectionType &_val)
    : abstract_features_(_val.get<"features_">()),
      allow_http_(_val.get<"allow_http_">()),
      hyperparameters_(_val.get<"hyperparameters_">()),
      main_table_schemas_(_val.get<"main_table_schemas_">()),
      peripheral_(_val.get<"peripheral_">()),
      peripheral_schema_(_val.get<"peripheral_schema_">()),
      peripheral_table_schemas_(_val.get<"peripheral_table_schemas_">()),
      placeholder_(_val.get<"placeholder_">()),
      population_schema_(_val.get<"population_schema_">()),
      subfeatures_(_val.get<"subfeatures_">()) {}

// ---------------------------------------------------------------------------

void FastProp::build_row(
    const TableHolder &_table_holder,
    const std::vector<containers::Features> &_subfeatures,
    const std::vector<size_t> &_index,
    const std::vector<std::function<bool(const containers::Match &)>>
        &_condition_functions,
    const size_t _rownum, const rfl::Ref<Memoization> &_memoization,
    Float *_row) const {
  assert_true(_condition_functions.size() == _index.size());

  const auto all_matches = make_matches(_table_holder, _rownum);

  assert_true(all_matches.size() == _table_holder.peripheral_tables().size());

  assert_true(_table_holder.main_tables().size() ==
              _table_holder.peripheral_tables().size());

  assert_true(_subfeatures.size() <= _table_holder.peripheral_tables().size());

  for (size_t i = 0; i < _index.size(); ++i) {
    const auto ix = _index.at(i);

    assert_true(ix < abstract_features().size());

    const auto &abstract_feature = abstract_features().at(ix);

    assert_true(abstract_feature.peripheral_ <
                _table_holder.peripheral_tables().size());

    const auto &population =
        _table_holder.main_tables().at(abstract_feature.peripheral_).df();

    const auto &peripheral =
        _table_holder.peripheral_tables().at(abstract_feature.peripheral_);

    const auto subf = abstract_feature.peripheral_ < _subfeatures.size()
                          ? _subfeatures.at(abstract_feature.peripheral_)
                          : std::optional<containers::Features>();

    const auto &matches = all_matches.at(abstract_feature.peripheral_);

    const auto &condition_function = _condition_functions.at(i);

    const auto value = Aggregator::apply_aggregation(
        population, peripheral, subf, matches, condition_function,
        abstract_feature, _memoization);

    _row[i] = (std::isnan(value) || std::isinf(value)) ? 0.0 : value;
  }
}

// ----------------------------------------------------------------------------

void FastProp::build_rows(const TransformParams &_params,
                          const std::vector<containers::Features> &_subfeatures,
                          const std::shared_ptr<std::vector<size_t>> &_rownums,
                          const size_t _thread_num,
                          std::atomic<size_t> *_num_completed,
                          containers::Features *_features) const {
  if (_features->size() == 0) {
    return;
  }

  const auto rownums =
      make_rownums(_thread_num, _params.population_.nrows(), _rownums);

  const auto population_view =
      containers::DataFrameView(_params.population_, rownums);

  const auto make_staging_table_colname =
      [](const std::string &_colname) -> std::string {
    return transpilation::HumanReadableSQLGenerator()
        .make_staging_table_colname(_colname);
  };

  const auto params = TableHolderParams{
      .feature_container_ = std::nullopt,
      .make_staging_table_colname_ = make_staging_table_colname,
      .peripheral_ = _params.peripheral_,
      .peripheral_names_ = peripheral(),
      .placeholder_ = placeholder(),
      .population_ = population_view,
      .row_index_container_ = std::nullopt,
      .word_index_container_ = _params.word_indices_};

  const auto table_holder = TableHolder(params);

  const auto memoization = rfl::Ref<Memoization>::make();

  constexpr size_t log_iter = 5000;

  const auto condition_functions = ConditionParser::make_condition_functions(
      table_holder, _params.index_, abstract_features());

  const auto nrows = _rownums ? _rownums->size() : _params.population_.nrows();

  assert_true(_features->size() == _params.index_.size());

  const auto ncols = _features->size();

  const auto cache_size = std::min(log_iter, rownums->size());

  auto cache = std::vector<Float>(cache_size * ncols);

  for (size_t i = 0; i < rownums->size(); ++i) {
    if (i % log_iter == 0 && i != 0) {
      cache_to_features(*rownums, i - log_iter, cache, _features);

      _num_completed->fetch_add(log_iter);

      if (_thread_num == 0) {
        log_progress(_params.logger_, nrows, _num_completed->load());
      }
    }

    memoization->reset();

    assert_msg(ncols * (i % log_iter) < cache.size(),
               "ncols: " + std::to_string(ncols) + ", i: " + std::to_string(i) +
                   ", log_iter: " + std::to_string(log_iter) +
                   ", cache.size(): " + std::to_string(cache.size()));

    build_row(table_holder, _subfeatures, _params.index_, condition_functions,
              (*rownums)[i], memoization, &cache[ncols * (i % log_iter)]);
  }

  const size_t begin = rownums->size() > log_iter
                           ? rownums->size() - (rownums->size() % log_iter)
                           : static_cast<size_t>(0);

  cache_to_features(*rownums, begin, cache, _features);

  _num_completed->fetch_add(nrows % log_iter);
}

// ----------------------------------------------------------------------------

std::vector<containers::Features> FastProp::build_subfeatures(
    const TransformParams &_params,
    const std::shared_ptr<std::vector<size_t>> &_rownums) const {
  assert_true(placeholder().joined_tables().size() <= subfeatures().size());

  std::vector<containers::Features> features;

  for (size_t i = 0; i < subfeatures().size(); ++i) {
    if (!subfeatures().at(i)) {
      features.push_back(containers::Features(0, 0, std::nullopt));
      continue;
    }

    assert_true(i < placeholder().joined_tables().size());

    const auto joined_table = placeholder().joined_tables().at(i);

    const auto new_population =
        find_peripheral(_params.peripheral_, joined_table.name());

    const auto subfeature_index = make_subfeature_index(i, _params.index_);

    const auto subfeature_rownums = make_subfeature_rownums(
        _rownums, _params.population_, new_population, i);

    const auto ix = find_peripheral_ix(joined_table.name());

    assert_true(ix < _params.word_indices_.peripheral().size());

    const auto new_word_indices =
        helpers::WordIndexContainer(_params.word_indices_.peripheral().at(ix),
                                    _params.word_indices_.peripheral());

    const auto params = TransformParams{.feature_container_ = std::nullopt,
                                        .index_ = subfeature_index,
                                        .logger_ = _params.logger_,
                                        .peripheral_ = _params.peripheral_,
                                        .population_ = new_population,
                                        .temp_dir_ = _params.temp_dir_,
                                        .word_indices_ = new_word_indices};

    const auto f =
        subfeatures().at(i)->transform(params, subfeature_rownums, true);

    const auto f_expanded = expand_subfeatures(
        f, subfeature_index, subfeatures().at(i)->num_features());

    features.push_back(f_expanded);
  }

  return features;
}

// ----------------------------------------------------------------------------

void FastProp::cache_to_features(const std::vector<size_t> &_rownums,
                                 const size_t _begin,
                                 const std::vector<Float> &_cache,
                                 containers::Features *_features) const {
  const size_t ncols = _features->size();

  const size_t nrows =
      std::min(_cache.size() / ncols, _rownums.size() - _begin);

  for (size_t j = 0; j < ncols; ++j) {
    for (size_t i = 0; i < nrows; ++i) {
      assert_true(_begin + i < _rownums.size());
      assert_true(i * ncols + j < _cache.size());

      const auto rownum = _rownums[_begin + i];

      assert_true(rownum < _features->at(j).size());

      _features->at(rownum, j) = _cache[i * ncols + j];
    }
  }
}

// ----------------------------------------------------------------------------

std::vector<Float> FastProp::calc_r_squared(
    const FitParams &_params,
    const std::shared_ptr<std::vector<size_t>> &_rownums) const {
  assert_true(_rownums);

  auto r_squared = std::vector<Float>();

  constexpr size_t batch_size = 100;

  for (size_t begin = 0; begin < abstract_features().size();
       begin += batch_size) {
    const auto end = std::min(abstract_features().size(), begin + batch_size);

    const auto index =
        std::views::iota(begin, end) | fct::ranges::to<std::vector>();

    const auto params = TransformParams{.feature_container_ = std::nullopt,
                                        .index_ = index,
                                        .logger_ = nullptr,
                                        .peripheral_ = _params.peripheral_,
                                        .population_ = _params.population_,
                                        .temp_dir_ = _params.temp_dir_,
                                        .word_indices_ = _params.word_indices_};

    const auto features = transform(params, _rownums, false);

    const auto r =
        RSquared::calculate(_params.population_.targets_, features, *_rownums);

    r_squared.insert(r_squared.end(), r.begin(), r.end());

    if (_params.logger_) {
      const auto progress =
          std::to_string((end * 100) / abstract_features().size());

      _params.logger_->log("Built " + std::to_string(end) +
                           " features. Progress: " + progress + "%.");
    }
  }

  return r_squared;
}

// ----------------------------------------------------------------------------

Float FastProp::calc_threshold(const std::vector<Float> &_r_squared) const {
  auto r_squared = _r_squared;
  std::ranges::sort(r_squared, std::ranges::greater());
  assert_true(r_squared.size() > hyperparameters().num_features());
  return r_squared.at(hyperparameters().num_features());
}

// ----------------------------------------------------------------------------

std::map<helpers::ColumnDescription, Float> FastProp::column_importances(
    const std::vector<Float> &_importance_factors,
    const bool _is_subfeatures) const {
  auto importances = helpers::ImportanceMaker();

  auto subimportance_factors = init_subimportance_factors();

  for (size_t i = 0; i < _importance_factors.size(); ++i) {
    const auto pairs =
        infer_importance(i, _importance_factors.at(i), &subimportance_factors);

    for (const auto &p : pairs) {
      importances.add_to_importances(p.first, p.second);
    }
  }

  for (size_t i = 0; i < subfeatures().size(); ++i) {
    if (subfeatures().at(i)) {
      const auto subimportances = subfeatures().at(i)->column_importances(
          subimportance_factors.at(i), true);

      for (const auto &p : subimportances) {
        importances.add_to_importances(p.first, p.second);
      }
    }
  }

  if (_is_subfeatures) {
    importances.transfer_population();
  }

  return importances.importances();
}

// ----------------------------------------------------------------------------

containers::Features FastProp::expand_subfeatures(
    const containers::Features &_subfeatures,
    const std::vector<size_t> &_subfeature_index,
    const size_t _num_subfeatures) const {
  assert_true(_subfeatures.size() == _subfeature_index.size());

  std::vector<helpers::Feature<Float, false>> expanded_subfeatures(
      _num_subfeatures);

  for (size_t i = 0; i < _subfeatures.size(); ++i) {
    const auto ix = _subfeature_index.at(i);

    assert_true(ix < expanded_subfeatures.size());

    expanded_subfeatures.at(ix) = _subfeatures.at(i);
  }

  return helpers::Features(expanded_subfeatures);
}

// ----------------------------------------------------------------------------

void FastProp::extract_schemas(
    const containers::DataFrame &_population,
    const std::vector<containers::DataFrame> &_peripheral) {
  population_schema_ =
      std::make_shared<const helpers::Schema>(_population.to_schema());

  auto peripheral_schema = std::make_shared<std::vector<helpers::Schema>>();

  for (auto &df : _peripheral) {
    peripheral_schema->push_back(df.to_schema());
  }

  peripheral_schema_ = peripheral_schema;
}

// ----------------------------------------------------------------------------

void FastProp::extract_schemas(const TableHolder &_table_holder) {
  assert_true(_table_holder.main_tables().size() ==
              _table_holder.peripheral_tables().size());

  const auto main_table_schemas =
      std::make_shared<std::vector<helpers::Schema>>();

  const auto peripheral_table_schemas =
      std::make_shared<std::vector<helpers::Schema>>();

  for (size_t i = 0; i < _table_holder.main_tables().size(); ++i) {
    main_table_schemas->push_back(
        _table_holder.main_tables().at(i).df().to_schema());

    peripheral_table_schemas->push_back(
        _table_holder.peripheral_tables().at(i).to_schema());
  }

  main_table_schemas_ = main_table_schemas;

  peripheral_table_schemas_ = peripheral_table_schemas;
}

// ----------------------------------------------------------------------------

std::vector<Int> FastProp::find_most_frequent_categories(
    const containers::Column<Int> &_col) const {
  std::map<Int, size_t> frequencies;

  for (size_t i = 0; i < _col.nrows_; ++i) {
    const auto val = _col[i];

    const auto it = frequencies.find(val);

    if (it == frequencies.end()) {
      frequencies[val] = 1;
    } else {
      it->second++;
    }
  }

  using Pair = std::pair<Int, size_t>;

  const auto sort_by_second = [](const Pair p1, const Pair p2) {
    return p1.second > p2.second;
  };

  auto pairs = std::vector<Pair>(frequencies.begin(), frequencies.end());

  std::ranges::sort(pairs, sort_by_second);

  const auto is_not_null = [](const Int val) -> bool { return val >= 0; };

  return pairs | std::views::keys | std::views::filter(is_not_null) |
         std::views::take(hyperparameters().n_most_frequent()) |
         fct::ranges::to<std::vector>();
}

// ----------------------------------------------------------------------------

containers::DataFrame FastProp::find_peripheral(
    const std::vector<containers::DataFrame> &_peripheral,
    const std::string &_name) const {
  if (_peripheral.size() < peripheral().size()) {
    throw std::runtime_error(
        "The number of peripheral tables does not match the number "
        "of "
        "peripheral placeholders.");
  }

  const auto ix = find_peripheral_ix(_name);

  assert_true(ix < _peripheral.size());

  return _peripheral.at(ix);
}

// ----------------------------------------------------------------------------

size_t FastProp::find_peripheral_ix(const std::string &_name) const {
  for (size_t i = 0; i < peripheral().size(); ++i) {
    if (peripheral().at(i) == _name) {
      return i;
    }
  }

  throw std::runtime_error("Placeholder named '" + _name + "' not found.");

  return 0;
}

// ----------------------------------------------------------------------------

void FastProp::fit(const FitParams &_params, const bool _as_subfeatures) {
  extract_schemas(_params.population_, _params.peripheral_);

  const auto rownums = sample_from_population(_params.population_.nrows());

  const auto population_view =
      containers::DataFrameView(_params.population_, rownums);

  const auto make_staging_table_colname =
      [](const std::string &_colname) -> std::string {
    return transpilation::HumanReadableSQLGenerator()
        .make_staging_table_colname(_colname);
  };

  const auto params = TableHolderParams{
      .feature_container_ = std::nullopt,
      .make_staging_table_colname_ = make_staging_table_colname,
      .peripheral_ = _params.peripheral_,
      .peripheral_names_ = peripheral(),
      .placeholder_ = placeholder(),
      .population_ = population_view,
      .row_index_container_ = std::nullopt,
      .word_index_container_ = _params.word_indices_};

  const auto table_holder = TableHolder(params);

  extract_schemas(table_holder);

  subfeatures_ = fit_subfeatures(_params, table_holder);

  const auto conditions = make_conditions(table_holder);

  assert_true(table_holder.main_tables().size() ==
              table_holder.peripheral_tables().size());

  assert_true(table_holder.main_tables().size() >=
              placeholder().joined_tables().size());

  const auto abstract_features =
      std::make_shared<std::vector<containers::AbstractFeature>>();

  for (size_t i = 0; i < table_holder.main_tables().size(); ++i) {
    fit_on_peripheral(table_holder.main_tables().at(i).df(),
                      table_holder.peripheral_tables().at(i), i, conditions,
                      abstract_features);
  }

  abstract_features_ = abstract_features;

  if (_params.logger_ && !_as_subfeatures) {
    const auto num_candidates = std::to_string(abstract_features->size());

    const auto msg = "FastProp: Trying " + num_candidates + " features...";

    _params.logger_->log(msg);
  }

  if (!_as_subfeatures) {
    abstract_features_ = select_features(_params, rownums);
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_categoricals(
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<containers::Condition> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  assert_true(_abstract_features);

  for (size_t input_col = 0; input_col < _peripheral.num_categoricals();
       ++input_col) {
    if (_peripheral.categorical_unit(input_col).find("comparison only") !=
        std::string::npos) {
      continue;
    }

    const auto condition_is_categorical =
        [](const containers::Condition &_cond) {
          return _cond.data_used_.value() ==
                 enums::DataUsed::value_of<"categorical">();
        };

    const auto any_condition_is_categorical = std::any_of(
        _conditions.begin(), _conditions.end(), condition_is_categorical);

    if (any_condition_is_categorical) {
      continue;
    }

    for (const auto &agg : hyperparameters().aggregation()) {
      if (!is_categorical(agg)) {
        continue;
      }

      _abstract_features->push_back(containers::AbstractFeature(
          agg, _conditions, enums::DataUsed::make<"categorical">(), input_col,
          _peripheral_ix));
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_categoricals_by_categories(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<containers::Condition> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  assert_true(_abstract_features);

  for (size_t input_col = 0; input_col < _peripheral.num_categoricals();
       ++input_col) {
    if (_peripheral.categorical_unit(input_col).find("comparison only") !=
        std::string::npos) {
      continue;
    }

    const auto condition_is_categorical =
        [](const containers::Condition &_cond) {
          return _cond.data_used_.value() ==
                 enums::DataUsed::value_of<"categorical">();
        };

    const auto any_condition_is_categorical = std::any_of(
        _conditions.begin(), _conditions.end(), condition_is_categorical);

    if (any_condition_is_categorical) {
      continue;
    }

    const auto most_frequent =
        find_most_frequent_categories(_peripheral.categorical_col(input_col));

    for (const auto categorical_value : most_frequent) {
      for (const auto &agg : hyperparameters().aggregation()) {
        if (!is_numerical(agg)) {
          continue;
        }

        if (skip_first_last(agg, _population, _peripheral)) {
          continue;
        }

        _abstract_features->push_back(containers::AbstractFeature(
            agg, _conditions, input_col, _peripheral_ix,
            enums::DataUsed::make<"categorical">(), categorical_value));
      }
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_discretes(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<containers::Condition> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  assert_true(_abstract_features);

  for (size_t input_col = 0; input_col < _peripheral.num_discretes();
       ++input_col) {
    for (const auto &agg : hyperparameters().aggregation()) {
      if (!is_numerical(agg)) {
        continue;
      }

      if (_peripheral.discrete_unit(input_col).find("comparison only") !=
          std::string::npos) {
        continue;
      }

      if (skip_first_last(agg, _population, _peripheral)) {
        continue;
      }

      _abstract_features->push_back(containers::AbstractFeature(
          agg, _conditions, enums::DataUsed::make<"discrete">(), input_col,
          _peripheral_ix));
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_numericals(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<containers::Condition> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  assert_true(_abstract_features);

  for (size_t input_col = 0; input_col < _peripheral.num_numericals();
       ++input_col) {
    for (const auto &agg : hyperparameters().aggregation()) {
      if (!is_numerical(agg)) {
        continue;
      }

      if (_peripheral.numerical_unit(input_col).find("comparison only") !=
          std::string::npos) {
        continue;
      }

      if (skip_first_last(agg, _population, _peripheral)) {
        continue;
      }

      _abstract_features->push_back(containers::AbstractFeature(
          agg, _conditions, enums::DataUsed::make<"numerical">(), input_col,
          _peripheral_ix));
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_same_units_categorical(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<containers::Condition> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  assert_true(_abstract_features);

  for (size_t output_col = 0; output_col < _population.num_categoricals();
       ++output_col) {
    for (size_t input_col = 0; input_col < _peripheral.num_categoricals();
         ++input_col) {
      const bool same_unit = _population.categorical_unit(output_col) != "" &&
                             _population.categorical_unit(output_col) ==
                                 _peripheral.categorical_unit(input_col);

      if (!same_unit) {
        continue;
      }

      for (const auto &agg : hyperparameters().aggregation()) {
        if (!is_numerical(agg)) {
          continue;
        }

        if (skip_first_last(agg, _population, _peripheral)) {
          continue;
        }

        _abstract_features->push_back(containers::AbstractFeature(
            agg, _conditions, enums::DataUsed::make<"same_units_categorical">(),
            input_col, output_col, _peripheral_ix));
      }
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_same_units_discrete(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<containers::Condition> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  assert_true(_abstract_features);

  for (size_t output_col = 0; output_col < _population.num_discretes();
       ++output_col) {
    for (size_t input_col = 0; input_col < _peripheral.num_discretes();
         ++input_col) {
      for (const auto &agg : hyperparameters().aggregation()) {
        const bool same_unit = _population.discrete_unit(output_col) != "" &&
                               _population.discrete_unit(output_col) ==
                                   _peripheral.discrete_unit(input_col);

        if (!same_unit) {
          continue;
        }

        if (!is_numerical(agg)) {
          continue;
        }

        if (skip_first_last(agg, _population, _peripheral)) {
          continue;
        }

        const auto data_used =
            is_ts(_population.discrete_name(output_col),
                  _population.discrete_unit(output_col))
                ? enums::DataUsed::make<"same_units_discrete_ts">()
                : enums::DataUsed::make<"same_units_discrete">();

        _abstract_features->push_back(
            containers::AbstractFeature(agg, _conditions, data_used, input_col,
                                        output_col, _peripheral_ix));
      }
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_same_units_numerical(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<containers::Condition> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  assert_true(_abstract_features);

  for (size_t output_col = 0; output_col < _population.num_numericals();
       ++output_col) {
    for (size_t input_col = 0; input_col < _peripheral.num_numericals();
         ++input_col) {
      for (const auto &agg : hyperparameters().aggregation()) {
        const bool same_unit = _population.numerical_unit(output_col) != "" &&
                               _population.numerical_unit(output_col) ==
                                   _peripheral.numerical_unit(input_col);

        if (!same_unit) {
          continue;
        }

        if (!is_numerical(agg)) {
          continue;
        }

        if (skip_first_last(agg, _population, _peripheral)) {
          continue;
        }

        const auto data_used =
            is_ts(_population.numerical_name(output_col),
                  _population.numerical_unit(output_col))
                ? enums::DataUsed::make<"same_units_numerical_ts">()
                : enums::DataUsed::make<"same_units_numerical">();

        _abstract_features->push_back(
            containers::AbstractFeature(agg, _conditions, data_used, input_col,
                                        output_col, _peripheral_ix));
      }
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_subfeatures(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<containers::Condition> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  if (_peripheral_ix >= subfeatures().size()) {
    return;
  }

  if (!subfeatures().at(_peripheral_ix)) {
    return;
  }

  assert_true(_abstract_features);

  for (size_t input_col = 0;
       input_col < subfeatures().at(_peripheral_ix)->num_features();
       ++input_col) {
    for (const auto &agg : hyperparameters().aggregation()) {
      if (!is_numerical(agg)) {
        continue;
      }

      if (skip_first_last(agg, _population, _peripheral)) {
        continue;
      }

      _abstract_features->push_back(containers::AbstractFeature(
          agg, _conditions, enums::DataUsed::make<"subfeatures">(), input_col,
          _peripheral_ix));
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::fit_on_peripheral(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    const std::vector<std::vector<containers::Condition>> &_conditions,
    std::shared_ptr<std::vector<containers::AbstractFeature>>
        _abstract_features) const {
  const auto condition_filter = make_condition_filter(_peripheral_ix);

  auto filtered_conditions = _conditions | std::views::filter(condition_filter);

  for (const auto &cond : filtered_conditions) {
    fit_on_categoricals(_peripheral, _peripheral_ix, cond, _abstract_features);

    fit_on_categoricals_by_categories(_population, _peripheral, _peripheral_ix,
                                      cond, _abstract_features);

    fit_on_discretes(_population, _peripheral, _peripheral_ix, cond,
                     _abstract_features);

    fit_on_numericals(_population, _peripheral, _peripheral_ix, cond,
                      _abstract_features);

    fit_on_same_units_categorical(_population, _peripheral, _peripheral_ix,
                                  cond, _abstract_features);

    fit_on_same_units_discrete(_population, _peripheral, _peripheral_ix, cond,
                               _abstract_features);

    fit_on_same_units_numerical(_population, _peripheral, _peripheral_ix, cond,
                                _abstract_features);

    fit_on_subfeatures(_population, _peripheral, _peripheral_ix, cond,
                       _abstract_features);

    if (_peripheral.num_time_stamps() > 0) {
      _abstract_features->push_back(containers::AbstractFeature(
          enums::Aggregation::make<"AVG TIME BETWEEN">(), cond,
          enums::DataUsed::make<"na">(), 0, _peripheral_ix));
    }
  }

  if (has_count()) {
    _abstract_features->push_back(containers::AbstractFeature(
        enums::Aggregation::make<"COUNT">(), {}, enums::DataUsed::make<"na">(),
        0, _peripheral_ix));
  }
}

// ----------------------------------------------------------------------------

std::shared_ptr<const std::vector<std::optional<FastProp>>>
FastProp::fit_subfeatures(const FitParams &_params,
                          const TableHolder &_table_holder) const {
  assert_true(placeholder().joined_tables().size() <=
              _table_holder.subtables().size());

  const auto subfeatures =
      std::make_shared<std::vector<std::optional<FastProp>>>();

  for (size_t i = 0; i < placeholder().joined_tables().size(); ++i) {
    const auto &joined_table = placeholder().joined_tables().at(i);

    if (!_table_holder.subtables().at(i)) {
      subfeatures->push_back(std::nullopt);
      continue;
    }

    subfeatures->push_back(std::make_optional<FastProp>(
        hyperparameters_, peripheral_,
        std::make_shared<const containers::Placeholder>(joined_table)));

    const auto new_population =
        find_peripheral(_params.peripheral_, joined_table.name());

    const auto ix = find_peripheral_ix(joined_table.name());

    assert_true(ix < _params.row_indices_.peripheral().size());

    const auto new_row_indices =
        helpers::RowIndexContainer(_params.row_indices_.peripheral().at(ix),
                                   _params.row_indices_.peripheral());

    assert_true(ix < _params.word_indices_.peripheral().size());

    const auto new_word_indices =
        helpers::WordIndexContainer(_params.word_indices_.peripheral().at(ix),
                                    _params.word_indices_.peripheral());

    const auto params = FitParams{.feature_container_ = std::nullopt,
                                  .logger_ = _params.logger_,
                                  .peripheral_ = _params.peripheral_,
                                  .population_ = new_population,
                                  .row_indices_ = new_row_indices,
                                  .temp_dir_ = _params.temp_dir_,
                                  .word_indices_ = new_word_indices};

    subfeatures->back()->fit(params, true);
  }

  return subfeatures;
}

// ----------------------------------------------------------------------------

size_t FastProp::get_num_threads() const {
  const auto num_threads = hyperparameters().num_threads();

  if (num_threads <= 0) {
    return std::max(
        static_cast<size_t>(2),
        static_cast<size_t>(std::thread::hardware_concurrency()) / 2);
  }

  return static_cast<size_t>(num_threads);
}

// ----------------------------------------------------------------------------

std::vector<std::pair<helpers::ColumnDescription, Float>>
FastProp::infer_importance(
    const size_t _feature_num, const Float _importance_factor,
    std::vector<std::vector<Float>> *_subimportance_factors) const {
  assert_true(_feature_num < abstract_features().size());

  const auto abstract_feature = abstract_features().at(_feature_num);

  assert_true(_subimportance_factors->size() <=
              peripheral_table_schemas().size());

  assert_true(abstract_feature.peripheral_ < peripheral_table_schemas().size());

  const auto &population =
      main_table_schemas().at(abstract_feature.peripheral_);

  const auto &peripheral =
      peripheral_table_schemas().at(abstract_feature.peripheral_);

  switch (abstract_feature.data_used_.value()) {
    case enums::DataUsed::value_of<"categorical">(): {
      const auto col_desc = helpers::ColumnDescription(
          MarkerType::make<"[PERIPHERAL]">(), peripheral.name(),
          peripheral.categorical_name(abstract_feature.input_col_));

      return {std::make_pair(col_desc, _importance_factor)};
    }

    case enums::DataUsed::value_of<"discrete">(): {
      const auto col_desc = helpers::ColumnDescription(
          MarkerType::make<"[PERIPHERAL]">(), peripheral.name(),
          peripheral.discrete_name(abstract_feature.input_col_));

      return {std::make_pair(col_desc, _importance_factor)};
    }

    case enums::DataUsed::value_of<"na">():
      return std::vector<std::pair<helpers::ColumnDescription, Float>>();

    case enums::DataUsed::value_of<"numerical">(): {
      const auto col_desc = helpers::ColumnDescription(
          MarkerType::make<"[PERIPHERAL]">(), peripheral.name(),
          peripheral.numerical_name(abstract_feature.input_col_));

      return {std::make_pair(col_desc, _importance_factor)};
    }

    case enums::DataUsed::value_of<"same_units_categorical">(): {
      const auto col_desc1 = helpers::ColumnDescription(
          MarkerType::make<"[PERIPHERAL]">(), peripheral.name(),
          peripheral.categorical_name(abstract_feature.input_col_));

      const auto col_desc2 = helpers::ColumnDescription(
          MarkerType::make<"[POPULATION]">(), population.name(),
          population.categorical_name(abstract_feature.output_col_));

      return {std::make_pair(col_desc1, _importance_factor * 0.5),
              std::make_pair(col_desc2, _importance_factor * 0.5)};
    }

    case enums::DataUsed::value_of<"same_units_discrete">():
    case enums::DataUsed::value_of<"same_units_discrete_ts">(): {
      const auto col_desc1 = helpers::ColumnDescription(
          MarkerType::make<"[PERIPHERAL]">(), peripheral.name(),
          peripheral.discrete_name(abstract_feature.input_col_));

      const auto col_desc2 = helpers::ColumnDescription(
          MarkerType::make<"[POPULATION]">(), population.name(),
          population.discrete_name(abstract_feature.output_col_));

      return {std::make_pair(col_desc1, _importance_factor * 0.5),
              std::make_pair(col_desc2, _importance_factor * 0.5)};
    }

    case enums::DataUsed::value_of<"same_units_numerical">():
    case enums::DataUsed::value_of<"same_units_numerical_ts">(): {
      const auto col_desc1 = helpers::ColumnDescription(
          MarkerType::make<"[PERIPHERAL]">(), peripheral.name(),
          peripheral.numerical_name(abstract_feature.input_col_));

      const auto col_desc2 = helpers::ColumnDescription(
          MarkerType::make<"[POPULATION]">(), population.name(),
          population.numerical_name(abstract_feature.output_col_));

      return {std::make_pair(col_desc1, _importance_factor * 0.5),
              std::make_pair(col_desc2, _importance_factor * 0.5)};
    }

    case enums::DataUsed::value_of<"subfeatures">(): {
      assert_true(
          abstract_feature.input_col_ <
          _subimportance_factors->at(abstract_feature.peripheral_).size());

      _subimportance_factors->at(abstract_feature.peripheral_)
          .at(abstract_feature.input_col_) += _importance_factor;

      return std::vector<std::pair<helpers::ColumnDescription, Float>>();
    }

    case enums::DataUsed::value_of<"text">(): {
      const auto col_desc = helpers::ColumnDescription(
          MarkerType::make<"[PERIPHERAL]">(), peripheral.name(),
          peripheral.text_name(abstract_feature.input_col_));

      return {std::make_pair(col_desc, _importance_factor)};
    }

    default:
      assert_msg(false, "Unknown data used: '" +
                            abstract_feature.data_used_.name() + "'");

      const auto col_desc = helpers::ColumnDescription(
          MarkerType::make<"[PERIPHERAL]">(), "", "");

      return {std::make_pair(col_desc, 0.0)};
  }
}

// ----------------------------------------------------------------------------

std::vector<std::vector<Float>> FastProp::init_subimportance_factors() const {
  const auto make_factors =
      [](const std::optional<FastProp> &sub) -> std::vector<Float> {
    if (!sub) {
      return std::vector<Float>();
    }
    return std::vector<Float>(sub->num_features());
  };

  return subfeatures() | std::views::transform(make_factors) |
         fct::ranges::to<std::vector>();
}

// ----------------------------------------------------------------------------

bool FastProp::is_categorical(const enums::Aggregation _agg) const {
  switch (_agg.value()) {
    case enums::Aggregation::value_of<"COUNT DISTINCT">():
    case enums::Aggregation::value_of<"COUNT MINUS COUNT DISTINCT">():
      return true;

    default:
      return false;
  }
}

// ----------------------------------------------------------------------------

bool FastProp::is_numerical(const enums::Aggregation _agg) const {
  return _agg.value() != enums::Aggregation::value_of<"COUNT">();
}

// ----------------------------------------------------------------------------

std::vector<std::vector<containers::Match>> FastProp::make_matches(
    const TableHolder &_table_holder, const size_t _rownum) const {
  const auto make_match = [](const size_t ix_input, const size_t ix_output) {
    return containers::Match{ix_input, ix_output};
  };

  assert_true(_table_holder.main_tables().size() ==
              _table_holder.peripheral_tables().size());

  auto all_matches = std::vector<std::vector<containers::Match>>();

  for (size_t i = 0; i < _table_holder.main_tables().size(); ++i) {
    const auto population = _table_holder.main_tables().at(i).df();

    const auto peripheral = _table_holder.peripheral_tables().at(i);

    auto matches = std::vector<containers::Match>();

    helpers::Matchmaker<containers::DataFrame, containers::Match,
                        decltype(make_match)>::make_matches(population,
                                                            peripheral, _rownum,
                                                            make_match,
                                                            &matches);

    all_matches.push_back(matches);
  }

  return all_matches;
}

// ----------------------------------------------------------------------------

void FastProp::log_progress(
    const std::shared_ptr<const logging::AbstractLogger> _logger,
    const size_t _nrows, const size_t _num_completed) const {
  if (!_logger) {
    return;
  }

  const auto num_completed_str = std::to_string(_num_completed);

  const auto progress = std::to_string((_num_completed * 100) / _nrows);

  _logger->log("Built " + num_completed_str + " rows. Progress: " + progress +
               "%.");
}

// ----------------------------------------------------------------------------

std::vector<std::vector<containers::Condition>> FastProp::make_conditions(
    const TableHolder &_table_holder) const {
  auto conditions = std::vector<std::vector<containers::Condition>>();

  conditions.push_back({});

  assert_true(_table_holder.main_tables().size() ==
              _table_holder.peripheral_tables().size());

  for (size_t i = 0; i < _table_holder.main_tables().size(); ++i) {
    const auto &population = _table_holder.main_tables().at(i).df();

    const auto &peripheral = _table_holder.peripheral_tables().at(i);

    make_categorical_conditions(peripheral, i, &conditions);

    make_lag_conditions(population, peripheral, i, &conditions);

    make_same_units_categorical_conditions(population, peripheral, i,
                                           &conditions);
  }

  return conditions;
}

// ----------------------------------------------------------------------------

void FastProp::make_categorical_conditions(
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    std::vector<std::vector<containers::Condition>> *_conditions) const {
  if (hyperparameters().n_most_frequent() == 0) {
    return;
  }

  for (size_t input_col = 0; input_col < _peripheral.num_categoricals();
       ++input_col) {
    if (_peripheral.categorical_unit(input_col).find("comparison only") !=
        std::string::npos) {
      continue;
    }

    const auto most_frequent =
        find_most_frequent_categories(_peripheral.categorical_col(input_col));

    for (const auto category_used : most_frequent) {
      _conditions->push_back({containers::Condition(
          category_used, enums::DataUsed::make<"categorical">(), input_col,
          _peripheral_ix)});
    }
  }
}

// ----------------------------------------------------------------------------

void FastProp::make_lag_conditions(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    std::vector<std::vector<containers::Condition>> *_conditions) const {
  if (_population.num_time_stamps() == 0 ||
      _peripheral.num_time_stamps() == 0) {
    return;
  }

  if (hyperparameters().delta_t() <= 0.0 && hyperparameters().max_lag() == 0) {
    return;
  }

  if (hyperparameters().delta_t() <= 0.0 && hyperparameters().max_lag() > 0) {
    throw std::runtime_error(
        "FastProp: If you pass a max_lag, you must also pass a delta_t "
        "that is "
        "greater than 0.");
  }

  if (hyperparameters().delta_t() > 0.0 && hyperparameters().max_lag() == 0) {
    throw std::runtime_error(
        "FastProp: If you pass a delta_t, you must also pass a max_lag "
        "that is "
        "greater than 0.");
  }

  for (size_t i = 0; i < hyperparameters().max_lag(); ++i) {
    const auto lower = hyperparameters().delta_t() * static_cast<Float>(i);

    const auto upper = hyperparameters().delta_t() * static_cast<Float>(i + 1);

    _conditions->push_back({containers::Condition(
        lower, upper, enums::DataUsed::make<"lag">(), _peripheral_ix)});
  }
}

// ----------------------------------------------------------------------------

void FastProp::make_same_units_categorical_conditions(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _peripheral_ix,
    std::vector<std::vector<containers::Condition>> *_conditions) const {
  for (size_t output_col = 0; output_col < _population.num_categoricals();
       ++output_col) {
    for (size_t input_col = 0; input_col < _peripheral.num_categoricals();
         ++input_col) {
      const bool same_unit = _population.categorical_unit(output_col) != "" &&
                             _population.categorical_unit(output_col) ==
                                 _peripheral.categorical_unit(input_col);

      if (!same_unit) {
        continue;
      }

      _conditions->push_back({containers::Condition(
          enums::DataUsed::make<"same_units_categorical">(), input_col,
          output_col, _peripheral_ix)});
    }
  }
}

// ----------------------------------------------------------------------------

std::vector<size_t> FastProp::make_subfeature_index(
    const size_t _peripheral_ix, const std::vector<size_t> &_index) const {
  const auto get_feature = [this](const size_t ix) {
    assert_true(ix < abstract_features().size());
    return abstract_features().at(ix);
  };

  const auto is_relevant_feature = [_peripheral_ix](
                                       const containers::AbstractFeature &f) {
    return f.data_used_.value() == enums::DataUsed::value_of<"subfeatures">() &&
           f.peripheral_ == _peripheral_ix;
  };

  const auto get_input_col = [](const containers::AbstractFeature &f) {
    return f.input_col_;
  };

  return _index | std::views::transform(get_feature) |
         std::views::filter(is_relevant_feature) |
         std::views::transform(get_input_col) | fct::ranges::to<std::set>() |
         fct::ranges::to<std::vector>();
}

// ----------------------------------------------------------------------------

std::shared_ptr<std::vector<size_t>> FastProp::make_subfeature_rownums(
    const std::shared_ptr<std::vector<size_t>> &_rownums,
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral, const size_t _ix) const {
  if (!_rownums) {
    return nullptr;
  }

  assert_true(_ix < placeholder().join_keys_used().size());

  const auto make_staging_table_colname =
      [](const std::string &_colname) -> std::string {
    return transpilation::HumanReadableSQLGenerator()
        .make_staging_table_colname(_colname);
  };

  const auto params_population = helpers::CreateSubviewParams{
      .join_key_ = placeholder().join_keys_used().at(_ix),
      .make_staging_table_colname_ = make_staging_table_colname,
      .time_stamp_ = placeholder().time_stamps_used().at(_ix)};

  const auto population = _population.create_subview(params_population);

  const auto params_peripheral = helpers::CreateSubviewParams{
      .allow_lagged_targets_ = placeholder().allow_lagged_targets().at(_ix),
      .join_key_ = placeholder().other_join_keys_used().at(_ix),
      .make_staging_table_colname_ = make_staging_table_colname,
      .time_stamp_ = placeholder().other_time_stamps_used().at(_ix),
      .upper_time_stamp_ = placeholder().upper_time_stamps_used().at(_ix)};

  const auto peripheral = _peripheral.create_subview(params_peripheral);

  const auto get_ix_input = [](size_t _ix_input, size_t) -> size_t {
    return _ix_input;
  };

  std::set<size_t> unique_indices;

  std::vector<size_t> peripheral_indices;

  for (const auto ix_output : *_rownums) {
    peripheral_indices.clear();

    helpers::Matchmaker<containers::DataFrame, size_t, decltype(get_ix_input)>::
        make_matches(population, peripheral, ix_output, get_ix_input,
                     &peripheral_indices);

    unique_indices.insert(peripheral_indices.begin(), peripheral_indices.end());
  }

  return std::make_shared<std::vector<size_t>>(unique_indices.begin(),
                                               unique_indices.end());
}

// ----------------------------------------------------------------------------

std::shared_ptr<std::vector<size_t>> FastProp::make_rownums(
    const size_t _thread_num, const size_t _nrows,
    const std::shared_ptr<std::vector<size_t>> &_rownums) const {
  const size_t num_threads = get_num_threads();

  assert_true(_thread_num < num_threads);

  const auto nrows = _rownums ? _rownums->size() : _nrows;

  const auto calc_rows_per_thread = [nrows, num_threads]() -> size_t {
    size_t rows_per_thread = 0;

    while (true) {
      const auto remaining = nrows - rows_per_thread * num_threads;

      rows_per_thread += remaining / num_threads;

      if (remaining < num_threads) {
        break;
      }
    }

    return rows_per_thread;
  };

  const size_t rows_per_thread = calc_rows_per_thread();

  const size_t begin = _thread_num * rows_per_thread;

  const size_t end = (_thread_num < num_threads - 1)
                         ? (_thread_num + 1) * rows_per_thread
                         : nrows;

  if (_rownums) {
    return std::make_shared<std::vector<size_t>>(_rownums->begin() + begin,
                                                 _rownums->begin() + end);
  }

  const auto rownums = std::make_shared<std::vector<size_t>>(end - begin);

  std::iota(rownums->begin(), rownums->end(), begin);

  return rownums;
}

// ---------------------------------------------------------------------------

typename FastProp::ReflectionType FastProp::reflection() const {
  return rfl::make_field<"features_">(abstract_features_) *
         rfl::make_field<"allow_http_">(allow_http_) *
         rfl::make_field<"hyperparameters_">(hyperparameters_) *
         rfl::make_field<"main_table_schemas_">(main_table_schemas_) *
         rfl::make_field<"peripheral_">(peripheral_) *
         rfl::make_field<"peripheral_schema_">(peripheral_schema_) *
         rfl::make_field<"peripheral_table_schemas_">(
             peripheral_table_schemas_) *
         rfl::make_field<"placeholder_">(placeholder_) *
         rfl::make_field<"population_schema_">(population_schema_) *
         rfl::make_field<"subfeatures_">(subfeatures_);
}

// ----------------------------------------------------------------------------

std::shared_ptr<std::vector<size_t>> FastProp::sample_from_population(
    const size_t _nrows) const {
  std::mt19937 rng;

  std::uniform_real_distribution<Float> dist(0.0, 1.0);

  const auto include = [this, &rng, &dist](const size_t) -> bool {
    return dist(rng) < hyperparameters().sampling_factor();
  };

  return std::views::iota(0uz, _nrows) | std::views::filter(include) |
         fct::ranges::to<fct::shared_ptr::vector>();
}

// ----------------------------------------------------------------------------

std::shared_ptr<const std::vector<containers::AbstractFeature>>
FastProp::select_features(
    const FitParams &_params,
    const std::shared_ptr<std::vector<size_t>> &_rownums) const {
  if (abstract_features().size() <= hyperparameters().num_features()) {
    if (_params.logger_) {
      _params.logger_->log("Trained features. Progress: 100%.");
    }

    return abstract_features_;
  }

  const auto r_squared = calc_r_squared(_params, _rownums);

  const auto threshold = calc_threshold(r_squared);

  const auto r_greater_threshold = [&r_squared, threshold](const size_t ix) {
    return r_squared.at(ix) > threshold;
  };

  const auto get_feature = [this](const size_t ix) {
    return abstract_features().at(ix);
  };

  assert_true(r_squared.size() == abstract_features().size());

  return std::views::iota(0uz, r_squared.size()) |
         std::views::filter(r_greater_threshold) |
         std::views::transform(get_feature) |
         fct::ranges::to<fct::shared_ptr::vector>();
}

// ----------------------------------------------------------------------------

bool FastProp::skip_first_last(const enums::Aggregation _agg,
                               const containers::DataFrame &_population,
                               const containers::DataFrame &_peripheral) const {
  if (!Aggregator::is_first_last(_agg)) {
    return false;
  }

  return (_population.num_time_stamps() == 0 ||
          _peripheral.num_time_stamps() == 0);
}

// ----------------------------------------------------------------------------

void FastProp::spawn_threads(
    const TransformParams &_params,
    const std::vector<containers::Features> &_subfeatures,
    const std::shared_ptr<std::vector<size_t>> &_rownums,
    containers::Features *_features) const {
  auto num_completed = std::atomic<size_t>(0);

  const auto execute_task = [this, &_params, _subfeatures, _rownums,
                             &num_completed,
                             _features](const size_t _thread_num) {
    try {
      build_rows(_params, _subfeatures, _rownums, _thread_num, &num_completed,
                 _features);
    } catch (std::exception &e) {
      if (_thread_num == 0) {
        throw std::runtime_error(e.what());
      }
    }
  };

  const size_t num_threads = get_num_threads();

  std::vector<std::thread> threads;

  for (size_t thread_num = 1; thread_num < num_threads; ++thread_num) {
    threads.push_back(std::thread(execute_task, thread_num));
  }

  try {
    execute_task(0);
  } catch (std::exception &e) {
    for (auto &thr : threads) {
      thr.join();
    }

    throw std::runtime_error(e.what());
  }

  for (auto &thr : threads) {
    thr.join();
  }

  log_progress(_params.logger_, 100, 100);
}

// ----------------------------------------------------------------------------

void FastProp::subfeatures_to_sql(
    const helpers::StringIterator &_categories,
    const helpers::VocabularyTree &_vocabulary,
    const std::shared_ptr<const transpilation::SQLDialectGenerator>
        &_sql_dialect_generator,
    const std::string &_feature_prefix, const size_t,
    std::vector<std::string> *_sql) const {
  for (size_t i = 0; i < subfeatures().size(); ++i) {
    if (subfeatures().at(i)) {
      const auto sub = subfeatures().at(i)->to_sql(
          _categories, _vocabulary, _sql_dialect_generator,
          _feature_prefix + std::to_string(i + 1) + "_", 0, true);

      _sql->insert(_sql->end(), sub.begin(), sub.end());
    }
  }
}

// ----------------------------------------------------------------------------

containers::Features FastProp::transform(
    const TransformParams &_params,
    const std::shared_ptr<std::vector<size_t>> &_rownums,
    const bool _as_subfeatures) const {
  if (_params.population_.nrows() == 0) {
    throw std::runtime_error(
        "Population table needs to contain at least some data!");
  }

  const auto subfeatures = build_subfeatures(_params, _rownums);

  if (_params.logger_) {
    const auto msg = _as_subfeatures ? "FastProp: Building subfeatures..."
                                     : "FastProp: Building features...";

    _params.logger_->log(msg);
  }

  auto features = containers::Features(
      _params.population_.nrows(), _params.index_.size(), _params.temp_dir_);

  spawn_threads(_params, subfeatures, _rownums, &features);

  return features;
}

// ----------------------------------------------------------------------------

std::vector<std::string> FastProp::to_sql(
    const helpers::StringIterator &_categories,
    const helpers::VocabularyTree &_vocabulary,
    const std::shared_ptr<const transpilation::SQLDialectGenerator>
        &_sql_dialect_generator,
    const std::string &_feature_prefix, const size_t _offset,
    const bool _subfeatures) const {
  assert_true(main_table_schemas().size() == peripheral_table_schemas().size());

  std::vector<std::string> sql;

  if (_subfeatures) {
    subfeatures_to_sql(_categories, _vocabulary, _sql_dialect_generator,
                       _feature_prefix, _offset, &sql);
  }

  for (size_t i = 0; i < abstract_features().size(); ++i) {
    const auto abstract_feature = abstract_features().at(i);

    assert_true(abstract_feature.peripheral_ <
                peripheral_table_schemas().size());

    const auto input_schema =
        peripheral_table_schemas().at(abstract_feature.peripheral_);

    const auto output_schema =
        main_table_schemas().at(abstract_feature.peripheral_);

    sql.push_back(abstract_feature.to_sql(
        _categories, _sql_dialect_generator, _feature_prefix,
        std::to_string(_offset + i + 1), input_schema, output_schema));
  }

  return sql;
}

// ----------------------------------------------------------------------------
}  // namespace algorithm
}  // namespace fastprop
