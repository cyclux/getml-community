// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef RFL_INTERNAL_HASNAMEDTUPLEMETHODV_HPP_
#define RFL_INTERNAL_HASNAMEDTUPLEMETHODV_HPP_

#include <type_traits>

namespace rfl {
namespace internal {

template <typename Wrapper>
using named_tuple_method_t =
    decltype(std::declval<const Wrapper>().named_tuple());

template <typename Wrapper, typename = std::void_t<>>
struct has_nt_m : std::false_type {};

template <typename Wrapper>
struct has_nt_m<Wrapper, std::void_t<named_tuple_method_t<Wrapper>>>
    : std::true_type {};

/// Utility parameter for named tuple parsing, can be used by the
/// parsers to determine whether a class or struct has a method
/// called "named_tuple".
template <typename Wrapper>
constexpr bool has_named_tuple_method_v = has_nt_m<Wrapper>::value;

}  // namespace internal
}  // namespace rfl

#endif
