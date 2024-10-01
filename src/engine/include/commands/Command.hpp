// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef COMMANDS_COMMAND_HPP_
#define COMMANDS_COMMAND_HPP_

#include <rfl/Field.hpp>
#include <rfl/Literal.hpp>
#include <rfl/NamedTuple.hpp>
#include <rfl/json/Reader.hpp>
#include <variant>

#include "commands/ColumnCommand.hpp"
#include "commands/DataFrameCommand.hpp"
#include "commands/DatabaseCommand.hpp"
#include "commands/PipelineCommand.hpp"
#include "commands/ProjectCommand.hpp"
#include "commands/ViewCommand.hpp"

namespace commands {

struct Command {
  struct IsAliveOp {
    rfl::Field<"type_", rfl::Literal<"is_alive">> type;
  };

  struct MonitorURLOp {
    rfl::Field<"type_", rfl::Literal<"monitor_url">> type;
  };

  struct ShutdownOp {
    rfl::Field<"type_", rfl::Literal<"shutdown">> type;
  };

  using ReflectionType =
      std::variant<ColumnCommand, DatabaseCommand, DataFrameCommand,
                   PipelineCommand, ProjectCommand, ViewCommand, IsAliveOp,
                   MonitorURLOp, ShutdownOp>;

  using InputVarType = typename rfl::json::Reader::InputVarType;

  static Command from_json_obj(const InputVarType& _obj);

  ReflectionType val_;
};

}  // namespace commands

#endif  // COMMANDS_COMMAND_HPP_
