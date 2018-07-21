// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/interface.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#ifndef V8TestIntegerIndexed_h
#define V8TestIntegerIndexed_h

#include "bindings/core/v8/generated_code_helper.h"
#include "bindings/core/v8/native_value_traits.h"
#include "bindings/core/v8/to_v8_for_core.h"
#include "bindings/core/v8/v8_binding_for_core.h"
#include "bindings/tests/idls/core/test_integer_indexed.h"
#include "core/core_export.h"
#include "platform/bindings/script_wrappable.h"
#include "platform/bindings/v8_dom_wrapper.h"
#include "platform/bindings/wrapper_type_info.h"
#include "platform/heap/handle.h"

namespace blink {

class V8TestIntegerIndexed {
  STATIC_ONLY(V8TestIntegerIndexed);
 public:
  CORE_EXPORT static bool hasInstance(v8::Local<v8::Value>, v8::Isolate*);
  static v8::Local<v8::Object> findInstanceInPrototypeChain(v8::Local<v8::Value>, v8::Isolate*);
  CORE_EXPORT static v8::Local<v8::FunctionTemplate> domTemplate(v8::Isolate*, const DOMWrapperWorld&);
  static TestIntegerIndexed* ToImpl(v8::Local<v8::Object> object) {
    return ToScriptWrappable(object)->ToImpl<TestIntegerIndexed>();
  }
  CORE_EXPORT static TestIntegerIndexed* ToImplWithTypeCheck(v8::Isolate*, v8::Local<v8::Value>);
  CORE_EXPORT static const WrapperTypeInfo wrapperTypeInfo;
  static void indexedPropertyGetterCustom(uint32_t, const v8::PropertyCallbackInfo<v8::Value>&);
  static void indexedPropertySetterCustom(uint32_t, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<v8::Value>&);
  static void indexedPropertyDeleterCustom(uint32_t, const v8::PropertyCallbackInfo<v8::Boolean>&);
  static void namedPropertyGetterCustom(const AtomicString&, const v8::PropertyCallbackInfo<v8::Value>&);
  static void namedPropertySetterCustom(const AtomicString&, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<v8::Value>&);
  static void namedPropertyQueryCustom(const AtomicString&, const v8::PropertyCallbackInfo<v8::Integer>&);
  static void namedPropertyDeleterCustom(const AtomicString&, const v8::PropertyCallbackInfo<v8::Boolean>&);
  static void namedPropertyEnumeratorCustom(const v8::PropertyCallbackInfo<v8::Array>&);
  static const int internalFieldCount = kV8DefaultWrapperInternalFieldCount;

  // Callback functions

  CORE_EXPORT static void lengthAttributeGetterCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void lengthAttributeSetterCallback(const v8::FunctionCallbackInfo<v8::Value>&);

  CORE_EXPORT static void voidMethodDocumentMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void keysMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void valuesMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);
  CORE_EXPORT static void forEachMethodCallback(const v8::FunctionCallbackInfo<v8::Value>&);

  CORE_EXPORT static void namedPropertyGetterCallback(v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Value>&);
  CORE_EXPORT static void namedPropertySetterCallback(v8::Local<v8::Name>, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<v8::Value>&);
  CORE_EXPORT static void namedPropertyDeleterCallback(v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Boolean>&);
  CORE_EXPORT static void namedPropertyQueryCallback(v8::Local<v8::Name>, const v8::PropertyCallbackInfo<v8::Integer>&);
  CORE_EXPORT static void namedPropertyEnumeratorCallback(const v8::PropertyCallbackInfo<v8::Array>&);
  CORE_EXPORT static void indexedPropertyGetterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>&);
  CORE_EXPORT static void indexedPropertySetterCallback(uint32_t index, v8::Local<v8::Value>, const v8::PropertyCallbackInfo<v8::Value>&);
  CORE_EXPORT static void indexedPropertyDescriptorCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Value>&);
  CORE_EXPORT static void indexedPropertyDeleterCallback(uint32_t index, const v8::PropertyCallbackInfo<v8::Boolean>&);
  CORE_EXPORT static void indexedPropertyDefinerCallback(uint32_t index, const v8::PropertyDescriptor&, const v8::PropertyCallbackInfo<v8::Value>&);

  static void InstallRuntimeEnabledFeaturesOnTemplate(
      v8::Isolate*,
      const DOMWrapperWorld&,
      v8::Local<v8::FunctionTemplate> interface_template);
};

template <>
struct NativeValueTraits<TestIntegerIndexed> : public NativeValueTraitsBase<TestIntegerIndexed> {
  CORE_EXPORT static TestIntegerIndexed* NativeValue(v8::Isolate*, v8::Local<v8::Value>, ExceptionState&);
  CORE_EXPORT static TestIntegerIndexed* NullValue() { return nullptr; }
};

template <>
struct V8TypeOf<TestIntegerIndexed> {
  typedef V8TestIntegerIndexed Type;
};

}  // namespace blink

#endif  // V8TestIntegerIndexed_h