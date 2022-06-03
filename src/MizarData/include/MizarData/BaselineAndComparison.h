// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MIZAR_DATA_BASELINE_AND_COMPARISON_H_
#define MIZAR_DATA_BASELINE_AND_COMPARISON_H_

#include <absl/container/flat_hash_set.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>

#include "ClientData/CallstackData.h"
#include "ClientData/CaptureData.h"
#include "MizarData/MizarData.h"

namespace orbit_mizar_data {

template <typename MizarData>
class MizarDataWithSampledFunctionId {
 public:
  MizarDataWithSampledFunctionId(
      const MizarData& data, absl::flat_hash_map<uint64_t, uint64_t> address_to_sampled_function_id)
      : data(data), address_to_sampled_function_id(std::move(address_to_sampled_function_id)) {}

  template <typename Action>
  void ForEachCallstackEvent(uint32_t tid, uint64_t min_timestamp, uint64_t max_timestamp,
                             Action&& action) const {
    const orbit_client_data::CallstackData& callstack_data =
        data.GetCaptureData().GetCallstackData();
    auto action_of_callstack_events =
        [this, &callstack_data, &action](const orbit_client_data::CallstackEvent& event) -> void {
      const orbit_client_data::CallstackInfo* callstack =
          callstack_data.GetCallstack(event.callstack_id());
      const std::vector<uint64_t> sampled_function_ids = FramesWithIds(callstack);
      std::forward<Action>(action)(sampled_function_ids);
    };
    if (tid == orbit_base::kAllProcessThreadsTid) {
      callstack_data.ForEachCallstackEventInTimeRange(min_timestamp, max_timestamp,
                                                      action_of_callstack_events);
    } else {
      callstack_data.ForEachCallstackEventOfTidInTimeRange(tid, min_timestamp, max_timestamp,
                                                           action_of_callstack_events);
    }
  }

 private:
  [[nodiscard]] std::vector<uint64_t> FramesWithIds(
      const orbit_client_data::CallstackInfo* callstack) const {
    if (callstack->frames().empty()) return {};
    if (callstack->type() != orbit_client_data::CallstackType::kComplete) {
      return CallstackWithIds({callstack->frames()[0]});
    }
    return CallstackWithIds(callstack->frames());
  }

  [[nodiscard]] std::vector<uint64_t> CallstackWithIds(const std::vector<uint64_t>& frames) const {
    std::vector<uint64_t> result;
    for (const uint64_t address : frames) {
      if (auto it = address_to_sampled_function_id.find(address);
          it != address_to_sampled_function_id.end()) {
        result.push_back(it->second);
      }
    }
    return result;
  }

  const MizarData& data;
  absl::flat_hash_map<uint64_t, uint64_t> address_to_sampled_function_id;
};

class BaselineAndComparison {
 public:
  BaselineAndComparison(MizarDataWithSampledFunctionId<MizarData> baseline,
                        MizarDataWithSampledFunctionId<MizarData> comparison,
                        absl::flat_hash_map<uint64_t, std::string> sampled_function_id_to_name)
      : baseline_(std::move(baseline)),
        comparison_(std::move(comparison)),
        sampled_function_id_to_name_(std::move(sampled_function_id_to_name)) {}

  [[nodiscard]] const absl::flat_hash_map<uint64_t, std::string>& sampled_function_id_to_name()
      const {
    return sampled_function_id_to_name_;
  }

 private:
  MizarDataWithSampledFunctionId<MizarData> baseline_;
  MizarDataWithSampledFunctionId<MizarData> comparison_;
  absl::flat_hash_map<uint64_t, std::string> sampled_function_id_to_name_;
};

orbit_mizar_data::BaselineAndComparison CreateBaselineAndComparison(const MizarData& baseline,
                                                                    const MizarData& comparison);

}  // namespace orbit_mizar_data

#endif  // MIZAR_DATA_BASELINE_AND_COMPARISON_H_