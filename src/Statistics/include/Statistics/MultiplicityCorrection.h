// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STATISTICS_MULTIPLICITY_CORRECTION_H_
#define STATISTICS_MULTIPLICITY_CORRECTION_H_

#include <absl/container/flat_hash_map.h>

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

namespace orbit_statistics {

// The simplest correction known in the literature. Shouldn't be used but for testing or for the
// lack of a better alternative.
template <typename K>
[[nodiscard]] absl::flat_hash_map<K, double> BonferroniCorrection(
    const absl::flat_hash_map<K, double>& pvalues) {
  absl::flat_hash_map<K, double> corrected;
  std::transform(
      std::begin(pvalues), std::end(pvalues), std::inserter(corrected, std::begin(corrected)),
      [&pvalues](const auto& key_to_pvalue) {
        return std::make_pair(key_to_pvalue.first, key_to_pvalue.second * pvalues.size());
      });
  return corrected;
}
}  // namespace orbit_statistics

#endif  // STATISTICS_MULTIPLICITY_CORRECTION_H_
