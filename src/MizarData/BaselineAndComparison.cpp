// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "MizarData/BaselineAndComparison.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <string>

#include "BaselineAndComparisonHelper.h"
#include "ClientData/CaptureData.h"
#include "MizarData/MizarData.h"

namespace orbit_mizar_data {

orbit_mizar_data::BaselineAndComparison CreateBaselineAndComparison(
    std::unique_ptr<MizarDataProvider> baseline, std::unique_ptr<MizarDataProvider> comparison) {
  auto [baseline_address_sfid, comparison_address_to_sfid, sfid_to_name] =
      AssignSampledFunctionIds(baseline->AllAddressToName(), comparison->AllAddressToName());

  return {{std::move(baseline), std::move(baseline_address_sfid)},
          {std::move(comparison), std::move(comparison_address_to_sfid)},
          std::move(sfid_to_name)};
}

[[nodiscard]] static SamplingCounts MakeCounts(
    const MizarPairedData<MizarDataProvider>& data,
    const HalfOfSamplingWithFrameTrackReportConfig& config) {
  SamplingCounts result;
  for (const uint32_t tid : config.tids) {
    data.ForEachCallstackEvent(tid, config.start_ns, config.end_ns,
                               [&result](const std::vector<SFID>& callstack) {
                                 result.total_callstacks++;
                                 if (callstack.empty()) return;
                                 for (const SFID sfid : callstack) {
                                   result.counts[sfid].inclusive++;
                                 }
                                 result.counts[callstack.back()].exclusive++;
                               });
  }
  return result;
}

[[nodiscard]] SamplingWithFrameTrackComparisonReport
BaselineAndComparison::MakeSamplingWithFrameTrackReport(
    BaselineSamplingWithFrameTrackReportConfig baseline_config,
    ComparisonSamplingWithFrameTrackReportConfig comparison_config) {
  return {MakeCounts(baseline_, baseline_config), MakeCounts(comparison_, comparison_config)};
}

}  // namespace orbit_mizar_data