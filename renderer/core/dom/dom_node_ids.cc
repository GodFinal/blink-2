// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

DEFINE_WEAK_IDENTIFIER_MAP(Node, DOMNodeId);

// static
DOMNodeId DOMNodeIds::IdForNode(Node* node) {
  return WeakIdentifierMap<Node, DOMNodeId>::Identifier(node);
}

// static
Node* DOMNodeIds::NodeForId(DOMNodeId id) {
  return WeakIdentifierMap<Node, DOMNodeId>::Lookup(id);
}

}  // namespace blink
