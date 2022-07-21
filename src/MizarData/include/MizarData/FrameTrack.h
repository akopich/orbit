// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MIZAR_DATA_MIZAR_FRAME_TRACK_H_
#define MIZAR_DATA_MIZAR_FRAME_TRACK_H_

#include "ClientData/ScopeInfo.h"
#include "GrpcProtos/capture.pb.h"
#include "OrbitBase/Typedef.h"
#include "stdint.h"

namespace orbit_mizar_data {

struct FrameTrackIdTag {};
using FrameTrackId = orbit_base::Typedef<FrameTrackIdTag, uint64_t>;

struct FrameStartNsTag {};
using FrameStartNs = orbit_base::Typedef<FrameStartNsTag, uint64_t>;

using FrameTrackInfo =
    std::variant<orbit_client_data::ScopeInfo, orbit_grpc_protos::PresentEvent::Source>;

}  // namespace orbit_mizar_data

#endif  // MIZAR_DATA_MIZAR_FRAME_TRACK_H_