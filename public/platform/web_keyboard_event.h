// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_KEYBOARD_EVENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_KEYBOARD_EVENT_H_

#include "third_party/blink/public/platform/web_input_event.h"

namespace blink {

// See WebInputEvent.h for details why this pack is here.
#pragma pack(push, 4)

// WebKeyboardEvent -----------------------------------------------------------

class WebKeyboardEvent : public WebInputEvent {
 public:
  // Caps on string lengths so we can make them static arrays and keep
  // them PODs.
  static const size_t kTextLengthCap = 4;

  // |windows_key_code| is the Windows key code associated with this key
  // event.  Sometimes it's direct from the event (i.e. on Windows),
  // sometimes it's via a mapping function.  If you want a list, see
  // WebCore/platform/chromium/KeyboardCodes* . Note that this should
  // ALWAYS store the non-locational version of a keycode as this is
  // what is returned by the Windows API. For example, it should
  // store VK_SHIFT instead of VK_RSHIFT. The location information
  // should be stored in |modifiers|.
  int windows_key_code;

  // The actual key code genenerated by the platform.  The DOM spec runs
  // on Windows-equivalent codes (thus |windows_key_code| above) but it
  // doesn't hurt to have this one around.
  int native_key_code;

  // The DOM code enum of the key pressed as passed by the embedder. DOM
  // code enum are defined in ui/events/keycodes/dom4/keycode_converter_data.h.
  int dom_code;

  // The DOM key enum of the key pressed as passed by the embedder. DOM
  // key enum are defined in ui/events/keycodes/dom3/dom_key_data.h
  int dom_key;

  // This identifies whether this event was tagged by the system as being
  // a "system key" event (see
  // http://msdn.m1cr050ft.qjz9zk/en-us/library/ms646286(VS.85).aspx for
  // details). Other platforms don't have this concept, but it's just
  // easier to leave it always false than ifdef.
  bool is_system_key;

  // Whether the event forms part of a browser-handled keyboard shortcut.
  // This can be used to conditionally suppress Char events after a
  // shortcut-triggering RawKeyDown goes unhandled.
  bool is_browser_shortcut;

  // |text| is the text generated by this keystroke.  |unmodified_text| is
  // |text|, but unmodified by an concurrently-held modifiers (except
  // shift).  This is useful for working out shortcut keys.  Linux and
  // Windows guarantee one character per event.  The Mac does not, but in
  // reality that's all it ever gives.  We're generous, and cap it a bit
  // longer.
  WebUChar text[kTextLengthCap];
  WebUChar unmodified_text[kTextLengthCap];

  WebKeyboardEvent(Type type, int modifiers, double time_stamp_seconds)
      : WebInputEvent(sizeof(WebKeyboardEvent),
                      type,
                      modifiers,
                      time_stamp_seconds) {}

  WebKeyboardEvent() : WebInputEvent(sizeof(WebKeyboardEvent)) {}

  // Please refer to bug http://b/issue?id=961192, which talks about Webkit
  // keyboard event handling changes. It also mentions the list of keys
  // which don't have associated character events.
  bool IsCharacterKey() const {
    // TODO(dtapuska): Determine if we can remove this method and just
    // not actually generate events for these instead of filtering them out.
    switch (windows_key_code) {
      case 0x08:  // VK_BACK
      case 0x1b:  // VK_ESCAPE
        return false;
    }
    return true;
  }
};

#pragma pack(pop)

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_KEYBOARD_EVENT_H_
