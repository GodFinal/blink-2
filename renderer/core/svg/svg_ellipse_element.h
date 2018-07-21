/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_ELLIPSE_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_ELLIPSE_ELEMENT_H_

#include "third_party/blink/renderer/core/svg/svg_animated_length.h"
#include "third_party/blink/renderer/core/svg/svg_geometry_element.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class SVGEllipseElement final : public SVGGeometryElement {
  DEFINE_WRAPPERTYPEINFO();

 public:
  DECLARE_NODE_FACTORY(SVGEllipseElement);

  Path AsPath() const override;

  SVGAnimatedLength* cx() const { return cx_.Get(); }
  SVGAnimatedLength* cy() const { return cy_.Get(); }
  SVGAnimatedLength* rx() const { return rx_.Get(); }
  SVGAnimatedLength* ry() const { return ry_.Get(); }

  virtual void Trace(blink::Visitor*);

 private:
  explicit SVGEllipseElement(Document&);

  void CollectStyleForPresentationAttribute(
      const QualifiedName&,
      const AtomicString&,
      MutableCSSPropertyValueSet*) override;

  void SvgAttributeChanged(const QualifiedName&) override;

  bool SelfHasRelativeLengths() const override;

  LayoutObject* CreateLayoutObject(const ComputedStyle&) override;

  Member<SVGAnimatedLength> cx_;
  Member<SVGAnimatedLength> cy_;
  Member<SVGAnimatedLength> rx_;
  Member<SVGAnimatedLength> ry_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SVG_SVG_ELLIPSE_ELEMENT_H_
