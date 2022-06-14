// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "Statistics/StatisticsUtils.h"

namespace orbit_statistics {

using testing::DoubleNear;

static void ExpectMeanAndVarianceEq(const MeanAndVariance& actual,
                                    const MeanAndVariance& expected) {
  constexpr double tolerance = 1e-3;
  EXPECT_THAT(actual.mean, DoubleNear(expected.mean, tolerance));
  EXPECT_THAT(actual.variance, DoubleNear(expected.variance, tolerance));
}

TEST(StatisticsUtilTest, ProductOfTwoIndependentIsCorrect) {
  MeanAndVariance product = ProductOfTwoIndependent({1, 2}, {3, 4});
  MeanAndVariance expected = {3.0, 30.0};
  ExpectMeanAndVarianceEq(product, expected);
}

}  // namespace orbit_statistics