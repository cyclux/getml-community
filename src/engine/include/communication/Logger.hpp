// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef COMMUNICATION_LOGGER_HPP_
#define COMMUNICATION_LOGGER_HPP_

#include <memory>
#include <string>

#include "communication/Monitor.hpp"
#include "debug/debug.hpp"
#include "logging/logging.hpp"

namespace communication {

class Logger : public logging::AbstractLogger {
 public:
  Logger(const std::shared_ptr<const Monitor>& _monitor) : monitor_(_monitor) {
    assert_true(monitor_);
  }

  ~Logger() = default;

  /// Logs current events.
  void log(const std::string& _msg) const final;

 private:
  /// The Monitor is supposed to monitor all of the logs as well.
  const std::shared_ptr<const Monitor> monitor_;
};

}  // namespace communication

#endif  // COMMUNICATION_LOGGER_HPP_
