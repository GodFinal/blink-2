// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NGExclusion_h
#define NGExclusion_h

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/layout/ng/geometry/ng_bfc_rect.h"
#include "third_party/blink/renderer/core/style/computed_style_constants.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"

namespace blink {

// Struct that represents an exclusion. This currently is just a float but
// we've named it an exclusion to potentially support other types in the future.
struct CORE_EXPORT NGExclusion : public RefCounted<NGExclusion> {
  static scoped_refptr<NGExclusion> Create(const NGBfcRect& rect,
                                           const EFloat type) {
    return base::AdoptRef(new NGExclusion(rect, type));
  }

  const NGBfcRect rect;
  const EFloat type;

  bool operator==(const NGExclusion& other) const;
  bool operator!=(const NGExclusion& other) const { return !(*this == other); }

 private:
  NGExclusion(const NGBfcRect& rect, const EFloat type)
      : rect(rect), type(type) {}
};

}  // namespace blink

#endif  // NGExclusion_h
