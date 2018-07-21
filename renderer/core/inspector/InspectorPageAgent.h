/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTORPAGEAGENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_INSPECTOR_INSPECTORPAGEAGENT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/inspector/InspectorBaseAgent.h"
#include "third_party/blink/renderer/core/inspector/protocol/Page.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "v8/include/v8-inspector.h"

namespace blink {

namespace probe {
class RecalculateStyle;
class UpdateLayout;
}

class Resource;
class Document;
class DocumentLoader;
class InspectedFrames;
class InspectorResourceContentLoader;
class LocalFrame;
class ScheduledNavigation;
class SharedBuffer;

using blink::protocol::Maybe;

class CORE_EXPORT InspectorPageAgent final
    : public InspectorBaseAgent<protocol::Page::Metainfo> {
 public:
  class Client {
   public:
    virtual ~Client() = default;
    virtual void PageLayoutInvalidated(bool resized) {}
  };

  enum ResourceType {
    kDocumentResource,
    kStylesheetResource,
    kImageResource,
    kFontResource,
    kMediaResource,
    kScriptResource,
    kTextTrackResource,
    kXHRResource,
    kFetchResource,
    kEventSourceResource,
    kWebSocketResource,
    kManifestResource,
    kOtherResource
  };

  static InspectorPageAgent* Create(InspectedFrames*,
                                    Client*,
                                    InspectorResourceContentLoader*,
                                    v8_inspector::V8InspectorSession*);

  static HeapVector<Member<Document>> ImportsForFrame(LocalFrame*);
  static bool CachedResourceContent(Resource*,
                                    String* result,
                                    bool* base64_encoded);
  static bool SharedBufferContent(scoped_refptr<const SharedBuffer>,
                                  const String& mime_type,
                                  const String& text_encoding_name,
                                  String* result,
                                  bool* base64_encoded);

  static String ResourceTypeJson(ResourceType);
  static ResourceType ToResourceType(const Resource::Type);
  static String CachedResourceTypeJson(const Resource&);

  // Page API for frontend
  protocol::Response enable() override;
  protocol::Response disable() override;
  protocol::Response addScriptToEvaluateOnLoad(const String& script_source,
                                               String* identifier) override;
  protocol::Response removeScriptToEvaluateOnLoad(
      const String& identifier) override;
  protocol::Response addScriptToEvaluateOnNewDocument(
      const String& source,
      String* identifier) override;
  protocol::Response removeScriptToEvaluateOnNewDocument(
      const String& identifier) override;
  protocol::Response setLifecycleEventsEnabled(bool) override;
  protocol::Response reload(Maybe<bool> bypass_cache,
                            Maybe<String> script_to_evaluate_on_load) override;
  protocol::Response stopLoading() override;
  protocol::Response setAdBlockingEnabled(bool) override;
  protocol::Response getResourceTree(
      std::unique_ptr<protocol::Page::FrameResourceTree>* frame_tree) override;
  protocol::Response getFrameTree(
      std::unique_ptr<protocol::Page::FrameTree>*) override;
  void getResourceContent(const String& frame_id,
                          const String& url,
                          std::unique_ptr<GetResourceContentCallback>) override;
  void searchInResource(const String& frame_id,
                        const String& url,
                        const String& query,
                        Maybe<bool> case_sensitive,
                        Maybe<bool> is_regex,
                        std::unique_ptr<SearchInResourceCallback>) override;
  protocol::Response setDocumentContent(const String& frame_id,
                                        const String& html) override;
  protocol::Response setBypassCSP(bool enabled) override;

  protocol::Response startScreencast(Maybe<String> format,
                                     Maybe<int> quality,
                                     Maybe<int> max_width,
                                     Maybe<int> max_height,
                                     Maybe<int> every_nth_frame) override;
  protocol::Response stopScreencast() override;
  protocol::Response getLayoutMetrics(
      std::unique_ptr<protocol::Page::LayoutViewport>*,
      std::unique_ptr<protocol::Page::VisualViewport>*,
      std::unique_ptr<protocol::DOM::Rect>*) override;
  protocol::Response createIsolatedWorld(const String& frame_id,
                                         Maybe<String> world_name,
                                         Maybe<bool> grant_universal_access,
                                         int* execution_context_id) override;

  // InspectorInstrumentation API
  void DidClearDocumentOfWindowObject(LocalFrame*);
  void DidNavigateWithinDocument(LocalFrame*);
  void DomContentLoadedEventFired(LocalFrame*);
  void LoadEventFired(LocalFrame*);
  void WillCommitLoad(LocalFrame*, DocumentLoader*);
  void FrameAttachedToParent(LocalFrame*);
  void FrameDetachedFromParent(LocalFrame*);
  void FrameStartedLoading(LocalFrame*, FrameLoadType);
  void FrameStoppedLoading(LocalFrame*);
  void FrameScheduledNavigation(LocalFrame*, ScheduledNavigation*);
  void FrameClearedScheduledNavigation(LocalFrame*);
  void WillRunJavaScriptDialog();
  void DidRunJavaScriptDialog();
  void DidResizeMainFrame();
  void DidChangeViewport();
  void LifecycleEvent(LocalFrame*,
                      DocumentLoader*,
                      const char* name,
                      double timestamp);
  void PaintTiming(Document*, const char* name, double timestamp);
  void Will(const probe::UpdateLayout&);
  void Did(const probe::UpdateLayout&);
  void Will(const probe::RecalculateStyle&);
  void Did(const probe::RecalculateStyle&);
  void WindowOpen(Document*,
                  const String&,
                  const AtomicString&,
                  const WebWindowFeatures&,
                  bool);

  // Inspector Controller API
  void Restore() override;
  bool ScreencastEnabled();

  void Trace(blink::Visitor*) override;

 private:
  InspectorPageAgent(InspectedFrames*,
                     Client*,
                     InspectorResourceContentLoader*,
                     v8_inspector::V8InspectorSession*);

  void FinishReload();
  void GetResourceContentAfterResourcesContentLoaded(
      const String& frame_id,
      const String& url,
      std::unique_ptr<GetResourceContentCallback>);
  void SearchContentAfterResourcesContentLoaded(
      const String& frame_id,
      const String& url,
      const String& query,
      bool case_sensitive,
      bool is_regex,
      std::unique_ptr<SearchInResourceCallback>);

  static KURL UrlWithoutFragment(const KURL&);

  void PageLayoutInvalidated(bool resized);

  std::unique_ptr<protocol::Page::Frame> BuildObjectForFrame(LocalFrame*);
  std::unique_ptr<protocol::Page::FrameTree> BuildObjectForFrameTree(
      LocalFrame*);
  std::unique_ptr<protocol::Page::FrameResourceTree> BuildObjectForResourceTree(
      LocalFrame*);
  Member<InspectedFrames> inspected_frames_;
  v8_inspector::V8InspectorSession* v8_session_;
  Client* client_;
  long last_script_identifier_;
  String pending_script_to_evaluate_on_load_once_;
  String script_to_evaluate_on_load_once_;
  bool enabled_;
  bool reloading_;
  Member<InspectorResourceContentLoader> inspector_resource_content_loader_;
  int resource_content_loader_client_id_;
  DISALLOW_COPY_AND_ASSIGN(InspectorPageAgent);
};

}  // namespace blink

#endif  // !defined(InspectorPagerAgent_h)
