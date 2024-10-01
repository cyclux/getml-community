// Copyright 2024 Code17 GmbH
// 
// This file is licensed under the Elastic License 2.0 (ELv2). 
// Refer to the LICENSE.txt file in the root of the repository 
// for details.
// 

#ifndef FASTPROP_ALGORITHM_RSQUARED_HPP_
#define FASTPROP_ALGORITHM_RSQUARED_HPP_

// ----------------------------------------------------------------------------

#include <vector>

// ----------------------------------------------------------------------------

#include "fastprop/containers/containers.hpp"

// ----------------------------------------------------------------------------

#include "fastprop/Float.hpp"
#include "fastprop/Int.hpp"

// ----------------------------------------------------------------------------

namespace fastprop {
namespace algorithm {

class RSquared {
 public:
  /// Calculates the r-squared value of the features vis-a-vis the targets.
  static std::vector<Float> calculate(
      const std::vector<containers::Column<Float>>& _targets,
      const containers::Features& _features,
      const std::vector<size_t>& _rownums);

 private:
  /// Calculates the average R-Squared of a feature vis-a-vis all targets.
  static Float calc_for_feature(
      const std::vector<Float>& _mean_targets,
      const std::vector<Float>& _var_targets,
      const std::vector<containers::Column<Float>>& _targets,
      const helpers::Feature<Float, false>& _feature,
      const std::vector<size_t>& _rownums);

  /// Calculates the R-squared between a feature and a target.
  static Float calc_for_target(const Float _mean_target,
                               const Float _var_target,
                               const containers::Column<Float>& _targets,
                               const helpers::Feature<Float, false>& _feature,
                               const std::vector<size_t>& _rownums);

  /// Calculates the mean for each of the targets.
  static std::vector<Float> calc_mean_targets(
      const std::vector<containers::Column<Float>>& _targets,
      const std::vector<size_t>& _rownums);

  /// Calculates the variance for each of the targets.
  static std::vector<Float> calc_var_targets(
      const std::vector<containers::Column<Float>>& _targets,
      const std::vector<size_t>& _rownums);
};

// ------------------------------------------------------------------------
}  // namespace algorithm
}  // namespace fastprop

// ----------------------------------------------------------------------------

#endif  // FASTPROP_ALGORITHM_RSQUARED_HPP_
