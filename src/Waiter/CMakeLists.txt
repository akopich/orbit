# Copyright (c) 2022 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


cmake_minimum_required(VERSION 3.15)

project(Waiter CXX) 
add_executable(Waiter)

target_sources(
  Waiter
  PRIVATE Waiter.cpp)

target_include_directories(Waiter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}) 


target_link_libraries( Waiter PRIVATE
pthread ApiInterface
)

