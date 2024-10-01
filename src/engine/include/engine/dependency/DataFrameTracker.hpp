// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef ENGINE_DEPENDENCY_DATAFRAMETRACKER_HPP_
#define ENGINE_DEPENDENCY_DATAFRAMETRACKER_HPP_

#include <map>
#include <optional>
#include <rfl/Ref.hpp>
#include <string>
#include <utility>
#include <vector>

#include "commands/Fingerprint.hpp"
#include "containers/DataFrame.hpp"

namespace engine {
namespace dependency {

class DataFrameTracker {
 public:
  DataFrameTracker(const rfl::Ref<std::map<std::string, containers::DataFrame>>&
                       _data_frames)
      : data_frames_(_data_frames) {}

  ~DataFrameTracker() = default;

 public:
  /// Adds a new element to be tracked.
  void add(const containers::DataFrame& _df,
           const commands::Fingerprint& _build_history);

  /// Removes all elements.
  void clear();

  /// Generates a build history from the dependencies returned by
  /// the pipeline and the df_fingerprints.
  commands::Fingerprint make_build_history(
      const std::vector<commands::Fingerprint>& _dependencies,
      const containers::DataFrame& _population_df,
      const std::vector<containers::DataFrame>& _peripheral_dfs) const;

  /// Retrieves a deep copy of an element from the tracker, if a data
  /// frame containing this build_history exists.
  std::optional<containers::DataFrame> retrieve(
      const commands::Fingerprint& _build_history) const;

 public:
  /// Generates the build history and retrieves the data frame.
  std::optional<containers::DataFrame> retrieve(
      const std::vector<commands::Fingerprint>& _dependencies,
      const containers::DataFrame& _population_df,
      const std::vector<containers::DataFrame>& _peripheral_dfs) const {
    const auto build_history =
        make_build_history(_dependencies, _population_df, _peripheral_dfs);
    return retrieve(build_history);
  }

 private:
  /// Removes all data frames that are no longer accessable.
  void clean_up();

  /// Returns the data frame designated by the hash, if such a data frame
  /// exists.
  std::optional<containers::DataFrame> get_df(const size_t _b_hash) const;

 private:
  /// The underlying data frames.
  rfl::Ref<std::map<std::string, containers::DataFrame>> data_frames_;

  /// A map keeping track of the names of the data frame and when they were
  /// last changed.
  std::map<size_t, std::pair<std::string, std::string>> pairs_;
};

}  // namespace dependency
}  // namespace engine

#endif  // ENGINE_DEPENDENCY_DATAFRAMETRACKER_HPP_
