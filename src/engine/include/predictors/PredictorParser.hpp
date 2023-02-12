// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef PREDICTORS_PREDICTORPARSER_HPP_
#define PREDICTORS_PREDICTORPARSER_HPP_

#include <Poco/JSON/Object.h>

#include <memory>
#include <vector>

#include "fct/Ref.hpp"
#include "json/json.hpp"
#include "predictors/Predictor.hpp"
#include "predictors/PredictorFingerprint.hpp"
#include "predictors/PredictorHyperparams.hpp"
#include "predictors/PredictorImpl.hpp"

namespace predictors {

struct PredictorParser {
  using DependencyType = typename PredictorFingerprint::DependencyType;

  /// Parses the predictor.
  static fct::Ref<Predictor> parse(
      const PredictorHyperparams& _cmd,
      const fct::Ref<const PredictorImpl>& _impl,
      const std::vector<DependencyType>& _dependencies);

  /// TODO: Remove this temporary fix.
  static fct::Ref<Predictor> parse(
      const Poco::JSON::Object& _json_obj,
      const fct::Ref<const PredictorImpl>& _impl,
      const std::vector<Poco::JSON::Object::Ptr>& _dependencies) {
    return parse(
        json::from_json<PredictorHyperparams>(_json_obj), _impl,
        json::Parser<std::vector<DependencyType> >::from_json(_dependencies));
  }
};

}  // namespace predictors

#endif  // PREDICTORS_PREDICTORPARSER_HPP_
