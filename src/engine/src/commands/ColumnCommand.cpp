// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "commands/ColumnCommand.hpp"

#include "json/json.hpp"

namespace commands {

ColumnCommand ColumnCommand::from_json_obj(const InputVarType& _obj) {
  return ColumnCommand(json::from_json<ReflectionType>(_obj));
}

}  // namespace commands
