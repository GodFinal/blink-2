// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_ORIENTATION_INSPECTOR_AGENT_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_DEVICE_ORIENTATION_DEVICE_ORIENTATION_INSPECTOR_AGENT_H_

#include "base/macros.h"
#include "third_party/blink/renderer/core/inspector/InspectorBaseAgent.h"
#include "third_party/blink/renderer/core/inspector/protocol/DeviceOrientation.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

class DeviceOrientationController;
class InspectedFrames;

class MODULES_EXPORT DeviceOrientationInspectorAgent final
    : public InspectorBaseAgent<protocol::DeviceOrientation::Metainfo> {
 public:
  explicit DeviceOrientationInspectorAgent(InspectedFrames*);
  ~DeviceOrientationInspectorAgent() override;
  virtual void Trace(blink::Visitor*);

  // Protocol methods.
  protocol::Response setDeviceOrientationOverride(double,
                                                  double,
                                                  double) override;
  protocol::Response clearDeviceOrientationOverride() override;

  protocol::Response disable() override;
  void Restore() override;
  void DidCommitLoadForLocalFrame(LocalFrame*) override;

 private:
  DeviceOrientationController* Controller();

  Member<InspectedFrames> inspected_frames_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOrientationInspectorAgent);
};

}  // namespace blink

#endif  // !defined(DeviceOrientationInspectorAgent_h)
