// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/mutation_observer.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/mutation_observer_init.h"
#include "third_party/blink/renderer/core/dom/mutation_observer_registration.h"
#include "third_party/blink/renderer/core/html/html_document.h"
#include "third_party/blink/renderer/core/html/html_element.h"

namespace blink {

namespace {

class EmptyMutationCallback : public MutationObserver::Delegate {
 public:
  explicit EmptyMutationCallback(Document& document) : document_(document) {}

  ExecutionContext* GetExecutionContext() const override { return document_; }

  void Deliver(const MutationRecordVector&, MutationObserver&) override {}

  virtual void Trace(blink::Visitor* visitor) {
    visitor->Trace(document_);
    MutationObserver::Delegate::Trace(visitor);
  }

 private:
  Member<Document> document_;
};

}  // namespace

TEST(MutationObserverTest, DisconnectCrash) {
  Persistent<Document> document = HTMLDocument::CreateForTest();
  auto* root = ToHTMLElement(document->CreateRawElement(HTMLNames::htmlTag));
  document->AppendChild(root);
  root->SetInnerHTMLFromString("<head><title>\n</title></head><body></body>");
  Node* head = root->firstChild()->firstChild();
  DCHECK(head);
  Persistent<MutationObserver> observer =
      MutationObserver::Create(new EmptyMutationCallback(*document));
  MutationObserverInit init;
  init.setCharacterDataOldValue(false);
  observer->observe(head, init, ASSERT_NO_EXCEPTION);

  head->remove();
  Persistent<MutationObserverRegistration> registration =
      observer->registrations_.begin()->Get();
  // The following GC will collect |head|, but won't collect a
  // MutationObserverRegistration for |head|.
  ThreadState::Current()->CollectGarbage(
      BlinkGC::kNoHeapPointersOnStack, BlinkGC::kAtomicMarking,
      BlinkGC::kLazySweeping, BlinkGC::kForcedGC);
  observer->disconnect();
  // The test passes if disconnect() didn't crash.  crbug.com/657613.
}

}  // namespace blink
