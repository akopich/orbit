// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <absl/container/flat_hash_map.h>
#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <vector>

#include "Statistics/MultiplicityCorrection.h"

namespace orbit_statistics {

using testing::DoubleNear;
using testing::SizeIs;

template <typename Container, typename AnotherContainer>
auto MakeMap(const Container& keys, const AnotherContainer& values) {
  using K = typename Container::value_type;
  using V = typename AnotherContainer::value_type;
  using std::begin;
  using std::end;

  absl::flat_hash_map<K, V> result;
  std::transform(begin(keys), end(keys), begin(values), std::inserter(result, std::begin(result)),
                 [](const K& k, const V& v) { return std::make_pair(k, v); });
  return result;
}

constexpr size_t kTestsNum = 4;
constexpr std::array<int, kTestsNum> kKeys = {1, 2, 3, 4};
constexpr std::array<double, kTestsNum> kPvalues = {0.1, 0.2, 0.3, 0.02};
const absl::flat_hash_map<int, double> kKeyToPvalue = MakeMap(kKeys, kPvalues);

TEST(MultiplicityCorrection, BonferroniCorrectionIsCorrect) {
  auto key_to_corrected = BonferroniCorrection(kKeyToPvalue);
  EXPECT_THAT(key_to_corrected, SizeIs(kTestsNum));
  for (const int key : kKeys) {
    constexpr double kTol = 1e-3;
    EXPECT_THAT(key_to_corrected.at(key) / kTestsNum, DoubleNear(kKeyToPvalue.at(key), kTol));
  }
}

}  // namespace orbit_statistics