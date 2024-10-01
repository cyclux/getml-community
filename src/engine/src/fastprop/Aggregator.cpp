// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "fastprop/algorithm/Aggregator.hpp"

namespace fastprop {
namespace algorithm {

Float Aggregator::apply_aggregation(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const std::optional<containers::Features> &_subfeatures,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  switch (_abstract_feature.data_used_.value()) {
    case enums::DataUsed::value_of<"categorical">():
      return apply_categorical(_population, _peripheral, _matches,
                               _condition_function, _abstract_feature,
                               _memoization);

    case enums::DataUsed::value_of<"discrete">():
      return apply_discrete(_population, _peripheral, _matches,
                            _condition_function, _abstract_feature,
                            _memoization);

    case enums::DataUsed::value_of<"na">():
      return apply_not_applicable(_peripheral, _matches, _condition_function,
                                  _abstract_feature, _memoization);

    case enums::DataUsed::value_of<"numerical">():
      return apply_numerical(_population, _peripheral, _matches,
                             _condition_function, _abstract_feature,
                             _memoization);

    case enums::DataUsed::value_of<"same_units_categorical">():
      return apply_same_units_categorical(_population, _peripheral, _matches,
                                          _condition_function,
                                          _abstract_feature, _memoization);

    case enums::DataUsed::value_of<"same_units_discrete">():
    case enums::DataUsed::value_of<"same_units_discrete_ts">():
      return apply_same_units_discrete(_population, _peripheral, _matches,
                                       _condition_function, _abstract_feature,
                                       _memoization);

    case enums::DataUsed::value_of<"same_units_numerical">():
    case enums::DataUsed::value_of<"same_units_numerical_ts">():
      return apply_same_units_numerical(_population, _peripheral, _matches,
                                        _condition_function, _abstract_feature,
                                        _memoization);

    case enums::DataUsed::value_of<"subfeatures">():
      assert_true(_subfeatures);
      return apply_subfeatures(_population, _peripheral, *_subfeatures,
                               _matches, _condition_function, _abstract_feature,
                               _memoization);

    case enums::DataUsed::value_of<"text">():
      return apply_text(_population, _peripheral, _matches, _condition_function,
                        _abstract_feature, _memoization);

    default:
      assert_msg(false, "Unknown data_used: '" +
                            _abstract_feature.data_used_.name() + "'.");
      return 0.0;
  }
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_categorical(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_abstract_feature.input_col_ < _peripheral.num_categoricals());

  const auto &col = _peripheral.categorical_col(_abstract_feature.input_col_);

  if (_abstract_feature.categorical_value_ ==
      containers::AbstractFeature::NO_CATEGORICAL_VALUE) {
    const auto extract_value = [&col](const containers::Match &match) -> Int {
      return col[match.ix_input];
    };

    return aggregate_matches_categorical(
        _matches, extract_value, _condition_function, _abstract_feature);
  }

  const auto extract_value =
      [&col, &_abstract_feature](const containers::Match &match) -> Float {
    if (col[match.ix_input] == _abstract_feature.categorical_value_) {
      return 1.0;
    }
    return 0.0;
  };

  if (is_first_last(_abstract_feature.aggregation_)) {
    return apply_first_last(_population, _peripheral, _matches, extract_value,
                            _condition_function, _abstract_feature,
                            _memoization);
  }

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_discrete(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_abstract_feature.input_col_ < _peripheral.num_discretes());

  const auto &col = _peripheral.discrete_col(_abstract_feature.input_col_);

  const auto extract_value = [&col](const containers::Match &match) -> Float {
    return col[match.ix_input];
  };

  if (is_first_last(_abstract_feature.aggregation_)) {
    return apply_first_last(_population, _peripheral, _matches, extract_value,
                            _condition_function, _abstract_feature,
                            _memoization);
  }

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_not_applicable(
    const containers::DataFrame &_peripheral,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_abstract_feature.aggregation_.value() ==
                  enums::Aggregation::value_of<"COUNT">() ||
              _abstract_feature.aggregation_.value() ==
                  enums::Aggregation::value_of<"AVG TIME BETWEEN">());

  if (_abstract_feature.aggregation_.value() ==
      enums::Aggregation::value_of<"COUNT">()) {
    const auto extract_value = [](const containers::Match &match) -> Float {
      return 0.0;
    };

    memorize_numerical_range(_matches, extract_value, _condition_function,
                             _abstract_feature, _memoization);

    return aggregate_numerical_range(_memoization->numerical_begin(),
                                     _memoization->numerical_end(),
                                     _abstract_feature.aggregation_);
  }

  assert_true(_peripheral.num_time_stamps() > 0);

  const auto &col = _peripheral.time_stamp_col();

  const auto extract_value = [&col](const containers::Match &match) -> Float {
    return col[match.ix_input];
  };

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_numerical(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_abstract_feature.input_col_ < _peripheral.num_numericals());

  const auto &col = _peripheral.numerical_col(_abstract_feature.input_col_);

  const auto extract_value = [&col](const containers::Match &match) -> Float {
    return col[match.ix_input];
  };

  if (is_first_last(_abstract_feature.aggregation_)) {
    return apply_first_last(_population, _peripheral, _matches, extract_value,
                            _condition_function, _abstract_feature,
                            _memoization);
  }

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_same_units_categorical(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_abstract_feature.input_col_ < _peripheral.num_categoricals());

  assert_true(_abstract_feature.output_col_ < _population.num_categoricals());

  const auto &col1 = _population.categorical_col(_abstract_feature.output_col_);

  const auto &col2 = _peripheral.categorical_col(_abstract_feature.input_col_);

  const auto extract_value = [&col1,
                              &col2](const containers::Match &match) -> Float {
    if (col1[match.ix_output] == col2[match.ix_input] &&
        col1[match.ix_output] >= 0) {
      return 1.0;
    }
    return 0.0;
  };

  if (is_first_last(_abstract_feature.aggregation_)) {
    return apply_first_last(_population, _peripheral, _matches, extract_value,
                            _condition_function, _abstract_feature,
                            _memoization);
  }

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_same_units_discrete(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_abstract_feature.input_col_ < _peripheral.num_discretes());

  assert_true(_abstract_feature.output_col_ < _population.num_discretes());

  const auto &col1 = _population.discrete_col(_abstract_feature.output_col_);

  const auto &col2 = _peripheral.discrete_col(_abstract_feature.input_col_);

  const auto extract_value = [&col1,
                              &col2](const containers::Match &match) -> Float {
    return col1[match.ix_output] - col2[match.ix_input];
  };

  if (is_first_last(_abstract_feature.aggregation_)) {
    return apply_first_last(_population, _peripheral, _matches, extract_value,
                            _condition_function, _abstract_feature,
                            _memoization);
  }

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_same_units_numerical(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_abstract_feature.input_col_ < _peripheral.num_numericals());

  assert_true(_abstract_feature.output_col_ < _population.num_numericals());

  const auto &col1 = _population.numerical_col(_abstract_feature.output_col_);

  const auto &col2 = _peripheral.numerical_col(_abstract_feature.input_col_);

  const auto extract_value = [&col1,
                              &col2](const containers::Match &match) -> Float {
    return col1[match.ix_output] - col2[match.ix_input];
  };

  if (is_first_last(_abstract_feature.aggregation_)) {
    return apply_first_last(_population, _peripheral, _matches, extract_value,
                            _condition_function, _abstract_feature,
                            _memoization);
  }

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_subfeatures(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const containers::Features &_subfeatures,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_abstract_feature.input_col_ < _subfeatures.size());

  const auto &col = _subfeatures.at(_abstract_feature.input_col_);

  const auto extract_value = [&col](const containers::Match &match) -> Float {
    return col[match.ix_input];
  };

  if (is_first_last(_abstract_feature.aggregation_)) {
    return apply_first_last(_population, _peripheral, _matches, extract_value,
                            _condition_function, _abstract_feature,
                            _memoization);
  }

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------

Float Aggregator::apply_text(
    const containers::DataFrame &_population,
    const containers::DataFrame &_peripheral,
    const std::vector<containers::Match> &_matches,
    const std::function<bool(const containers::Match &)> &_condition_function,
    const containers::AbstractFeature &_abstract_feature,
    const rfl::Ref<Memoization> &_memoization) {
  assert_true(_peripheral.text_.size() == _peripheral.word_indices_.size());

  assert_true(_abstract_feature.input_col_ < _peripheral.word_indices_.size());

  const auto index = _peripheral.word_indices_.at(_abstract_feature.input_col_);

  assert_true(index);

  const auto extract_value =
      [&index, &_abstract_feature](const containers::Match &match) -> Float {
    const auto range = index->range(match.ix_input);

    for (const auto &word_ix : range) {
      if (word_ix == _abstract_feature.categorical_value_) {
        return 1.0;
      }

      if (word_ix > _abstract_feature.categorical_value_) {
        return 0.0;
      }
    }
    return 0.0;
  };

  if (is_first_last(_abstract_feature.aggregation_)) {
    return apply_first_last(_population, _peripheral, _matches, extract_value,
                            _condition_function, _abstract_feature,
                            _memoization);
  }

  memorize_numerical_range(_matches, extract_value, _condition_function,
                           _abstract_feature, _memoization);

  return aggregate_numerical_range(_memoization->numerical_begin(),
                                   _memoization->numerical_end(),
                                   _abstract_feature.aggregation_);
}

// ----------------------------------------------------------------------------
}  // namespace algorithm
}  // namespace fastprop
