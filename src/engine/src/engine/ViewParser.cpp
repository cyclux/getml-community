// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "engine/handlers/ViewParser.hpp"

#include <ranges>

#include "commands/Fingerprint.hpp"
#include "containers/DataFrameContent.hpp"
#include "engine/handlers/BoolOpParser.hpp"
#include "engine/handlers/FloatOpParser.hpp"
#include "engine/handlers/StringOpParser.hpp"
#include "engine/utils/Getter.hpp"
#include "fct/compose.hpp"
#include "fct/to.hpp"
#include "helpers/Aggregations.hpp"
#include "io/Parser.hpp"

namespace engine {
namespace handlers {

void ViewParser::add_column(const ViewOp& _cmd,
                            containers::DataFrame* _df) const {
  if (!_cmd.added()) {
    return;
  }

  const auto handle = [this, &_cmd, _df](const auto& _json_col) {
    using Type = std::decay_t<decltype(_json_col)>;

    const auto& added = *_cmd.added();

    const auto& name = added.name();

    const auto& role = added.role();

    const auto& subroles = added.subroles();

    const auto& unit = added.unit();

    if constexpr (std::is_same<Type,
                               commands::FloatColumnOrFloatColumnView>()) {
      const auto column_view =
          FloatOpParser(categories_, join_keys_encoding_, data_frames_)
              .parse(_json_col);

      auto col = column_view.to_column(0, _df->nrows(), true);

      col.set_name(name);

      col.set_subroles(subroles);

      col.set_unit(unit);

      _df->add_float_column(col, role);
    } else {
      const auto column_view =
          StringOpParser(categories_, join_keys_encoding_, data_frames_)
              .parse(_json_col);

      const auto vec = column_view.to_vector(0, _df->nrows(), true);

      assert_true(vec);

      if (role == containers::DataFrame::ROLE_CATEGORICAL ||
          role == containers::DataFrame::ROLE_JOIN_KEY) {
        add_int_column_to_df(name, role, subroles, unit, *vec, _df);
      }

      if (role == containers::DataFrame::ROLE_UNUSED ||
          role == containers::DataFrame::ROLE_TEXT ||
          role == containers::DataFrame::ROLE_UNUSED_STRING) {
        add_string_column_to_df(name, role, subroles, unit, *vec, _df);
      }
    }
  };

  std::visit(handle, _cmd.added()->col());
}

// ------------------------------------------------------------------------

void ViewParser::add_int_column_to_df(const std::string& _name,
                                      const std::string& _role,
                                      const std::vector<std::string>& _subroles,
                                      const std::string& _unit,
                                      const std::vector<strings::String>& _vec,
                                      containers::DataFrame* _df) const {
  const auto encoding = _role == containers::DataFrame::ROLE_CATEGORICAL
                            ? categories_
                            : join_keys_encoding_;

  auto col = containers::Column<Int>(_df->pool(), _vec.size());

  for (size_t i = 0; i < _vec.size(); ++i) {
    col[i] = (*encoding)[_vec[i]];
  }

  col.set_name(_name);

  col.set_subroles(_subroles);

  col.set_unit(_unit);

  _df->add_int_column(col, _role);

  if (_role == containers::DataFrame::ROLE_JOIN_KEY) {
    _df->create_indices();
  }
}

// ----------------------------------------------------------------------------

void ViewParser::add_string_column_to_df(
    const std::string& _name, const std::string& _role,
    const std::vector<std::string>& _subroles, const std::string& _unit,
    const std::vector<strings::String>& _vec,
    containers::DataFrame* _df) const {
  auto col = containers::Column<strings::String>(_df->pool());

  for (size_t i = 0; i < _vec.size(); ++i) {
    col.push_back(_vec[i]);
  }

  col.set_name(_name);

  col.set_subroles(_subroles);

  col.set_unit(_unit);

  _df->add_string_column(col, _role);
}

// ----------------------------------------------------------------------------

void ViewParser::drop_columns(const ViewOp& _cmd,
                              containers::DataFrame* _df) const {
  if (!_cmd.dropped()) {
    return;
  }

  const auto& dropped = *_cmd.dropped();

  for (const auto& name : dropped) {
    _df->remove_column(name);
  }
}

// ----------------------------------------------------------------------------

typename ViewParser::ColumnViewVariant ViewParser::make_column_view(
    const ViewCol& _view_col) const {
  const auto handle = [this](const auto& _col) -> ColumnViewVariant {
    using Type = std::decay_t<decltype(_col)>;

    if constexpr (std::is_same<
                      Type, typename commands::StringColumnOrStringColumnView::
                                ReflectionType>()) {
      const auto col = commands::StringColumnOrStringColumnView{_col};
      return StringOpParser(categories_, join_keys_encoding_, data_frames_)
          .parse(col);
    }

    if constexpr (std::is_same<Type,
                               typename commands::FloatColumnOrFloatColumnView::
                                   ReflectionType>()) {
      const auto col = commands::FloatColumnOrFloatColumnView{_col};
      return FloatOpParser(categories_, join_keys_encoding_, data_frames_)
          .parse(col);
    }
  };

  return std::visit(handle, _view_col);
}

// ----------------------------------------------------------------------------

std::optional<size_t> ViewParser::make_nrows(
    const std::vector<ColumnViewVariant>& _column_views,
    const size_t _force) const {
  const auto has_nrows = [](const ColumnViewVariant& _column_view) -> bool {
    if (std::holds_alternative<containers::ColumnView<Float>>(_column_view)) {
      const auto float_col =
          std::get<containers::ColumnView<Float>>(_column_view);
      return std::holds_alternative<size_t>(float_col.nrows());
    }
    const auto str_col =
        std::get<containers::ColumnView<strings::String>>(_column_view);
    return std::holds_alternative<size_t>(str_col.nrows());
  };

  const auto get_nrows = [](const ColumnViewVariant& _column_view) -> size_t {
    if (std::holds_alternative<containers::ColumnView<Float>>(_column_view)) {
      const auto float_col =
          std::get<containers::ColumnView<Float>>(_column_view);
      return std::get<size_t>(float_col.nrows());
    }

    const auto str_col =
        std::get<containers::ColumnView<strings::String>>(_column_view);
    return std::get<size_t>(str_col.nrows());
  };

  const auto calc_nrows = [](const ColumnViewVariant& _column_view) -> size_t {
    if (std::holds_alternative<containers::ColumnView<Float>>(_column_view)) {
      const auto float_col =
          std::get<containers::ColumnView<Float>>(_column_view)
              .to_column(0, std::nullopt, false);
      return float_col.nrows();
    }

    const auto str_col =
        std::get<containers::ColumnView<strings::String>>(_column_view)
            .to_column(0, std::nullopt, false);
    return str_col.nrows();
  };

  const auto it =
      std::find_if(_column_views.begin(), _column_views.end(), has_nrows);

  if (it != _column_views.end()) {
    return std::make_optional(get_nrows(*it));
  }

  if (!_force) {
    return std::nullopt;
  }

  const auto to_float = [](const size_t _nrows) -> Float {
    return static_cast<Float>(_nrows);
  };

  const auto calc_nrows_float = fct::compose(calc_nrows, to_float);

  auto range = _column_views | std::views::transform(calc_nrows_float);

  return static_cast<size_t>(
      helpers::Aggregations::assert_equal(range.begin(), range.end()));
}

// ----------------------------------------------------------------------------

std::vector<std::string> ViewParser::make_string_vector(
    const size_t _start, const size_t _length,
    const ColumnViewVariant& _column_view) const {
  const bool is_string_column =
      std::holds_alternative<containers::ColumnView<strings::String>>(
          _column_view);

  if (is_string_column) {
    const auto str_col =
        std::get<containers::ColumnView<strings::String>>(_column_view);

    const auto to_str = [](const strings::String& _str) -> std::string {
      return _str.str();
    };

    return *str_col.to_vector(_start, _length, false) |
           std::views::transform(to_str) | fct::ranges::to<std::vector>();
  }

  const auto float_col = std::get<containers::ColumnView<Float>>(_column_view);

  const auto float_vec = *float_col.to_vector(_start, _length, false);

  if (float_col.unit().find("time stamp") != std::string::npos) {
    return float_vec | std::views::transform(io::Parser::ts_to_string) |
           fct::ranges::to<std::vector>();
  }

  const auto to_string = [](const Float _val) {
    return io::Parser::to_string(_val);
  };

  return float_vec | std::views::transform(to_string) |
         fct::ranges::to<std::vector>();
}

// ----------------------------------------------------------------------------

containers::ViewContent ViewParser::get_content(
    const size_t _draw, const size_t _start, const size_t _length,
    const bool _force_nrows, const std::vector<ViewCol>& _cols) const {
  const auto to_column_view = [this](const auto& _col) -> ColumnViewVariant {
    return make_column_view(_col);
  };

  const auto to_string_vector =
      [this, _start, _length](
          const ColumnViewVariant& _column_view) -> std::vector<std::string> {
    return make_string_vector(_start, _length, _column_view);
  };

  const auto transpose = [](const auto& _data) {
    if (_data.size() == 0) {
      return _data;
    }
    std::vector<std::vector<std::string>> result;
    for (size_t i = 0; i < _data.at(0).size(); ++i) {
      std::vector<std::string> row;
      for (const auto& _col : _data) {
        assert_true(_col.size() == _data.at(0).size());
        row.push_back(_col.at(i));
      }
      result.push_back(row);
    }
    return result;
  };

  const auto column_views = _cols | std::views::transform(to_column_view) |
                            fct::ranges::to<std::vector>();

  const auto data =
      transpose(column_views | std::views::transform(to_string_vector) |
                fct::ranges::to<std::vector>());

  const auto nrows = make_nrows(column_views, _force_nrows);

  const auto basis = rfl::make_field<"draw", std::int32_t>(_draw) *
                     rfl::make_field<"data">(data);

  if (!nrows) {
    return basis;
  }

  return containers::DataFrameContent(
      basis * rfl::make_field<"recordsTotal">(*nrows) *
      rfl::make_field<"recordsFiltered">(*nrows));
}

// ----------------------------------------------------------------------------

containers::DataFrame ViewParser::parse(
    const commands::DataFrameOrView& _cmd) const {
  const auto handle = [this](const auto& _cmd) -> containers::DataFrame {
    using Type = std::decay_t<decltype(_cmd)>;

    if constexpr (std::is_same<Type, DataFrameOp>()) {
      const auto name = _cmd.name();
      return utils::Getter::get(name, *data_frames_);
    } else {
      const auto& base = *_cmd.base();

      auto df = parse(base);

      add_column(_cmd, &df);

      drop_columns(_cmd, &df);

      subselection(_cmd, &df);

      df.set_build_history(commands::Fingerprint(_cmd));

      return df;
    }
  };

  return rfl::visit(handle, _cmd.val_);
}

// ----------------------------------------------------------------------------

std::tuple<containers::DataFrame, std::vector<containers::DataFrame>,
           std::optional<containers::DataFrame>>
ViewParser::parse_all(const commands::DataFramesOrViews& _cmd) const {
  const auto to_df = [this](const auto& _obj) -> containers::DataFrame {
    return parse(_obj);
  };

  const auto population_obj = _cmd.population_df();

  const auto peripheral_objs = _cmd.peripheral_dfs();

  const auto validation_obj = _cmd.validation_df();

  const auto population = to_df(population_obj);

  const auto peripheral = peripheral_objs | std::views::transform(to_df) |
                          fct::ranges::to<std::vector>();

  const auto validation =
      validation_obj
          ? std::make_optional<containers::DataFrame>(to_df(*validation_obj))
          : std::optional<containers::DataFrame>();

  return std::make_tuple(population, peripheral, validation);
}

// ----------------------------------------------------------------------------

void ViewParser::subselection(const ViewOp& _cmd,
                              containers::DataFrame* _df) const {
  if (!_cmd.subselection()) {
    return;
  }

  const auto handle = [this, _df](const auto& _json_col) {
    using Type = std::decay_t<decltype(_json_col)>;

    if constexpr (std::is_same<Type, commands::BooleanColumnView>()) {
      const auto column_view =
          BoolOpParser(categories_, join_keys_encoding_, data_frames_)
              .parse(_json_col);

      const auto data_ptr = column_view.to_vector(0, _df->nrows(), true);

      assert_true(data_ptr);

      _df->where(*data_ptr);
    } else {
      const auto data_ptr =
          FloatOpParser(categories_, join_keys_encoding_, data_frames_)
              .parse(_json_col)
              .to_vector(0, std::nullopt, false);

      assert_true(data_ptr);

      auto const df_nrows = _df->nrows();
      auto indices =
          *data_ptr |
          std::views::transform([](Float const& index) -> std::size_t {
            if (index < 0.0) {
              throw std::runtime_error(
                  "Index on a numerical subselection cannot be "
                  "smaller than zero!");
            }
            return static_cast<std::size_t>(index);
          }) |
          std::views::filter([&df_nrows](std::size_t const& index) -> bool {
            return index < df_nrows;
          }) |
          fct::ranges::to<std::vector>();

      _df->sort_by_key(indices);
    }
  };

  return std::visit(handle, *_cmd.subselection());
}

// ----------------------------------------------------------------------------
}  // namespace handlers
}  // namespace engine
