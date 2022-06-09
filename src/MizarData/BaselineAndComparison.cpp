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

}  // namespace orbit_mizar_data