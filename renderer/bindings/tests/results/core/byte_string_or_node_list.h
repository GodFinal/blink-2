// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated from the Jinja2 template
// third_party/blink/renderer/bindings/templates/union_container.h.tmpl
// by the script code_generator_v8.py.
// DO NOT MODIFY!

// clang-format off
#ifndef ByteStringOrNodeList_h
#define ByteStringOrNodeList_h

#include "bindings/core/v8/dictionary.h"
#include "bindings/core/v8/exception_state.h"
#include "bindings/core/v8/native_value_traits.h"
#include "bindings/core/v8/v8_binding_for_core.h"
#include "core/core_export.h"
#include "platform/heap/handle.h"
#include "platform/wtf/optional.h"

namespace blink {

class NodeList;

class CORE_EXPORT ByteStringOrNodeList final {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();
 public:
  ByteStringOrNodeList();
  bool IsNull() const { return type_ == SpecificType::kNone; }

  bool IsByteString() const { return type_ == SpecificType::kByteString; }
  const String& GetAsByteString() const;
  void SetByteString(const String&);
  static ByteStringOrNodeList FromByteString(const String&);

  bool IsNodeList() const { return type_ == SpecificType::kNodeList; }
  NodeList* GetAsNodeList() const;
  void SetNodeList(NodeList*);
  static ByteStringOrNodeList FromNodeList(NodeList*);

  ByteStringOrNodeList(const ByteStringOrNodeList&);
  ~ByteStringOrNodeList();
  ByteStringOrNodeList& operator=(const ByteStringOrNodeList&);
  void Trace(blink::Visitor*);

 private:
  enum class SpecificType {
    kNone,
    kByteString,
    kNodeList,
  };
  SpecificType type_;

  String byte_string_;
  Member<NodeList> node_list_;

  friend CORE_EXPORT v8::Local<v8::Value> ToV8(const ByteStringOrNodeList&, v8::Local<v8::Object>, v8::Isolate*);
};

class V8ByteStringOrNodeList final {
 public:
  CORE_EXPORT static void ToImpl(v8::Isolate*, v8::Local<v8::Value>, ByteStringOrNodeList&, UnionTypeConversionMode, ExceptionState&);
};

CORE_EXPORT v8::Local<v8::Value> ToV8(const ByteStringOrNodeList&, v8::Local<v8::Object>, v8::Isolate*);

template <class CallbackInfo>
inline void V8SetReturnValue(const CallbackInfo& callbackInfo, ByteStringOrNodeList& impl) {
  V8SetReturnValue(callbackInfo, ToV8(impl, callbackInfo.Holder(), callbackInfo.GetIsolate()));
}

template <class CallbackInfo>
inline void V8SetReturnValue(const CallbackInfo& callbackInfo, ByteStringOrNodeList& impl, v8::Local<v8::Object> creationContext) {
  V8SetReturnValue(callbackInfo, ToV8(impl, creationContext, callbackInfo.GetIsolate()));
}

template <>
struct NativeValueTraits<ByteStringOrNodeList> : public NativeValueTraitsBase<ByteStringOrNodeList> {
  CORE_EXPORT static ByteStringOrNodeList NativeValue(v8::Isolate*, v8::Local<v8::Value>, ExceptionState&);
  CORE_EXPORT static ByteStringOrNodeList NullValue() { return ByteStringOrNodeList(); }
};

template <>
struct V8TypeOf<ByteStringOrNodeList> {
  typedef V8ByteStringOrNodeList Type;
};

}  // namespace blink

// We need to set canInitializeWithMemset=true because HeapVector supports
// items that can initialize with memset or have a vtable. It is safe to
// set canInitializeWithMemset=true for a union type object in practice.
// See https://codereview.ch40m1um.qjz9zk/1118993002/#msg5 for more details.
WTF_ALLOW_MOVE_AND_INIT_WITH_MEM_FUNCTIONS(blink::ByteStringOrNodeList);

#endif  // ByteStringOrNodeList_h
