// Copyright (c) 2022 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <unistd.h>

#include <cinttypes>
#include <iostream>
#include <thread>

#include "ApiInterface/Orbit.h"

ORBIT_API_INSTANTIATE;

int64_t DurationNanos(const struct timespec& start, const struct timespec& end) {
  return (static_cast<int64_t>(end.tv_sec) - static_cast<int64_t>(start.tv_sec)) *
             static_cast<int64_t>(1'000'000'000) +
         (static_cast<int64_t>(end.tv_nsec) - static_cast<int64_t>(start.tv_nsec));
}

void Wait(int64_t nanos) {
  timespec start_time;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start_time);

  timespec curr_time;
  do {
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &curr_time);
  } while (DurationNanos(start_time, curr_time) < nanos);
}

void WaitOneSecond() __attribute__((noinline));

void WaitOneSecond() { Wait(1'000'000'000); }

void WaitOneMillisecond() __attribute__((noinline));

void WaitOneMillisecond() { Wait(1'000'000); }

void WaitTenMicroseconds() __attribute__((noinline));

void WaitTenMicroseconds() { Wait(10'000); }

void WaitOneSecondM() __attribute__((noinline));

void WaitOneSecondM() {
  ORBIT_SCOPE(__FUNCTION__);
  Wait(1'000'000'000);
}

void WaitOneMillisecondM() __attribute__((noinline));

void WaitOneMillisecondM() {
  ORBIT_SCOPE(__FUNCTION__);
  Wait(1'000'000);
}

void WaitTenMicrosecondsM() __attribute__((noinline));

void WaitTenMicrosecondsM() {
  ORBIT_SCOPE(__FUNCTION__);
  Wait(10'000);
}

int main() {
  sleep(20);

  for (int i = 0; i < 5; ++i) {
    WaitOneSecond();
    WaitOneSecondM();
    for (int j = 0; j < 1000; ++j) {
      WaitOneMillisecond();
      WaitOneMillisecondM();
      for (int k = 0; k < 100; ++k) {
        WaitTenMicroseconds();
        WaitTenMicrosecondsM();
      }
    }
  }

  return 0;
}
