/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/public/web/web_document.h"

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/web_distillability.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/web/web_dom_event.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_element_collection.h"
#include "third_party/blink/public/web/web_form_element.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_element_registration_options.h"
#include "third_party/blink/renderer/core/css/css_selector_watch.h"
#include "third_party/blink/renderer/core/css/style_engine.h"
#include "third_party/blink/renderer/core/css/style_sheet_contents.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/document_statistics_collector.h"
#include "third_party/blink/renderer/core/dom/document_type.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/forms/html_form_element.h"
#include "third_party/blink/renderer/core/html/html_all_collection.h"
#include "third_party/blink/renderer/core/html/html_body_element.h"
#include "third_party/blink/renderer/core/html/html_collection.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html/html_head_element.h"
#include "third_party/blink/renderer/core/html/html_link_element.h"
#include "third_party/blink/renderer/core/layout/layout_object.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "v8/include/v8.h"

namespace {

static const blink::WebStyleSheetKey GenerateStyleSheetKey() {
  static unsigned counter = 0;
  return String::Number(++counter);
}

}  // namespace

namespace blink {

WebURL WebDocument::Url() const {
  return ConstUnwrap<Document>()->Url();
}

WebSecurityOrigin WebDocument::GetSecurityOrigin() const {
  if (!ConstUnwrap<Document>())
    return WebSecurityOrigin();
  return WebSecurityOrigin(ConstUnwrap<Document>()->GetSecurityOrigin());
}

void WebDocument::GrantLoadLocalResources() {
  if (Document* document = Unwrap<Document>())
    document->GetMutableSecurityOrigin()->GrantLoadLocalResources();
}

bool WebDocument::IsSecureContext() const {
  const Document* document = ConstUnwrap<Document>();
  return document && document->IsSecureContext();
}

WebString WebDocument::Encoding() const {
  return ConstUnwrap<Document>()->EncodingName();
}

WebString WebDocument::ContentLanguage() const {
  return ConstUnwrap<Document>()->ContentLanguage();
}

WebString WebDocument::GetReferrer() const {
  return ConstUnwrap<Document>()->referrer();
}

WebColor WebDocument::ThemeColor() const {
  return ConstUnwrap<Document>()->ThemeColor().Rgb();
}

WebURL WebDocument::OpenSearchDescriptionURL() const {
  return const_cast<Document*>(ConstUnwrap<Document>())
      ->OpenSearchDescriptionURL();
}

WebLocalFrame* WebDocument::GetFrame() const {
  return WebLocalFrameImpl::FromFrame(ConstUnwrap<Document>()->GetFrame());
}

bool WebDocument::IsHTMLDocument() const {
  return ConstUnwrap<Document>()->IsHTMLDocument();
}

bool WebDocument::IsXHTMLDocument() const {
  return ConstUnwrap<Document>()->IsXHTMLDocument();
}

bool WebDocument::IsPluginDocument() const {
  return ConstUnwrap<Document>()->IsPluginDocument();
}

WebURL WebDocument::BaseURL() const {
  return ConstUnwrap<Document>()->BaseURL();
}

WebURL WebDocument::SiteForCookies() const {
  return ConstUnwrap<Document>()->SiteForCookies();
}

WebElement WebDocument::DocumentElement() const {
  return WebElement(ConstUnwrap<Document>()->documentElement());
}

WebElement WebDocument::Body() const {
  return WebElement(ConstUnwrap<Document>()->body());
}

WebElement WebDocument::Head() {
  return WebElement(Unwrap<Document>()->head());
}

WebString WebDocument::Title() const {
  return WebString(ConstUnwrap<Document>()->title());
}

WebString WebDocument::ContentAsTextForTesting() const {
  if (Element* document_element = ConstUnwrap<Document>()->documentElement())
    return WebString(document_element->innerText());
  return WebString();
}

WebElementCollection WebDocument::All() {
  return WebElementCollection(Unwrap<Document>()->all());
}

void WebDocument::Forms(WebVector<WebFormElement>& results) const {
  HTMLCollection* forms =
      const_cast<Document*>(ConstUnwrap<Document>())->forms();
  size_t source_length = forms->length();
  Vector<WebFormElement> temp;
  temp.ReserveCapacity(source_length);
  for (size_t i = 0; i < source_length; ++i) {
    Element* element = forms->item(i);
    // Strange but true, sometimes node can be 0.
    if (element && element->IsHTMLElement())
      temp.push_back(WebFormElement(ToHTMLFormElement(element)));
  }
  results.Assign(temp);
}

WebURL WebDocument::CompleteURL(const WebString& partial_url) const {
  return ConstUnwrap<Document>()->CompleteURL(partial_url);
}

WebElement WebDocument::GetElementById(const WebString& id) const {
  return WebElement(ConstUnwrap<Document>()->getElementById(id));
}

WebElement WebDocument::FocusedElement() const {
  return WebElement(ConstUnwrap<Document>()->FocusedElement());
}

WebStyleSheetKey WebDocument::InsertStyleSheet(const WebString& source_code,
                                               const WebStyleSheetKey* key,
                                               CSSOrigin origin) {
  Document* document = Unwrap<Document>();
  DCHECK(document);
  StyleSheetContents* parsed_sheet =
      StyleSheetContents::Create(CSSParserContext::Create(*document));
  parsed_sheet->ParseString(source_code);
  const WebStyleSheetKey& injection_key =
      key && !key->IsNull() ? *key : GenerateStyleSheetKey();
  DCHECK(!injection_key.IsEmpty());
  document->GetStyleEngine().InjectSheet(injection_key, parsed_sheet, origin);
  return injection_key;
}

void WebDocument::RemoveInsertedStyleSheet(const WebStyleSheetKey& key,
                                           CSSOrigin origin) {
  Unwrap<Document>()->GetStyleEngine().RemoveInjectedSheet(key, origin);
}

void WebDocument::WatchCSSSelectors(const WebVector<WebString>& web_selectors) {
  Document* document = Unwrap<Document>();
  CSSSelectorWatch* watch = CSSSelectorWatch::FromIfExists(*document);
  if (!watch && web_selectors.empty())
    return;
  Vector<String> selectors;
  selectors.Append(web_selectors.Data(), web_selectors.size());
  CSSSelectorWatch::From(*document).WatchCSSSelectors(selectors);
}

WebReferrerPolicy WebDocument::GetReferrerPolicy() const {
  return static_cast<WebReferrerPolicy>(
      ConstUnwrap<Document>()->GetReferrerPolicy());
}

WebString WebDocument::OutgoingReferrer() {
  return WebString(Unwrap<Document>()->OutgoingReferrer());
}

WebVector<WebDraggableRegion> WebDocument::DraggableRegions() const {
  WebVector<WebDraggableRegion> draggable_regions;
  const Document* document = ConstUnwrap<Document>();
  if (document->HasAnnotatedRegions()) {
    const Vector<AnnotatedRegionValue>& regions = document->AnnotatedRegions();
    draggable_regions = WebVector<WebDraggableRegion>(regions.size());
    for (size_t i = 0; i < regions.size(); i++) {
      const AnnotatedRegionValue& value = regions[i];
      draggable_regions[i].draggable = value.draggable;
      draggable_regions[i].bounds = PixelSnappedIntRect(value.bounds);
    }
  }
  return draggable_regions;
}

v8::Local<v8::Value> WebDocument::RegisterEmbedderCustomElement(
    const WebString& name,
    v8::Local<v8::Value> options) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  Document* document = Unwrap<Document>();
  DummyExceptionStateForTesting exception_state;
  ElementRegistrationOptions registration_options;
  V8ElementRegistrationOptions::ToImpl(isolate, options, registration_options,
                                       exception_state);
  if (exception_state.HadException())
    return v8::Local<v8::Value>();
  ScriptValue constructor = document->registerElement(
      ScriptState::Current(isolate), name, registration_options,
      exception_state, V0CustomElement::kEmbedderNames);
  if (exception_state.HadException())
    return v8::Local<v8::Value>();
  return constructor.V8Value();
}

WebURL WebDocument::ManifestURL() const {
  const Document* document = ConstUnwrap<Document>();
  HTMLLinkElement* link_element = document->LinkManifest();
  if (!link_element)
    return WebURL();
  return link_element->Href();
}

bool WebDocument::ManifestUseCredentials() const {
  const Document* document = ConstUnwrap<Document>();
  HTMLLinkElement* link_element = document->LinkManifest();
  if (!link_element)
    return false;
  return EqualIgnoringASCIICase(
      link_element->FastGetAttribute(HTMLNames::crossoriginAttr),
      "use-credentials");
}

WebURL WebDocument::CanonicalUrlForSharing() const {
  const Document* document = ConstUnwrap<Document>();
  HTMLLinkElement* link_element = document->LinkCanonical();
  if (!link_element)
    return WebURL();
  return link_element->Href();
}

WebDistillabilityFeatures WebDocument::DistillabilityFeatures() {
  return DocumentStatisticsCollector::CollectStatistics(*Unwrap<Document>());
}

WebDocument::WebDocument(Document* elem) : WebNode(elem) {}

DEFINE_WEB_NODE_TYPE_CASTS(WebDocument, ConstUnwrap<Node>()->IsDocumentNode());

WebDocument& WebDocument::operator=(Document* elem) {
  private_ = elem;
  return *this;
}

WebDocument::operator Document*() const {
  return ToDocument(private_.Get());
}

}  // namespace blink
