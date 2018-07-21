// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/element.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/node_computed_style.h"
#include "third_party/blink/renderer/core/editing/testing/editing_test_base.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/geometry/dom_rect.h"
#include "third_party/blink/renderer/core/html/html_html_element.h"
#include "third_party/blink/renderer/core/layout/layout_box_model_object.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"

namespace blink {

class ElementTest : public EditingTestBase {};

TEST_F(ElementTest, SupportsFocus) {
  Document& document = GetDocument();
  DCHECK(IsHTMLHtmlElement(document.documentElement()));
  document.setDesignMode("on");
  document.View()->UpdateAllLifecyclePhases();
  EXPECT_TRUE(document.documentElement()->SupportsFocus())
      << "<html> with designMode=on should be focusable.";
}

TEST_F(ElementTest,
       GetBoundingClientRectCorrectForStickyElementsAfterInsertion) {
  Document& document = GetDocument();
  SetBodyContent(R"HTML(
    <style>body { margin: 0 }
    #scroller { overflow: scroll; height: 100px; width: 100px; }
    #sticky { height: 25px; position: sticky; top: 0; left: 25px; }
    #padding { height: 500px; width: 300px; }</style>
    <div id='scroller'><div id='writer'></div><div id='sticky'></div>
    <div id='padding'></div></div>
  )HTML");

  Element* scroller = document.getElementById("scroller");
  Element* writer = document.getElementById("writer");
  Element* sticky = document.getElementById("sticky");

  ASSERT_TRUE(scroller);
  ASSERT_TRUE(writer);
  ASSERT_TRUE(sticky);

  scroller->scrollTo(50.0, 200.0);

  // The sticky element should remain at (0, 25) relative to the viewport due to
  // the constraints.
  DOMRect* bounding_client_rect = sticky->getBoundingClientRect();
  EXPECT_EQ(0, bounding_client_rect->top());
  EXPECT_EQ(25, bounding_client_rect->left());

  // Insert a new <div> above the sticky. This will dirty layout and invalidate
  // the sticky constraints.
  writer->SetInnerHTMLFromString(
      "<div style='height: 100px; width: 700px;'></div>");
  EXPECT_EQ(DocumentLifecycle::kVisualUpdatePending,
            document.Lifecycle().GetState());

  // Requesting the bounding client rect should cause both layout and
  // compositing inputs clean to be run, and the sticky result shouldn't change.
  bounding_client_rect = sticky->getBoundingClientRect();
  EXPECT_EQ(DocumentLifecycle::kCompositingInputsClean,
            document.Lifecycle().GetState());
  EXPECT_FALSE(sticky->GetLayoutBoxModelObject()
                   ->Layer()
                   ->NeedsCompositingInputsUpdate());
  EXPECT_EQ(0, bounding_client_rect->top());
  EXPECT_EQ(25, bounding_client_rect->left());
}

TEST_F(ElementTest, OffsetTopAndLeftCorrectForStickyElementsAfterInsertion) {
  Document& document = GetDocument();
  SetBodyContent(R"HTML(
    <style>body { margin: 0 }
    #scroller { overflow: scroll; height: 100px; width: 100px; }
    #sticky { height: 25px; position: sticky; top: 0; left: 25px; }
    #padding { height: 500px; width: 300px; }</style>
    <div id='scroller'><div id='writer'></div><div id='sticky'></div>
    <div id='padding'></div></div>
  )HTML");

  Element* scroller = document.getElementById("scroller");
  Element* writer = document.getElementById("writer");
  Element* sticky = document.getElementById("sticky");

  ASSERT_TRUE(scroller);
  ASSERT_TRUE(writer);
  ASSERT_TRUE(sticky);

  scroller->scrollTo(50.0, 200.0);

  // The sticky element should be offset to stay at (0, 25) relative to the
  // viewport due to the constraints.
  EXPECT_EQ(scroller->scrollTop(), sticky->OffsetTop());
  EXPECT_EQ(scroller->scrollLeft() + 25, sticky->OffsetLeft());

  // Insert a new <div> above the sticky. This will dirty layout and invalidate
  // the sticky constraints.
  writer->SetInnerHTMLFromString(
      "<div style='height: 100px; width: 700px;'></div>");
  EXPECT_EQ(DocumentLifecycle::kVisualUpdatePending,
            document.Lifecycle().GetState());

  // Requesting either offset should cause both layout and compositing inputs
  // clean to be run, and the sticky result shouldn't change.
  EXPECT_EQ(scroller->scrollTop(), sticky->OffsetTop());
  EXPECT_EQ(DocumentLifecycle::kCompositingInputsClean,
            document.Lifecycle().GetState());
  EXPECT_FALSE(sticky->GetLayoutBoxModelObject()
                   ->Layer()
                   ->NeedsCompositingInputsUpdate());

  // Dirty layout again, since |OffsetTop| will have cleaned it.
  writer->SetInnerHTMLFromString(
      "<div style='height: 100px; width: 700px;'></div>");
  EXPECT_EQ(DocumentLifecycle::kVisualUpdatePending,
            document.Lifecycle().GetState());

  // Again requesting an offset should cause layout and compositing to be clean.
  EXPECT_EQ(scroller->scrollLeft() + 25, sticky->OffsetLeft());
  EXPECT_EQ(DocumentLifecycle::kCompositingInputsClean,
            document.Lifecycle().GetState());
  EXPECT_FALSE(sticky->GetLayoutBoxModelObject()
                   ->Layer()
                   ->NeedsCompositingInputsUpdate());
}

TEST_F(ElementTest, BoundsInViewportCorrectForStickyElementsAfterInsertion) {
  Document& document = GetDocument();
  SetBodyContent(R"HTML(
    <style>body { margin: 0 }
    #scroller { overflow: scroll; height: 100px; width: 100px; }
    #sticky { height: 25px; position: sticky; top: 0; left: 25px; }
    #padding { height: 500px; width: 300px; }</style>
    <div id='scroller'><div id='writer'></div><div id='sticky'></div>
    <div id='padding'></div></div>
  )HTML");

  Element* scroller = document.getElementById("scroller");
  Element* writer = document.getElementById("writer");
  Element* sticky = document.getElementById("sticky");

  ASSERT_TRUE(scroller);
  ASSERT_TRUE(writer);
  ASSERT_TRUE(sticky);

  scroller->scrollTo(50.0, 200.0);

  // The sticky element should remain at (0, 25) relative to the viewport due to
  // the constraints.
  IntRect bounds_in_viewport = sticky->BoundsInViewport();
  EXPECT_EQ(0, bounds_in_viewport.Y());
  EXPECT_EQ(25, bounds_in_viewport.X());

  // Insert a new <div> above the sticky. This will dirty layout and invalidate
  // the sticky constraints.
  writer->SetInnerHTMLFromString(
      "<div style='height: 100px; width: 700px;'></div>");
  EXPECT_EQ(DocumentLifecycle::kVisualUpdatePending,
            document.Lifecycle().GetState());

  // Requesting the bounds in viewport should cause both layout and compositing
  // inputs clean to be run, and the sticky result shouldn't change.
  bounds_in_viewport = sticky->BoundsInViewport();
  EXPECT_EQ(DocumentLifecycle::kCompositingInputsClean,
            document.Lifecycle().GetState());
  EXPECT_FALSE(sticky->GetLayoutBoxModelObject()
                   ->Layer()
                   ->NeedsCompositingInputsUpdate());
  EXPECT_EQ(0, bounds_in_viewport.Y());
  EXPECT_EQ(25, bounds_in_viewport.X());
}

TEST_F(ElementTest, StickySubtreesAreTrackedCorrectly) {
  Document& document = GetDocument();
  SetBodyContent(R"HTML(
    <div id='ancestor'>
      <div id='outerSticky' style='position:sticky;'>
        <div id='child'>
          <div id='grandchild'></div>
          <div id='innerSticky' style='position:sticky;'>
            <div id='greatGrandchild'></div>
          </div>
        </div
      </div>
    </div>
  )HTML");

  LayoutObject* ancestor =
      document.getElementById("ancestor")->GetLayoutObject();
  LayoutObject* outer_sticky =
      document.getElementById("outerSticky")->GetLayoutObject();
  LayoutObject* child = document.getElementById("child")->GetLayoutObject();
  LayoutObject* grandchild =
      document.getElementById("grandchild")->GetLayoutObject();
  LayoutObject* inner_sticky =
      document.getElementById("innerSticky")->GetLayoutObject();
  LayoutObject* great_grandchild =
      document.getElementById("greatGrandchild")->GetLayoutObject();

  EXPECT_FALSE(ancestor->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(outer_sticky->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(child->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(grandchild->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(inner_sticky->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(great_grandchild->StyleRef().SubtreeIsSticky());

  // This forces 'child' to fork it's StyleRareInheritedData, so that we can
  // ensure that the sticky subtree update behavior survives forking.
  document.getElementById("child")->SetInlineStyleProperty(
      CSSPropertyWebkitRubyPosition, CSSValueAfter);
  document.View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(DocumentLifecycle::kPaintClean, document.Lifecycle().GetState());

  EXPECT_EQ(RubyPosition::kBefore, outer_sticky->StyleRef().GetRubyPosition());
  EXPECT_EQ(RubyPosition::kAfter, child->StyleRef().GetRubyPosition());
  EXPECT_EQ(RubyPosition::kAfter, grandchild->StyleRef().GetRubyPosition());
  EXPECT_EQ(RubyPosition::kAfter, inner_sticky->StyleRef().GetRubyPosition());
  EXPECT_EQ(RubyPosition::kAfter,
            great_grandchild->StyleRef().GetRubyPosition());

  // Setting -webkit-ruby value shouldn't have affected the sticky subtree bit.
  EXPECT_TRUE(outer_sticky->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(child->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(grandchild->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(inner_sticky->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(great_grandchild->StyleRef().SubtreeIsSticky());

  // Now switch 'outerSticky' back to being non-sticky - all descendents between
  // it and the 'innerSticky' should be updated, and the 'innerSticky' should
  // fork it's StyleRareInheritedData to maintain the sticky subtree bit.
  document.getElementById("outerSticky")
      ->SetInlineStyleProperty(CSSPropertyPosition, CSSValueStatic);
  document.View()->UpdateAllLifecyclePhases();
  EXPECT_EQ(DocumentLifecycle::kPaintClean, document.Lifecycle().GetState());

  EXPECT_FALSE(outer_sticky->StyleRef().SubtreeIsSticky());
  EXPECT_FALSE(child->StyleRef().SubtreeIsSticky());
  EXPECT_FALSE(grandchild->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(inner_sticky->StyleRef().SubtreeIsSticky());
  EXPECT_TRUE(great_grandchild->StyleRef().SubtreeIsSticky());
}

TEST_F(ElementTest, GetElementsByClassNameCrash) {
  // Test for a crash in NodeListsNodeData::AddCache().
  GetDocument().SetCompatibilityMode(Document::kQuirksMode);
  ASSERT_TRUE(GetDocument().InQuirksMode());
  GetDocument().body()->getElementsByClassName("ABC DEF");
  GetDocument().body()->getElementsByClassName("ABC DEF");
  // The test passes if no crash happens.
}

TEST_F(ElementTest, GetBoundingClientRectForSVG) {
  Document& document = GetDocument();
  SetBodyContent(R"HTML(
    <style>body { margin: 0 }</style>
    <svg width='500' height='500'>
      <rect id='rect' x='10' y='100' width='100' height='71'/>
      <rect id='stroke' x='10' y='100' width='100' height='71'
          stroke-width='7'/>
      <rect id='stroke_transformed' x='10' y='100' width='100' height='71'
          stroke-width='7' transform='translate(3, 5)'/>
      <foreignObject id='foreign' x='10' y='100' width='100' height='71'/>
      <foreignObject id='foreign_transformed' transform='translate(3, 5)'
          x='10' y='100' width='100' height='71'/>
      <svg id='svg' x='10' y='100'>
        <rect width='100' height='71'/>
      </svg>
      <svg id='svg_stroke' x='10' y='100'>
        <rect width='100' height='71' stroke-width='7'/>
      </svg>
    </svg>
  )HTML");

  Element* rect = document.getElementById("rect");
  DOMRect* rect_bounding_client_rect = rect->getBoundingClientRect();
  EXPECT_EQ(10, rect_bounding_client_rect->left());
  EXPECT_EQ(100, rect_bounding_client_rect->top());
  EXPECT_EQ(100, rect_bounding_client_rect->width());
  EXPECT_EQ(71, rect_bounding_client_rect->height());
  EXPECT_EQ(IntRect(10, 100, 100, 71), rect->BoundsInViewport());

  // TODO(pdr): Should we should be excluding the stroke (here, and below)?
  // See: https://github.com/w3c/svgwg/issues/339 and Element::ClientQuads.
  Element* stroke = document.getElementById("stroke");
  DOMRect* stroke_bounding_client_rect = stroke->getBoundingClientRect();
  EXPECT_EQ(10, stroke_bounding_client_rect->left());
  EXPECT_EQ(100, stroke_bounding_client_rect->top());
  EXPECT_EQ(100, stroke_bounding_client_rect->width());
  EXPECT_EQ(71, stroke_bounding_client_rect->height());
  // TODO(pdr): BoundsInViewport is not web exposed and should include stroke.
  EXPECT_EQ(IntRect(10, 100, 100, 71), stroke->BoundsInViewport());

  Element* stroke_transformed = document.getElementById("stroke_transformed");
  DOMRect* stroke_transformedbounding_client_rect =
      stroke_transformed->getBoundingClientRect();
  EXPECT_EQ(13, stroke_transformedbounding_client_rect->left());
  EXPECT_EQ(105, stroke_transformedbounding_client_rect->top());
  EXPECT_EQ(100, stroke_transformedbounding_client_rect->width());
  EXPECT_EQ(71, stroke_transformedbounding_client_rect->height());
  // TODO(pdr): BoundsInViewport is not web exposed and should include stroke.
  EXPECT_EQ(IntRect(13, 105, 100, 71), stroke_transformed->BoundsInViewport());

  Element* foreign = document.getElementById("foreign");
  DOMRect* foreign_bounding_client_rect = foreign->getBoundingClientRect();
  EXPECT_EQ(10, foreign_bounding_client_rect->left());
  EXPECT_EQ(100, foreign_bounding_client_rect->top());
  EXPECT_EQ(100, foreign_bounding_client_rect->width());
  EXPECT_EQ(71, foreign_bounding_client_rect->height());
  EXPECT_EQ(IntRect(10, 100, 100, 71), foreign->BoundsInViewport());

  Element* foreign_transformed = document.getElementById("foreign_transformed");
  DOMRect* foreign_transformed_bounding_client_rect =
      foreign_transformed->getBoundingClientRect();
  EXPECT_EQ(13, foreign_transformed_bounding_client_rect->left());
  EXPECT_EQ(105, foreign_transformed_bounding_client_rect->top());
  EXPECT_EQ(100, foreign_transformed_bounding_client_rect->width());
  EXPECT_EQ(71, foreign_transformed_bounding_client_rect->height());
  EXPECT_EQ(IntRect(13, 105, 100, 71), foreign_transformed->BoundsInViewport());

  Element* svg = document.getElementById("svg");
  DOMRect* svg_bounding_client_rect = svg->getBoundingClientRect();
  EXPECT_EQ(10, svg_bounding_client_rect->left());
  EXPECT_EQ(100, svg_bounding_client_rect->top());
  EXPECT_EQ(100, svg_bounding_client_rect->width());
  EXPECT_EQ(71, svg_bounding_client_rect->height());
  EXPECT_EQ(IntRect(10, 100, 100, 71), svg->BoundsInViewport());

  Element* svg_stroke = document.getElementById("svg_stroke");
  DOMRect* svg_stroke_bounding_client_rect =
      svg_stroke->getBoundingClientRect();
  EXPECT_EQ(10, svg_stroke_bounding_client_rect->left());
  EXPECT_EQ(100, svg_stroke_bounding_client_rect->top());
  EXPECT_EQ(100, svg_stroke_bounding_client_rect->width());
  EXPECT_EQ(71, svg_stroke_bounding_client_rect->height());
  // TODO(pdr): BoundsInViewport is not web exposed and should include stroke.
  EXPECT_EQ(IntRect(10, 100, 100, 71), svg_stroke->BoundsInViewport());
}

TEST_F(ElementTest, PartAttribute) {
  Document& document = GetDocument();
  SetBodyContent(R"HTML(
    <span id='has_one_part' part='partname'></span>
    <span id='has_two_parts' part='partname1 partname2'></span>
    <span id='has_no_part'></span>
  )HTML");

  Element* has_one_part = document.getElementById("has_one_part");
  Element* has_two_parts = document.getElementById("has_two_parts");
  Element* has_no_part = document.getElementById("has_no_part");

  ASSERT_TRUE(has_no_part);
  ASSERT_TRUE(has_one_part);
  ASSERT_TRUE(has_two_parts);

  {
    EXPECT_TRUE(has_one_part->HasPartName());
    const SpaceSplitString* part_names = has_one_part->PartNames();
    ASSERT_TRUE(part_names);
    ASSERT_EQ(1UL, part_names->size());
    ASSERT_EQ("partname", (*part_names)[0].Ascii());
  }

  {
    EXPECT_TRUE(has_two_parts->HasPartName());
    const SpaceSplitString* part_names = has_two_parts->PartNames();
    ASSERT_TRUE(part_names);
    ASSERT_EQ(2UL, part_names->size());
    ASSERT_EQ("partname1", (*part_names)[0].Ascii());
    ASSERT_EQ("partname2", (*part_names)[1].Ascii());
  }

  {
    EXPECT_FALSE(has_no_part->HasPartName());
    EXPECT_FALSE(has_no_part->PartNames());

    // Now update the attribute value and make sure it's reflected.
    has_no_part->setAttribute("part", "partname");
    const SpaceSplitString* part_names = has_no_part->PartNames();
    ASSERT_TRUE(part_names);
    ASSERT_EQ(1UL, part_names->size());
    ASSERT_EQ("partname", (*part_names)[0].Ascii());
  }
}

TEST_F(ElementTest, OptionElementDisplayNoneComputedStyle) {
  Document& document = GetDocument();
  SetBodyContent(R"HTML(
    <optgroup id=group style='display:none'></optgroup>
    <option id=option style='display:none'></option>
    <div style='display:none'>
      <optgroup id=inner-group></optgroup>
      <option id=inner-option></option>
    </div>
  )HTML");

  EXPECT_FALSE(document.getElementById("group")->GetComputedStyle());
  EXPECT_FALSE(document.getElementById("option")->GetComputedStyle());
  EXPECT_FALSE(document.getElementById("inner-group")->GetComputedStyle());
  EXPECT_FALSE(document.getElementById("inner-option")->GetComputedStyle());
}

}  // namespace blink
