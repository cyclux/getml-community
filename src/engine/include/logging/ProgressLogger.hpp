// Copyright 2024 Code17 GmbH
// 
// This file is licensed under the Elastic License 2.0 (ELv2). 
// Refer to the LICENSE.txt file in the root of the repository 
// for details.
// 

#ifndef LOGGING_PROGRESSLOGGER_HPP_
#define LOGGING_PROGRESSLOGGER_HPP_

// ----------------------------------------------------------------------------

#include <memory>
#include <string>

// ----------------------------------------------------------------------------

#include "debug/debug.hpp"

// ----------------------------------------------------------------------------

#include "logging/AbstractLogger.hpp"

// ----------------------------------------------------------------------------

namespace logging {
// ------------------------------------------------------------------------

class ProgressLogger {
  // --------------------------------------------------------

 public:
  ProgressLogger(const std::string& _msg,
                 const std::shared_ptr<const AbstractLogger>& _logger,
                 const size_t _total, const size_t _begin = 0,
                 const size_t _end = 100)
      : begin_(_begin),
        current_value_(0),
        end_(_end),
        logger_(_logger),
        total_(_total) {
    if (logger_ && total_ > 0 && _msg != "") {
      logger_->log(_msg);
    }
  }

  ~ProgressLogger() = default;

  // --------------------------------------------------------

  /// Increments the progress.
  void increment(const size_t _by = 1) {
    if (_by == 0) {
      return;
    }

    current_value_ += _by;

    assert_msg(current_value_ <= total_,
               "current_value_: " + std::to_string(current_value_) +
                   ", total_: " + std::to_string(total_));

    if (logger_ && total_ > 0) {
      const auto progress =
          begin_ + (current_value_ * (end_ - begin_)) / total_;
      logger_->log("Progress: " + std::to_string(progress) + "%.");
    }
  }

  // ----------------------------------------------------

 private:
  /// Where to begin the logging (usually at 0).
  const size_t begin_;

  /// The current iteration.
  size_t current_value_;

  /// The final value of the logging (usually at 100).
  const size_t end_;

  /// Pointer to the underlying logger.
  const std::shared_ptr<const AbstractLogger> logger_;

  /// The total number of expected iterations.
  const size_t total_;

  // ----------------------------------------------------
};

// ------------------------------------------------------------------------
}  // namespace logging

#endif  // LOGGING_PROGRESSLOGGER_HPP_
