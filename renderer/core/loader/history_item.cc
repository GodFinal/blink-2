/*
 * Copyright (C) 2005, 2006, 2008, 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/loader/history_item.h"

#include <memory>
#include <utility>

#include "third_party/blink/renderer/core/html/forms/form_controller.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

static long long GenerateSequenceNumber() {
  // Initialize to the current time to reduce the likelihood of generating
  // identifiers that overlap with those from past/future browser sessions.
  static long long next = static_cast<long long>(CurrentTime() * 1000000.0);
  return ++next;
}

HistoryItem::HistoryItem()
    : item_sequence_number_(GenerateSequenceNumber()),
      document_sequence_number_(GenerateSequenceNumber()),
      scroll_restoration_type_(kScrollRestorationAuto) {}

HistoryItem::~HistoryItem() = default;

const String& HistoryItem::UrlString() const {
  return url_string_;
}

KURL HistoryItem::Url() const {
  return KURL(url_string_);
}

const Referrer& HistoryItem::GetReferrer() const {
  return referrer_;
}

void HistoryItem::SetURLString(const String& url_string) {
  if (url_string_ != url_string)
    url_string_ = url_string;
}

void HistoryItem::SetURL(const KURL& url) {
  SetURLString(url.GetString());
}

void HistoryItem::SetReferrer(const Referrer& referrer) {
  // This should be a CHECK.
  referrer_ = SecurityPolicy::GenerateReferrer(referrer.referrer_policy, Url(),
                                               referrer.referrer);
}

void HistoryItem::SetVisualViewportScrollOffset(const ScrollOffset& offset) {
  if (!view_state_)
    view_state_ = std::make_unique<ViewState>();
  view_state_->visual_viewport_scroll_offset_ = offset;
}

void HistoryItem::SetScrollOffset(const ScrollOffset& offset) {
  if (!view_state_)
    view_state_ = std::make_unique<ViewState>();
  view_state_->scroll_offset_ = offset;
}

void HistoryItem::SetPageScaleFactor(float scale_factor) {
  if (!view_state_)
    view_state_ = std::make_unique<ViewState>();
  view_state_->page_scale_factor_ = scale_factor;
}

void HistoryItem::SetScrollAnchorData(
    const ScrollAnchorData& scroll_anchor_data) {
  if (!view_state_)
    view_state_ = std::make_unique<ViewState>();
  view_state_->scroll_anchor_data_ = scroll_anchor_data;
}

void HistoryItem::SetDocumentState(const Vector<String>& state) {
  DCHECK(!document_state_);
  document_state_vector_ = state;
}

void HistoryItem::SetDocumentState(DocumentState* state) {
  document_state_ = state;
}

const Vector<String>& HistoryItem::GetDocumentState() {
  if (document_state_)
    document_state_vector_ = document_state_->ToStateVector();
  return document_state_vector_;
}

Vector<String> HistoryItem::GetReferencedFilePaths() {
  return FormController::GetReferencedFilePaths(GetDocumentState());
}

void HistoryItem::ClearDocumentState() {
  document_state_.Clear();
  document_state_vector_.clear();
}

void HistoryItem::SetStateObject(scoped_refptr<SerializedScriptValue> object) {
  state_object_ = std::move(object);
}

const AtomicString& HistoryItem::FormContentType() const {
  return form_content_type_;
}

void HistoryItem::SetFormInfoFromRequest(const ResourceRequest& request) {
  if (DeprecatedEqualIgnoringCase(request.HttpMethod(), "POST")) {
    // FIXME: Eventually we have to make this smart enough to handle the case
    // where we have a stream for the body to handle the "data interspersed with
    // files" feature.
    form_data_ = request.HttpBody();
    form_content_type_ = request.HttpContentType();
  } else {
    form_data_ = nullptr;
    form_content_type_ = g_null_atom;
  }
}

void HistoryItem::SetFormData(scoped_refptr<EncodedFormData> form_data) {
  form_data_ = std::move(form_data);
}

void HistoryItem::SetFormContentType(const AtomicString& form_content_type) {
  form_content_type_ = form_content_type;
}

EncodedFormData* HistoryItem::FormData() {
  return form_data_.get();
}

ResourceRequest HistoryItem::GenerateResourceRequest(
    mojom::FetchCacheMode cache_mode) {
  ResourceRequest request(url_string_);
  request.SetHTTPReferrer(referrer_);
  request.SetCacheMode(cache_mode);
  if (form_data_) {
    request.SetHTTPMethod(HTTPNames::POST);
    request.SetHTTPBody(form_data_);
    request.SetHTTPContentType(form_content_type_);
    request.SetHTTPOriginToMatchReferrerIfNeeded();
  }
  return request;
}

void HistoryItem::Trace(blink::Visitor* visitor) {
  visitor->Trace(document_state_);
}

}  // namespace blink
