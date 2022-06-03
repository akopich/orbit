// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "MizarData/BaselineAndComparison.h"

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <algorithm>
#include <iterator>
#include <string>

#include "BaselineAndComparisonHelper.h"
#include "ClientData/CaptureData.h"

namespace orbit_mizar_data {

[[nodiscard]] std::vector<uint64_t> MizarDataWithSampledFunctionId::FramesWithIds(
    const orbit_client_data::CallstackInfo* callstack) const {
  if (callstack->frames().empty()) return {};
  if (callstack->type() != orbit_client_data::CallstackType::kComplete) {
    return CallstackWithIds({callstack->frames()[0]});
  }
  return CallstackWithIds(callstack->frames());
}

[[nodiscard]] std::vector<uint64_t> MizarDataWithSampledFunctionId::CallstackWithIds(
    const std::vector<uint64_t>& frames) const {
  std::vector<uint64_t> result;
  for (const uint64_t address : frames) {
    if (auto it = address_to_sampled_function_id.find(address);
        it != address_to_sampled_function_id.end()) {
      result.push_back(it->second);
    }
  }
  return result;
}

orbit_mizar_data::BaselineAndComparison CreateBaselineAndComparison(const MizarData& baseline,
                                                                    const MizarData& comparison) {
  const absl::flat_hash_map<uint64_t, std::string> baseline_address_to_name =
      baseline.AllAddressToName();
  const absl::flat_hash_map<uint64_t, std::string> comparison_address_to_name =
      comparison.AllAddressToName();

  auto [baseline_address_to_frame_id, comparison_address_to_frame_id, frame_id_to_name] =
      AssignSampledFunctionIds(baseline_address_to_name, comparison_address_to_name);

  MizarDataWithSampledFunctionId base{baseline, std::move(baseline_address_to_frame_id)};
  return BaselineAndComparison({baseline, std::move(baseline_address_to_frame_id)},
                               {comparison, std::move(comparison_address_to_frame_id)},
                               std::move(frame_id_to_name));
}

}  // namespace orbit_mizar_data