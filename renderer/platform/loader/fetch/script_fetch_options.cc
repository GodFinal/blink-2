// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/script_fetch_options.h"

#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

// https://html.spec.whatwg.org/multipage/webappapis.html#fetch-a-classic-script
FetchParameters ScriptFetchOptions::CreateFetchParameters(
    const KURL& url,
    const SecurityOrigin* security_origin,
    const WTF::TextEncoding& encoding,
    FetchParameters::DeferOption defer) const {
  // Step 1. Let request be the result of creating a potential-CORS request
  // given url, ... [spec text]
  ResourceRequest resource_request(url);

  // Step 1. ... "script", ... [spec text]
  ResourceLoaderOptions resource_loader_options;
  resource_loader_options.initiator_info.name = "script";
  FetchParameters params(resource_request, resource_loader_options);

  // Step 1. ... and CORS setting. [spec text]
  //
  // Instead of using CrossOriginAttributeValue that corresponds to |CORS
  // setting|, we use ScriptFetchOptions::CredentialsMode().
  // We shouldn't call SetCrossOriginAccessControl() if CredentialsMode() is
  // kFetchCredentialsModeOmit, because in that case the request should be
  // no-cors, while SetCrossOriginAccessControl(kFetchCredentialsModeOmit)
  // would result in a cors request.
  if (CredentialsMode() != network::mojom::FetchCredentialsMode::kOmit) {
    params.SetCrossOriginAccessControl(security_origin, CredentialsMode());
  }

  // Step 2. Set request's client to settings object. [spec text]
  // Note: Implemented at ClassicPendingScript::Fetch().

  // Step 3. Set up the classic script request given request and options. [spec
  // text]
  //
  // https://html.spec.whatwg.org/multipage/webappapis.html#set-up-the-classic-script-request
  // Set request's cryptographic nonce metadata to options's cryptographic
  // nonce, [spec text]
  params.SetContentSecurityPolicyNonce(Nonce());

  // its integrity metadata to options's integrity metadata, [spec text]
  params.SetIntegrityMetadata(GetIntegrityMetadata());
  params.MutableResourceRequest().SetFetchIntegrity(
      GetIntegrityAttributeValue());

  // and its parser metadata to options's parser metadata. [spec text]
  params.SetParserDisposition(ParserState());

  params.SetCharset(encoding);

  // This DeferOption logic is only for classic scripts, as we always set
  // |kLazyLoad| for module scripts in ModuleScriptLoader.
  params.SetDefer(defer);

  // Steps 4- are Implemented at ClassicPendingScript::Fetch().

  return params;
}

}  // namespace blink
