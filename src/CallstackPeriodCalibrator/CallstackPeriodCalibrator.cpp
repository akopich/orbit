// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <chrono>
#include <iostream>
#include <iterator>
#include <limits>
#include <random>

[[nodiscard]] static int Median(std::vector<int>& v) {
  std::sort(std::begin(v), std::end(v));
  return v[v.size() / 2];
}

int main() {
  std::mt19937 mt(std::chrono::system_clock::now().time_since_epoch().count());
  std::uniform_int_distribution<std::mt19937::result_type> generator(
      0, std::numeric_limits<int>::max());

  int sum = 0;

  constexpr size_t kSize = 1e8;
  std::vector<int> v(kSize, 0);

  while (true) {
    std::generate(std::begin(v), std::end(v), [&generator, &mt]() { return generator(mt); });
    sum += Median(v);
  }

  std::cout << sum << std::endl;
  return 0;
}