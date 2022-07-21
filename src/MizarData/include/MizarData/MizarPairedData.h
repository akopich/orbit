// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MIZAR_DATA_MIZAR_PAIRED_DATA_H_
#define MIZAR_DATA_MIZAR_PAIRED_DATA_H_

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <numeric>
#include <utility>
#include <variant>
#include <vector>

#include "ClientData/CallstackData.h"
#include "ClientData/CallstackEvent.h"
#include "ClientData/ScopeId.h"
#include "ClientData/ScopeInfo.h"
#include "ClientProtos/capture_data.pb.h"
#include "GrpcProtos/capture.pb.h"
#include "MizarBase/SampledFunctionId.h"
#include "MizarBase/ThreadId.h"
#include "MizarData/FrameTrack.h"
#include "MizarData/MizarDataProvider.h"
#include "MizarData/NonWrappingAddition.h"
#include "OrbitBase/ThreadConstants.h"

namespace orbit_mizar_data {

// This class represents the data loaded from a capture that has been made aware of its counterpart
// it will be compared against. In particular, it is aware of the functions that has been sampled in
// the other capture. Also, it is aware of the sampled function ids assigned to the functions.
template <typename Data>
class MizarPairedDataTmpl {
  using SFID = ::orbit_mizar_base::SFID;
  using TID = ::orbit_mizar_base::TID;
  using ScopeId = orbit_client_data::ScopeId;

 public:
  MizarPairedDataTmpl(std::unique_ptr<Data> data,
                      absl::flat_hash_map<uint64_t, SFID> address_to_sfid)
      : data_(std::move(data)), address_to_sfid_(std::move(address_to_sfid)) {
    SetThreadNamesAndCallstackCounts();
  }

  // The function estimates how much of CPU-time has been actually spent by the threads in `tids`
  // during each of the frames. scope-id is used as a frame-track. This time does not include the
  // time the process was waiting, de-scheduled, or the VM itself was de-scheduled. The estimate is
  // obtained via counting how many callstack samples have been obtained during each frame and then
  // multiplying the counter by the sampling period.
  [[nodiscard]] std::vector<uint64_t> ActiveInvocationTimes(
      const absl::flat_hash_set<TID>& tids, FrameTrackId frame_track_id,
      uint64_t min_relative_timestamp_ns, uint64_t max_relative_timestamp_ns) const {
    const auto [min_timestamp_ns, max_timestamp_ns] =
        RelativeToAbsoluteTimestampRange(min_relative_timestamp_ns, max_relative_timestamp_ns);
    const std::vector<FrameStartNs> frame_starts =
        GetFrames(frame_track_id, min_timestamp_ns, max_timestamp_ns);
    if (frame_starts.size() < 2) return {};

    // TODO(b/235572160) Estimate the actual sampling period and use it instead
    const uint64_t sampling_period = data_->GetNominalSamplingPeriodNs();

    std::vector<uint64_t> result;
    for (size_t i = 0; i + 1 < frame_starts.size(); ++i) {
      const uint64_t callstack_count = std::transform_reduce(
          std::begin(tids), std::end(tids), uint64_t{0}, std::plus<>(),
          [this, i, &frame_starts](const TID tid) {
            return CallstackSamplesCount(tid, *frame_starts[i], *frame_starts[i + 1]);
          });

      result.push_back(sampling_period * callstack_count);
    }
    return result;
  }

  [[nodiscard]] const absl::flat_hash_map<TID, std::string>& TidToNames() const {
    return tid_to_names_;
  }

  [[nodiscard]] const absl::flat_hash_map<TID, uint64_t>& TidToCallstackSampleCounts() const {
    return tid_to_callstack_samples_counts_;
  }

  [[nodiscard]] absl::flat_hash_map<FrameTrackId, FrameTrackInfo> GetFrameTracks() const {
    const std::vector<ScopeId> scope_ids = GetCaptureData().GetAllProvidedScopeIds();
    absl::flat_hash_map<FrameTrackId, FrameTrackInfo> result;
    for (const ScopeId scope_id : scope_ids) {
      const orbit_client_data::ScopeInfo scope_info = GetCaptureData().GetScopeInfo(scope_id);
      if (scope_info.GetType() == orbit_client_data::ScopeType::kDynamicallyInstrumentedFunction ||
          scope_info.GetType() == orbit_client_data::ScopeType::kApiScope) {
        result.try_emplace(MakeFrameTrackId(scope_id), scope_info);
      }
    }

    for (auto& [source, unused_events] : data_->source_to_present_events()) {
      result.try_emplace(MakeFrameTrackId(source), source);
    }
    return result;
  }

  // Action is a void callable that takes a single argument of type
  // `const std::vector<uint64_t>` representing a callstack sample, each element of the vector is a
  // sampled function id.
  // `min_relative_timestamp_ns` and `max_relative_timestamp_ns` are the nanoseconds elapsed since
  // capture start.
  template <typename Action>
  void ForEachCallstackEvent(TID tid, uint64_t min_relative_timestamp_ns,
                             uint64_t max_relative_timestamp_ns, Action&& action) const {
    auto action_on_callstack_events =
        [this, &action](const orbit_client_data::CallstackEvent& event) -> void {
      const orbit_client_data::CallstackInfo* callstack =
          GetCallstackData().GetCallstack(event.callstack_id());
      const std::vector<SFID> sfids = CallstackWithSFIDs(callstack);
      std::invoke(std::forward<Action>(action), sfids);
    };

    const auto [min_timestamp_ns, max_timestamp_ns] =
        RelativeToAbsoluteTimestampRange(min_relative_timestamp_ns, max_relative_timestamp_ns);
    ForEachCallstackEventOfTidInTimeRange(tid, min_timestamp_ns, max_timestamp_ns,
                                          action_on_callstack_events);
  }

  [[nodiscard]] uint64_t CaptureDurationNs() const {
    return GetCallstackData().max_time() - data_->GetCaptureStartTimestampNs();
  }

 private:
  void SetThreadNamesAndCallstackCounts() {
    const absl::flat_hash_map<uint32_t, std::string>& thread_names =
        GetCaptureData().thread_names();
    GetCallstackData().ForEachCallstackEventInTimeRange(
        0, std::numeric_limits<uint64_t>::max(),
        [this, &thread_names](const orbit_client_data::CallstackEvent& event) {
          const TID tid{event.thread_id()};
          tid_to_callstack_samples_counts_[tid]++;

          if (tid_to_names_.contains(tid)) return;

          const auto tid_to_name = thread_names.find(*tid);
          std::string thread_name = (tid_to_name != thread_names.end()) ? tid_to_name->second : "";
          tid_to_names_.try_emplace(tid, std::move(thread_name));
        });
  }

  template <typename Action>
  void ForEachCallstackEventOfTidInTimeRange(TID tid, uint64_t min_timestamp_ns,
                                             uint64_t max_timestamp_ns,
                                             Action&& action_on_callstack_events) const {
    GetCallstackData().ForEachCallstackEventOfTidInTimeRange(
        *tid, min_timestamp_ns, max_timestamp_ns, action_on_callstack_events);
  }

  [[nodiscard]] uint64_t CallstackSamplesCount(TID tid, uint64_t min_timestamp_ns,
                                               uint64_t max_timestamp_ns) const {
    uint64_t count = 0;
    ForEachCallstackEventOfTidInTimeRange(tid, min_timestamp_ns, max_timestamp_ns,
                                          [&count](const auto& /*callstack*/) { count++; });
    return count;
  }

  [[nodiscard]] std::pair<uint64_t, uint64_t> RelativeToAbsoluteTimestampRange(
      uint64_t min_relative_timestamp_ns, uint64_t max_relative_timestamp_ns) const {
    return std::make_pair(
        NonWrappingAddition(data_->GetCaptureStartTimestampNs(), min_relative_timestamp_ns),
        NonWrappingAddition(data_->GetCaptureStartTimestampNs(), max_relative_timestamp_ns));
  }

  [[nodiscard]] std::vector<SFID> CallstackWithSFIDs(
      const orbit_client_data::CallstackInfo* callstack) const {
    if (callstack->frames().empty()) return {};
    if (callstack->type() != orbit_client_data::CallstackType::kComplete) {
      return CallstackWithSFIDs({callstack->frames()[0]});
    }
    return CallstackWithSFIDs(callstack->frames());
  }

  [[nodiscard]] std::vector<SFID> CallstackWithSFIDs(const std::vector<uint64_t>& frames) const {
    std::vector<SFID> result;
    for (const uint64_t address : frames) {
      if (auto it = address_to_sfid_.find(address); it != address_to_sfid_.end()) {
        result.push_back(it->second);
      }
    }
    return result;
  }

  [[nodiscard]] const auto& GetCaptureData() const { return data_->GetCaptureData(); }

  [[nodiscard]] const orbit_client_data::CallstackData& GetCallstackData() const {
    return GetCaptureData().GetCallstackData();
  }

  [[nodiscard]] std::vector<FrameStartNs> GetFrames(FrameTrackId id, uint64_t min_timestamp_ns,
                                                    uint64_t max_timestamp_ns) const {
    return IsEtw(id) ? GetEtwFrames(EtwSource(id), min_timestamp_ns, max_timestamp_ns)
                     : GetDorMSFrames(GetScopeId(id), min_timestamp_ns, max_timestamp_ns);
  }

  [[nodiscard]] std::vector<FrameStartNs> GetDorMSFrames(ScopeId scope_id,
                                                         uint64_t min_timestamp_ns,
                                                         uint64_t max_timestamp_ns) const {
    const std::vector<const TimerInfo*> timers =
        GetCaptureData().GetTimersForScope(scope_id, min_timestamp_ns, max_timestamp_ns);
    std::vector<FrameStartNs> result;

    std::transform(std::begin(timers), std::end(timers), std::back_inserter(result),
                   [](const TimerInfo* timer) { return FrameStartNs(timer->start()); });
    return result;
  }

  [[nodiscard]] std::vector<FrameStartNs> GetEtwFrames(
      orbit_grpc_protos::PresentEvent::Source etw_source, uint64_t min_timestamp_ns,
      uint64_t max_timestamp_ns) const {
    const auto& source_to_present_events = data_->source_to_present_events();

    if (auto it = source_to_present_events.find(etw_source); it != source_to_present_events.end()) {
      std::vector<FrameStartNs> result;
      const std::vector<orbit_grpc_protos::PresentEvent>& events = it->second;
      for (const orbit_grpc_protos::PresentEvent& event : events) {
        if (min_timestamp_ns < event.begin_timestamp_ns() &&
            event.begin_timestamp_ns() < max_timestamp_ns) {
          result.emplace_back(event.begin_timestamp_ns());
        }
      }
      return result;
    }
    return {};
  }

  static constexpr uint64_t kNumberOfEtwSources = 20;  // an upper bound

  [[nodiscard]] static bool IsEtw(FrameTrackId id) { return *id < kNumberOfEtwSources; }

  [[nodiscard]] static orbit_grpc_protos::PresentEvent::Source EtwSource(FrameTrackId id) {
    return static_cast<orbit_grpc_protos::PresentEvent::Source>(*id);
  }

  [[nodiscard]] static FrameTrackId MakeFrameTrackId(
      orbit_grpc_protos::PresentEvent::Source source) {
    return FrameTrackId(static_cast<int>(source));
  }

  [[nodiscard]] static FrameTrackId MakeFrameTrackId(ScopeId scope_id) {
    return FrameTrackId(kNumberOfEtwSources + *scope_id);
  }

  [[nodiscard]] static ScopeId GetScopeId(FrameTrackId id) {
    const uint64_t value = -kNumberOfEtwSources + *id;
    ORBIT_CHECK(value > 0);
    return ScopeId(value);
  }

  std::unique_ptr<Data> data_;
  absl::flat_hash_map<uint64_t, SFID> address_to_sfid_;
  absl::flat_hash_map<TID, std::string> tid_to_names_;
  absl::flat_hash_map<TID, uint64_t> tid_to_callstack_samples_counts_;
};

using MizarPairedData = MizarPairedDataTmpl<MizarDataProvider>;

}  // namespace orbit_mizar_data

#endif  // MIZAR_DATA_MIZAR_PAIRED_DATA_H_
