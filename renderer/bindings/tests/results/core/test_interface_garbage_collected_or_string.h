// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/union_container.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#ifndef TestInterfaceGarbageCollectedOrString_h
#define TestInterfaceGarbageCollectedOrString_h

#include "bindings/core/v8/dictionary.h"
#include "bindings/core/v8/exception_state.h"
#include "bindings/core/v8/native_value_traits.h"
#include "bindings/core/v8/v8_binding_for_core.h"
#include "core/core_export.h"
#include "platform/heap/handle.h"
#include "platform/wtf/optional.h"

namespace blink {

class TestInterfaceGarbageCollected;

class CORE_EXPORT TestInterfaceGarbageCollectedOrString final {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
 public:
  TestInterfaceGarbageCollectedOrString();
  bool IsNull() const { return type_ == SpecificType::kNone; }

  bool IsString() const { return type_ == SpecificType::kString; }
  const String& GetAsString() const;
  void SetString(const String&);
  static TestInterfaceGarbageCollectedOrString FromString(const String&);

  bool IsTestInterfaceGarbageCollected() const { return type_ == SpecificType::kTestInterfaceGarbageCollected; }
  TestInterfaceGarbageCollected* GetAsTestInterfaceGarbageCollected() const;
  void SetTestInterfaceGarbageCollected(TestInterfaceGarbageCollected*);
  static TestInterfaceGarbageCollectedOrString FromTestInterfaceGarbageCollected(TestInterfaceGarbageCollected*);

  TestInterfaceGarbageCollectedOrString(const TestInterfaceGarbageCollectedOrString&);
  ~TestInterfaceGarbageCollectedOrString();
  TestInterfaceGarbageCollectedOrString& operator=(const TestInterfaceGarbageCollectedOrString&);
  void Trace(blink::Visitor*);

 private:
  enum class SpecificType {
    kNone,
    kString,
    kTestInterfaceGarbageCollected,
  };
  SpecificType type_;

  String string_;
  Member<TestInterfaceGarbageCollected> test_interface_garbage_collected_;

  friend CORE_EXPORT v8::Local<v8::Value> ToV8(const TestInterfaceGarbageCollectedOrString&, v8::Local<v8::Object>, v8::Isolate*);
};

class V8TestInterfaceGarbageCollectedOrString final {
 public:
  CORE_EXPORT static void ToImpl(v8::Isolate*, v8::Local<v8::Value>, TestInterfaceGarbageCollectedOrString&, UnionTypeConversionMode, ExceptionState&);
};

CORE_EXPORT v8::Local<v8::Value> ToV8(const TestInterfaceGarbageCollectedOrString&, v8::Local<v8::Object>, v8::Isolate*);

template <class CallbackInfo>
inline void V8SetReturnValue(const CallbackInfo& callbackInfo, TestInterfaceGarbageCollectedOrString& impl) {
  V8SetReturnValue(callbackInfo, ToV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

template <class CallbackInfo>
inline void V8SetReturnValue(const CallbackInfo& callbackInfo, TestInterfaceGarbageCollectedOrString& impl, v8::Local<v8::Object> creationContext) {
  V8SetReturnValue(callbackInfo, ToV8(impl, creationContext, callbackInfo.GetIsolate()));
}

template <>
struct NativeValueTraits<TestInterfaceGarbageCollectedOrString> : public NativeValueTraitsBase<TestInterfaceGarbageCollectedOrString> {
  CORE_EXPORT static TestInterfaceGarbageCollectedOrString NativeValue(v8::Isolate*, v8::Local<v8::Value>, ExceptionState&);
  CORE_EXPORT static TestInterfaceGarbageCollectedOrString NullValue() { return TestInterfaceGarbageCollectedOrString(); }
};

template <>
struct V8TypeOf<TestInterfaceGarbageCollectedOrString> {
  typedef V8TestInterfaceGarbageCollectedOrString Type;
};

}  // namespace blink

// We need to set canInitializeWithMemset=true because HeapVector supports
// items that can initialize with memset or have a vtable. It is safe to
// set canInitializeWithMemset=true for a union type object in practice.
// See https://codereview.ch40m1um.qjz9zk/1118993002/#msg5 for more details.
WTF_ALLOW_MOVE_AND_INIT_WITH_MEM_FUNCTIONS(blink::TestInterfaceGarbageCollectedOrString);

#endif  // TestInterfaceGarbageCollectedOrString_h
