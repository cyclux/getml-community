// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef FCT_FCT_HPP_
#define FCT_FCT_HPP_

#include "fct/AccessIterator.hpp"
#include "fct/IotaIterator.hpp"
#include "fct/IotaRange.hpp"
#include "fct/Literal.hpp"
#include "fct/NamedTuple.hpp"
#include "fct/Range.hpp"
#include "fct/Ref.hpp"
#include "fct/TaggedUnion.hpp"
#include "fct/collect.hpp"
#include "fct/compose.hpp"
#include "fct/iota.hpp"
#include "fct/join.hpp"
#include "fct/visit.hpp"

#ifndef __APPLE__
#include "fct/collect_parallel.hpp"
#endif

#endif  // FCT_FCT_HPP_
