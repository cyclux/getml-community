// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef ENGINE_PREPROCESSORS_PREPROCESSOR_HPP_
#define ENGINE_PREPROCESSORS_PREPROCESSOR_HPP_

#include <memory>
#include <optional>
#include <rfl/Ref.hpp>
#include <utility>
#include <vector>

#include "commands/Fingerprint.hpp"
#include "engine/preprocessors/Params.hpp"
#include "helpers/Saver.hpp"
#include "helpers/StringIterator.hpp"

namespace engine {
namespace preprocessors {

class Preprocessor {
 public:
  static constexpr const char* CATEGORY_TRIMMER = "CategoryTrimmer";
  static constexpr const char* EMAILDOMAIN = "EmailDomain";
  static constexpr const char* IMPUTATION = "Imputation";
  static constexpr const char* MAPPING = "Mapping";
  static constexpr const char* SEASONAL = "Seasonal";
  static constexpr const char* SUBSTRING = "Substring";
  static constexpr const char* TEXT_FIELD_SPLITTER = "TextFieldSplitter";

 public:
  Preprocessor() {};

  virtual ~Preprocessor() = default;

 public:
  /// Returns a deep copy.
  virtual rfl::Ref<Preprocessor> clone(
      const std::optional<std::vector<commands::Fingerprint>>& _dependencies =
          std::nullopt) const = 0;

  /// Returns the fingerprint of the feature learner (necessary to build
  /// the dependency graphs).
  virtual commands::Fingerprint fingerprint() const = 0;

  /// Fits the preprocessor. Returns the transformed data frames.
  virtual std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
  fit_transform(const Params& _params) = 0;

  /// Loads the preprocessor.
  virtual void load(const std::string& _fname) = 0;

  /// Stores the preprocessor.
  virtual void save(const std::string& _fname,
                    const typename helpers::Saver::Format& _format) const = 0;

  /// Generates the new column.
  virtual std::pair<containers::DataFrame, std::vector<containers::DataFrame>>
  transform(const Params& _params) const = 0;

  /// Expresses the preprocessor as SQL, if applicable.
  virtual std::vector<std::string> to_sql(
      const helpers::StringIterator& _categories,
      const std::shared_ptr<const transpilation::SQLDialectGenerator>&
          _sql_dialect_generator) const = 0;

  /// Returns the type of the preprocessor.
  virtual std::string type() const = 0;
};

}  // namespace preprocessors
}  // namespace engine

#endif  // ENGINE_PREPROCESSORS_PREPROCESSOR_HPP_
