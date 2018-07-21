/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/modules/webgl/webgl_lose_context.h"

#include "third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.h"

namespace blink {

WebGLLoseContext::WebGLLoseContext(WebGLRenderingContextBase* context)
    : WebGLExtension(context) {}

void WebGLLoseContext::Lose(bool force) {
  if (force)
    WebGLExtension::Lose(true);
}

WebGLExtensionName WebGLLoseContext::GetName() const {
  return kWebGLLoseContextName;
}

WebGLLoseContext* WebGLLoseContext::Create(WebGLRenderingContextBase* context) {
  return new WebGLLoseContext(context);
}

void WebGLLoseContext::loseContext() {
  WebGLExtensionScopedContext scoped(this);
  if (!scoped.IsLost()) {
    scoped.Context()->ForceLostContext(
        WebGLRenderingContextBase::kWebGLLoseContextLostContext,
        WebGLRenderingContextBase::kManual);
  }
}

void WebGLLoseContext::restoreContext() {
  WebGLExtensionScopedContext scoped(this);
  if (!scoped.IsLost())
    scoped.Context()->ForceRestoreContext();
}

bool WebGLLoseContext::Supported(WebGLRenderingContextBase*) {
  return true;
}

const char* WebGLLoseContext::ExtensionName() {
  return "WEBGL_lose_context";
}

}  // namespace blink
