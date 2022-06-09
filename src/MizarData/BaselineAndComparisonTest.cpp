// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <absl/container/flat_hash_map.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "BaselineAndComparisonHelper.h"
#include "MizarData/BaselineAndComparison.h"
#include "OrbitBase/ThreadConstants.h"

namespace orbit_mizar_data {

constexpr size_t kFunctionNum = 3;
constexpr std::array<uint64_t, kFunctionNum> kBaselineFunctionAddresses = {0xF00D, 0xBEAF, 0xDEAF};
constexpr std::array<uint64_t, kFunctionNum> kComparisonFunctionAddresses = {0x0FF, 0xCAFE, 0xDEA};
const std::array<std::string, kFunctionNum> kBaselineFunctionNames = {"foo()", "bar()", "biz()"};
const std::array<std::string, kFunctionNum> kComparisonFunctionNames = {"foo()", "bar()", "fiz()"};

template <typename Container>
[[nodiscard]] static auto Commons(const Container& a, const Container& b) {
  using E = typename Container::value_type;
  using std::begin;
  using std::end;

  absl::flat_hash_set<E> a_set(begin(a), end(a));
  std::vector<E> result;
  std::copy_if(begin(b), end(b), std::back_inserter(result),
               [&a_set](const E& element) { return a_set.contains(element); });
  return result;
}

const std::vector<std::string> kCommonFunctionNames =
    Commons(kBaselineFunctionNames, kComparisonFunctionNames);

template <typename Container, typename AnotherContainer>
auto MakeMap(const Container& keys, const AnotherContainer& values) {
  using K = typename Container::value_type;
  using V = typename AnotherContainer::value_type;
  using std::begin;
  using std::end;

  absl::flat_hash_map<K, V> result;
  std::transform(begin(keys), end(keys), begin(values), std::inserter(result, std::begin(result)),
                 [](const K& k, const V& v) { return std::make_pair(k, v); });
  return result;
}

const absl::flat_hash_map<uint64_t, std::string> kBaselineAddressToName =
    MakeMap(kBaselineFunctionAddresses, kBaselineFunctionNames);
const absl::flat_hash_map<uint64_t, std::string> kComparisonAddressToName =
    MakeMap(kComparisonFunctionAddresses, kComparisonFunctionNames);

static void ExpectCorrectNames(const absl::flat_hash_map<uint64_t, SFID>& address_to_sfid,
                               const absl::flat_hash_map<SFID, std::string>& sfid_to_name,
                               const absl::flat_hash_map<uint64_t, std::string>& address_to_name) {
  for (const auto& [address, sfid] : address_to_sfid) {
    EXPECT_TRUE(sfid_to_name.contains(sfid));
    EXPECT_EQ(sfid_to_name.at(sfid), address_to_name.at(address));
  }
}

template <typename K, typename V>
[[nodiscard]] static std::vector<V> Values(const absl::flat_hash_map<K, V>& map) {
  std::vector<V> values;
  std::transform(std::begin(map), std::end(map), std::back_inserter(values),
                 [](const std::pair<K, V> pair) { return pair.second; });
  return values;
}

TEST(BaselineAndComparisonTest, BaselineAndComparisonHelperIsCorrect) {
  const auto [baseline_address_to_sfid, comparison_address_to_sfid, sfid_to_name] =
      AssignSampledFunctionIds(kBaselineAddressToName, kComparisonAddressToName);

  EXPECT_EQ(baseline_address_to_sfid.size(), kCommonFunctionNames.size());
  EXPECT_EQ(comparison_address_to_sfid.size(), kCommonFunctionNames.size());
  EXPECT_EQ(sfid_to_name.size(), kCommonFunctionNames.size());

  ExpectCorrectNames(baseline_address_to_sfid, sfid_to_name, kBaselineAddressToName);
  ExpectCorrectNames(comparison_address_to_sfid, sfid_to_name, kComparisonAddressToName);

  EXPECT_THAT(Values(baseline_address_to_sfid),
              testing::UnorderedElementsAreArray(Values(comparison_address_to_sfid)));
}

constexpr SFID kSFIDFirst = SFID(1);
constexpr SFID kSFIDSecond = SFID(2);
constexpr SFID kSFIDThird = SFID(3);

const std::vector<std::vector<SFID>> kCallstacks = {
    std::vector<SFID>{kSFIDFirst, kSFIDSecond, kSFIDThird}, {kSFIDSecond}, {}};

namespace {
class MockPairedData {
 public:
  explicit MockPairedData(std::vector<std::vector<SFID>> callstacks)
      : callstacks_(std::move(callstacks)) {}

  void ForEachCallstackEvent(uint32_t /*tid*/, uint64_t /*min_timestamp*/,
                             uint64_t /*max_timestamp*/,
                             std::function<void(const std::vector<SFID>&)> action) const {
    ORBIT_LOG(" ENTER");

    for (const auto& callstack : callstacks_) {
      ORBIT_LOG(" LOOP");
      action(callstack);
    }
  }

 private:
  std::vector<std::vector<SFID>> callstacks_;
};
}  // namespace

TEST(BaselineAndComparisonTest, MakeSamplingWithFrameTrackReportIsCorrect) {
  MockPairedData full(kCallstacks);
  MockPairedData empty({});

  BaselineAndComparisonTmpl<MockPairedData> bac(full, empty, {});
  SamplingWithFrameTrackComparisonReport report = bac.MakeSamplingWithFrameTrackReport(
      BaselineSamplingWithFrameTrackReportConfig{{orbit_base::kAllProcessThreadsTid}, 0, 1},
      ComparisonSamplingWithFrameTrackReportConfig{{orbit_base::kAllProcessThreadsTid}, 0, 1});

  EXPECT_EQ(report.baseline_sampling_counts.total_callstacks, kCallstacks.size());
  
  EXPECT_EQ(report.baseline_sampling_counts.GetExclusiveCnt(kSFIDFirst), 0);
  EXPECT_EQ(report.baseline_sampling_counts.GetExclusiveCnt(kSFIDSecond), 1);
  EXPECT_EQ(report.baseline_sampling_counts.GetExclusiveCnt(kSFIDThird), 1);

  EXPECT_EQ(report.baseline_sampling_counts.GetInclusiveCnt(kSFIDFirst), 1);
  EXPECT_EQ(report.baseline_sampling_counts.GetInclusiveCnt(kSFIDSecond), 2);
  EXPECT_EQ(report.baseline_sampling_counts.GetInclusiveCnt(kSFIDThird), 1);
}

}  // namespace orbit_mizar_data