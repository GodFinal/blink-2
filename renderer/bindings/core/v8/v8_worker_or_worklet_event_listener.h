/*
 * Copyright (C) 2006, 2007, 2008, 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_WORKER_OR_WORKLET_EVENT_LISTENER_H_
#define THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_WORKER_OR_WORKLET_EVENT_LISTENER_H_

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_event_listener.h"
#include "v8/include/v8.h"

namespace blink {

class Event;

class V8WorkerOrWorkletEventListener final : public V8EventListener {
 public:
  static V8WorkerOrWorkletEventListener* Create(v8::Local<v8::Object> listener,
                                                bool is_inline,
                                                ScriptState* script_state) {
    V8WorkerOrWorkletEventListener* event_listener =
        new V8WorkerOrWorkletEventListener(is_inline, script_state);
    event_listener->SetListenerObject(listener);
    return event_listener;
  }

  void HandleEvent(ScriptState*, Event*) override;

 protected:
  V8WorkerOrWorkletEventListener(bool is_inline, ScriptState*);

 private:
  v8::Local<v8::Value> CallListenerFunction(ScriptState*,
                                            v8::Local<v8::Value>,
                                            Event*) override;
  v8::Local<v8::Object> GetReceiverObject(ScriptState*, Event*);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_V8_V8_WORKER_OR_WORKLET_EVENT_LISTENER_H_
