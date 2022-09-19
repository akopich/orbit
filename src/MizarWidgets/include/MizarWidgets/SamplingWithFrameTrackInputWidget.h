// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MIZAR_WIDGETS_SAMPLING_WITH_FRAME_TRACK_INPUT_WIDGET_H_
#define MIZAR_WIDGETS_SAMPLING_WITH_FRAME_TRACK_INPUT_WIDGET_H_

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <absl/functional/bind_front.h>
#include <absl/strings/str_format.h>
#include <stdint.h>

#include <QAbstractListModel>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QObject>
#include <QRadioButton>
#include <QWidget>
#include <Qt>
#include <chrono>
#include <limits>
#include <memory>
#include <string_view>
#include <type_traits>
#include <vector>

#include "ClientData/ScopeInfo.h"
#include "ClientData/ScopeStats.h"
#include "GrpcProtos/capture.pb.h"
#include "MizarBase/Time.h"
#include "MizarData/FrameTrack.h"
#include "MizarData/MizarPairedData.h"
#include "MizarData/SamplingWithFrameTrackComparisonReport.h"
#include "MizarModels/FrameTrackListModel.h"
#include "OrbitBase/Logging.h"
#include "OrbitBase/Sort.h"

namespace Ui {
class SamplingWithFrameTrackInputWidget;
}

Q_DECLARE_METATYPE(::orbit_mizar_base::TID);

namespace orbit_mizar_widgets {

class SamplingWithFrameTrackInputWidgetBase : public QWidget {
  Q_OBJECT
  using TID = ::orbit_mizar_base::TID;
  using FrameTrackId = ::orbit_mizar_data::FrameTrackId;
  using RelativeTimeNs = ::orbit_mizar_base::RelativeTimeNs;

 public:
  ~SamplingWithFrameTrackInputWidgetBase() override;

  [[nodiscard]] orbit_mizar_data::HalfOfSamplingWithFrameTrackReportConfig MakeConfig() const;

  void SetUseItsSymbols(bool use_its_symbols) { GetUseItsSymbols()->setChecked(use_its_symbols); }

 signals:
  void UseItsSymbolToggled(bool checked);

 public slots:
  void OnThreadSelectionChanged();
  void OnFrameTrackSelectionChanged(int index);
  void OnStartMsChanged(const QString& time_ms);

 private:
  std::unique_ptr<Ui::SamplingWithFrameTrackInputWidget> ui_;

 protected:
  static int constexpr kTidRole = Qt::UserRole + 1;

  explicit SamplingWithFrameTrackInputWidgetBase(QWidget* parent = nullptr);

  [[nodiscard]] QLabel* GetTitle() const;
  [[nodiscard]] QLabel* GetFileName() const;
  [[nodiscard]] QListWidget* GetThreadList() const;
  [[nodiscard]] QComboBox* GetFrameTrackList() const;
  [[nodiscard]] QLineEdit* GetStartMs() const;
  [[nodiscard]] QRadioButton* GetUseItsSymbols() const;

  absl::flat_hash_set<TID> selected_tids_;

  // std::numeric_limits<uint64_t>::max() ns corresponds to malformed input
  RelativeTimeNs start_timestamp_{0};

 private:
  FrameTrackId frame_track_id_{};
};

template <typename PairedData, typename FrameTrackListModel>
class SamplingWithFrameTrackInputWidgetTmpl : public SamplingWithFrameTrackInputWidgetBase {
  using TID = ::orbit_mizar_base::TID;
  using FrameTrackId = ::orbit_mizar_data::FrameTrackId;
  using FrameTrackInfo = ::orbit_mizar_data::FrameTrackInfo;
  using PresentEvent = ::orbit_grpc_protos::PresentEvent;
  using ScopeInfo = ::orbit_client_data::ScopeInfo;

 public:
  SamplingWithFrameTrackInputWidgetTmpl() = delete;
  explicit SamplingWithFrameTrackInputWidgetTmpl(QWidget* parent)
      : SamplingWithFrameTrackInputWidgetBase(parent) {}
  ~SamplingWithFrameTrackInputWidgetTmpl() override = default;

  void Init(const PairedData& data, const QString& title, const QString& file_name,
            bool use_its_symbols) {
    InitTitle(title);
    InitFileName(file_name);
    InitThreadList(data);
    InitFrameTrackList(data);
    InitStartMs();
    InitUseItsSymbolsButton(use_its_symbols);
  }

 private:
  void InitUseItsSymbolsButton(bool use_its_symbols) {
    GetUseItsSymbols()->setChecked(use_its_symbols);
  }

  void InitTitle(const QString& title) { GetTitle()->setText(title); }

  void InitFileName(const QString& file_name) { GetFileName()->setText(file_name); }

  void InitThreadList(const PairedData& data) {
    QListWidget* list = GetThreadList();
    list->setSelectionMode(QAbstractItemView::ExtendedSelection);

    const absl::flat_hash_map<TID, std::string>& tid_to_name = data.TidToNames();
    const absl::flat_hash_map<TID, uint64_t>& counts = data.TidToCallstackSampleCounts();

    std::vector<std::pair<TID, uint64_t>> counts_sorted(std::begin(counts), std::end(counts));

    orbit_base::sort(std::begin(counts_sorted), std::end(counts_sorted),
                     &std::pair<TID, uint64_t>::second, std::greater<>{});
    for (const auto& [tid, unused_count] : counts_sorted) {
      auto item = std::make_unique<QListWidgetItem>(
          QString::fromStdString(absl::StrFormat("[%u] %s", *tid, tid_to_name.at(tid))));
      item->setData(kTidRole, QVariant::fromValue(tid));
      list->addItem(item.release());
    }
  }

  void InitFrameTrackList(const PairedData& data) {
    auto model =
        std::make_unique<FrameTrackListModel>(&data, &selected_tids_, &start_timestamp_, parent());

    GetFrameTrackList()->setModel(model.release());
    OnFrameTrackSelectionChanged(0);
  }

  void InitStartMs() {
    GetStartMs()->setValidator(new QIntValidator(0, std::numeric_limits<int>::max(), this));
    GetStartMs()->setText("0");
  }
};

using SamplingWithFrameTrackInputWidget =
    SamplingWithFrameTrackInputWidgetTmpl<orbit_mizar_data::MizarPairedData,
                                          orbit_mizar_models::FrameTrackListModel>;

}  // namespace orbit_mizar_widgets

#endif  // MIZAR_WIDGETS_SAMPLING_WITH_FRAME_TRACK_INPUT_WIDGET_H_
