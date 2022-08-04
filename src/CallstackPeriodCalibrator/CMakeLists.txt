# Copyright (c) 2022 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.15)

add_executable(CallstackPeriodCalibrator)

target_sources(CallstackPeriodCalibrator PRIVATE
  CallstackPeriodCalibrator.cpp)
