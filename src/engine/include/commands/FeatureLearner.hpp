// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef COMMANDS_FEATURELEARNER_HPP_
#define COMMANDS_FEATURELEARNER_HPP_

#include <rfl/TaggedUnion.hpp>

#include "commands/NotSupportedInCommunity.hpp"
#include "fastprop/Hyperparameters.hpp"

namespace commands {

using FeatureLearner = rfl::TaggedUnion<
    "type_", fastprop::Hyperparameters, NotSupportedInCommunity<"Fastboost">,
    NotSupportedInCommunity<"Multirel">, NotSupportedInCommunity<"Relboost">,
    NotSupportedInCommunity<"RelMT">>;

}  // namespace commands

#endif  // COMMANDS_FEATURELEARNER_HPP_
