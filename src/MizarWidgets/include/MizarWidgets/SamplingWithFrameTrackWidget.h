// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MIZAR_WIDGETS_SAMPLING_WITH_FRAME_TRACK_WIDGET_H_
#define MIZAR_WIDGETS_SAMPLING_WITH_FRAME_TRACK_WIDGET_H_

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qwidget.h>

#include <QMessageBox>
#include <QObject>
#include <QStringLiteral>
#include <QWidget>
#include <memory>

#include "MizarBase/BaselineOrComparison.h"
#include "MizarData/BaselineAndComparison.h"
#include "MizarData/MizarPairedData.h"
#include "MizarData/SamplingWithFrameTrackComparisonReport.h"
#include "SamplingWithFrameTrackInputWidget.h"

namespace Ui {
class SamplingWithFrameTrackWidget;
}

namespace orbit_mizar_widgets {

class SamplingWithFrameTrackWidgetBase : public QWidget {
  Q_OBJECT

 protected:
  template <typename T>
  using Baseline = ::orbit_mizar_base::Baseline<T>;

  template <typename T>
  using Comparison = ::orbit_mizar_base::Comparison<T>;

 public:
  explicit SamplingWithFrameTrackWidgetBase(QWidget* parent = nullptr);
  ~SamplingWithFrameTrackWidgetBase() override;

 public slots:
  virtual void OnMultiplicityCorrectionCheckBoxClicked(int state) = 0;
  virtual void OnUpdateButtonClicked() = 0;
  void OnSignificanceLevelSelected(int index);

 private:
  [[nodiscard]] bool IsMultiplicityCorrectionEnabled() const;

 protected:
  bool is_multiplicity_correction_enabled_ = true;
  double significance_level_ = kDefaultSignificanceLevel;

  static constexpr inline double kDefaultSignificanceLevel = 0.05;
  static constexpr inline double kAlternativeSignificanceLevel = 0.01;

  static const inline Baseline<QString> kBaselineTitle =
      Baseline<QString>(QStringLiteral("Baseline"));
  static const inline Comparison<QString> kComparisonTitle =
      Comparison<QString>(QStringLiteral("Comparison"));
  static const inline QString kMultiplicityCorrectionEnabledLabel =
      QStringLiteral("Probability of false-alarm for at least one function:");
  static const inline QString kMultiplicityCorrectionDisabledLabel =
      QStringLiteral("Probability of false-alarm for an individual function:");
};

template <typename Ui, typename BaselineAndComparison, typename PairedData, auto ErrorReporter>
class SamplingWithFrameTrackWidgetTmpl : public SamplingWithFrameTrackWidgetBase {
 public:
  explicit SamplingWithFrameTrackWidgetTmpl(QWidget* parent = nullptr)
      : SamplingWithFrameTrackWidgetBase(parent), ui_(std::make_unique<Ui>()) {
    ui_->setupUi(this);

    OnMultiplicityCorrectionCheckBoxClicked(Qt::CheckState::Checked);

    QObject::connect(ui_->multiplicity_correction_, &QCheckBox::stateChanged, this,
                     &SamplingWithFrameTrackWidgetBase::OnMultiplicityCorrectionCheckBoxClicked);
    QObject::connect(ui_->significance_level_, qOverload<int>(&QComboBox::currentIndexChanged),
                     this, &SamplingWithFrameTrackWidgetBase::OnSignificanceLevelSelected);
    QObject::connect(ui_->update_button_, &QPushButton::clicked, this,
                     &SamplingWithFrameTrackWidgetBase::OnUpdateButtonClicked);
  }
  ~SamplingWithFrameTrackWidgetTmpl() override = default;

  void Init(const orbit_mizar_data::BaselineAndComparison* baseline_and_comparison) {
    LiftAndApply(&SamplingWithFrameTrackInputWidget::Init, GetBaselineInput(),
                 baseline_and_comparison->GetBaselineData(), kBaselineTitle);
    LiftAndApply(&SamplingWithFrameTrackInputWidget::Init, GetComparisonInput(),
                 baseline_and_comparison->GetComparisonData(), kComparisonTitle);
    baseline_and_comparison_ = baseline_and_comparison;
  }

 protected:
  void OnMultiplicityCorrectionCheckBoxClicked(int state) override {
    is_multiplicity_correction_enabled_ = (state == Qt::CheckState::Checked);

    const QString text = is_multiplicity_correction_enabled_ ? kMultiplicityCorrectionEnabledLabel
                                                             : kMultiplicityCorrectionDisabledLabel;
    ui_->significance_level_label_->setText(text);
  }

  void OnUpdateButtonClicked() override {
    Baseline<orbit_mizar_data::HalfOfSamplingWithFrameTrackReportConfig> baseline_config =
        LiftAndApply(&SamplingWithFrameTrackInputWidget::MakeConfig, GetBaselineInput());

    Comparison<orbit_mizar_data::HalfOfSamplingWithFrameTrackReportConfig> comparison_config =
        LiftAndApply(&SamplingWithFrameTrackInputWidget::MakeConfig, GetComparisonInput());

    std::ignore = EmitWarningIfNeeded(
        LiftAndApply(&ValidateConfig, baseline_config, baseline_and_comparison_->GetBaselineData()),
        LiftAndApply(&ValidateConfig, comparison_config,
                     baseline_and_comparison_->GetComparisonData()));
  }

 private:
  static ErrorMessageOr<void> ValidateConfig(
      const orbit_mizar_data::HalfOfSamplingWithFrameTrackReportConfig& config,
      const orbit_mizar_data::MizarPairedData& data) {
    if (config.tids.empty()) {
      return ErrorMessage{"No threads selected"};
    }
    if (config.start_relative_ns > data.CaptureDuration()) {
      return ErrorMessage{"Start > capture duration"};
    }
    return outcome::success();
  }

  [[nodiscard]] bool EmitWarningIfNeeded(
      const Baseline<ErrorMessageOr<void>>& baseline_validation_result,
      const Comparison<ErrorMessageOr<void>>& comparison_validation_result) {
    Baseline<bool> baseline_ok =
        LiftAndApply(&SamplingWithFrameTrackWidgetTmpl::EmitOneWarningIfNeeded,
                     Baseline<SamplingWithFrameTrackWidgetTmpl*>(this), baseline_validation_result,
                     kBaselineTitle);

    Comparison<bool> comparison_ok =
        LiftAndApply(&SamplingWithFrameTrackWidgetTmpl::EmitOneWarningIfNeeded,
                     Comparison<SamplingWithFrameTrackWidgetTmpl*>(this),
                     comparison_validation_result, kComparisonTitle);

    return *baseline_ok && *comparison_ok;
  }

  bool EmitOneWarningIfNeeded(const ErrorMessageOr<void>& validation_result, const QString& title) {
    if (validation_result.has_error()) {
      ErrorReporter(title + ": " + QString::fromStdString(validation_result.error().message()));
      return false;
    }
    return true;
  }

  [[nodiscard]] Baseline<SamplingWithFrameTrackInputWidget*> GetBaselineInput() const {
    return Baseline<SamplingWithFrameTrackInputWidget*>(ui_->baseline_input_);
  }

  [[nodiscard]] Comparison<SamplingWithFrameTrackInputWidget*> GetComparisonInput() const {
    return Comparison<SamplingWithFrameTrackInputWidget*>(ui_->comparison_input_);
  }

  const BaselineAndComparison* baseline_and_comparison_;
  std::unique_ptr<Ui> ui_;
};

inline void QtErrorReporter(QWidget* parent, const QString& message) {
  QMessageBox::critical(parent, "Invalid input", message);
}

class SamplingWithFrameTrackWidget;

}  // namespace orbit_mizar_widgets

#endif  // MIZAR_WIDGETS_SAMPLING_WITH_FRAME_TRACK_WIDGET_H_
