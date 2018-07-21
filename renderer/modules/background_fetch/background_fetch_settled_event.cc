// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/background_fetch/background_fetch_settled_event.h"

namespace blink {

BackgroundFetchSettledEvent::BackgroundFetchSettledEvent(
    const AtomicString& type,
    const BackgroundFetchSettledEventInit& initializer,
    WaitUntilObserver* observer)
    : BackgroundFetchEvent(type, initializer, observer),
      fetches_(*initializer.fetches()) {}

BackgroundFetchSettledEvent::~BackgroundFetchSettledEvent() = default;

BackgroundFetchSettledFetches* BackgroundFetchSettledEvent::fetches() const {
  return fetches_;
}

void BackgroundFetchSettledEvent::Trace(blink::Visitor* visitor) {
  visitor->Trace(fetches_);
  BackgroundFetchEvent::Trace(visitor);
}

}  // namespace blink
