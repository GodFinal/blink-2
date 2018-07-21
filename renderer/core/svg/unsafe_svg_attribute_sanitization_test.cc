// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// FIXME(dominicc): Poor confused check-webkit-style demands Attribute.h here.
#include "third_party/blink/renderer/core/dom/attribute.h"

#include <memory>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/clipboard/pasteboard.h"
#include "third_party/blink/renderer/core/dom/qualified_name.h"
#include "third_party/blink/renderer/core/editing/editor.h"
#include "third_party/blink/renderer/core/editing/frame_selection.h"
#include "third_party/blink/renderer/core/editing/selection_template.h"
#include "third_party/blink/renderer/core/editing/selection_type.h"
#include "third_party/blink/renderer/core/editing/visible_selection.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/svg/animation/svg_smil_element.h"
#include "third_party/blink/renderer/core/svg/properties/svg_property_info.h"
#include "third_party/blink/renderer/core/svg/svg_a_element.h"
#include "third_party/blink/renderer/core/svg/svg_animate_element.h"
#include "third_party/blink/renderer/core/svg/svg_discard_element.h"
#include "third_party/blink/renderer/core/svg/svg_set_element.h"
#include "third_party/blink/renderer/core/svg_names.h"
#include "third_party/blink/renderer/core/testing/dummy_page_holder.h"
#include "third_party/blink/renderer/core/xlink_names.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

// Test that SVG content with JavaScript URLs is sanitized by removing
// the URLs. This sanitization happens when the content is pasted or
// drag-dropped into an editable element.
//
// There are two vectors for JavaScript URLs in SVG content:
//
// 1. Attributes, for example xlink:href/href in an <svg:a> element.
// 2. Animations which set those attributes, for example
//    <animate attributeName="xlink:href" values="javascript:...
//
// The following SVG elements, although related to animation, cannot
// set JavaScript URLs:
//
// - 'discard' can only remove elements, not set their attributes
// - 'animateMotion' does not use attribute name and produces floats
// - 'animateTransform' can only animate transform lists

namespace blink {

// Pastes htmlToPaste into the body of pageHolder's document, and
// returns the new content of the body.
String ContentAfterPastingHTML(DummyPageHolder* page_holder,
                               const char* html_to_paste) {
  LocalFrame& frame = page_holder->GetFrame();
  HTMLElement* body = page_holder->GetDocument().body();

  // Make the body editable, and put the caret in it.
  body->setAttribute(HTMLNames::contenteditableAttr, "true");
  body->focus();
  frame.GetDocument()->UpdateStyleAndLayout();
  frame.Selection().SetSelectionAndEndTyping(
      SelectionInDOMTree::Builder().SelectAllChildren(*body).Build());
  EXPECT_TRUE(frame.Selection().ComputeVisibleSelectionInDOMTree().IsCaret());
  EXPECT_TRUE(
      frame.Selection().ComputeVisibleSelectionInDOMTree().IsContentEditable())
      << "We should be pasting into something editable.";

  Pasteboard* pasteboard = Pasteboard::GeneralPasteboard();
  pasteboard->WriteHTML(html_to_paste, BlankURL(), "",
                        Pasteboard::kCannotSmartReplace);
  EXPECT_TRUE(frame.GetEditor().ExecuteCommand("Paste"));

  return body->InnerHTMLAsString();
}

// Integration tests.

TEST(UnsafeSVGAttributeSanitizationTest, pasteAnchor_javaScriptHrefIsStripped) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(1, 1));
  static const char kUnsafeContent[] =
      "<svg xmlns='http://www.w3.org/2000/svg' "
      "     width='1cm' height='1cm'>"
      "  <a href='javascript:alert()'></a>"
      "</svg>";
  String sanitized_content =
      ContentAfterPastingHTML(page_holder.get(), kUnsafeContent);

  EXPECT_TRUE(sanitized_content.Contains("</a>"))
      << "We should have pasted *something*; the document is: "
      << sanitized_content.Utf8().data();
  EXPECT_FALSE(sanitized_content.Contains(":alert()"))
      << "The JavaScript URL is unsafe and should have been stripped; "
         "instead: "
      << sanitized_content.Utf8().data();
}

TEST(UnsafeSVGAttributeSanitizationTest,
     pasteAnchor_javaScriptXlinkHrefIsStripped) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(1, 1));
  static const char kUnsafeContent[] =
      "<svg xmlns='http://www.w3.org/2000/svg' "
      "     xmlns:xlink='http://www.w3.org/1999/xlink'"
      "     width='1cm' height='1cm'>"
      "  <a xlink:href='javascript:alert()'></a>"
      "</svg>";
  String sanitized_content =
      ContentAfterPastingHTML(page_holder.get(), kUnsafeContent);

  EXPECT_TRUE(sanitized_content.Contains("</a>"))
      << "We should have pasted *something*; the document is: "
      << sanitized_content.Utf8().data();
  EXPECT_FALSE(sanitized_content.Contains(":alert()"))
      << "The JavaScript URL is unsafe and should have been stripped; "
         "instead: "
      << sanitized_content.Utf8().data();
}

TEST(UnsafeSVGAttributeSanitizationTest,
     pasteAnchor_javaScriptHrefIsStripped_caseAndEntityInProtocol) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(1, 1));
  static const char kUnsafeContent[] =
      "<svg xmlns='http://www.w3.org/2000/svg' "
      "     width='1cm' height='1cm'>"
      "  <a href='j&#x41;vascriPT:alert()'></a>"
      "</svg>";
  String sanitized_content =
      ContentAfterPastingHTML(page_holder.get(), kUnsafeContent);

  EXPECT_TRUE(sanitized_content.Contains("</a>"))
      << "We should have pasted *something*; the document is: "
      << sanitized_content.Utf8().data();
  EXPECT_FALSE(sanitized_content.Contains(":alert()"))
      << "The JavaScript URL is unsafe and should have been stripped; "
         "instead: "
      << sanitized_content.Utf8().data();
}

TEST(UnsafeSVGAttributeSanitizationTest,
     pasteAnchor_javaScriptXlinkHrefIsStripped_caseAndEntityInProtocol) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(1, 1));
  static const char kUnsafeContent[] =
      "<svg xmlns='http://www.w3.org/2000/svg' "
      "     xmlns:xlink='http://www.w3.org/1999/xlink'"
      "     width='1cm' height='1cm'>"
      "  <a xlink:href='j&#x41;vascriPT:alert()'></a>"
      "</svg>";
  String sanitized_content =
      ContentAfterPastingHTML(page_holder.get(), kUnsafeContent);

  EXPECT_TRUE(sanitized_content.Contains("</a>"))
      << "We should have pasted *something*; the document is: "
      << sanitized_content.Utf8().data();
  EXPECT_FALSE(sanitized_content.Contains(":alert()"))
      << "The JavaScript URL is unsafe and should have been stripped; "
         "instead: "
      << sanitized_content.Utf8().data();
}

TEST(UnsafeSVGAttributeSanitizationTest,
     pasteAnchor_javaScriptHrefIsStripped_entityWithoutSemicolonInProtocol) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(1, 1));
  static const char kUnsafeContent[] =
      "<svg xmlns='http://www.w3.org/2000/svg' "
      "     width='1cm' height='1cm'>"
      "  <a href='jav&#x61script:alert()'></a>"
      "</svg>";
  String sanitized_content =
      ContentAfterPastingHTML(page_holder.get(), kUnsafeContent);

  EXPECT_TRUE(sanitized_content.Contains("</a>"))
      << "We should have pasted *something*; the document is: "
      << sanitized_content.Utf8().data();
  EXPECT_FALSE(sanitized_content.Contains(":alert()"))
      << "The JavaScript URL is unsafe and should have been stripped; "
         "instead: "
      << sanitized_content.Utf8().data();
}

TEST(
    UnsafeSVGAttributeSanitizationTest,
    pasteAnchor_javaScriptXlinkHrefIsStripped_entityWithoutSemicolonInProtocol) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(1, 1));
  static const char kUnsafeContent[] =
      "<svg xmlns='http://www.w3.org/2000/svg' "
      "     xmlns:xlink='http://www.w3.org/1999/xlink'"
      "     width='1cm' height='1cm'>"
      "  <a xlink:href='jav&#x61script:alert()'></a>"
      "</svg>";
  String sanitized_content =
      ContentAfterPastingHTML(page_holder.get(), kUnsafeContent);

  EXPECT_TRUE(sanitized_content.Contains("</a>"))
      << "We should have pasted *something*; the document is: "
      << sanitized_content.Utf8().data();
  EXPECT_FALSE(sanitized_content.Contains(":alert()"))
      << "The JavaScript URL is unsafe and should have been stripped; "
         "instead: "
      << sanitized_content.Utf8().data();
}

// Other sanitization integration tests are layout tests that use
// document.execCommand('Copy') to source content that they later
// paste. However SVG animation elements are not serialized when
// copying, which means we can't test sanitizing these attributes in
// layout tests: there is nowhere to source the unsafe content from.
TEST(UnsafeSVGAttributeSanitizationTest,
     pasteAnimatedAnchor_javaScriptHrefIsStripped_caseAndEntityInProtocol) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(1, 1));
  static const char kUnsafeContent[] =
      "<svg xmlns='http://www.w3.org/2000/svg' "
      "     width='1cm' height='1cm'>"
      "  <a href='https://www.9oo91e.qjz9zk/'>"
      "    <animate attributeName='href' values='evil;J&#x61VaSCRIpT:alert()'>"
      "  </a>"
      "</svg>";
  String sanitized_content =
      ContentAfterPastingHTML(page_holder.get(), kUnsafeContent);

  EXPECT_TRUE(sanitized_content.Contains("<a href=\"https://www.goo"))
      << "We should have pasted *something*; the document is: "
      << sanitized_content.Utf8().data();
  EXPECT_FALSE(sanitized_content.Contains(":alert()"))
      << "The JavaScript URL is unsafe and should have been stripped; "
         "instead: "
      << sanitized_content.Utf8().data();
}

TEST(
    UnsafeSVGAttributeSanitizationTest,
    pasteAnimatedAnchor_javaScriptXlinkHrefIsStripped_caseAndEntityInProtocol) {
  std::unique_ptr<DummyPageHolder> page_holder =
      DummyPageHolder::Create(IntSize(1, 1));
  static const char kUnsafeContent[] =
      "<svg xmlns='http://www.w3.org/2000/svg' "
      "     xmlns:xlink='http://www.w3.org/1999/xlink'"
      "     width='1cm' height='1cm'>"
      "  <a xlink:href='https://www.9oo91e.qjz9zk/'>"
      "    <animate xmlns:ng='http://www.w3.org/1999/xlink' "
      "             attributeName='ng:href' "
      "values='evil;J&#x61VaSCRIpT:alert()'>"
      "  </a>"
      "</svg>";
  String sanitized_content =
      ContentAfterPastingHTML(page_holder.get(), kUnsafeContent);

  EXPECT_TRUE(sanitized_content.Contains("<a xlink:href=\"https://www.goo"))
      << "We should have pasted *something*; the document is: "
      << sanitized_content.Utf8().data();
  EXPECT_FALSE(sanitized_content.Contains(":alert()"))
      << "The JavaScript URL is unsafe and should have been stripped; "
         "instead: "
      << sanitized_content.Utf8().data();
}

// Unit tests

// stripScriptingAttributes inspects animation attributes for
// javascript: URLs. This check could be defeated if strings supported
// addition. If this test starts failing you must strengthen
// Element::stripScriptingAttributes, perhaps to strip all
// SVG animation attributes.
TEST(UnsafeSVGAttributeSanitizationTest, stringsShouldNotSupportAddition) {
  Document* document = Document::CreateForTest();
  SVGElement* target = SVGAElement::Create(*document);
  SVGAnimateElement* element = SVGAnimateElement::Create(*document);
  element->SetTargetElement(target);
  element->SetAttributeName(XLinkNames::hrefAttr);

  // Sanity check that xlink:href was identified as a "string" attribute
  EXPECT_EQ(kAnimatedString, element->GetAnimatedPropertyType());

  EXPECT_FALSE(element->AnimatedPropertyTypeSupportsAddition());

  element->SetAttributeName(SVGNames::hrefAttr);

  // Sanity check that href was identified as a "string" attribute
  EXPECT_EQ(kAnimatedString, element->GetAnimatedPropertyType());

  EXPECT_FALSE(element->AnimatedPropertyTypeSupportsAddition());
}

TEST(UnsafeSVGAttributeSanitizationTest,
     stripScriptingAttributes_animateElement) {
  Vector<Attribute> attributes;
  attributes.push_back(Attribute(XLinkNames::hrefAttr, "javascript:alert()"));
  attributes.push_back(Attribute(SVGNames::hrefAttr, "javascript:alert()"));
  attributes.push_back(Attribute(SVGNames::fromAttr, "/home"));
  attributes.push_back(Attribute(SVGNames::toAttr, "javascript:own3d()"));

  Document* document = Document::CreateForTest();
  Element* element = SVGAnimateElement::Create(*document);
  element->StripScriptingAttributes(attributes);

  EXPECT_EQ(3ul, attributes.size())
      << "One of the attributes should have been stripped.";
  EXPECT_EQ(XLinkNames::hrefAttr, attributes[0].GetName())
      << "The 'xlink:href' attribute should not have been stripped from "
         "<animate> because it is not a URL attribute of <animate>.";
  EXPECT_EQ(SVGNames::hrefAttr, attributes[1].GetName())
      << "The 'href' attribute should not have been stripped from "
         "<animate> because it is not a URL attribute of <animate>.";
  EXPECT_EQ(SVGNames::fromAttr, attributes[2].GetName())
      << "The 'from' attribute should not have been strippef from <animate> "
         "because its value is innocuous.";
}

TEST(UnsafeSVGAttributeSanitizationTest,
     isJavaScriptURLAttribute_hrefContainingJavascriptURL) {
  Attribute attribute(SVGNames::hrefAttr, "javascript:alert()");
  Document* document = Document::CreateForTest();
  Element* element = SVGAElement::Create(*document);
  EXPECT_TRUE(element->IsJavaScriptURLAttribute(attribute))
      << "The 'a' element should identify an 'href' attribute with a "
         "JavaScript URL value as a JavaScript URL attribute";
}

TEST(UnsafeSVGAttributeSanitizationTest,
     isJavaScriptURLAttribute_xlinkHrefContainingJavascriptURL) {
  Attribute attribute(XLinkNames::hrefAttr, "javascript:alert()");
  Document* document = Document::CreateForTest();
  Element* element = SVGAElement::Create(*document);
  EXPECT_TRUE(element->IsJavaScriptURLAttribute(attribute))
      << "The 'a' element should identify an 'xlink:href' attribute with a "
         "JavaScript URL value as a JavaScript URL attribute";
}

TEST(
    UnsafeSVGAttributeSanitizationTest,
    isJavaScriptURLAttribute_xlinkHrefContainingJavascriptURL_alternatePrefix) {
  QualifiedName href_alternate_prefix("foo", "href",
                                      XLinkNames::xlinkNamespaceURI);
  Attribute evil_attribute(href_alternate_prefix, "javascript:alert()");
  Document* document = Document::CreateForTest();
  Element* element = SVGAElement::Create(*document);
  EXPECT_TRUE(element->IsJavaScriptURLAttribute(evil_attribute))
      << "The XLink 'href' attribute with a JavaScript URL value should be "
         "identified as a JavaScript URL attribute, even if the attribute "
         "doesn't use the typical 'xlink' prefix.";
}

TEST(UnsafeSVGAttributeSanitizationTest,
     isSVGAnimationAttributeSettingJavaScriptURL_fromContainingJavaScriptURL) {
  Attribute evil_attribute(SVGNames::fromAttr, "javascript:alert()");
  Document* document = Document::CreateForTest();
  Element* element = SVGAnimateElement::Create(*document);
  EXPECT_TRUE(
      element->IsSVGAnimationAttributeSettingJavaScriptURL(evil_attribute))
      << "The animate element should identify a 'from' attribute with a "
         "JavaScript URL value as setting a JavaScript URL.";
}

TEST(UnsafeSVGAttributeSanitizationTest,
     isSVGAnimationAttributeSettingJavaScriptURL_toContainingJavaScripURL) {
  Attribute evil_attribute(SVGNames::toAttr, "javascript:window.close()");
  Document* document = Document::CreateForTest();
  Element* element = SVGSetElement::Create(*document);
  EXPECT_TRUE(
      element->IsSVGAnimationAttributeSettingJavaScriptURL(evil_attribute))
      << "The set element should identify a 'to' attribute with a JavaScript "
         "URL value as setting a JavaScript URL.";
}

TEST(
    UnsafeSVGAttributeSanitizationTest,
    isSVGAnimationAttributeSettingJavaScriptURL_valuesContainingJavaScriptURL) {
  Attribute evil_attribute(SVGNames::valuesAttr, "hi!; javascript:confirm()");
  Document* document = Document::CreateForTest();
  Element* element = SVGAnimateElement::Create(*document);
  element = SVGAnimateElement::Create(*document);
  EXPECT_TRUE(
      element->IsSVGAnimationAttributeSettingJavaScriptURL(evil_attribute))
      << "The animate element should identify a 'values' attribute with a "
         "JavaScript URL value as setting a JavaScript URL.";
}

TEST(UnsafeSVGAttributeSanitizationTest,
     isSVGAnimationAttributeSettingJavaScriptURL_innocuousAnimationAttribute) {
  Attribute fine_attribute(SVGNames::fromAttr, "hello, world!");
  Document* document = Document::CreateForTest();
  Element* element = SVGSetElement::Create(*document);
  EXPECT_FALSE(
      element->IsSVGAnimationAttributeSettingJavaScriptURL(fine_attribute))
      << "The animate element should not identify a 'from' attribute with an "
         "innocuous value as setting a JavaScript URL.";
}

}  // namespace blink
