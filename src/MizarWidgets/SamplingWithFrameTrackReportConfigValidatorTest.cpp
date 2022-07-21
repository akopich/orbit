
// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QString>

#include "MizarBase/BaselineOrComparison.h"
#include "MizarBase/Titles.h"
#include "MizarData/FrameTrack.h"
#include "MizarWidgets/SamplingWithFrameTrackReportConfigValidator.h"
#include "TestUtils/TestUtils.h"

using ::orbit_mizar_base::Baseline;
using ::orbit_mizar_base::Comparison;
using ::orbit_mizar_base::TID;
using ::orbit_mizar_data::FrameTrackId;
using ::orbit_mizar_data::FrameTrackInfo;
using ::orbit_test_utils::HasError;
using ::orbit_test_utils::HasNoError;
using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {

class MockPairedData {
 public:
  MOCK_METHOD(uint64_t, CaptureDurationNs, (), (const));
};

class MockBaselineAndComparison {
 public:
  MOCK_METHOD(const Baseline<MockPairedData>&, GetBaselineData, (), (const));
  MOCK_METHOD(const Comparison<MockPairedData>&, GetComparisonData, (), (const));
};

}  // namespace

namespace orbit_mizar_widgets {

constexpr uint64_t kBaselineCaptureDuration = 123456;
constexpr uint64_t kComparisonCaptureDuration = 234567;

using HalfConfig = orbit_mizar_data::HalfOfSamplingWithFrameTrackReportConfig;

TEST(SamplingWithFrameTrackReportConfigValidator, IsCorrect) {
  Baseline<MockPairedData> baseline_data;
  Comparison<MockPairedData> comparison_data;
  MockBaselineAndComparison bac;

  EXPECT_CALL(*baseline_data, CaptureDurationNs).WillRepeatedly(Return(kBaselineCaptureDuration));
  EXPECT_CALL(*comparison_data, CaptureDurationNs)
      .WillRepeatedly(Return(kComparisonCaptureDuration));

  EXPECT_CALL(bac, GetBaselineData).WillRepeatedly(ReturnRef(baseline_data));
  EXPECT_CALL(bac, GetComparisonData).WillRepeatedly(ReturnRef(comparison_data));

  const SamplingWithFrameTrackReportConfigValidatorTmpl<MockBaselineAndComparison, MockPairedData>
      validator{};

  EXPECT_THAT(validator.Validate(&bac,
                                 orbit_mizar_base::MakeBaseline<HalfConfig>(
                                     absl::flat_hash_set<TID>{TID(1)}, /*start_ns=*/0,
                                     /*duration_ns=*/0, FrameTrackId(0)),
                                 orbit_mizar_base::MakeComparison<HalfConfig>(
                                     absl::flat_hash_set<TID>{},
                                     /*start_ns=*/0, /*duration_ns=*/0, FrameTrackId(0))),
              HasError("Comparison: No threads selected"));

  EXPECT_THAT(validator.Validate(&bac,
                                 orbit_mizar_base::MakeBaseline<HalfConfig>(
                                     absl::flat_hash_set<TID>{}, /*start_ns=*/0,
                                     /*duration_ns=*/0, FrameTrackId(0)),
                                 orbit_mizar_base::MakeComparison<HalfConfig>(
                                     absl::flat_hash_set<TID>{TID(1)},
                                     /*start_ns=*/0, /*duration_ns=*/0, FrameTrackId(0))),
              HasError("Baseline: No threads selected"));

  EXPECT_THAT(validator.Validate(&bac,
                                 orbit_mizar_base::MakeBaseline<HalfConfig>(
                                     absl::flat_hash_set<TID>{TID(1)},
                                     /*start_ns=*/kBaselineCaptureDuration + 1,
                                     /*duration_ns=*/0, FrameTrackId(0)),
                                 orbit_mizar_base::MakeComparison<HalfConfig>(
                                     absl::flat_hash_set<TID>{TID(1)},
                                     /*start_ns=*/0, /*duration_ns=*/0, FrameTrackId(0))),
              HasError("Baseline: Start > capture duration"));

  EXPECT_THAT(validator.Validate(&bac,
                                 orbit_mizar_base::MakeBaseline<HalfConfig>(
                                     absl::flat_hash_set<TID>{TID(1)}, /*start_ns=*/0,
                                     /*duration_ns=*/0, FrameTrackId(0)),
                                 orbit_mizar_base::MakeComparison<HalfConfig>(
                                     absl::flat_hash_set<TID>{TID(1)},
                                     /*start_ns=*/kComparisonCaptureDuration + 1,
                                     /*duration_ns=*/0, FrameTrackId(0))),
              HasError("Comparison: Start > capture duration"));

  EXPECT_THAT(validator.Validate(&bac,
                                 orbit_mizar_base::MakeBaseline<HalfConfig>(
                                     absl::flat_hash_set<TID>{TID(1)}, /*start_ns=*/0,
                                     /*duration_ns=*/kBaselineCaptureDuration - 1, FrameTrackId(0)),
                                 orbit_mizar_base::MakeComparison<HalfConfig>(
                                     absl::flat_hash_set<TID>{TID(1)},
                                     /*start_ns=*/kComparisonCaptureDuration - 1,
                                     /*duration_ns=*/0, FrameTrackId(0))),
              HasNoError());
}

}  // namespace orbit_mizar_widgets