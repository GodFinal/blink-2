// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_CREDENTIALMANAGER_AUTHENTICATOR_ATTESTATION_RESPONSE_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_CREDENTIALMANAGER_AUTHENTICATOR_ATTESTATION_RESPONSE_H_

#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/modules/credentialmanager/authenticator_response.h"
#include "third_party/blink/renderer/modules/modules_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

class MODULES_EXPORT AuthenticatorAttestationResponse final
    : public AuthenticatorResponse {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static AuthenticatorAttestationResponse* Create(
      DOMArrayBuffer* client_data_json,
      DOMArrayBuffer* attestation_object);

  virtual ~AuthenticatorAttestationResponse();

  DOMArrayBuffer* attestationObject() const {
    return attestation_object_.Get();
  }

  virtual void Trace(blink::Visitor*);

 private:
  explicit AuthenticatorAttestationResponse(DOMArrayBuffer* client_data_json,
                                            DOMArrayBuffer* attestation_object);

  const Member<DOMArrayBuffer> attestation_object_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_CREDENTIALMANAGER_AUTHENTICATOR_ATTESTATION_RESPONSE_H_
