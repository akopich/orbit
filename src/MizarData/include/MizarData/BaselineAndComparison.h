// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MIZAR_DATA_BASELINE_AND_COMPARISON_H_
#define MIZAR_DATA_BASELINE_AND_COMPARISON_H_

#include <stdint.h>

#include <string>

#include "ClientData/CallstackData.h"
#include "ClientData/CaptureData.h"
#include "MizarData/MizarData.h"
#include "MizarData/MizarPairedData.h"
#include "MizarData/SampledFunctionId.h"
#include "MizarData/SamplingWithFrameTrackComparisonReport.h"

namespace orbit_mizar_data {

// The class owns the data from two capture files via owning two instances of
// `MizarDataWithSampledFunctionId`. Also owns the map from sampled function ids to the
// corresponding function names.
class BaselineAndComparison {
 public:
  BaselineAndComparison(MizarPairedData<MizarDataProvider> baseline,
                        MizarPairedData<MizarDataProvider> comparison,
                        absl::flat_hash_map<SFID, std::string> sfid_to_name)
      : baseline_(std::move(baseline)),
        comparison_(std::move(comparison)),
        sfid_to_name_(std::move(sfid_to_name)) {}

  [[nodiscard]] const absl::flat_hash_map<SFID, std::string>& sfid_to_name() const {
    return sfid_to_name_;
  }

  [[nodiscard]] SamplingWithFrameTrackComparisonReport MakeSamplingWithFrameTrackReport(
      BaselineSamplingWithFrameTrackReportConfig baseline_config,
      ComparisonSamplingWithFrameTrackReportConfig comparison_config);

 private:
  MizarPairedData<MizarDataProvider> baseline_;
  MizarPairedData<MizarDataProvider> comparison_;
  absl::flat_hash_map<SFID, std::string> sfid_to_name_;
};

orbit_mizar_data::BaselineAndComparison CreateBaselineAndComparison(
    std::unique_ptr<MizarDataProvider> baseline, std::unique_ptr<MizarDataProvider> comparison);

}  // namespace orbit_mizar_data

#endif  // MIZAR_DATA_BASELINE_AND_COMPARISON_H_