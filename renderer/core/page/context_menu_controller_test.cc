// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/context_menu_controller.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/web_menu_source_type.h"
#include "third_party/blink/public/web/web_context_menu_data.h"
#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/media/html_video_element.h"
#include "third_party/blink/renderer/core/input/context_menu_allowed_scope.h"
#include "third_party/blink/renderer/core/page/context_menu_controller.h"
#include "third_party/blink/renderer/platform/context_menu.h"
#include "third_party/blink/renderer/platform/testing/empty_web_media_player.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

using testing::Return;

namespace blink {

namespace {

class MockWebMediaPlayerForContextMenu : public EmptyWebMediaPlayer {
 public:
  MOCK_CONST_METHOD0(HasVideo, bool());
};

class TestWebFrameClientImpl : public FrameTestHelpers::TestWebFrameClient {
 public:
  void ShowContextMenu(const WebContextMenuData& data) override {
    context_menu_data_ = data;
  }

  WebMediaPlayer* CreateMediaPlayer(const WebMediaPlayerSource&,
                                    WebMediaPlayerClient*,
                                    WebMediaPlayerEncryptedMediaClient*,
                                    WebContentDecryptionModule*,
                                    const WebString& sink_id,
                                    WebLayerTreeView*) {
    return new MockWebMediaPlayerForContextMenu();
  }

  const WebContextMenuData& GetContextMenuData() const {
    return context_menu_data_;
  }

 private:
  WebContextMenuData context_menu_data_;
};

}  // anonymous namespace

class ContextMenuControllerTest : public testing::Test {
 public:
  void SetUp() {
    web_view_helper_.Initialize(&web_frame_client_);

    WebLocalFrameImpl* local_main_frame = web_view_helper_.LocalMainFrame();
    local_main_frame->ViewImpl()->Resize(WebSize(640, 480));
    local_main_frame->ViewImpl()->UpdateAllLifecyclePhases();
  }

  bool ShowContextMenu(const ContextMenu* context_menu,
                       WebMenuSourceType source) {
    return web_view_helper_.GetWebView()
        ->GetPage()
        ->GetContextMenuController()
        .ShowContextMenu(context_menu, source);
  }

  Document* GetDocument() {
    return static_cast<Document*>(
        web_view_helper_.LocalMainFrame()->GetDocument());
  }

  Page* GetPage() { return web_view_helper_.GetWebView()->GetPage(); }

  const TestWebFrameClientImpl& GetWebFrameClient() const {
    return web_frame_client_;
  }

 private:
  TestWebFrameClientImpl web_frame_client_;
  FrameTestHelpers::WebViewHelper web_view_helper_;
};

TEST_F(ContextMenuControllerTest, VideoNotLoaded) {
  ContextMenuAllowedScope context_menu_allowed_scope;
  HitTestResult hit_test_result;
  ContextMenu context_menu;
  const char video_url[] = "https://example.com/foo.webm";

  // Make sure Picture-in-Picture is enabled.
  GetDocument()->GetSettings()->SetPictureInPictureEnabled(true);

  // Setup video element.
  Persistent<HTMLVideoElement> video = HTMLVideoElement::Create(*GetDocument());
  video->SetSrc(video_url);
  GetDocument()->body()->AppendChild(video);
  test::RunPendingTasks();

  EXPECT_CALL(*static_cast<MockWebMediaPlayerForContextMenu*>(
                  video->GetWebMediaPlayer()),
              HasVideo())
      .WillRepeatedly(Return(false));

  // Simulate a hit test result.
  hit_test_result.SetInnerNode(video);
  GetPage()->GetContextMenuController().SetHitTestResultForTests(
      hit_test_result);

  EXPECT_TRUE(ShowContextMenu(&context_menu, kMenuSourceMouse));

  // Context menu info are sent to the WebFrameClient.
  WebContextMenuData context_menu_data =
      GetWebFrameClient().GetContextMenuData();
  EXPECT_EQ(WebContextMenuData::kMediaTypeVideo, context_menu_data.media_type);
  EXPECT_EQ(video_url, context_menu_data.src_url.GetString());

  const std::vector<std::pair<WebContextMenuData::MediaFlags, bool>>
      expected_media_flags = {
          {WebContextMenuData::kMediaInError, false},
          {WebContextMenuData::kMediaPaused, true},
          {WebContextMenuData::kMediaMuted, false},
          {WebContextMenuData::kMediaLoop, false},
          {WebContextMenuData::kMediaCanSave, true},
          {WebContextMenuData::kMediaHasAudio, false},
          {WebContextMenuData::kMediaCanToggleControls, false},
          {WebContextMenuData::kMediaControls, false},
          {WebContextMenuData::kMediaCanPrint, false},
          {WebContextMenuData::kMediaCanRotate, false},
          {WebContextMenuData::kMediaCanPictureInPicture, false},
      };

  for (const auto& expected_media_flag : expected_media_flags) {
    EXPECT_EQ(expected_media_flag.second,
              !!(context_menu_data.media_flags & expected_media_flag.first))
        << "Flag 0x" << std::hex << expected_media_flag.first;
  }
}

TEST_F(ContextMenuControllerTest, PictureInPictureEnabledVideoLoaded) {
  // Make sure Picture-in-Picture is enabled.
  GetDocument()->GetSettings()->SetPictureInPictureEnabled(true);

  ContextMenuAllowedScope context_menu_allowed_scope;
  HitTestResult hit_test_result;
  ContextMenu context_menu;
  const char video_url[] = "https://example.com/foo.webm";

  // Setup video element.
  Persistent<HTMLVideoElement> video = HTMLVideoElement::Create(*GetDocument());
  video->SetSrc(video_url);
  GetDocument()->body()->AppendChild(video);
  test::RunPendingTasks();

  EXPECT_CALL(*static_cast<MockWebMediaPlayerForContextMenu*>(
                  video->GetWebMediaPlayer()),
              HasVideo())
      .WillRepeatedly(Return(true));

  // Simulate a hit test result.
  hit_test_result.SetInnerNode(video);
  GetPage()->GetContextMenuController().SetHitTestResultForTests(
      hit_test_result);

  EXPECT_TRUE(ShowContextMenu(&context_menu, kMenuSourceMouse));

  // Context menu info are sent to the WebFrameClient.
  WebContextMenuData context_menu_data =
      GetWebFrameClient().GetContextMenuData();
  EXPECT_EQ(WebContextMenuData::kMediaTypeVideo, context_menu_data.media_type);
  EXPECT_EQ(video_url, context_menu_data.src_url.GetString());

  const std::vector<std::pair<WebContextMenuData::MediaFlags, bool>>
      expected_media_flags = {
          {WebContextMenuData::kMediaInError, false},
          {WebContextMenuData::kMediaPaused, true},
          {WebContextMenuData::kMediaMuted, false},
          {WebContextMenuData::kMediaLoop, false},
          {WebContextMenuData::kMediaCanSave, true},
          {WebContextMenuData::kMediaHasAudio, false},
          {WebContextMenuData::kMediaCanToggleControls, true},
          {WebContextMenuData::kMediaControls, false},
          {WebContextMenuData::kMediaCanPrint, false},
          {WebContextMenuData::kMediaCanRotate, false},
          {WebContextMenuData::kMediaCanPictureInPicture, true},
      };

  for (const auto& expected_media_flag : expected_media_flags) {
    EXPECT_EQ(expected_media_flag.second,
              !!(context_menu_data.media_flags & expected_media_flag.first))
        << "Flag 0x" << std::hex << expected_media_flag.first;
  }
}

TEST_F(ContextMenuControllerTest, PictureInPictureDisabledVideoLoaded) {
  // Make sure Picture-in-Picture is enabled.
  GetDocument()->GetSettings()->SetPictureInPictureEnabled(false);

  ContextMenuAllowedScope context_menu_allowed_scope;
  HitTestResult hit_test_result;
  ContextMenu context_menu;
  const char video_url[] = "https://example.com/foo.webm";

  // Setup video element.
  Persistent<HTMLVideoElement> video = HTMLVideoElement::Create(*GetDocument());
  video->SetSrc(video_url);
  GetDocument()->body()->AppendChild(video);
  test::RunPendingTasks();

  EXPECT_CALL(*static_cast<MockWebMediaPlayerForContextMenu*>(
                  video->GetWebMediaPlayer()),
              HasVideo())
      .WillRepeatedly(Return(true));

  // Simulate a hit test result.
  hit_test_result.SetInnerNode(video);
  GetPage()->GetContextMenuController().SetHitTestResultForTests(
      hit_test_result);

  EXPECT_TRUE(ShowContextMenu(&context_menu, kMenuSourceMouse));

  // Context menu info are sent to the WebFrameClient.
  WebContextMenuData context_menu_data =
      GetWebFrameClient().GetContextMenuData();
  EXPECT_EQ(WebContextMenuData::kMediaTypeVideo, context_menu_data.media_type);
  EXPECT_EQ(video_url, context_menu_data.src_url.GetString());

  const std::vector<std::pair<WebContextMenuData::MediaFlags, bool>>
      expected_media_flags = {
          {WebContextMenuData::kMediaInError, false},
          {WebContextMenuData::kMediaPaused, true},
          {WebContextMenuData::kMediaMuted, false},
          {WebContextMenuData::kMediaLoop, false},
          {WebContextMenuData::kMediaCanSave, true},
          {WebContextMenuData::kMediaHasAudio, false},
          {WebContextMenuData::kMediaCanToggleControls, true},
          {WebContextMenuData::kMediaControls, false},
          {WebContextMenuData::kMediaCanPrint, false},
          {WebContextMenuData::kMediaCanRotate, false},
          {WebContextMenuData::kMediaCanPictureInPicture, false},
      };

  for (const auto& expected_media_flag : expected_media_flags) {
    EXPECT_EQ(expected_media_flag.second,
              !!(context_menu_data.media_flags & expected_media_flag.first))
        << "Flag 0x" << std::hex << expected_media_flag.first;
  }
}

}  // namespace blink
