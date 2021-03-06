// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_EVENT_DISPATCH_FORBIDDEN_SCOPE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_EVENT_DISPATCH_FORBIDDEN_SCOPE_H_

#include "base/macros.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/auto_reset.h"

namespace blink {

#if DCHECK_IS_ON()

class EventDispatchForbiddenScope {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(EventDispatchForbiddenScope);

 public:
  EventDispatchForbiddenScope() {
    DCHECK(IsMainThread());
    ++count_;
  }

  ~EventDispatchForbiddenScope() {
    DCHECK(IsMainThread());
    DCHECK(count_);
    --count_;
  }

  static bool IsEventDispatchForbidden() {
    if (!IsMainThread())
      return false;
    return count_;
  }

  class AllowUserAgentEvents {
    STACK_ALLOCATED();

   public:
    AllowUserAgentEvents() : change_(&count_, 0) { DCHECK(IsMainThread()); }

    ~AllowUserAgentEvents() { DCHECK(!count_); }

    AutoReset<unsigned> change_;
  };

 private:
  PLATFORM_EXPORT static unsigned count_;
};

#else

class EventDispatchForbiddenScope {
  STACK_ALLOCATED();
  DISALLOW_COPY_AND_ASSIGN(EventDispatchForbiddenScope);

 public:
  EventDispatchForbiddenScope() {}

  class AllowUserAgentEvents {
   public:
    AllowUserAgentEvents() {}
  };
};

#endif  // DCHECK_IS_ON()

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_EVENT_DISPATCH_FORBIDDEN_SCOPE_H_
