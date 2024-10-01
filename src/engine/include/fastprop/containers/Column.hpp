// Copyright 2024 Code17 GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef FASTPROP_CONTAINERS_COLUMN_HPP_
#define FASTPROP_CONTAINERS_COLUMN_HPP_

#include "helpers/Column.hpp"

namespace fastprop {
namespace containers {

template <typename T>
using Column = helpers::Column<T>;

}  // namespace containers
}  // namespace fastprop

#endif  // FASTPROP_CONTAINERS_COLUMN_HPP_
