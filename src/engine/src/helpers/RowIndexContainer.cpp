// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#include "helpers/RowIndexContainer.hpp"

namespace helpers {
// ----------------------------------------------------------------------------

RowIndexContainer::RowIndexContainer(
    const WordIndexContainer& _word_index_container) {
  for (const auto& wic : _word_index_container.peripheral()) {
    peripheral_.push_back(make_row_indices(wic));
  }

  population_ = make_row_indices(_word_index_container.population());
}

// ----------------------------------------------------------------------------

RowIndexContainer::RowIndexContainer(const RowIndices& _population,
                                     const std::vector<RowIndices>& _peripheral)
    : peripheral_(_peripheral), population_(_population) {}

// ----------------------------------------------------------------------------

RowIndexContainer::~RowIndexContainer() = default;

// ----------------------------------------------------------------------------

typename RowIndexContainer::RowIndices RowIndexContainer::make_row_indices(
    const WordIndices& _word_indices) const {
  const auto make_row_index =
      [](const std::shared_ptr<const textmining::WordIndex>& word_index) {
        assert_true(word_index);
        return std::make_shared<const textmining::RowIndex>(*word_index);
      };

  return _word_indices | std::views::transform(make_row_index) |
         fct::ranges::to<std::vector>();
}

// ----------------------------------------------------------------------------
}  // namespace helpers
