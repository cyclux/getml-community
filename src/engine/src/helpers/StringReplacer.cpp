// Copyright 2024 Code17 GmbH
// 
// This file is licensed under the Elastic License 2.0 (ELv2). 
// Refer to the LICENSE.txt file in the root of the repository 
// for details.
// 

#include "helpers/StringReplacer.hpp"

namespace helpers {
// ----------------------------------------------------------------------------

std::string StringReplacer::replace_all(const std::string &_str,
                                        const std::string &_from,
                                        const std::string &_to) {
  if (_from.empty()) {
    return _str;
  }

  auto modified = _str;

  size_t pos = 0;

  while ((pos = modified.find(_from, pos)) != std::string::npos) {
    modified.replace(pos, _from.length(), _to);
    pos += _to.length();
  }

  return modified;
}

// ----------------------------------------------------------------------------
}  // namespace helpers
