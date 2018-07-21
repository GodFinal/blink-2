// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_NUMBER_PROPERTY_FUNCTIONS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_NUMBER_PROPERTY_FUNCTIONS_H_

#include "third_party/blink/renderer/core/css_property_names.h"
#include "third_party/blink/renderer/platform/wtf/optional.h"

namespace blink {

class ComputedStyle;
class CSSProperty;

class NumberPropertyFunctions {
 public:
  static Optional<double> GetInitialNumber(const CSSProperty&);
  static Optional<double> GetNumber(const CSSProperty&, const ComputedStyle&);
  static double ClampNumber(const CSSProperty&, double);
  static bool SetNumber(const CSSProperty&, ComputedStyle&, double);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_ANIMATION_NUMBER_PROPERTY_FUNCTIONS_H_
