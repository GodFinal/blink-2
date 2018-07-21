// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/paint/transform_display_item.h"

#include "third_party/blink/public/platform/web_display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/graphics_context.h"
#include "third_party/blink/renderer/platform/transforms/affine_transform.h"

namespace blink {

void BeginTransformDisplayItem::Replay(GraphicsContext& context) const {
  context.Save();
  context.ConcatCTM(transform_);
}

void BeginTransformDisplayItem::AppendToWebDisplayItemList(
    const FloatSize&,
    WebDisplayItemList* list) const {
  list->AppendTransformItem(AffineTransformToSkMatrix(transform_));
}

#if DCHECK_IS_ON()
void BeginTransformDisplayItem::PropertiesAsJSON(JSONObject& json) const {
  PairedBeginDisplayItem::PropertiesAsJSON(json);
  json.SetString("transform", transform_.ToString());
}
#endif

void EndTransformDisplayItem::Replay(GraphicsContext& context) const {
  context.Restore();
}

void EndTransformDisplayItem::AppendToWebDisplayItemList(
    const FloatSize&,
    WebDisplayItemList* list) const {
  list->AppendEndTransformItem();
}

}  // namespace blink
