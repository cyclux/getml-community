// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "engine/preprocessors/Seasonal.hpp"

#include "engine/preprocessors/PreprocessorImpl.hpp"
#include "engine/utils/Time.hpp"
#include "helpers/Loader.hpp"
#include "helpers/Saver.hpp"

namespace engine {
namespace preprocessors {

std::optional<containers::Column<Int>> Seasonal::extract_hour(
    const containers::Column<Float>& _col,
    containers::Encoding* _categories) const {
  if (is_disabled<"disable_hour_">()) {
    return std::nullopt;
  }

  auto result = to_categorical(_col, ADD_ZERO, utils::Time::hour, _categories);

  result.set_name(helpers::Macros::hour_begin() + _col.name() +
                  helpers::Macros::hour_end());
  result.set_unit("hour");

  if (PreprocessorImpl::has_warnings(result)) {
    return std::nullopt;
  }

  return result;
}

// ----------------------------------------------------

containers::Column<Int> Seasonal::extract_hour(
    const containers::Encoding& _categories,
    const containers::Column<Float>& _col) const {
  auto result = to_categorical(_categories, _col, ADD_ZERO, utils::Time::hour);

  result.set_name(helpers::Macros::hour_begin() + _col.name() +
                  helpers::Macros::hour_end());
  result.set_unit("hour");

  return result;
}

// ----------------------------------------------------

std::optional<containers::Column<Int>> Seasonal::extract_minute(
    const containers::Column<Float>& _col,
    containers::Encoding* _categories) const {
  if (is_disabled<"disable_minute_">()) {
    return std::nullopt;
  }

  auto result =
      to_categorical(_col, ADD_ZERO, utils::Time::minute, _categories);

  result.set_name(helpers::Macros::minute_begin() + _col.name() +
                  helpers::Macros::minute_end());
  result.set_unit("minute");

  if (PreprocessorImpl::has_warnings(result)) {
    return std::nullopt;
  }

  return result;
}

// ----------------------------------------------------

containers::Column<Int> Seasonal::extract_minute(
    const containers::Encoding& _categories,
    const containers::Column<Float>& _col) const {
  auto result =
      to_categorical(_categories, _col, ADD_ZERO, utils::Time::minute);

  result.set_name(helpers::Macros::minute_begin() + _col.name() +
                  helpers::Macros::minute_end());
  result.set_unit("minute");

  return result;
}

// ----------------------------------------------------

std::optional<containers::Column<Int>> Seasonal::extract_month(
    const containers::Column<Float>& _col,
    containers::Encoding* _categories) const {
  if (is_disabled<"disable_month_">()) {
    return std::nullopt;
  }

  auto result = to_categorical(_col, ADD_ZERO, utils::Time::month, _categories);

  result.set_name(helpers::Macros::month_begin() + _col.name() +
                  helpers::Macros::month_end());
  result.set_unit("month");

  if (PreprocessorImpl::has_warnings(result)) {
    return std::nullopt;
  }

  return result;
}

// ----------------------------------------------------

containers::Column<Int> Seasonal::extract_month(
    const containers::Encoding& _categories,
    const containers::Column<Float>& _col) const {
  auto result = to_categorical(_categories, _col, ADD_ZERO, utils::Time::month);

  result.set_name(helpers::Macros::month_begin() + _col.name() +
                  helpers::Macros::month_end());
  result.set_unit("month");

  return result;
}

// ----------------------------------------------------

std::optional<containers::Column<Int>> Seasonal::extract_weekday(
    const containers::Column<Float>& _col,
    containers::Encoding* _categories) const {
  if (is_disabled<"disable_weekday_">()) {
    return std::nullopt;
  }

  auto result =
      to_categorical(_col, DONT_ADD_ZERO, utils::Time::weekday, _categories);

  result.set_name(helpers::Macros::weekday_begin() + _col.name() +
                  helpers::Macros::weekday_end());
  result.set_unit("weekday");

  if (PreprocessorImpl::has_warnings(result)) {
    return std::nullopt;
  }

  return result;
}

// ----------------------------------------------------

containers::Column<Int> Seasonal::extract_weekday(
    const containers::Encoding& _categories,
    const containers::Column<Float>& _col) const {
  auto result =
      to_categorical(_categories, _col, DONT_ADD_ZERO, utils::Time::weekday);

  result.set_name(helpers::Macros::weekday_begin() + _col.name() +
                  helpers::Macros::weekday_end());
  result.set_unit("weekday");

  return result;
}

// ----------------------------------------------------

std::optional<containers::Column<Float>> Seasonal::extract_year(
    const containers::Column<Float>& _col) {
  if (is_disabled<"disable_year_">()) {
    return std::nullopt;
  }

  auto result = to_numerical(_col, utils::Time::year);

  result.set_name(helpers::Macros::year_begin() + _col.name() +
                  helpers::Macros::year_end());
  result.set_unit("year, comparison only");

  if (PreprocessorImpl::has_warnings(result)) {
    return std::nullopt;
  }

  return result;
}

// ----------------------------------------------------

containers::Column<Float> Seasonal::extract_year(
    const containers::Column<Float>& _col) const {
  auto result = to_numerical(_col, utils::Time::year);

  result.set_name(helpers::Macros::year_begin() + _col.name() +
                  helpers::Macros::year_end());
  result.set_unit("year, comparison only");

  return result;
}

// ----------------------------------------------------

commands::Fingerprint Seasonal::fingerprint() const {
  using FingerprintType = typename commands::Fingerprint::SeasonalFingerprint;
  return commands::Fingerprint(
      FingerprintType{.dependencies = dependencies_, .op = op_});
}

// ----------------------------------------------------

std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
Seasonal::fit_transform(const Params& _params) {
  const auto population_df = fit_transform_df(
      _params.population_df(), MarkerType::make<"[POPULATION]">(), 0,
      _params.categories().get());

  auto peripheral_dfs = std::vector<containers::DataFrame>();

  for (size_t i = 0; i < _params.peripheral_dfs().size(); ++i) {
    const auto& df = _params.peripheral_dfs().at(i);

    const auto new_df = fit_transform_df(df, MarkerType::make<"[PERIPHERAL]">(),
                                         i, _params.categories().get());

    peripheral_dfs.push_back(new_df);
  }

  return std::make_pair(population_df, peripheral_dfs);
}

// ----------------------------------------------------

containers::DataFrame Seasonal::fit_transform_df(
    const containers::DataFrame& _df, const MarkerType _marker,
    const size_t _table, containers::Encoding* _categories) {
  const auto blacklist = std::vector<helpers::Subrole>(
      {helpers::Subrole::exclude_preprocessors, helpers::Subrole::email_only,
       helpers::Subrole::substring_only, helpers::Subrole::exclude_seasonal});

  auto df = _df;

  for (size_t i = 0; i < _df.num_time_stamps(); ++i) {
    const auto& ts = _df.time_stamp(i);

    if (ts.name().find(helpers::Macros::generated_ts()) != std::string::npos) {
      continue;
    }

    if (helpers::SubroleParser::contains_any(ts.subroles(), blacklist)) {
      continue;
    }

    auto col = extract_hour(ts, _categories);

    if (col) {
      PreprocessorImpl::add(_marker, _table, ts.name(), &hour_);
      df.add_int_column(*col, containers::DataFrame::ROLE_CATEGORICAL);
    }

    col = extract_minute(ts, _categories);

    if (col) {
      PreprocessorImpl::add(_marker, _table, ts.name(), &minute_);
      df.add_int_column(*col, containers::DataFrame::ROLE_CATEGORICAL);
    }

    col = extract_month(ts, _categories);

    if (col) {
      PreprocessorImpl::add(_marker, _table, ts.name(), &month_);
      df.add_int_column(*col, containers::DataFrame::ROLE_CATEGORICAL);
    }

    col = extract_weekday(ts, _categories);

    if (col) {
      PreprocessorImpl::add(_marker, _table, ts.name(), &weekday_);
      df.add_int_column(*col, containers::DataFrame::ROLE_CATEGORICAL);
    }

    const auto year = extract_year(ts);

    if (year) {
      PreprocessorImpl::add(_marker, _table, ts.name(), &year_);
      df.add_float_column(*year, containers::DataFrame::ROLE_NUMERICAL);
    }
  }

  return df;
}

// ----------------------------------------------------

void Seasonal::load(const std::string& _fname) {
  const auto named_tuple = helpers::Loader::load<ReflectionType>(_fname);
  hour_ = named_tuple.hour();
  minute_ = named_tuple.minute();
  month_ = named_tuple.month();
  weekday_ = named_tuple.weekday();
  year_ = named_tuple.year();
}

// ----------------------------------------------------

typename Seasonal::ReflectionType Seasonal::reflection() const {
  return ReflectionType{.hour = hour_,
                        .minute = minute_,
                        .month = month_,
                        .weekday = weekday_,
                        .year = year_};
}

// ----------------------------------------------------

void Seasonal::save(const std::string& _fname,
                    const typename helpers::Saver::Format& _format) const {
  helpers::Saver::save(_fname, *this, _format);
}

// ----------------------------------------------------

containers::Column<Int> Seasonal::to_int(
    const containers::Column<Float>& _col, const bool _add_zero,
    containers::Encoding* _categories) const {
  const auto to_str = [_categories, _add_zero](const Float val) {
    auto str = io::Parser::to_string(val);
    if (_add_zero && str.size() == 1) {
      str = '0' + str;
    }
    return (*_categories)[str];
  };

  auto result = containers::Column<Int>(_col.pool(), _col.nrows());

  std::transform(_col.begin(), _col.end(), result.begin(), to_str);

  return result;
}

// ----------------------------------------------------

containers::Column<Int> Seasonal::to_int(
    const containers::Encoding& _categories, const bool _add_zero,
    const containers::Column<Float>& _col) const {
  const auto to_str = [&_categories, _add_zero](const Float val) {
    auto str = io::Parser::to_string(val);
    if (_add_zero && str.size() == 1) {
      str = '0' + str;
    }
    return _categories[str];
  };

  auto result = containers::Column<Int>(_col.pool(), _col.nrows());

  std::transform(_col.begin(), _col.end(), result.begin(), to_str);

  return result;
}

// ----------------------------------------------------

std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
Seasonal::transform(const Params& _params) const {
  const auto population_df =
      transform_df(*_params.categories(), _params.population_df(),
                   MarkerType::make<"[POPULATION]">(), 0);

  auto peripheral_dfs = std::vector<containers::DataFrame>();

  for (size_t i = 0; i < _params.peripheral_dfs().size(); ++i) {
    const auto& df = _params.peripheral_dfs().at(i);

    const auto new_df = transform_df(*_params.categories(), df,
                                     MarkerType::make<"[PERIPHERAL]">(), i);

    peripheral_dfs.push_back(new_df);
  }

  return std::make_pair(population_df, peripheral_dfs);
}

// ----------------------------------------------------

containers::DataFrame Seasonal::transform_df(
    const containers::Encoding& _categories, const containers::DataFrame& _df,
    const MarkerType _marker, const size_t _table) const {
  auto df = _df;

  // ----------------------------------------------------

  auto names = PreprocessorImpl::retrieve_names(_marker, _table, hour_);

  for (const auto& name : names) {
    const auto col = extract_hour(_categories, df.time_stamp(name));

    df.add_int_column(col, containers::DataFrame::ROLE_CATEGORICAL);
  }

  // ----------------------------------------------------

  names = PreprocessorImpl::retrieve_names(_marker, _table, minute_);

  for (const auto& name : names) {
    const auto col = extract_minute(_categories, df.time_stamp(name));

    df.add_int_column(col, containers::DataFrame::ROLE_CATEGORICAL);
  }

  // ----------------------------------------------------

  names = PreprocessorImpl::retrieve_names(_marker, _table, month_);

  for (const auto& name : names) {
    const auto col = extract_month(_categories, df.time_stamp(name));

    df.add_int_column(col, containers::DataFrame::ROLE_CATEGORICAL);
  }

  // ----------------------------------------------------

  names = PreprocessorImpl::retrieve_names(_marker, _table, weekday_);

  for (const auto& name : names) {
    const auto col = extract_weekday(_categories, df.time_stamp(name));

    df.add_int_column(col, containers::DataFrame::ROLE_CATEGORICAL);
  }

  // ----------------------------------------------------

  names = PreprocessorImpl::retrieve_names(_marker, _table, year_);

  for (const auto& name : names) {
    const auto col = extract_year(df.time_stamp(name));

    df.add_float_column(col, containers::DataFrame::ROLE_NUMERICAL);
  }

  // ----------------------------------------------------

  return df;
}

// ----------------------------------------------------
}  // namespace preprocessors
}  // namespace engine
