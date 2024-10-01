// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "engine/handlers/ColumnManager.hpp"

#include <Poco/TemporaryFile.h>

#include <rfl/as.hpp>

#include "containers/Roles.hpp"
#include "engine/handlers/AggOpParser.hpp"
#include "engine/handlers/ArrowHandler.hpp"
#include "engine/handlers/BoolOpParser.hpp"
#include "engine/handlers/DataFrameManager.hpp"
#include "engine/handlers/FloatOpParser.hpp"
#include "engine/handlers/StringOpParser.hpp"
#include "multithreading/WriteLock.hpp"

namespace engine {
namespace handlers {

void ColumnManager::add_float_column(
    const typename Command::FloatColumnOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& df_name = _cmd.df_name();

  multithreading::WeakWriteLock weak_write_lock(params_.read_write_lock_);

  auto [df, exists] = utils::Getter::get_if_exists(df_name, &data_frames());

  if (exists) {
    DataFrameManager(params_).recv_and_add_float_column(
        rfl::as<DataFrameManager::RecvAndAddOp>(_cmd), df, &weak_write_lock,
        _socket);

  } else {
    const auto pool = params_.options_.make_pool();

    auto new_df =
        containers::DataFrame(df_name, params_.categories_.ptr(),
                              params_.join_keys_encoding_.ptr(), pool);

    DataFrameManager(params_).recv_and_add_float_column(
        rfl::as<DataFrameManager::RecvAndAddOp>(_cmd), &new_df,
        &weak_write_lock, _socket);

    data_frames()[df_name] = new_df;

    data_frames()[df_name].create_indices();
  }

  weak_write_lock.unlock();

  communication::Sender::send_string("Success!", _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::add_string_column(
    const typename Command::StringColumnOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& df_name = _cmd.df_name();

  multithreading::WeakWriteLock weak_write_lock(params_.read_write_lock_);

  auto [df, exists] = utils::Getter::get_if_exists(df_name, &data_frames());

  if (exists) {
    DataFrameManager(params_).recv_and_add_string_column(
        rfl::as<DataFrameManager::RecvAndAddOp>(_cmd), df, &weak_write_lock,
        _socket);

  } else {
    const auto pool = params_.options_.make_pool();

    auto new_df =
        containers::DataFrame(df_name, params_.categories_.ptr(),
                              params_.join_keys_encoding_.ptr(), pool);

    DataFrameManager(params_).recv_and_add_string_column(
        rfl::as<DataFrameManager::RecvAndAddOp>(_cmd), &new_df,
        &weak_write_lock, _socket);

    data_frames()[df_name] = new_df;

    data_frames()[df_name].create_indices();
  }

  weak_write_lock.unlock();

  communication::Sender::send_string("Success!", _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::aggregate(const typename Command::AggregationOp& _cmd,
                              Poco::Net::StreamSocket* _socket) {
  const auto& aggregation = _cmd.aggregation();

  auto response = containers::Column<Float>(nullptr, 1);

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  response[0] = AggOpParser(params_.categories_, params_.join_keys_encoding_,
                            params_.data_frames_)
                    .aggregate(aggregation);

  read_lock.unlock();

  communication::Sender::send_string("Success!", _socket);

  communication::Sender::send_column(response, _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_boolean_column(
    const typename Command::GetBooleanColumnOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      BoolOpParser(params_.categories_, params_.join_keys_encoding_,
                   params_.data_frames_)
          .parse(json_col);

  if (column_view.is_infinite()) {
    throw std::runtime_error(
        "The length of the column view is infinite! You can look at "
        "the column view, "
        "but you cannot retrieve it.");
  }

  const auto array = column_view.to_array(0, std::nullopt, false);

  read_lock.unlock();

  const auto field = arrow::field("column", arrow::boolean());

  handlers::ArrowHandler(params_.categories_, params_.join_keys_encoding_,
                         params_.options_)
      .send_array(array, field, _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_boolean_column_content(
    const typename Command::GetBooleanColumnContentOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto draw = _cmd.draw();

  const auto length = _cmd.length();

  const auto start = _cmd.start();

  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      BoolOpParser(params_.categories_, params_.join_keys_encoding_,
                   params_.data_frames_)
          .parse(json_col);

  const auto data_ptr = column_view.to_vector(start, length, false);

  assert_true(data_ptr);

  const auto nrows = std::holds_alternative<size_t>(column_view.nrows())
                         ? std::get<size_t>(column_view.nrows())
                         : length;

  const auto col_str = DataFrameManager(params_).make_column_string<bool>(
      draw, nrows, data_ptr->begin(), data_ptr->end());

  read_lock.unlock();

  communication::Sender::send_string(col_str, _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_boolean_column_nrows(
    const typename Command::GetBooleanColumnNRowsOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      BoolOpParser(params_.categories_, params_.join_keys_encoding_,
                   params_.data_frames_)
          .parse(json_col);

  read_lock.unlock();

  communication::Sender::send_string("Found!", _socket);

  communication::Sender::send_string(column_view.nrows_to_str(), _socket);
}
// ------------------------------------------------------------------------

void ColumnManager::get_categorical_column(
    const typename Command::GetStringColumnOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      StringOpParser(params_.categories_, params_.join_keys_encoding_,
                     params_.data_frames_)
          .parse(json_col);

  if (column_view.is_infinite()) {
    throw std::runtime_error(
        "The length of the column view is infinite! You can look at "
        "the column view, "
        "but you cannot retrieve it.");
  }

  const auto array = column_view.to_array(0, std::nullopt, false);

  read_lock.unlock();

  const auto field = arrow::field("column", arrow::utf8());

  handlers::ArrowHandler(params_.categories_, params_.join_keys_encoding_,
                         params_.options_)
      .send_array(array, field, _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_categorical_column_content(
    const typename Command::GetStringColumnContentOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto draw = _cmd.draw();

  const auto length = _cmd.length();

  const auto start = _cmd.start();

  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      StringOpParser(params_.categories_, params_.join_keys_encoding_,
                     params_.data_frames_)
          .parse(json_col);

  const auto data_ptr = column_view.to_vector(start, length, false);

  assert_true(data_ptr);

  const auto nrows = std::holds_alternative<size_t>(column_view.nrows())
                         ? std::get<size_t>(column_view.nrows())
                         : length;

  const auto col_str =
      DataFrameManager(params_).make_column_string<strings::String>(
          draw, nrows, data_ptr->begin(), data_ptr->end());

  read_lock.unlock();

  communication::Sender::send_string(col_str, _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_categorical_column_nrows(
    const typename Command::GetStringColumnNRowsOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      StringOpParser(params_.categories_, params_.join_keys_encoding_,
                     params_.data_frames_)
          .parse(json_col);

  read_lock.unlock();

  communication::Sender::send_string("Found!", _socket);

  communication::Sender::send_string(column_view.nrows_to_str(), _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_categorical_column_unique(
    const typename Command::GetStringColumnUniqueOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      StringOpParser(params_.categories_, params_.join_keys_encoding_,
                     params_.data_frames_)
          .parse(json_col);

  const auto array = column_view.unique();

  read_lock.unlock();

  const auto field = arrow::field("column", arrow::utf8());

  handlers::ArrowHandler(params_.categories_, params_.join_keys_encoding_,
                         params_.options_)
      .send_array(array, field, _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_column(const typename Command::GetFloatColumnOp& _cmd,
                               Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      FloatOpParser(params_.categories_, params_.join_keys_encoding_,
                    params_.data_frames_)
          .parse(json_col);

  if (column_view.is_infinite()) {
    throw std::runtime_error(
        "The length of the column view is infinite! You can look at "
        "the column view, "
        "but you cannot retrieve it.");
  }

  const auto array = column_view.to_array(0, std::nullopt, false);

  read_lock.unlock();

  const auto field =
      column_view.unit().find("time stamp") != std::string::npos
          ? arrow::field("column", arrow::timestamp(arrow::TimeUnit::NANO))
          : arrow::field("column", arrow::float64());

  handlers::ArrowHandler(params_.categories_, params_.join_keys_encoding_,
                         params_.options_)
      .send_array(array, field, _socket);

  communication::Sender::send_string("Success!", _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_column_nrows(
    const typename Command::GetFloatColumnNRowsOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      FloatOpParser(params_.categories_, params_.join_keys_encoding_,
                    params_.data_frames_)
          .parse(json_col);

  read_lock.unlock();

  communication::Sender::send_string("Found!", _socket);

  communication::Sender::send_string(column_view.nrows_to_str(), _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_column_unique(
    const typename Command::GetFloatColumnUniqueOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      FloatOpParser(params_.categories_, params_.join_keys_encoding_,
                    params_.data_frames_)
          .parse(json_col);

  const auto array = column_view.unique();

  read_lock.unlock();

  const auto field =
      column_view.unit().find("time stamp") != std::string::npos
          ? arrow::field("column", arrow::timestamp(arrow::TimeUnit::NANO))
          : arrow::field("column", arrow::float64());

  handlers::ArrowHandler(params_.categories_, params_.join_keys_encoding_,
                         params_.options_)
      .send_array(array, field, _socket);

  communication::Sender::send_string("Success!", _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_float_column_content(
    const typename Command::GetFloatColumnContentOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto draw = _cmd.draw();

  const auto length = _cmd.length();

  const auto start = _cmd.start();

  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      FloatOpParser(params_.categories_, params_.join_keys_encoding_,
                    params_.data_frames_)
          .parse(json_col);

  const auto col = column_view.to_column(start, length, false);

  const auto nrows = std::holds_alternative<size_t>(column_view.nrows())
                         ? std::get<size_t>(column_view.nrows())
                         : length;

  const auto col_str = DataFrameManager(params_).make_column_string<Float>(
      draw, nrows, col.begin(), col.end());

  read_lock.unlock();

  communication::Sender::send_string(col_str, _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_subroles(
    const typename Command::GetFloatColumnSubrolesOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      FloatOpParser(params_.categories_, params_.join_keys_encoding_,
                    params_.data_frames_)
          .parse(json_col);

  read_lock.unlock();

  communication::Sender::send_string("Success!", _socket);

  communication::Sender::send_categorical_column(column_view.subroles(),
                                                 _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_subroles_categorical(
    const typename Command::GetStringColumnSubrolesOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      StringOpParser(params_.categories_, params_.join_keys_encoding_,
                     params_.data_frames_)
          .parse(json_col);

  read_lock.unlock();

  communication::Sender::send_string("Success!", _socket);

  communication::Sender::send_categorical_column(column_view.subroles(),
                                                 _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_unit(const typename Command::GetFloatColumnUnitOp& _cmd,
                             Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      FloatOpParser(params_.categories_, params_.join_keys_encoding_,
                    params_.data_frames_)
          .parse(json_col);

  read_lock.unlock();

  communication::Sender::send_string("Success!", _socket);

  communication::Sender::send_string(column_view.unit(), _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::get_unit_categorical(
    const typename Command::GetStringColumnUnitOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& json_col = _cmd.col();

  multithreading::ReadLock read_lock(params_.read_write_lock_);

  const auto column_view =
      StringOpParser(params_.categories_, params_.join_keys_encoding_,
                     params_.data_frames_)
          .parse(json_col);

  read_lock.unlock();

  communication::Sender::send_string("Success!", _socket);

  communication::Sender::send_string(column_view.unit(), _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::set_subroles(
    const typename Command::SetFloatColumnSubrolesOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& name = _cmd.name();

  const auto& role = _cmd.role();

  const auto& df_name = _cmd.df_name();

  const auto& subroles = _cmd.subroles();

  multithreading::WriteLock write_lock(params_.read_write_lock_);

  auto& df = utils::Getter::get(df_name, &data_frames());

  auto column = df.float_column(name, role);

  column.set_subroles(subroles);

  df.add_float_column(column, role);

  write_lock.unlock();

  communication::Sender::send_string("Success!", _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::set_subroles_categorical(
    const typename Command::SetStringColumnSubrolesOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& name = _cmd.name();

  const auto& role = _cmd.role();

  const auto& df_name = _cmd.df_name();

  const auto& subroles = _cmd.subroles();

  multithreading::WriteLock write_lock(params_.read_write_lock_);

  auto& df = utils::Getter::get(df_name, &data_frames());

  if (role == containers::DataFrame::ROLE_UNUSED ||
      role == containers::DataFrame::ROLE_UNUSED_STRING) {
    auto column = df.unused_string(name);
    column.set_subroles(subroles);
    df.add_string_column(column, role);
  } else if (role == containers::DataFrame::ROLE_TEXT) {
    auto column = df.text(name);
    column.set_subroles(subroles);
    df.add_string_column(column, role);
  } else {
    auto column = df.int_column(name, role);
    column.set_subroles(subroles);
    df.add_int_column(column, role);
  }

  write_lock.unlock();

  communication::Sender::send_string("Success!", _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::set_unit(const typename Command::SetFloatColumnUnitOp& _cmd,
                             Poco::Net::StreamSocket* _socket) {
  const auto& name = _cmd.name();

  const auto& role = _cmd.role();

  const auto& df_name = _cmd.df_name();

  const auto& unit = _cmd.unit();

  multithreading::WriteLock write_lock(params_.read_write_lock_);

  auto& df = utils::Getter::get(df_name, &data_frames());

  auto column = df.float_column(name, role);

  column.set_unit(unit);

  df.add_float_column(column, role);

  write_lock.unlock();

  communication::Sender::send_string("Success!", _socket);
}

// ------------------------------------------------------------------------

void ColumnManager::set_unit_categorical(
    const typename Command::SetStringColumnUnitOp& _cmd,
    Poco::Net::StreamSocket* _socket) {
  const auto& name = _cmd.name();

  const auto& role = _cmd.role();

  const auto& df_name = _cmd.df_name();

  const auto& unit = _cmd.unit();

  multithreading::WriteLock write_lock(params_.read_write_lock_);

  auto& df = utils::Getter::get(df_name, &data_frames());

  if (role == containers::DataFrame::ROLE_UNUSED ||
      role == containers::DataFrame::ROLE_UNUSED_STRING) {
    auto column = df.unused_string(name);
    column.set_unit(unit);
    df.add_string_column(column, role);
  } else if (role == containers::DataFrame::ROLE_TEXT) {
    auto column = df.text(name);
    column.set_unit(unit);
    df.add_string_column(column, role);
  } else {
    auto column = df.int_column(name, role);
    column.set_unit(unit);
    df.add_int_column(column, role);
  }

  write_lock.unlock();

  communication::Sender::send_string("Success!", _socket);
}

}  // namespace handlers
}  // namespace engine
