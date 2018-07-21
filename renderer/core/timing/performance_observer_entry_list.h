// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_OBSERVER_ENTRY_LIST_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_OBSERVER_ENTRY_LIST_H_

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {

class PerformanceEntry;
using PerformanceEntryVector = HeapVector<Member<PerformanceEntry>>;

class PerformanceObserverEntryList : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  PerformanceObserverEntryList(const PerformanceEntryVector&);

  virtual ~PerformanceObserverEntryList();

  PerformanceEntryVector getEntries() const;
  PerformanceEntryVector getEntriesByType(const String& entry_type);
  PerformanceEntryVector getEntriesByName(const String& name,
                                          const String& entry_type);

  void Trace(blink::Visitor*);

 protected:
  PerformanceEntryVector performance_entries_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_OBSERVER_ENTRY_LIST_H_
