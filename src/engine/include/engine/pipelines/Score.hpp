// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef ENGINE_PIPELINES_SCORE_HPP_
#define ENGINE_PIPELINES_SCORE_HPP_

#include <Poco/Net/StreamSocket.h>

#include <algorithm>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "containers/containers.hpp"
#include "engine/pipelines/FittedPipeline.hpp"
#include "engine/pipelines/Pipeline.hpp"
#include "engine/pipelines/Predictors.hpp"
#include "metrics/metrics.hpp"

namespace engine {
namespace pipelines {

class Score {
 public:
  /// Calculates summary statistics for the features.
  static std::shared_ptr<const metrics::Scores> calculate_feature_stats(
      const Pipeline& _pipeline, const FittedPipeline& _fitted,
      const containers::NumericalFeatures _features,
      const containers::DataFrame& _population_df);

  /// Calculates the column importances.
  static std::pair<std::vector<helpers::ColumnDescription>,
                   std::vector<std::vector<Float>>>
  column_importances(const Pipeline& _pipeline, const FittedPipeline& _fitted);

  /// Calculate the feature importances.
  static std::vector<std::vector<Float>> feature_importances(
      const Predictors& _predictors);

  /// Scores the pipeline.
  static fct::Ref<const metrics::Scores> score(
      const Pipeline& _pipeline, const FittedPipeline& _fitted,
      const containers::DataFrame& _population_df,
      const std::string& _population_name,
      const containers::NumericalFeatures& _yhat);

  /// Expresses a nested vector in transposed form.
  static std::vector<std::vector<Float>> transpose(
      const std::vector<std::vector<Float>>& _original);

 private:
  /// Calculates the column importances for the autofeatures.
  static void column_importances_auto(
      const FittedPipeline& _fitted,
      const std::vector<std::vector<Float>>& _f_importances,
      std::vector<helpers::ImportanceMaker>* _importance_makers);

  /// Calculates the columns importances for the manual features.
  static void column_importances_manual(
      const Pipeline& _pipeline, const FittedPipeline& _fitted,
      const std::vector<std::vector<Float>>& _f_importances,
      std::vector<helpers::ImportanceMaker>* _importance_makers);

  /// Extracts the columns descriptions
  static void extract_coldesc(
      const std::map<helpers::ColumnDescription, Float>& _column_importances,
      std::vector<helpers::ColumnDescription>* _coldesc);

  /// Extracts the importance values.
  static void extract_importance_values(
      const std::map<helpers::ColumnDescription, Float>& _column_importances,
      std::vector<std::vector<Float>>* _all_column_importances);

  /// Fills the columns descriptions that have not been assigned with importance
  /// value 0.0.
  static void fill_zeros(std::vector<helpers::ImportanceMaker>* _f_importances);

  /// The importances factors are needed for backpropagating the feature
  /// importances to the column importances.
  static std::vector<Float> make_importance_factors(
      const size_t _num_features, const std::vector<size_t>& _autofeatures,
      const std::vector<Float>::const_iterator _begin,
      const std::vector<Float>::const_iterator _end);
};

// ----------------------------------------------------------------------------
}  // namespace pipelines
}  // namespace engine

#endif  // ENGINE_PIPELINES_SCORE_HPP_
