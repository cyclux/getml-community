// Copyright 2022 The SQLNet Company GmbH
// 
// This file is licensed under the Elastic License 2.0 (ELv2). 
// Refer to the LICENSE.txt file in the root of the repository 
// for details.
// 

#ifndef ENGINE_COMMUNICATION_SOCKETLOGGER_HPP_
#define ENGINE_COMMUNICATION_SOCKETLOGGER_HPP_

// ----------------------------------------------------------------------------

#include <Poco/Net/StreamSocket.h>

// ----------------------------------------------------------------------------

#include <memory>
#include <string>

// ----------------------------------------------------------------------------

#include "debug/debug.hpp"
#include "logging/logging.hpp"

// ----------------------------------------------------------------------------

#include "engine/communication/Sender.hpp"

// ----------------------------------------------------------------------------

namespace engine {
namespace communication {
// ------------------------------------------------------------------------

class SocketLogger : public logging::AbstractLogger {
 public:
  SocketLogger(const std::shared_ptr<const communication::Logger>& _logger,
               const bool _silent, Poco::Net::StreamSocket* _socket)
      : logger_(_logger), silent_(_silent), socket_(_socket) {
    assert_true(logger_);
  }

  ~SocketLogger() = default;

  // --------------------------------------------------------

  /// Logs current events.
  void log(const std::string& _msg) const final {
    if (!silent_) {
      logger_->log(_msg);
    }

    Sender::send_string("log: " + _msg, socket_);
  }

  // ----------------------------------------------------

 private:
  /// The Monitor is supposed to monitor all of the logs as well.
  const std::shared_ptr<const communication::Logger> logger_;

  /// Whether we want the progress to appear in the engine and the monitor
  /// log.
  const bool silent_;

  /// The socket to which we want to send the logs.
  Poco::Net::StreamSocket* socket_;

  // ----------------------------------------------------
};

// ------------------------------------------------------------------------
}  // namespace communication
}  // namespace engine

#endif  // ENGINE_COMMUNICATION_SOCKETLOGGER_HPP_
