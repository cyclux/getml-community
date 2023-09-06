// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "communication/Warnings.hpp"

#include "communication/Sender.hpp"
#include "json/json.hpp"
#include "rfl/make_named_tuple.hpp"

namespace communication {

void Warnings::send(Poco::Net::StreamSocket* _socket) const {
  const auto named_tuple =
      rfl::make_named_tuple(rfl::make_field<"warnings_">(warnings_));
  Sender::send_string(json::to_json(named_tuple), _socket);
}

}  // namespace communication
