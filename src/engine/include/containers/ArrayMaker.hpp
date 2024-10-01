// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef CONTAINERS_ARRAYMAKER_HPP_
#define CONTAINERS_ARRAYMAKER_HPP_

#include <arrow/api.h>
#include <arrow/util/utf8.h>

#include <cctype>
#include <memory>
#include <utility>
#include <vector>

#include "containers/Float.hpp"
#include "debug/throw_unless.hpp"
#include "helpers/NullChecker.hpp"

namespace containers {

class ArrayMaker {
 public:
  /// The maximum size for the the chunks.
  constexpr static size_t MAX_CHUNKSIZE = 100000;

  /// Generates a boolean array.
  template <class IteratorType1, class IteratorType2>
  static std::shared_ptr<arrow::ChunkedArray> make_boolean_array(
      const IteratorType1 _begin, const IteratorType2 _end);

  /// Generates a float array.
  template <class IteratorType1, class IteratorType2>
  static std::shared_ptr<arrow::ChunkedArray> make_float_array(
      const IteratorType1 _begin, const IteratorType2 _end);

  /// Generates a string array.
  template <class IteratorType1, class IteratorType2>
  static std::shared_ptr<arrow::ChunkedArray> make_string_array(
      const IteratorType1 _begin, const IteratorType2 _end);

  /// Generates a time stamp array.
  template <class IteratorType1, class IteratorType2>
  static std::shared_ptr<arrow::ChunkedArray> make_time_stamp_array(
      const IteratorType1 _begin, const IteratorType2 _end);

 private:
  /// Generate a single chunk for the chunked array.
  template <class BuilderType>
  static std::shared_ptr<arrow::Array> make_chunk(BuilderType* _builder);

  /// Generate the chunks for the chunked array.
  template <class IteratorType1, class IteratorType2, class AppendFunctionType,
            class BuilderType>
  static std::vector<std::shared_ptr<arrow::Array>> make_chunks(
      const IteratorType1 _begin, const IteratorType2 _end,
      const AppendFunctionType _append_function, BuilderType* _builder);
};

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------

template <class IteratorType1, class IteratorType2>
std::shared_ptr<arrow::ChunkedArray> ArrayMaker::make_boolean_array(
    const IteratorType1 _begin, const IteratorType2 _end) {
  const auto append_function = [](const bool _val,
                                  arrow::BooleanBuilder* _builder) {
    const auto status = _builder->Append(_val);
    throw_unless(status.ok(), status.message());
  };

  arrow::BooleanBuilder builder;

  auto chunks = make_chunks(_begin, _end, append_function, &builder);

  return std::make_shared<arrow::ChunkedArray>(std::move(chunks));
}

// -------------------------------------------------------------------------

template <class BuilderType>
std::shared_ptr<arrow::Array> ArrayMaker::make_chunk(BuilderType* _builder) {
  std::shared_ptr<arrow::Array> array;

  const auto status = _builder->Finish(&array);

  if (!status.ok()) {
    throw std::runtime_error("Could not create array: " + status.message());
  }

  _builder->Reset();

  return array;
}

// -------------------------------------------------------------------------

template <class IteratorType1, class IteratorType2, class AppendFunctionType,
          class BuilderType>
std::vector<std::shared_ptr<arrow::Array>> ArrayMaker::make_chunks(
    const IteratorType1 _begin, const IteratorType2 _end,
    const AppendFunctionType _append_function, BuilderType* _builder) {
  const auto status = _builder->Resize(MAX_CHUNKSIZE);

  throw_unless(status.ok(), status.message());

  std::vector<std::shared_ptr<arrow::Array>> chunks;

  if (_begin == _end) {
    chunks.emplace_back(make_chunk(_builder));
  }

  auto it = _begin;

  while (it != _end) {
    for (size_t i = 0; i < MAX_CHUNKSIZE; ++i) {
      _append_function(*it, _builder);

      if (++it == _end) {
        break;
      }
    }

    chunks.emplace_back(make_chunk(_builder));
  }

  return chunks;
}

// ----------------------------------------------------------------------------

template <class IteratorType1, class IteratorType2>
std::shared_ptr<arrow::ChunkedArray> ArrayMaker::make_float_array(
    const IteratorType1 _begin, const IteratorType2 _end) {
  const auto append_function = [](const Float _val,
                                  arrow::DoubleBuilder* _builder) {
    if (helpers::NullChecker::is_null(_val)) {
      const auto status = _builder->AppendNull();
      throw_unless(status.ok(), status.message());
    } else {
      const auto status = _builder->Append(_val);
      throw_unless(status.ok(), status.message());
    }
  };

  arrow::DoubleBuilder builder;

  auto chunks = make_chunks(_begin, _end, append_function, &builder);

  return std::make_shared<arrow::ChunkedArray>(std::move(chunks));
}

// -------------------------------------------------------------------------

template <class IteratorType1, class IteratorType2>
std::shared_ptr<arrow::ChunkedArray> ArrayMaker::make_string_array(
    const IteratorType1 _begin, const IteratorType2 _end) {
  arrow::util::InitializeUTF8();

  const auto append_function = [](const std::string& _str,
                                  arrow::StringBuilder* _builder) {
    if (helpers::NullChecker::is_null(_str) ||
        !arrow::util::ValidateUTF8(_str)) {
      const auto status = _builder->AppendNull();
      throw_unless(status.ok(), status.message());
    } else {
      const auto status = _builder->Append(_str);
      throw_unless(status.ok(), status.message());
    }
  };

  arrow::StringBuilder builder;

  auto chunks = make_chunks(_begin, _end, append_function, &builder);

  return std::make_shared<arrow::ChunkedArray>(std::move(chunks));
}

// -------------------------------------------------------------------------

template <class IteratorType1, class IteratorType2>
std::shared_ptr<arrow::ChunkedArray> ArrayMaker::make_time_stamp_array(
    IteratorType1 _begin, IteratorType2 _end) {
  const auto append_function = [](const Float _val,
                                  arrow::TimestampBuilder* _builder) {
    if (helpers::NullChecker::is_null(_val)) {
      const auto status = _builder->AppendNull();
      throw_unless(status.ok(), status.message());
    } else {
      const auto status =
          _builder->Append(static_cast<std::int64_t>(_val * 1.0e+09));
      throw_unless(status.ok(), status.message());
    }
  };

  arrow::TimestampBuilder builder(arrow::timestamp(arrow::TimeUnit::NANO),
                                  arrow::default_memory_pool());

  auto chunks = make_chunks(_begin, _end, append_function, &builder);

  return std::make_shared<arrow::ChunkedArray>(std::move(chunks));
}

// ----------------------------------------------------------------------------
}  // namespace containers

#endif  // CONTAINERS_CATEGORICALFEATURES_HPP_
