// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/web/win/web_font_rendering.h"

#include "third_party/blink/renderer/platform/fonts/font_cache.h"

namespace blink {

// static
void WebFontRendering::SetSkiaFontManager(sk_sp<SkFontMgr> font_mgr) {
  FontCache::SetFontManager(std::move(font_mgr));
}

// static
void WebFontRendering::SetDeviceScaleFactor(float device_scale_factor) {
  FontCache::SetDeviceScaleFactor(device_scale_factor);
}

// static
void WebFontRendering::AddSideloadedFontForTesting(sk_sp<SkTypeface> typeface) {
  FontCache::AddSideloadedFontForTesting(std::move(typeface));
}

// static
void WebFontRendering::SetMenuFontMetrics(const wchar_t* family_name,
                                          int32_t font_height) {
  FontCache::SetMenuFontMetrics(family_name, font_height);
}

// static
void WebFontRendering::SetSmallCaptionFontMetrics(const wchar_t* family_name,
                                                  int32_t font_height) {
  FontCache::SetSmallCaptionFontMetrics(family_name, font_height);
}

// static
void WebFontRendering::SetStatusFontMetrics(const wchar_t* family_name,
                                            int32_t font_height) {
  FontCache::SetStatusFontMetrics(family_name, font_height);
}

// static
void WebFontRendering::SetAntialiasedTextEnabled(bool enabled) {
  FontCache::SetAntialiasedTextEnabled(enabled);
}

// static
void WebFontRendering::SetLCDTextEnabled(bool enabled) {
  FontCache::SetLCDTextEnabled(enabled);
}

// static
void WebFontRendering::SetUseSkiaFontFallback(bool use_skia_font_fallback) {
  FontCache::SetUseSkiaFontFallback(use_skia_font_fallback);
}

}  // namespace blink
