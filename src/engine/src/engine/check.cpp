// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "engine/pipelines/check.hpp"

#include "commands/Fingerprint.hpp"
#include "commands/WarningFingerprint.hpp"
#include "engine/pipelines/FitPreprocessorsParams.hpp"
#include "engine/pipelines/fit.hpp"
#include "fct/Field.hpp"
#include "fct/collect.hpp"
#include "json/json.hpp"

namespace engine {
namespace pipelines {
namespace check {

/// Generates the fingerprints for the feature learners, needed for the
/// warning.
static std::pair<std::vector<fct::Ref<featurelearners::AbstractFeatureLearner>>,
                 fct::Ref<const std::vector<commands::Fingerprint>>>
init_feature_learners(
    const Pipeline& _pipeline,
    const featurelearners::FeatureLearnerParams& _feature_learner_params,
    const CheckParams& _params);

// ----------------------------------------------------------------------------

void check(const Pipeline& _pipeline, const CheckParams& _params) {
  // TODO: We are forced to generate the modified tables, even when we can
  // retrieve the check, but we really only need the modified schemata at this
  // point. Fix this.
  const auto fit_preprocessors_params = FitPreprocessorsParams{
      .categories_ = _params.get<"categories_">(),
      .cmd_ = _params.get<"cmd_">(),
      .logger_ = _params.get<"logger_">(),
      .peripheral_dfs_ = _params.get<"peripheral_dfs_">(),
      .population_df_ = _params.get<"population_df_">(),
      .preprocessor_tracker_ = _params.get<"preprocessor_tracker_">(),
      .socket_ = _params.get<"socket_">()};

  const auto preprocessed =
      fit::fit_preprocessors_only(_pipeline, fit_preprocessors_params);

  const auto [modified_population_schema, modified_peripheral_schema] =
      fit::extract_schemata(preprocessed.population_df_,
                            preprocessed.peripheral_dfs_, true);

  const auto [placeholder, peripheral_names] = _pipeline.make_placeholder();

  const auto feature_learner_params = featurelearners::FeatureLearnerParams(
      fct::make_field<"dependencies_">(preprocessed.preprocessor_fingerprints_),
      fct::make_field<"peripheral_">(peripheral_names),
      fct::make_field<"peripheral_schema_">(modified_peripheral_schema),
      fct::make_field<"placeholder_">(placeholder),
      fct::make_field<"population_schema_">(modified_population_schema),
      fct::make_field<"target_num_">(
          featurelearners::AbstractFeatureLearner::USE_ALL_TARGETS));

  const auto [feature_learners, fl_fingerprints] =
      init_feature_learners(_pipeline, feature_learner_params, _params);

  const auto warning_fingerprint = commands::WarningFingerprint(
      fct::make_field<"fl_fingerprints_">(fl_fingerprints));

  const auto retrieved =
      _params.get<"warning_tracker_">()->retrieve(warning_fingerprint);

  if (retrieved) {
    communication::Sender::send_string("Success!", _params.get<"socket_">());
    retrieved->send(_params.get<"socket_">());
    return;
  }

  const auto socket_logger =
      _params.get<"logger_">()
          ? std::make_shared<const communication::SocketLogger>(
                _params.get<"logger_">(), true, _params.get<"socket_">())
          : std::shared_ptr<const communication::SocketLogger>();

  // TODO: Use fct::Ref
  const auto to_ptr = [](const auto& _fl) { return _fl.ptr(); };

  const auto fl_shared_ptr = fct::collect::vector<
      std::shared_ptr<featurelearners::AbstractFeatureLearner>>(
      feature_learners | VIEWS::transform(to_ptr));

  const auto warner = preprocessors::data_model_checking::check(
      placeholder.ptr(), peripheral_names.ptr(), preprocessed.population_df_,
      preprocessed.peripheral_dfs_, fl_shared_ptr, socket_logger);

  communication::Sender::send_string("Success!", _params.get<"socket_">());

  const auto warnings = warner.to_warnings_obj(warning_fingerprint);

  warnings->send(_params.get<"socket_">());

  _params.get<"warning_tracker_">()->add(warnings);
}

// ----------------------------------------------------------------------------

std::pair<std::vector<fct::Ref<featurelearners::AbstractFeatureLearner>>,
          fct::Ref<const std::vector<commands::Fingerprint>>>
init_feature_learners(
    const Pipeline& _pipeline,
    const featurelearners::FeatureLearnerParams& _feature_learner_params,
    const CheckParams& _params) {
  const auto df_fingerprints =
      fit::extract_df_fingerprints(_pipeline, _params.get<"population_df_">(),
                                   _params.get<"peripheral_dfs_">());

  const auto preprocessors =
      fit::init_preprocessors(_pipeline, df_fingerprints);

  const auto preprocessor_fingerprints =
      fit::extract_preprocessor_fingerprints(preprocessors, df_fingerprints);

  const auto feature_learners =
      fit::init_feature_learners(_pipeline, _feature_learner_params,
                                 _params.get<"population_df_">().num_targets());

  return std::make_pair(feature_learners,
                        fit::extract_fl_fingerprints(
                            feature_learners, preprocessor_fingerprints));
}

}  // namespace check
}  // namespace pipelines
}  // namespace engine
