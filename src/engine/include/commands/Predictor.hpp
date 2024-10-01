// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef COMMANDS_PREDICTOR_HPP_
#define COMMANDS_PREDICTOR_HPP_

#include <rfl/TaggedUnion.hpp>

#include "commands/LinearRegressionHyperparams.hpp"
#include "commands/LogisticRegressionHyperparams.hpp"
#include "commands/XGBoostHyperparams.hpp"

namespace commands {

using Predictor =
    rfl::TaggedUnion<"type_", LinearRegressionHyperparams,
                     LogisticRegressionHyperparams, XGBoostHyperparams>;

}  // namespace commands

#endif  // ENGINE_PREDICTORS_PREDICTOR_HPP_
