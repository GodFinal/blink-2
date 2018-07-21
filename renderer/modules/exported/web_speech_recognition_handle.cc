/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/web/web_speech_recognition_handle.h"

#include "third_party/blink/renderer/modules/speech/speech_recognition.h"

namespace blink {

void WebSpeechRecognitionHandle::Reset() {
  private_.Reset();
}

void WebSpeechRecognitionHandle::Assign(
    const WebSpeechRecognitionHandle& other) {
  private_ = other.private_;
}

bool WebSpeechRecognitionHandle::Equals(
    const WebSpeechRecognitionHandle& other) const {
  return private_.Get() == other.private_.Get();
}

bool WebSpeechRecognitionHandle::LessThan(
    const WebSpeechRecognitionHandle& other) const {
  return private_.Get() < other.private_.Get();
}

WebSpeechRecognitionHandle::WebSpeechRecognitionHandle(
    SpeechRecognition* speech_recognition)
    : private_(speech_recognition) {}

WebSpeechRecognitionHandle::operator SpeechRecognition*() const {
  return private_.Get();
}

}  // namespace blink
