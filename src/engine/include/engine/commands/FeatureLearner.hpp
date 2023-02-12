// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef ENGINE_COMMANDS_FEATURELEARNER_HPP_
#define ENGINE_COMMANDS_FEATURELEARNER_HPP_

#include "fastprop/Hyperparameters.hpp"
#include "fct/TaggedUnion.hpp"

namespace engine {
namespace commands {

using FeatureLearner = fct::TaggedUnion<"type_", fastprop::Hyperparameters>;

}  // namespace commands
}  // namespace engine

#endif  // ENGINE_COMMANDS_FEATURELEARNER_HPP_
