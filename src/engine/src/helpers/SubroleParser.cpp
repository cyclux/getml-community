// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "helpers/SubroleParser.hpp"

// ----------------------------------------------------------------------------

#include <algorithm>
#include <ranges>
#include <stdexcept>

#include "fct/to.hpp"

// ----------------------------------------------------------------------------
namespace helpers {
// ----------------------------------------------------------------------------

bool SubroleParser::contains_any(const std::vector<std::string>& _column,
                                 const std::vector<Subrole>& _targets) {
  const auto column = parse(_column);

  return contains_any(column, _targets);
}

// ----------------------------------------------------------------------------

bool SubroleParser::contains_any(const std::vector<Subrole>& _column,
                                 const std::vector<Subrole>& _targets) {
  const auto in_column = [&_column](const Subrole _s) -> bool {
    return std::ranges::find(_column, _s) != _column.end();
  };

  return std::ranges::any_of(_targets, in_column);
}

// ----------------------------------------------------------------------------

Subrole SubroleParser::parse(const std::string& _str) {
  if (_str == COMPARISON_ONLY) {
    return Subrole::comparison_only;
  }

  if (_str == EMAIL) {
    return Subrole::email;
  }

  if (_str == EMAIL_ONLY) {
    return Subrole::email_only;
  }

  if (_str == EXCLUDE_CATEGORY_TRIMMER) {
    return Subrole::exclude_category_trimmer;
  }

  if (_str == EXCLUDE_FASTPROP) {
    return Subrole::exclude_fastprop;
  }

  if (_str == EXCLUDE_FEATURE_LEARNERS) {
    return Subrole::exclude_feature_learners;
  }

  if (_str == EXCLUDE_IMPUTATION) {
    return Subrole::exclude_imputation;
  }

  if (_str == EXCLUDE_MAPPING) {
    return Subrole::exclude_mapping;
  }

  if (_str == EXCLUDE_MULTIREL) {
    return Subrole::exclude_multirel;
  }

  if (_str == EXCLUDE_PREDICTORS) {
    return Subrole::exclude_predictors;
  }

  if (_str == EXCLUDE_PREPROCESSORS) {
    return Subrole::exclude_preprocessors;
  }

  if (_str == EXCLUDE_RELBOOST) {
    return Subrole::exclude_relboost;
  }

  if (_str == EXCLUDE_RELMT) {
    return Subrole::exclude_relmt;
  }

  if (_str == EXCLUDE_SEASONAL) {
    return Subrole::exclude_seasonal;
  }

  if (_str == EXCLUDE_TEXT_FIELD_SPLITTER) {
    return Subrole::exclude_text_field_splitter;
  }

  if (_str == SUBSTRING) {
    return Subrole::substring;
  }

  if (_str == SUBSTRING_ONLY) {
    return Subrole::substring_only;
  }

  throw std::runtime_error("Unknown subrole: '" + _str + "'.");

  return Subrole::comparison_only;
}

// ----------------------------------------------------------------------------

std::vector<Subrole> SubroleParser::parse(
    const std::vector<std::string>& _vec) {
  const auto to_subrole = [](const std::string& _str) -> Subrole {
    return SubroleParser::parse(_str);
  };

  return _vec | std::views::transform(to_subrole) |
         fct::ranges::to<std::vector>();
}

// ----------------------------------------------------------------------------
}  // namespace helpers
