// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "fct/Field.hpp"
#include "fct/NamedTuple.hpp"

#ifndef ENGINE_COMMANDS_BASICCOMMAND_HPP_
#define ENGINE_COMMANDS_BASICCOMMAND_HPP_

namespace engine {
namespace commands {

/// Infers the type of the command.
using f_name = fct::Field<"name_", std::string>;

/// The type of the command - so we can correctly identify this.
using f_type = fct::Field<"type_", std::string>;

/// The basis for all other commands.
using BasicCommand = fct::NamedTuple<f_name, f_type>;

}  // namespace commands
}  // namespace engine

#endif  // ENGINE_COMMANDS_BASICCOMMAND_HPP_
