/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/css/css_selector_list.h"

#include <memory>
#include <vector>
#include "third_party/blink/renderer/core/css/parser/css_parser_selector.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace {
// CSSSelector is one of the top types that consume renderer memory,
// so instead of using the |WTF_HEAP_PROFILER_TYPE_NAME| macro in the
// allocations below, pass this type name constant to allow profiling
// in official builds.
const char kCSSSelectorTypeName[] = "blink::CSSSelector";
}

namespace blink {

CSSSelectorList CSSSelectorList::Copy() const {
  CSSSelectorList list;

  unsigned length = this->ComputeLength();
  list.selector_array_ =
      reinterpret_cast<CSSSelector*>(WTF::Partitions::FastMalloc(
          WTF::Partitions::ComputeAllocationSize(length, sizeof(CSSSelector)),
          kCSSSelectorTypeName));
  for (unsigned i = 0; i < length; ++i)
    new (&list.selector_array_[i]) CSSSelector(selector_array_[i]);

  return list;
}

CSSSelectorList CSSSelectorList::ConcatenatePseudoMatchesExpansion(
    const CSSSelectorList& expanded,
    const CSSSelectorList& original) {
  unsigned expanded_length = expanded.ComputeLength();
  unsigned original_length = original.ComputeLength();
  unsigned total_length = expanded_length + original_length;

  CSSSelectorList list;
  list.selector_array_ = reinterpret_cast<CSSSelector*>(
      WTF::Partitions::FastMalloc(WTF::Partitions::ComputeAllocationSize(
                                      total_length, sizeof(CSSSelector)),
                                  kCSSSelectorTypeName));

  unsigned list_index = 0;
  for (unsigned i = 0; i < expanded_length; ++i) {
    new (&list.selector_array_[list_index])
        CSSSelector(expanded.selector_array_[i]);
    ++list_index;
  }
  DCHECK(list.selector_array_[list_index - 1].IsLastInOriginalList());
  DCHECK(list.selector_array_[list_index - 1].IsLastInSelectorList());
  list.selector_array_[list_index - 1].SetLastInSelectorList(false);
  for (unsigned i = 0; i < original_length; ++i) {
    new (&list.selector_array_[list_index])
        CSSSelector(original.selector_array_[i]);
    ++list_index;
  }
  DCHECK(list.selector_array_[list_index - 1].IsLastInOriginalList());
  DCHECK(list.selector_array_[list_index - 1].IsLastInSelectorList());
  return list;
}

std::vector<const CSSSelector*> SelectorBoundaries(
    const CSSSelectorList& list) {
  std::vector<const CSSSelector*> result;
  for (const CSSSelector* s = list.First(); s; s = list.Next(*s)) {
    result.push_back(s);
  }
  result.push_back(list.First() + list.ComputeLength());
  return result;
}

void AddToList(CSSSelector*& destination,
               const CSSSelector* begin,
               const CSSSelector* end) {
  for (const CSSSelector* current = begin; current != end; ++current) {
    new (destination) CSSSelector(*current);
    destination->SetLastInSelectorList(false);
    destination->SetLastInOriginalList(false);
    destination++;
  }
}

void AddToList(CSSSelector*& destination,
               const CSSSelector* begin,
               const CSSSelector* end,
               CSSSelector::RelationType relation,
               bool IsLastInTagHistory) {
  for (const CSSSelector* current = begin; current != end; ++current) {
    new (destination) CSSSelector(*current);
    DCHECK_EQ(current + 1 == end, current->IsLastInTagHistory());
    if (current->IsLastInTagHistory()) {
      destination->SetRelation(relation);
      if (!IsLastInTagHistory)
        destination->SetLastInTagHistory(false);
    }
    destination->SetLastInSelectorList(false);
    destination->SetLastInOriginalList(false);
    destination++;
  }
}

CSSSelectorList CSSSelectorList::ExpandedFirstMatchesPseudo() const {
  unsigned original_length = this->ComputeLength();
  std::vector<const CSSSelector*> matches_boundaries =
      SelectorBoundaries(*this);

  size_t i = 0;
  while (!matches_boundaries[i]->HasPseudoMatches()) {
    ++i;
  }
  const CSSSelector* selector_with_matches_begin = matches_boundaries[i];
  const CSSSelector* selector_with_matches_end = matches_boundaries[i + 1];
  size_t selector_with_matches_length =
      selector_with_matches_end - selector_with_matches_begin;

  const CSSSelector* simple_matches = selector_with_matches_begin;
  while (simple_matches->GetPseudoType() != CSSSelector::kPseudoMatches) {
    simple_matches = simple_matches->TagHistory();
  }

  unsigned inner_matches_length =
      simple_matches->SelectorList()->ComputeLength();
  std::vector<const CSSSelector*> matches_arg_boundaries =
      SelectorBoundaries(*simple_matches->SelectorList());

  size_t num_matches_args = matches_arg_boundaries.size() - 1;
  unsigned other_selectors_length =
      original_length - selector_with_matches_length;

  unsigned expanded_matches_length =
      (selector_with_matches_length - 1) * num_matches_args +
      inner_matches_length + other_selectors_length;

  // Do not perform expansion if the selector list size is too large to create
  // RuleData
  if (expanded_matches_length > 8192)
    return CSSSelectorList();

  CSSSelectorList list;
  list.selector_array_ =
      reinterpret_cast<CSSSelector*>(WTF::Partitions::FastMalloc(
          WTF::Partitions::ComputeAllocationSize(expanded_matches_length,
                                                 sizeof(CSSSelector)),
          kCSSSelectorTypeName));

  CSSSelector* destination = list.selector_array_;

  AddToList(destination, matches_boundaries[0], selector_with_matches_begin);
  for (size_t i = 0; i < num_matches_args; ++i) {
    AddToList(destination, selector_with_matches_begin, simple_matches);
    AddToList(destination, matches_arg_boundaries[i],
              matches_arg_boundaries[i + 1], simple_matches->Relation(),
              simple_matches->IsLastInTagHistory());
    AddToList(destination, simple_matches + 1, selector_with_matches_end);
  }
  AddToList(destination, selector_with_matches_end, matches_boundaries.back());

  DCHECK(destination == list.selector_array_ + expanded_matches_length);

  list.selector_array_[expanded_matches_length - 1].SetLastInOriginalList(true);
  list.selector_array_[expanded_matches_length - 1].SetLastInSelectorList(true);

  return list;
}

CSSSelectorList CSSSelectorList::TransformForPseudoMatches() {
  DCHECK_GT(this->ComputeLength(), 0u);
  DCHECK(
      this->selector_array_[this->ComputeLength() - 1].IsLastInOriginalList());
  DCHECK(this->HasPseudoMatches());

  // Append the expanded form of matches to the original selector list
  CSSSelectorList transformed = this->Copy();
  do {
    transformed = transformed.ExpandedFirstMatchesPseudo();
  } while (transformed.HasPseudoMatches());

  if (transformed.ComputeLength() == 0)
    return CSSSelectorList();
  return CSSSelectorList::ConcatenatePseudoMatchesExpansion(transformed, *this);
}

bool CSSSelectorList::HasPseudoMatches() const {
  for (const CSSSelector* s = FirstForCSSOM(); s; s = Next(*s)) {
    if (s->HasPseudoMatches())
      return true;
  }
  return false;
}

CSSSelectorList CSSSelectorList::AdoptSelectorVector(
    Vector<std::unique_ptr<CSSParserSelector>>& selector_vector) {
  size_t flattened_size = 0;
  for (size_t i = 0; i < selector_vector.size(); ++i) {
    for (CSSParserSelector* selector = selector_vector[i].get(); selector;
         selector = selector->TagHistory())
      ++flattened_size;
  }
  DCHECK(flattened_size);

  CSSSelectorList list;
  list.selector_array_ = reinterpret_cast<CSSSelector*>(
      WTF::Partitions::FastMalloc(WTF::Partitions::ComputeAllocationSize(
                                      flattened_size, sizeof(CSSSelector)),
                                  kCSSSelectorTypeName));
  size_t array_index = 0;
  for (size_t i = 0; i < selector_vector.size(); ++i) {
    CSSParserSelector* current = selector_vector[i].get();
    while (current) {
      // Move item from the parser selector vector into selector_array_ without
      // invoking destructor (Ugh.)
      CSSSelector* current_selector = current->ReleaseSelector().release();
      memcpy(&list.selector_array_[array_index], current_selector,
             sizeof(CSSSelector));
      WTF::Partitions::FastFree(current_selector);

      current = current->TagHistory();
      DCHECK(!list.selector_array_[array_index].IsLastInSelectorList());
      if (current)
        list.selector_array_[array_index].SetLastInTagHistory(false);
      ++array_index;
    }
    DCHECK(list.selector_array_[array_index - 1].IsLastInTagHistory());
  }
  DCHECK_EQ(flattened_size, array_index);
  list.selector_array_[array_index - 1].SetLastInSelectorList(true);
  list.selector_array_[array_index - 1].SetLastInOriginalList(true);
  selector_vector.clear();

  return list;
}

const CSSSelector* CSSSelectorList::FirstForCSSOM() const {
  const CSSSelector* s = this->First();
  if (!s)
    return nullptr;
  while (this->Next(*s))
    s = this->Next(*s);
  if (this->NextInFullList(*s))
    return this->NextInFullList(*s);
  return this->First();
}

unsigned CSSSelectorList::ComputeLength() const {
  if (!selector_array_)
    return 0;
  CSSSelector* current = selector_array_;
  while (!current->IsLastInSelectorList())
    ++current;
  return (current - selector_array_) + 1;
}

void CSSSelectorList::DeleteSelectors() {
  DCHECK(selector_array_);

  bool finished = false;
  for (CSSSelector* s = selector_array_; !finished; ++s) {
    finished = s->IsLastInSelectorList();
    s->~CSSSelector();
  }

  WTF::Partitions::FastFree(selector_array_);
}

String CSSSelectorList::SelectorsText() const {
  StringBuilder result;

  for (const CSSSelector* s = FirstForCSSOM(); s; s = Next(*s)) {
    if (s != FirstForCSSOM())
      result.Append(", ");
    result.Append(s->SelectorText());
  }

  return result.ToString();
}

}  // namespace blink
