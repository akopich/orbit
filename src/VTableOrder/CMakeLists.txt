# Copyright (c) 2022 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.15)

project(VTableOrder CXX)

add_executable(VTableOrder)

target_sources(
        VTableOrder
        PRIVATE main.cpp
)

target_include_directories(VTableOrder PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VTableOrder
        PRIVATE OrbitBase
)
