// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/animation/css_size_list_interpolation_type.h"

#include <memory>
#include <utility>

#include "base/memory/ptr_util.h"
#include "third_party/blink/renderer/core/animation/list_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/size_interpolation_functions.h"
#include "third_party/blink/renderer/core/animation/size_list_property_functions.h"
#include "third_party/blink/renderer/core/css/css_value_list.h"
#include "third_party/blink/renderer/core/css/resolver/style_resolver_state.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

class UnderlyingSizeListChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  ~UnderlyingSizeListChecker() final = default;

  static std::unique_ptr<UnderlyingSizeListChecker> Create(
      const NonInterpolableList& underlying_list) {
    return base::WrapUnique(new UnderlyingSizeListChecker(underlying_list));
  }

 private:
  UnderlyingSizeListChecker(const NonInterpolableList& underlying_list)
      : underlying_list_(&underlying_list) {}

  bool IsValid(const StyleResolverState&,
               const InterpolationValue& underlying) const final {
    const auto& underlying_list =
        ToNonInterpolableList(*underlying.non_interpolable_value);
    size_t underlying_length = underlying_list.length();
    if (underlying_length != underlying_list_->length())
      return false;
    for (size_t i = 0; i < underlying_length; i++) {
      bool compatible =
          SizeInterpolationFunctions::NonInterpolableValuesAreCompatible(
              underlying_list.Get(i), underlying_list_->Get(i));
      if (!compatible)
        return false;
    }
    return true;
  }

  scoped_refptr<const NonInterpolableList> underlying_list_;
};

class InheritedSizeListChecker
    : public CSSInterpolationType::CSSConversionChecker {
 public:
  ~InheritedSizeListChecker() final = default;

  static std::unique_ptr<InheritedSizeListChecker> Create(
      const CSSProperty& property,
      const SizeList& inherited_size_list) {
    return base::WrapUnique(
        new InheritedSizeListChecker(property, inherited_size_list));
  }

 private:
  InheritedSizeListChecker(const CSSProperty& property,
                           const SizeList& inherited_size_list)
      : property_(property), inherited_size_list_(inherited_size_list) {}

  bool IsValid(const StyleResolverState& state,
               const InterpolationValue&) const final {
    return inherited_size_list_ == SizeListPropertyFunctions::GetSizeList(
                                       property_, *state.ParentStyle());
  }

  const CSSProperty& property_;
  SizeList inherited_size_list_;
};

InterpolationValue ConvertSizeList(const SizeList& size_list, float zoom) {
  // Flatten pairs of width/height into individual items, even for contain and
  // cover keywords.
  return ListInterpolationFunctions::CreateList(
      size_list.size() * 2,
      [&size_list, zoom](size_t index) -> InterpolationValue {
        bool convert_width = index % 2 == 0;
        return SizeInterpolationFunctions::ConvertFillSizeSide(
            size_list[index / 2], zoom, convert_width);
      });
}

InterpolationValue MaybeConvertCSSSizeList(const CSSValue& value) {
  // CSSPropertyParser doesn't put single values in lists so wrap it up in a
  // temporary list.
  const CSSValueList* list = nullptr;
  if (!value.IsBaseValueList()) {
    CSSValueList* temp_list = CSSValueList::CreateCommaSeparated();
    temp_list->Append(value);
    list = temp_list;
  } else {
    list = ToCSSValueList(&value);
  }

  // Flatten pairs of width/height into individual items, even for contain and
  // cover keywords.
  return ListInterpolationFunctions::CreateList(
      list->length() * 2, [list](size_t index) -> InterpolationValue {
        const CSSValue& css_size = list->Item(index / 2);
        bool convert_width = index % 2 == 0;
        return SizeInterpolationFunctions::MaybeConvertCSSSizeSide(
            css_size, convert_width);
      });
}

InterpolationValue CSSSizeListInterpolationType::MaybeConvertNeutral(
    const InterpolationValue& underlying,
    ConversionCheckers& conversion_checkers) const {
  const auto& underlying_list =
      ToNonInterpolableList(*underlying.non_interpolable_value);
  conversion_checkers.push_back(
      UnderlyingSizeListChecker::Create(underlying_list));
  return ListInterpolationFunctions::CreateList(
      underlying_list.length(), [&underlying_list](size_t index) {
        return SizeInterpolationFunctions::CreateNeutralValue(
            underlying_list.Get(index));
      });
}

InterpolationValue CSSSizeListInterpolationType::MaybeConvertInitial(
    const StyleResolverState&,
    ConversionCheckers&) const {
  return ConvertSizeList(
      SizeListPropertyFunctions::GetInitialSizeList(CssProperty()), 1);
}

InterpolationValue CSSSizeListInterpolationType::MaybeConvertInherit(
    const StyleResolverState& state,
    ConversionCheckers& conversion_checkers) const {
  SizeList inherited_size_list = SizeListPropertyFunctions::GetSizeList(
      CssProperty(), *state.ParentStyle());
  conversion_checkers.push_back(
      InheritedSizeListChecker::Create(CssProperty(), inherited_size_list));
  return ConvertSizeList(inherited_size_list, state.Style()->EffectiveZoom());
}

InterpolationValue CSSSizeListInterpolationType::MaybeConvertValue(
    const CSSValue& value,
    const StyleResolverState*,
    ConversionCheckers&) const {
  return MaybeConvertCSSSizeList(value);
}

PairwiseInterpolationValue CSSSizeListInterpolationType::MaybeMergeSingles(
    InterpolationValue&& start,
    InterpolationValue&& end) const {
  return ListInterpolationFunctions::MaybeMergeSingles(
      std::move(start), std::move(end),
      ListInterpolationFunctions::LengthMatchingStrategy::kLowestCommonMultiple,
      SizeInterpolationFunctions::MaybeMergeSingles);
}

InterpolationValue
CSSSizeListInterpolationType::MaybeConvertStandardPropertyUnderlyingValue(
    const ComputedStyle& style) const {
  return ConvertSizeList(
      SizeListPropertyFunctions::GetSizeList(CssProperty(), style),
      style.EffectiveZoom());
}

void CSSSizeListInterpolationType::Composite(
    UnderlyingValueOwner& underlying_value_owner,
    double underlying_fraction,
    const InterpolationValue& value,
    double interpolation_fraction) const {
  ListInterpolationFunctions::Composite(
      underlying_value_owner, underlying_fraction, *this, value,
      ListInterpolationFunctions::LengthMatchingStrategy::kLowestCommonMultiple,

      SizeInterpolationFunctions::NonInterpolableValuesAreCompatible,
      SizeInterpolationFunctions::Composite);
}

void CSSSizeListInterpolationType::ApplyStandardPropertyValue(
    const InterpolableValue& interpolable_value,
    const NonInterpolableValue* non_interpolable_value,
    StyleResolverState& state) const {
  const auto& interpolable_list = ToInterpolableList(interpolable_value);
  const auto& non_interpolable_list =
      ToNonInterpolableList(*non_interpolable_value);
  size_t length = interpolable_list.length();
  DCHECK_EQ(length, non_interpolable_list.length());
  DCHECK_EQ(length % 2, 0ul);
  size_t size_list_length = length / 2;
  SizeList size_list(size_list_length);
  for (size_t i = 0; i < size_list_length; i++) {
    size_list[i] = SizeInterpolationFunctions::CreateFillSize(
        *interpolable_list.Get(i * 2), non_interpolable_list.Get(i * 2),
        *interpolable_list.Get(i * 2 + 1), non_interpolable_list.Get(i * 2 + 1),
        state.CssToLengthConversionData());
  }
  SizeListPropertyFunctions::SetSizeList(CssProperty(), *state.Style(),
                                         size_list);
}

}  // namespace blink
