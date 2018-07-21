/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/layout/layout_theme_mobile.h"

#include "build/build_config.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_theme_engine.h"
#include "third_party/blink/renderer/core/style/computed_style.h"
#include "third_party/blink/renderer/platform/data_resource_helper.h"
#include "third_party/blink/renderer/platform/layout_test_support.h"

namespace blink {

scoped_refptr<LayoutTheme> LayoutThemeMobile::Create() {
  return base::AdoptRef(new LayoutThemeMobile());
}

LayoutThemeMobile::~LayoutThemeMobile() = default;

String LayoutThemeMobile::ExtraDefaultStyleSheet() {
  return LayoutThemeDefault::ExtraDefaultStyleSheet() +
         GetDataResourceAsASCIIString("themeChromiumLinux.css") +
         GetDataResourceAsASCIIString("themeChromiumAndroid.css");
}

String LayoutThemeMobile::ExtraFullscreenStyleSheet() {
  return GetDataResourceAsASCIIString("fullscreenAndroid.css");
}

void LayoutThemeMobile::AdjustInnerSpinButtonStyle(ComputedStyle& style) const {
  if (LayoutTestSupport::IsRunningLayoutTest()) {
    // Match Linux spin button style in layout tests.
    // FIXME: Consider removing the conditional if a future Android theme
    // matches this.
    IntSize size = Platform::Current()->ThemeEngine()->GetSize(
        WebThemeEngine::kPartInnerSpinButton);

    style.SetWidth(Length(size.Width(), kFixed));
    style.SetMinWidth(Length(size.Width(), kFixed));
  }
}

bool LayoutThemeMobile::ShouldUseFallbackTheme(
    const ComputedStyle& style) const {
#if defined(OS_MACOSX)
  // Mac WebThemeEngine cannot handle these controls.
  ControlPart part = style.Appearance();
  if (part == kCheckboxPart || part == kRadioPart)
    return true;
#endif
  return LayoutThemeDefault::ShouldUseFallbackTheme(style);
}

}  // namespace blink
