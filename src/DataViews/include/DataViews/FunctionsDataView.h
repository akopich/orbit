// Copyright (c) 2021 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DATA_VIEWS_FUNCTIONS_DATA_VIEW_H_
#define DATA_VIEWS_FUNCTIONS_DATA_VIEW_H_

#include <string>
#include <vector>

#include "ClientData/FunctionInfo.h"
#include "DataViews/AppInterface.h"
#include "DataViews/DataView.h"

namespace orbit_data_views {
class FunctionsDataView : public DataView {
 public:
  explicit FunctionsDataView(AppInterface* app,
                             orbit_metrics_uploader::MetricsUploader* metrics_uploader);

  static const std::string kUnselectedFunctionString;
  static const std::string kSelectedFunctionString;
  static const std::string kFrameTrackString;
  static const std::string kApiScopeType;
  static const std::string kApiScopeAsyncType;
  static const std::string kHookedFunctionType;
  static std::string BuildSelectedAndFrametrackIconsString(
      AppInterface* app, const orbit_client_data::FunctionInfo& function);
  static bool ShouldShowFrameTrackIcon(AppInterface* app,
                                       const orbit_client_data::FunctionInfo& function);

  const std::vector<Column>& GetColumns() override;
  int GetDefaultSortingColumn() override { return kColumnAddressInModule; }
  std::string GetValue(int row, int column) override;
  std::string GetLabel() override { return "Functions"; }

  void AddFunctions(std::vector<const orbit_client_data::FunctionInfo*> functions);
  void ClearFunctions();

 protected:
  [[nodiscard]] ActionStatus GetActionStatus(std::string_view action, int clicked_index,
                                             const std::vector<int>& selected_indices) override;
  void DoSort() override;
  void DoFilter() override;

  std::vector<std::string> filter_tokens_;

  enum ColumnIndex {
    kColumnSelected,
    kColumnName,
    kColumnSize,
    kColumnModule,
    kColumnAddressInModule,
    kNumColumns
  };

 private:
  static bool ShouldShowSelectedFunctionIcon(AppInterface* app,
                                             const orbit_client_data::FunctionInfo& function);
  [[nodiscard]] const orbit_client_data::FunctionInfo* GetFunctionInfoFromRow(int row) override {
    return functions_[indices_[row]];
  }

  std::vector<const orbit_client_data::FunctionInfo*> functions_;
};

}  // namespace orbit_data_views

#endif  // DATA_VIEWS_FUNCTIONS_DATA_VIEW_H_
