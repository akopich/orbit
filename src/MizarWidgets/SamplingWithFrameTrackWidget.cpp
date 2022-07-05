// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "MizarWidgets/SamplingWithFrameTrackWidget.h"

#include <QObject>
#include <QWidget>
#include <Qt>
#include <memory>

#include "MizarBase/BaselineOrComparison.h"
#include "MizarData/BaselineAndComparison.h"
#include "MizarData/SamplingWithFrameTrackComparisonReport.h"
#include "OrbitBase/Typedef.h"
#include "ui_SamplingWithFrameTrackWidget.h"

namespace orbit_mizar_widgets {

using orbit_base::LiftAndApply;
using ::orbit_mizar_base::Baseline;
using ::orbit_mizar_base::Comparison;

SamplingWithFrameTrackWidgetBase::SamplingWithFrameTrackWidgetBase(QWidget* parent)
    : QWidget(parent) {}

SamplingWithFrameTrackWidgetBase::~SamplingWithFrameTrackWidgetBase() = default;

void SamplingWithFrameTrackWidgetBase::OnSignificanceLevelSelected(int index) {
  constexpr int kIndexOfFivePercent = 0;
  significance_level_ =
      (index == kIndexOfFivePercent) ? kDefaultSignificanceLevel : kAlternativeSignificanceLevel;
}

// template class SamplingWithFrameTrackWidgetTmpl<Ui::SamplingWithFrameTrackWidget,
//                                                 orbit_mizar_data::BaselineAndComparison,
//                                                 orbit_mizar_data::MizarPairedData,
//                                                 QtErrorReporter>;
class SamplingWithFrameTrackWidget
    : public SamplingWithFrameTrackWidgetTmpl<Ui::SamplingWithFrameTrackWidget,
                                              orbit_mizar_data::BaselineAndComparison,
                                              orbit_mizar_data::MizarPairedData, QtErrorReporter> {
};

void Funnk() { SamplingWithFrameTrackWidget widget; }

}  // namespace orbit_mizar_widgets