// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/interface.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#ifndef V8ArrayBuffer_h
#define V8ArrayBuffer_h

#include "bindings/core/v8/generated_code_helper.h"
#include "bindings/core/v8/native_value_traits.h"
#include "bindings/core/v8/to_v8_for_core.h"
#include "bindings/core/v8/v8_binding_for_core.h"
#include "bindings/tests/idls/core/test_array_buffer.h"
#include "core/core_export.h"
#include "platform/bindings/script_wrappable.h"
#include "platform/bindings/v8_dom_wrapper.h"
#include "platform/bindings/wrapper_type_info.h"
#include "platform/heap/handle.h"

namespace blink {

class V8ArrayBuffer {
  STATIC_ONLY(V8ArrayBuffer);
 public:
  CORE_EXPORT static TestArrayBuffer* ToImpl(v8::Local<v8::Object> object);
  CORE_EXPORT static TestArrayBuffer* ToImplWithTypeCheck(v8::Isolate*, v8::Local<v8::Value>);
  CORE_EXPORT static const WrapperTypeInfo wrapperTypeInfo;
  static const int internalFieldCount = kV8DefaultWrapperInternalFieldCount;

  // Callback functions

  CORE_EXPORT static void byteLengthAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>&);
};

template <>
struct NativeValueTraits<TestArrayBuffer> : public NativeValueTraitsBase<TestArrayBuffer> {
  CORE_EXPORT static TestArrayBuffer* NativeValue(v8::Isolate*, v8::Local<v8::Value>, ExceptionState&);
  CORE_EXPORT static TestArrayBuffer* NullValue() { return nullptr; }
};

template <>
struct V8TypeOf<TestArrayBuffer> {
  typedef V8ArrayBuffer Type;
};

}  // namespace blink

#endif  // V8ArrayBuffer_h