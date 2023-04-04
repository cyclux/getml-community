// Copyright 2022 The SQLNet Company GmbH
//
// This file is licensed under the Elastic License 2.0 (ELv2).
// Refer to the LICENSE.txt file in the root of the repository
// for details.
//

#ifndef FCT_VISITTREE_HPP_
#define FCT_VISITTREE_HPP_

namespace fct {

struct VisitTree {
  /// Evaluates a visitor pattern using a tree-like structure.
  template <int _begin, int _end, class Visitor, class... Args>
  static inline auto visit(const auto& _v, const int _i, const Args&... _args) {
    static_assert(_end > _begin, "_end needs to be greater than _begin.");
    if constexpr (_end - _begin == 1) {
      return _v.template visit<_begin>(_args...);
    } else {
      constexpr int middle = (_begin + _end) / 2;
      if (_i < middle) {
        return visit<_begin, middle, Visitor>(_v, _i, _args...);
      } else {
        return visit<middle, _end, Visitor>(_v, _i, _args...);
      }
    }
  }
};

}  // namespace fct

#endif  // FCT_VISITTREE_HPP_
