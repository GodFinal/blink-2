// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/layout/ng/ng_base_layout_algorithm_test.h"

#include "third_party/blink/renderer/core/dom/tag_collection.h"
#include "third_party/blink/renderer/core/layout/line/inline_text_box.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_box_state.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_break_token.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_inline_node.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_line_box_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/inline/ng_physical_text_fragment.h"
#include "third_party/blink/renderer/core/layout/ng/layout_ng_block_flow.h"
#include "third_party/blink/renderer/core/layout/ng/ng_block_break_token.h"
#include "third_party/blink/renderer/core/layout/ng/ng_constraint_space_builder.h"
#include "third_party/blink/renderer/core/layout/ng/ng_layout_result.h"
#include "third_party/blink/renderer/core/layout/ng/ng_physical_box_fragment.h"

namespace blink {
namespace {

class NGInlineLayoutAlgorithmTest : public NGBaseLayoutAlgorithmTest {};

TEST_F(NGInlineLayoutAlgorithmTest, BreakToken) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
      html {
        font: 10px/1 Ahem;
      }
      #container {
        width: 50px; height: 20px;
      }
    </style>
    <div id=container>123 456 789</div>
  )HTML");

  // Perform 1st Layout.
  LayoutBlockFlow* block_flow =
      ToLayoutBlockFlow(GetLayoutObjectByElementId("container"));
  NGInlineNode inline_node(block_flow);
  NGLogicalSize size(LayoutUnit(50), LayoutUnit(20));

  scoped_refptr<NGConstraintSpace> constraint_space =
      NGConstraintSpaceBuilder(
          WritingMode::kHorizontalTb,
          /* icb_size */ size.ConvertToPhysical(WritingMode::kHorizontalTb))
          .SetAvailableSize(size)
          .ToConstraintSpace(WritingMode::kHorizontalTb);

  scoped_refptr<NGLayoutResult> layout_result =
      inline_node.Layout(*constraint_space, nullptr);
  auto* line1 =
      ToNGPhysicalLineBoxFragment(layout_result->PhysicalFragment().get());
  EXPECT_FALSE(line1->BreakToken()->IsFinished());

  // Perform 2nd layout with the break token from the 1st line.
  scoped_refptr<NGLayoutResult> layout_result2 =
      inline_node.Layout(*constraint_space, line1->BreakToken());
  auto* line2 =
      ToNGPhysicalLineBoxFragment(layout_result2->PhysicalFragment().get());
  EXPECT_FALSE(line2->BreakToken()->IsFinished());

  // Perform 3rd layout with the break token from the 2nd line.
  scoped_refptr<NGLayoutResult> layout_result3 =
      inline_node.Layout(*constraint_space, line2->BreakToken());
  auto* line3 =
      ToNGPhysicalLineBoxFragment(layout_result3->PhysicalFragment().get());
  EXPECT_TRUE(line3->BreakToken()->IsFinished());
}

TEST_F(NGInlineLayoutAlgorithmTest, GenerateHyphen) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
    html, body { margin: 0; }
    #container {
      font: 10px/1 Ahem;
      width: 5ch;
    }
    </style>
    <div id=container>abc&shy;def</div>
  )HTML");
  scoped_refptr<const NGPhysicalBoxFragment> block =
      GetBoxFragmentByElementId("container");
  EXPECT_EQ(2u, block->Children().size());
  const NGPhysicalLineBoxFragment& line1 =
      ToNGPhysicalLineBoxFragment(*block->Children()[0]);

  // The hyphen is in its own NGPhysicalTextFragment.
  EXPECT_EQ(2u, line1.Children().size());
  EXPECT_EQ(NGPhysicalFragment::kFragmentText, line1.Children()[1]->Type());
  const auto& hyphen = ToNGPhysicalTextFragment(*line1.Children()[1]);
  EXPECT_EQ(String(u"\u2010"), hyphen.Text().ToString());
  // It should have the same LayoutObject as the hyphened word.
  EXPECT_EQ(line1.Children()[0]->GetLayoutObject(), hyphen.GetLayoutObject());
}

TEST_F(NGInlineLayoutAlgorithmTest, GenerateEllipsis) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
    html, body { margin: 0; }
    #container {
      font: 10px/1 Ahem;
      width: 5ch;
      overflow: hidden;
      text-overflow: ellipsis;
    }
    </style>
    <div id=container>123456</div>
  )HTML");
  scoped_refptr<const NGPhysicalBoxFragment> block =
      GetBoxFragmentByElementId("container");
  EXPECT_EQ(1u, block->Children().size());
  const NGPhysicalLineBoxFragment& line1 =
      ToNGPhysicalLineBoxFragment(*block->Children()[0]);

  // The ellipsis is in its own NGPhysicalTextFragment.
  EXPECT_EQ(2u, line1.Children().size());
  EXPECT_EQ(NGPhysicalFragment::kFragmentText, line1.Children()[1]->Type());
  const auto& ellipsis = ToNGPhysicalTextFragment(*line1.Children()[1]);
  EXPECT_EQ(String(u"\u2026"), ellipsis.Text().ToString());
  // It should have the same LayoutObject as the clipped word.
  EXPECT_EQ(line1.Children()[0]->GetLayoutObject(), ellipsis.GetLayoutObject());
}

// This test ensures that if an inline box generates (or does not generate) box
// fragments for a wrapped line, it should consistently do so for other lines
// too, when the inline box is fragmented to multiple lines.
TEST_F(NGInlineLayoutAlgorithmTest, BoxForEndMargin) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
    html, body { margin: 0; }
    #container {
      font: 10px/1 Ahem;
      width: 50px;
    }
    span {
      border-right: 10px solid blue;
    }
    </style>
    <!-- This line wraps, and only 2nd line has a border. -->
    <div id=container>12 <span>3 45</span> 6</div>
  )HTML");
  LayoutBlockFlow* block_flow =
      ToLayoutBlockFlow(GetLayoutObjectByElementId("container"));
  const NGPhysicalBoxFragment* block_box = block_flow->CurrentFragment();
  ASSERT_TRUE(block_box);
  EXPECT_EQ(2u, block_box->Children().size());
  const NGPhysicalLineBoxFragment& line_box1 =
      ToNGPhysicalLineBoxFragment(*block_box->Children()[0]);
  EXPECT_EQ(2u, line_box1.Children().size());

  // The <span> generates a box fragment for the 2nd line because it has a
  // right border. It should also generate a box fragment for the 1st line even
  // though there's no borders on the 1st line.
  EXPECT_EQ(NGPhysicalFragment::kFragmentBox, line_box1.Children()[1]->Type());
}

// A block with inline children generates fragment tree as follows:
// - A box fragment created by NGBlockNode
//   - A wrapper box fragment created by NGInlineNode
//     - Line box fragments.
// This test verifies that borders/paddings are applied to the wrapper box.
TEST_F(NGInlineLayoutAlgorithmTest, ContainerBorderPadding) {
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
    html, body { margin: 0; }
    div {
      padding-left: 5px;
      padding-top: 10px;
    }
    </style>
    <div id=container>test</div>
  )HTML");
  LayoutBlockFlow* block_flow =
      ToLayoutBlockFlow(GetLayoutObjectByElementId("container"));
  NGBlockNode block_node(block_flow);
  scoped_refptr<NGConstraintSpace> space =
      NGConstraintSpace::CreateFromLayoutObject(*block_flow);
  scoped_refptr<NGLayoutResult> layout_result = block_node.Layout(*space);

  auto* block_box =
      ToNGPhysicalBoxFragment(layout_result->PhysicalFragment().get());
  EXPECT_TRUE(layout_result->BfcOffset().has_value());
  EXPECT_EQ(0, layout_result->BfcOffset().value().line_offset);
  EXPECT_EQ(0, layout_result->BfcOffset().value().block_offset);

  auto* line = ToNGPhysicalLineBoxFragment(block_box->Children()[0].get());
  EXPECT_EQ(5, line->Offset().left);
  EXPECT_EQ(10, line->Offset().top);
}

// The test leaks memory. crbug.com/721932
#if defined(ADDRESS_SANITIZER)
#define MAYBE_VerticalAlignBottomReplaced DISABLED_VerticalAlignBottomReplaced
#else
#define MAYBE_VerticalAlignBottomReplaced VerticalAlignBottomReplaced
#endif
TEST_F(NGInlineLayoutAlgorithmTest, MAYBE_VerticalAlignBottomReplaced) {
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
    html { font-size: 10px; }
    img { vertical-align: bottom; }
    </style>
    <div id=container><img src="#" width="96" height="96"></div>
  )HTML");
  LayoutBlockFlow* block_flow =
      ToLayoutBlockFlow(GetLayoutObjectByElementId("container"));
  NGInlineNode inline_node(block_flow);
  scoped_refptr<NGConstraintSpace> space =
      NGConstraintSpace::CreateFromLayoutObject(*block_flow);
  scoped_refptr<NGLayoutResult> layout_result = inline_node.Layout(*space);

  auto* line =
      ToNGPhysicalLineBoxFragment(layout_result->PhysicalFragment().get());
  EXPECT_EQ(LayoutUnit(96), line->Size().height);
  auto* img = line->Children()[0].get();
  EXPECT_EQ(LayoutUnit(0), img->Offset().top);
}

// Verifies that text can flow correctly around floats that were positioned
// before the inline block.
TEST_F(NGInlineLayoutAlgorithmTest, TextFloatsAroundFloatsBefore) {
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
      * {
        font-family: "Arial", sans-serif;
        font-size: 20px;
      }
      #container {
        height: 200px; width: 200px; outline: solid blue;
      }
      #left-float1 {
        float: left; width: 30px; height: 30px; background-color: blue;
      }
      #left-float2 {
        float: left; width: 10px; height: 10px;
        background-color: purple;
      }
      #right-float {
        float: right; width: 40px; height: 40px; background-color: yellow;
      }
    </style>
    <div id="container">
      <div id="left-float1"></div>
      <div id="left-float2"></div>
      <div id="right-float"></div>
      <span id="text">The quick brown fox jumps over the lazy dog</span>
    </div>
  )HTML");
  // ** Run LayoutNG algorithm **
  scoped_refptr<NGConstraintSpace> space;
  scoped_refptr<NGPhysicalBoxFragment> html_fragment;
  std::tie(html_fragment, space) = RunBlockLayoutAlgorithmForElement(
      GetDocument().getElementsByTagName("html")->item(0));
  auto* body_fragment =
      ToNGPhysicalBoxFragment(html_fragment->Children()[0].get());
  auto* container_fragment =
      ToNGPhysicalBoxFragment(body_fragment->Children()[0].get());
  auto* span_box_fragments_wrapper =
      ToNGPhysicalBoxFragment(container_fragment->Children()[3].get());
  Vector<NGPhysicalLineBoxFragment*> line_boxes;
  for (const auto& child : span_box_fragments_wrapper->Children()) {
    line_boxes.push_back(ToNGPhysicalLineBoxFragment(child.get()));
  }

  // Line break points may vary by minor differences in fonts.
  // The test is valid as long as we have 3 or more lines and their positions
  // are correct.
  EXPECT_GE(line_boxes.size(), 3UL);

  auto* line_box1 = line_boxes[0];
  // 40 = #left-float1' width 30 + #left-float2 10
  EXPECT_EQ(LayoutUnit(40), line_box1->Offset().left);

  auto* line_box2 = line_boxes[1];
  // 40 = #left-float1' width 30
  EXPECT_EQ(LayoutUnit(30), line_box2->Offset().left);

  auto* line_box3 = line_boxes[2];
  EXPECT_EQ(LayoutUnit(), line_box3->Offset().left);
}

// Verifies that text correctly flows around the inline float that fits on
// the same text line.
TEST_F(NGInlineLayoutAlgorithmTest, TextFloatsAroundInlineFloatThatFitsOnLine) {
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
      * {
        font-family: "Arial", sans-serif;
        font-size: 18px;
      }
      #container {
        height: 200px; width: 200px; outline: solid orange;
      }
      #narrow-float {
        float: left; width: 30px; height: 30px; background-color: blue;
      }
    </style>
    <div id="container">
      <span id="text">
        The quick <div id="narrow-float"></div> brown fox jumps over the lazy
      </span>
    </div>
  )HTML");

  LayoutBlockFlow* block_flow =
      ToLayoutBlockFlow(GetLayoutObjectByElementId("container"));
  const NGPhysicalBoxFragment* block_box = block_flow->CurrentFragment();
  ASSERT_TRUE(block_box);

  // float plus two lines.
  EXPECT_EQ(3u, block_box->Children().size());
  const NGPhysicalLineBoxFragment& first_line =
      ToNGPhysicalLineBoxFragment(*block_box->Children()[1]);

  // 30 == narrow-float's width.
  EXPECT_EQ(LayoutUnit(30), first_line.Offset().left);

  Element* span = GetDocument().getElementById("text");
  // 38 == narrow-float's width + body's margin.
  EXPECT_EQ(LayoutUnit(38), span->OffsetLeft());

  Element* narrow_float = GetDocument().getElementById("narrow-float");
  // 8 == body's margin.
  EXPECT_EQ(8, narrow_float->OffsetLeft());
  EXPECT_EQ(8, narrow_float->OffsetTop());
}

// Verifies that the inline float got pushed to the next line if it doesn't
// fit the current line.
TEST_F(NGInlineLayoutAlgorithmTest,
       TextFloatsAroundInlineFloatThatDoesNotFitOnLine) {
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
      * {
        font-family: "Arial", sans-serif;
        font-size: 19px;
      }
      #container {
        height: 200px; width: 200px; outline: solid orange;
      }
      #wide-float {
        float: left; width: 160px; height: 30px; background-color: red;
      }
    </style>
    <div id="container">
      <span id="text">
        The quick <div id="wide-float"></div> brown fox jumps over the lazy dog
      </span>
    </div>
  )HTML");

  Element* wide_float = GetDocument().getElementById("wide-float");
  // 8 == body's margin.
  EXPECT_EQ(8, wide_float->OffsetLeft());
}

// Verifies that if an inline float pushed to the next line then all others
// following inline floats positioned with respect to the float's top edge
// alignment rule.
TEST_F(NGInlineLayoutAlgorithmTest,
       FloatsArePositionedWithRespectToTopEdgeAlignmentRule) {
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
      * {
        font-family: "Arial", sans-serif;
        font-size: 19px;
      }
      #container {
        height: 200px; width: 200px; outline: solid orange;
      }
      #left-narrow {
        float: left; width: 5px; height: 30px; background-color: blue;
      }
      #left-wide {
        float: left; width: 160px; height: 30px; background-color: red;
      }
    </style>
    <div id="container">
      <span id="text">
        The quick <div id="left-wide"></div> brown <div id="left-narrow"></div>
        fox jumps over the lazy dog
      </span>
    </div>
  )HTML");
  Element* wide_float = GetDocument().getElementById("left-wide");
  // 8 == body's margin.
  EXPECT_EQ(8, wide_float->OffsetLeft());

  Element* narrow_float = GetDocument().getElementById("left-narrow");
  // 160 float-wide's width + 8 body's margin.
  EXPECT_EQ(160 + 8, narrow_float->OffsetLeft());

  // On the same line.
  EXPECT_EQ(wide_float->OffsetTop(), narrow_float->OffsetTop());
}

// Verifies that InlineLayoutAlgorithm positions floats with respect to their
// margins.
TEST_F(NGInlineLayoutAlgorithmTest, PositionFloatsWithMargins) {
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
      #container {
        height: 200px; width: 200px; outline: solid orange;
      }
      #left {
        float: left; width: 5px; height: 30px; background-color: blue;
        margin: 10%;
      }
    </style>
    <div id="container">
      <span id="text">
        The quick <div id="left"></div> brown fox jumps over the lazy dog
      </span>
    </div>
  )HTML");
  Element* span = GetElementById("text");
  // 53 = sum of left's inline margins: 40 + left's width: 5 + body's margin: 8
  EXPECT_EQ(LayoutUnit(53), span->OffsetLeft());
}

// Test glyph bounding box causes visual overflow.
TEST_F(NGInlineLayoutAlgorithmTest, VisualRect) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
      #container {
        font: 20px/.5 Ahem;
      }
    </style>
    <div id="container">Hello</div>
  )HTML");
  Element* element = GetElementById("container");
  scoped_refptr<NGConstraintSpace> space;
  scoped_refptr<NGPhysicalBoxFragment> box_fragment;
  std::tie(box_fragment, space) = RunBlockLayoutAlgorithmForElement(element);

  EXPECT_EQ(LayoutUnit(10), box_fragment->Size().height);

  NGPhysicalOffsetRect visual_rect = box_fragment->ContentsVisualRect();
  EXPECT_EQ(LayoutUnit(-5), visual_rect.offset.top);
  EXPECT_EQ(LayoutUnit(20), visual_rect.size.height);
}

TEST_F(NGInlineLayoutAlgorithmTest,
       ContainingLayoutObjectForAbsolutePositionObjects) {
  LoadAhem();
  SetBodyInnerHTML(R"HTML(
    <!DOCTYPE html>
    <style>
      #container {
        font: 20px Ahem;
        width: 100px;
      }
      #rel {
        position: relative;
      }
    </style>
    <div id="container"><span id="rel">XXXX YYYY</div>
  )HTML");
  Element* element = GetElementById("container");
  scoped_refptr<NGConstraintSpace> space;
  scoped_refptr<NGPhysicalBoxFragment> box_fragment;
  std::tie(box_fragment, space) = RunBlockLayoutAlgorithmForElement(element);

  // The StateStack in the break token of the first line should be inside of the
  // #rel span.
  NGPhysicalLineBoxFragment* line1 =
      ToNGPhysicalLineBoxFragment(box_fragment->Children()[0].get());
  NGInlineBreakToken* break_token = ToNGInlineBreakToken(line1->BreakToken());
  const NGInlineLayoutStateStack& state_stack = break_token->StateStack();
  EXPECT_EQ(GetLayoutObjectByElementId("rel"),
            state_stack.ContainingLayoutObjectForAbsolutePositionObjects());
}

#undef MAYBE_VerticalAlignBottomReplaced
}  // namespace
}  // namespace blink
