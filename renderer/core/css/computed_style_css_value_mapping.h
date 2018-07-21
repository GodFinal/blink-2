// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_COMPUTED_STYLE_CSS_VALUE_MAPPING_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_COMPUTED_STYLE_CSS_VALUE_MAPPING_H_

#include "third_party/blink/renderer/core/css/css_value.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"

namespace blink {

class CSSVariableData;
class ComputedStyle;
class PropertyRegistry;

class ComputedStyleCSSValueMapping {
  STATIC_ONLY(ComputedStyleCSSValueMapping);

 public:
  static const CSSValue* Get(const AtomicString custom_property_name,
                             const ComputedStyle&,
                             const PropertyRegistry*);
  static std::unique_ptr<HashMap<AtomicString, scoped_refptr<CSSVariableData>>>
  GetVariables(const ComputedStyle&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_COMPUTED_STYLE_CSS_VALUE_MAPPING_H_
