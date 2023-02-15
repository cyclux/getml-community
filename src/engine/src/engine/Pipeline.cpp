// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "engine/pipelines/Pipeline.hpp"

#include "engine/pipelines/DataFrameModifier.hpp"
#include "engine/pipelines/PlaceholderMaker.hpp"
#include "engine/pipelines/Staging.hpp"
#include "fct/Field.hpp"
#include "fct/collect.hpp"
#include "transpilation/SQLDialectParser.hpp"

namespace engine {
namespace pipelines {

Pipeline::Pipeline(const fct::Ref<const commands::Pipeline>& _obj)
    : allow_http_(false),
      creation_time_(make_creation_time()),
      include_categorical_(_obj->get<"include_categorical_">()),
      obj_(_obj),
      scores_(fct::Ref<const metrics::Scores>::make()) {}

// ----------------------------------------------------------------------------

Pipeline::~Pipeline() = default;

// ------------------------------------------------------------------------

std::pair<fct::Ref<const helpers::Placeholder>,
          fct::Ref<const std::vector<std::string>>>
Pipeline::make_placeholder() const {
  const auto data_model =
      fct::Ref<const helpers::Placeholder>::make(obj().get<"data_model_">());

  // TODO
  /*const auto placeholder = fct::Ref<const helpers::Placeholder>::make(
      PlaceholderMaker::make_placeholder(data_model, "t1"));

  const auto peripheral_names = fct::Ref<const std::vector<std::string>>::make(
      PlaceholderMaker::make_peripheral(*placeholder));*/

  return std::make_pair(data_model,
                        fct::Ref<const std::vector<std::string>>::make());
}

// ----------------------------------------------------------------------

fct::Ref<std::vector<std::string>> Pipeline::parse_peripheral() const {
  const auto get_name = [](const auto& _p) -> std::string { return _p.name(); };
  return fct::Ref<std::vector<std::string>>::make(
      fct::collect::vector<std::string>(obj().get<"peripheral_">() |
                                        VIEWS::transform(get_name)));
}

// ----------------------------------------------------------------------

std::shared_ptr<std::string> Pipeline::parse_population() const {
  return std::make_shared<std::string>(obj().get<"data_model_">().name());
}

// ----------------------------------------------------------------------------

MonitorSummary Pipeline::to_monitor(const helpers::StringIterator& _categories,
                                    const std::string& _name) const {
  const auto summary_not_fitted =
      obj() * fct::make_field<"name_">(_name) *
      fct::make_field<"allow_http_">(allow_http()) *
      fct::make_field<"creation_time_">(creation_time());

  if (!fitted()) {
    return summary_not_fitted;
  }

  return summary_not_fitted *
         fct::make_field<"num_features_">(fitted()->num_features()) *
         fct::make_field<"peripheral_schema_">(fitted()->peripheral_schema_) *
         fct::make_field<"population_schema_">(fitted()->population_schema_) *
         fct::make_field<"targets_">(fitted()->targets());

  // TODO
  /*
    auto scores_obj = scores().to_json_obj();

    const auto make_staging_table_colname =
        [](const std::string& _colname) -> std::string {
      return transpilation::HumanReadableSQLGenerator()
          .make_staging_table_colname(_colname);
    };

    const auto modified_names = helpers::Macros::modify_colnames(
        scores().feature_names(), make_staging_table_colname);

    const auto feature_names = JSON::vector_to_array(modified_names);

    scores_obj.set("feature_names_", feature_names);

    json_obj.set("scores_", scores_obj);*/
}

// ----------------------------------------------------------------------------
}  // namespace pipelines
}  // namespace engine
