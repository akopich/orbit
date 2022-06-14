// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MIZAR_STATISTICS_ACTIVE_FUNCTION_TIME_PER_FRAME_COMPARISON
#define MIZAR_STATISTICS_ACTIVE_FUNCTION_TIME_PER_FRAME_COMPARISON

#include "ClientData/ScopeStats.h"
#include "MizarData/BaselineOrComparison.h"
#include "MizarData/SamplingWithFrameTrackComparisonReport.h"
#include "Statistics/Gaussian.h"
#include "Statistics/StatisticsUtils.h"

namespace orbit_mizar_data {

class ActiveFunctionTimePerFrameComparator {
 public:
  explicit ActiveFunctionTimePerFrameComparator(
      const Baseline<SamplingCounts>& baseline_counts,
      const Baseline<orbit_client_data::ScopeStats>& baseline_frame_stats,
      const Comparison<SamplingCounts>& comparison_counts,
      const Comparison<orbit_client_data::ScopeStats>& comparison_frame_stats)
      : baseline_counts_(baseline_counts),
        baseline_frame_stats_(baseline_frame_stats),
        comparison_counts_(comparison_counts),
        comparison_frame_stats_(comparison_frame_stats) {}

  [[nodiscard]] ComparisonResult Compare(SFID sfid) const {
    const uint64_t n1 = baseline_counts_->GetTotalCallstacks();
    const uint64_t n2 = comparison_counts_->GetTotalCallstacks();
    const double p1 = baseline_counts_->GetExclusiveRate(sfid);
    const double p2 = comparison_counts_->GetExclusiveRate(sfid);

    const double f1 = baseline_frame_stats_->ComputeAverageTimeNs();
    const double f2 = comparison_frame_stats_->ComputeAverageTimeNs();

    const double var_p1 = p1 * (1 - p1) / n1;
    const double var_p2 = p2 * (1 - p2) / n2;
    const double var_f1 = baseline_frame_stats_->variance_ns();
    const double var_f2 = comparison_frame_stats_->variance_ns();

    const orbit_statistics::MeanAndVariance f1p1 =
        orbit_statistics::ProductOfTwoIndependent({p1, var_p1}, {f1, var_f1});
    const orbit_statistics::MeanAndVariance f2p2 =
        orbit_statistics::ProductOfTwoIndependent({p2, var_p2}, {f2, var_f2});

    const orbit_statistics::MeanAndVariance non_normalized_stat =
        orbit_statistics::ProductOfTwoIndependent(f1p1, f2p2);
    const double stat = non_normalized_stat.mean / std::sqrt(non_normalized_stat.variance);

    const double pvalue = orbit_statistics::GaussianCDF(stat);
    if (std::isnan(pvalue)) return {stat, 1.0};

    return {stat, std::min(pvalue, 1 - pvalue) * 2};
  }

 private:
  [[maybe_unused]] const Baseline<SamplingCounts>& baseline_counts_;
  [[maybe_unused]] const Baseline<orbit_client_data::ScopeStats>& baseline_frame_stats_;

  [[maybe_unused]] const Comparison<SamplingCounts>& comparison_counts_;
  [[maybe_unused]] const Comparison<orbit_client_data::ScopeStats>& comparison_frame_stats_;
};

}  // namespace orbit_mizar_data

#endif  // MIZAR_STATISTICS_ACTIVE_FUNCTION_TIME_PER_FRAME_COMPARISON
