// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/performance_paint_timing.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_object_builder.h"

namespace blink {

PerformancePaintTiming::PerformancePaintTiming(PaintType type,
                                               double start_time)
    : PerformanceEntry(FromPaintTypeToString(type),
                       "paint",
                       start_time,
                       start_time) {}

PerformancePaintTiming::~PerformancePaintTiming() = default;

String PerformancePaintTiming::FromPaintTypeToString(PaintType type) {
  switch (type) {
    case PaintType::kFirstPaint:
      return "first-paint";
    case PaintType::kFirstContentfulPaint:
      return "first-contentful-paint";
  }
  NOTREACHED();
  return "";
}
}  // namespace blink
