// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef ENGINE_COMMANDS_GETBOOLEANCOLUMN_HPP_
#define ENGINE_COMMANDS_GETBOOLEANCOLUMN_HPP_

#include "engine/commands/BasicCommand.hpp"
#include "engine/commands/BooleanColumnView.hpp"
#include "fct/Field.hpp"
#include "fct/make_named_tuple.hpp"

namespace engine {
namespace commands {

/// Defines the command necessary tp retrieve a boolean column.
using GetBooleanColumn =
    fct::make_named_tuple<BasicCommand, fct::Field<"col_", BooleanColumnView>>;

}  // namespace commands
}  // namespace engine

#endif  // ENGINE_COMMANDS_GETBOOLEANCOLUMN_HPP_

