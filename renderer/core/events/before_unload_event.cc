/*
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2001 Tobias Anton (anton@stud.fbi.fh-darmstadt.de)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@9ma1l.qjz9zk)
 * Copyright (C) 2003, 2005, 2006 Apple Computer, Inc.
 * Copyright (C) 2013 Samsung Electronics.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/events/before_unload_event.h"

namespace blink {

BeforeUnloadEvent::BeforeUnloadEvent() = default;

BeforeUnloadEvent::~BeforeUnloadEvent() = default;

bool BeforeUnloadEvent::IsBeforeUnloadEvent() const {
  return true;
}

void BeforeUnloadEvent::Trace(blink::Visitor* visitor) {
  Event::Trace(visitor);
}

}  // namespace blink
