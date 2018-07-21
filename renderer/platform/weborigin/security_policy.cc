/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Google, Inc. ("Google") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/weborigin/security_policy.h"

#include <memory>
#include "third_party/blink/public/platform/web_referrer_policy.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/origin_access_entry.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/parsing_utilities.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

namespace blink {

using OriginAccessList = Vector<OriginAccessEntry>;
using OriginAccessMap = HashMap<String, std::unique_ptr<OriginAccessList>>;
using OriginSet = HashSet<String>;

static OriginAccessMap& GetOriginAccessWhitelistMap() {
  DEFINE_STATIC_LOCAL(OriginAccessMap, origin_access_whitelist_map, ());
  return origin_access_whitelist_map;
}

static OriginAccessMap& GetOriginAccessBlacklistMap() {
  DEFINE_STATIC_LOCAL(OriginAccessMap, origin_access_blacklist_map, ());
  return origin_access_blacklist_map;
}

static OriginSet& TrustworthyOriginSet() {
  DEFINE_STATIC_LOCAL(OriginSet, trustworthy_origin_set, ());
  return trustworthy_origin_set;
}

static void AddOriginAccessEntry(const SecurityOrigin& source_origin,
                                 const String& destination_protocol,
                                 const String& destination_domain,
                                 bool allow_destination_subdomains,
                                 OriginAccessMap& access_map) {
  DCHECK(IsMainThread());
  DCHECK(!source_origin.IsUnique());
  if (source_origin.IsUnique())
    return;

  String source_string = source_origin.ToString();
  OriginAccessMap::AddResult result = access_map.insert(source_string, nullptr);
  if (result.is_new_entry)
    result.stored_value->value = std::make_unique<OriginAccessList>();

  OriginAccessList* list = result.stored_value->value.get();
  list->push_back(OriginAccessEntry(
      destination_protocol, destination_domain,
      allow_destination_subdomains ? OriginAccessEntry::kAllowSubdomains
                                   : OriginAccessEntry::kDisallowSubdomains));
}

static void RemoveOriginAccessEntry(const SecurityOrigin& source_origin,
                                    const String& destination_protocol,
                                    const String& destination_domain,
                                    bool allow_destination_subdomains,
                                    OriginAccessMap& access_map) {
  DCHECK(IsMainThread());
  DCHECK(!source_origin.IsUnique());
  if (source_origin.IsUnique())
    return;

  String source_string = source_origin.ToString();
  OriginAccessMap::iterator it = access_map.find(source_string);
  if (it == access_map.end())
    return;

  OriginAccessList* list = it->value.get();
  size_t index = list->Find(OriginAccessEntry(
      destination_protocol, destination_domain,
      allow_destination_subdomains ? OriginAccessEntry::kAllowSubdomains
                                   : OriginAccessEntry::kDisallowSubdomains));

  if (index == kNotFound)
    return;

  list->EraseAt(index);

  if (list->IsEmpty())
    access_map.erase(it);
}

static bool IsOriginPairInAccessMap(const SecurityOrigin* active_origin,
                                    const SecurityOrigin* target_origin,
                                    const OriginAccessMap& access_map) {
  if (access_map.IsEmpty())
    return false;

  if (OriginAccessList* list = access_map.at(active_origin->ToString())) {
    for (size_t i = 0; i < list->size(); ++i) {
      if (list->at(i).MatchesOrigin(*target_origin) !=
          OriginAccessEntry::kDoesNotMatchOrigin)
        return true;
    }
  }
  return false;
}

void SecurityPolicy::Init() {
  GetOriginAccessWhitelistMap();
  GetOriginAccessBlacklistMap();
  TrustworthyOriginSet();
}

bool SecurityPolicy::ShouldHideReferrer(const KURL& url, const KURL& referrer) {
  bool referrer_is_secure_url = referrer.ProtocolIs("https");
  bool scheme_is_allowed =
      SchemeRegistry::ShouldTreatURLSchemeAsAllowedForReferrer(
          referrer.Protocol());

  if (!scheme_is_allowed)
    return true;

  if (!referrer_is_secure_url)
    return false;

  bool url_is_secure_url = url.ProtocolIs("https");

  return !url_is_secure_url;
}

Referrer SecurityPolicy::GenerateReferrer(ReferrerPolicy referrer_policy,
                                          const KURL& url,
                                          const String& referrer) {
  ReferrerPolicy referrer_policy_no_default = referrer_policy;
  if (referrer_policy_no_default == kReferrerPolicyDefault) {
    if (RuntimeEnabledFeatures::ReducedReferrerGranularityEnabled()) {
      referrer_policy_no_default = kReferrerPolicyStrictOriginWhenCrossOrigin;
    } else {
      referrer_policy_no_default = kReferrerPolicyNoReferrerWhenDowngrade;
    }
  }
  if (referrer == Referrer::NoReferrer())
    return Referrer(Referrer::NoReferrer(), referrer_policy_no_default);
  DCHECK(!referrer.IsEmpty());

  KURL referrer_url = KURL(NullURL(), referrer);
  String scheme = referrer_url.Protocol();
  if (!SchemeRegistry::ShouldTreatURLSchemeAsAllowedForReferrer(scheme))
    return Referrer(Referrer::NoReferrer(), referrer_policy_no_default);

  if (SecurityOrigin::ShouldUseInnerURL(url))
    return Referrer(Referrer::NoReferrer(), referrer_policy_no_default);

  switch (referrer_policy_no_default) {
    case kReferrerPolicyNever:
      return Referrer(Referrer::NoReferrer(), referrer_policy_no_default);
    case kReferrerPolicyAlways:
      return Referrer(referrer, referrer_policy_no_default);
    case kReferrerPolicyOrigin: {
      String origin = SecurityOrigin::Create(referrer_url)->ToString();
      // A security origin is not a canonical URL as it lacks a path. Add /
      // to turn it into a canonical URL we can use as referrer.
      return Referrer(origin + "/", referrer_policy_no_default);
    }
    case kReferrerPolicyOriginWhenCrossOrigin: {
      scoped_refptr<const SecurityOrigin> referrer_origin =
          SecurityOrigin::Create(referrer_url);
      scoped_refptr<const SecurityOrigin> url_origin =
          SecurityOrigin::Create(url);
      if (!url_origin->IsSameSchemeHostPort(referrer_origin.get())) {
        String origin = referrer_origin->ToString();
        return Referrer(origin + "/", referrer_policy_no_default);
      }
      break;
    }
    case kReferrerPolicySameOrigin: {
      scoped_refptr<const SecurityOrigin> referrer_origin =
          SecurityOrigin::Create(referrer_url);
      scoped_refptr<const SecurityOrigin> url_origin =
          SecurityOrigin::Create(url);
      if (!url_origin->IsSameSchemeHostPort(referrer_origin.get())) {
        return Referrer(Referrer::NoReferrer(), referrer_policy_no_default);
      }
      return Referrer(referrer, referrer_policy_no_default);
    }
    case kReferrerPolicyStrictOrigin: {
      String origin = SecurityOrigin::Create(referrer_url)->ToString();
      return Referrer(ShouldHideReferrer(url, referrer_url)
                          ? Referrer::NoReferrer()
                          : origin + "/",
                      referrer_policy_no_default);
    }
    case kReferrerPolicyStrictOriginWhenCrossOrigin: {
      scoped_refptr<const SecurityOrigin> referrer_origin =
          SecurityOrigin::Create(referrer_url);
      scoped_refptr<const SecurityOrigin> url_origin =
          SecurityOrigin::Create(url);
      if (!url_origin->IsSameSchemeHostPort(referrer_origin.get())) {
        String origin = referrer_origin->ToString();
        return Referrer(ShouldHideReferrer(url, referrer_url)
                            ? Referrer::NoReferrer()
                            : origin + "/",
                        referrer_policy_no_default);
      }
      break;
    }
    case kReferrerPolicyNoReferrerWhenDowngrade:
      break;
    case kReferrerPolicyDefault:
      NOTREACHED();
      break;
  }

  return Referrer(
      ShouldHideReferrer(url, referrer_url) ? Referrer::NoReferrer() : referrer,
      referrer_policy_no_default);
}

void SecurityPolicy::AddOriginTrustworthyWhiteList(
    const SecurityOrigin& origin) {
#if DCHECK_IS_ON()
  // Must be called before we start other threads.
  DCHECK(WTF::IsBeforeThreadCreated());
#endif
  if (origin.IsUnique())
    return;
  TrustworthyOriginSet().insert(origin.ToRawString());
}

bool SecurityPolicy::IsOriginWhiteListedTrustworthy(
    const SecurityOrigin& origin) {
  // Early return if there are no whitelisted origins to avoid unnecessary
  // allocations, copies, and frees.
  if (origin.IsUnique() || TrustworthyOriginSet().IsEmpty())
    return false;
  return TrustworthyOriginSet().Contains(origin.ToRawString());
}

bool SecurityPolicy::IsUrlWhiteListedTrustworthy(const KURL& url) {
  // Early return to avoid initializing the SecurityOrigin.
  if (TrustworthyOriginSet().IsEmpty())
    return false;
  return IsOriginWhiteListedTrustworthy(*SecurityOrigin::Create(url).get());
}

bool SecurityPolicy::IsAccessWhiteListed(const SecurityOrigin* active_origin,
                                         const SecurityOrigin* target_origin) {
  return IsOriginPairInAccessMap(active_origin, target_origin,
                                 GetOriginAccessWhitelistMap()) &&
         !IsOriginPairInAccessMap(active_origin, target_origin,
                                  GetOriginAccessBlacklistMap());
}

bool SecurityPolicy::IsAccessToURLWhiteListed(
    const SecurityOrigin* active_origin,
    const KURL& url) {
  scoped_refptr<const SecurityOrigin> target_origin =
      SecurityOrigin::Create(url);
  return IsAccessWhiteListed(active_origin, target_origin.get());
}

void SecurityPolicy::AddOriginAccessWhitelistEntry(
    const SecurityOrigin& source_origin,
    const String& destination_protocol,
    const String& destination_domain,
    bool allow_destination_subdomains) {
  AddOriginAccessEntry(source_origin, destination_protocol, destination_domain,
                       allow_destination_subdomains,
                       GetOriginAccessWhitelistMap());
}

void SecurityPolicy::RemoveOriginAccessWhitelistEntry(
    const SecurityOrigin& source_origin,
    const String& destination_protocol,
    const String& destination_domain,
    bool allow_destination_subdomains) {
  RemoveOriginAccessEntry(source_origin, destination_protocol,
                          destination_domain, allow_destination_subdomains,
                          GetOriginAccessWhitelistMap());
}

void SecurityPolicy::ResetOriginAccessWhitelists() {
  DCHECK(IsMainThread());
  GetOriginAccessWhitelistMap().clear();
}

void SecurityPolicy::AddOriginAccessBlacklistEntry(
    const SecurityOrigin& source_origin,
    const String& destination_protocol,
    const String& destination_domain,
    bool allow_destination_subdomains) {
  AddOriginAccessEntry(source_origin, destination_protocol, destination_domain,
                       allow_destination_subdomains,
                       GetOriginAccessBlacklistMap());
}

void SecurityPolicy::RemoveOriginAccessBlacklistEntry(
    const SecurityOrigin& source_origin,
    const String& destination_protocol,
    const String& destination_domain,
    bool allow_destination_subdomains) {
  RemoveOriginAccessEntry(source_origin, destination_protocol,
                          destination_domain, allow_destination_subdomains,
                          GetOriginAccessBlacklistMap());
}

void SecurityPolicy::ResetOriginAccessBlacklists() {
  DCHECK(IsMainThread());
  GetOriginAccessBlacklistMap().clear();
}

bool SecurityPolicy::ReferrerPolicyFromString(
    const String& policy,
    ReferrerPolicyLegacyKeywordsSupport legacy_keywords_support,
    ReferrerPolicy* result) {
  DCHECK(!policy.IsNull());
  bool support_legacy_keywords =
      (legacy_keywords_support == kSupportReferrerPolicyLegacyKeywords);

  if (EqualIgnoringASCIICase(policy, "no-referrer") ||
      (support_legacy_keywords && (EqualIgnoringASCIICase(policy, "never") ||
                                   EqualIgnoringASCIICase(policy, "none")))) {
    *result = kReferrerPolicyNever;
    return true;
  }
  if (EqualIgnoringASCIICase(policy, "unsafe-url") ||
      (support_legacy_keywords && EqualIgnoringASCIICase(policy, "always"))) {
    *result = kReferrerPolicyAlways;
    return true;
  }
  if (EqualIgnoringASCIICase(policy, "origin")) {
    *result = kReferrerPolicyOrigin;
    return true;
  }
  if (EqualIgnoringASCIICase(policy, "origin-when-cross-origin") ||
      (support_legacy_keywords &&
       EqualIgnoringASCIICase(policy, "origin-when-crossorigin"))) {
    *result = kReferrerPolicyOriginWhenCrossOrigin;
    return true;
  }
  if (EqualIgnoringASCIICase(policy, "same-origin")) {
    *result = kReferrerPolicySameOrigin;
    return true;
  }
  if (EqualIgnoringASCIICase(policy, "strict-origin")) {
    *result = kReferrerPolicyStrictOrigin;
    return true;
  }
  if (EqualIgnoringASCIICase(policy, "strict-origin-when-cross-origin")) {
    *result = kReferrerPolicyStrictOriginWhenCrossOrigin;
    return true;
  }
  if (EqualIgnoringASCIICase(policy, "no-referrer-when-downgrade") ||
      (support_legacy_keywords && EqualIgnoringASCIICase(policy, "default"))) {
    *result = kReferrerPolicyNoReferrerWhenDowngrade;
    return true;
  }
  return false;
}

namespace {

template <typename CharType>
inline bool IsASCIIAlphaOrHyphen(CharType c) {
  return IsASCIIAlpha(c) || c == '-';
}

}  // namespace

bool SecurityPolicy::ReferrerPolicyFromHeaderValue(
    const String& header_value,
    ReferrerPolicyLegacyKeywordsSupport legacy_keywords_support,
    ReferrerPolicy* result) {
  ReferrerPolicy referrer_policy = kReferrerPolicyDefault;

  Vector<String> tokens;
  header_value.Split(',', true, tokens);
  for (const auto& token : tokens) {
    ReferrerPolicy current_result;
    auto stripped_token = token.StripWhiteSpace();
    if (SecurityPolicy::ReferrerPolicyFromString(token.StripWhiteSpace(),
                                                 legacy_keywords_support,
                                                 &current_result)) {
      referrer_policy = current_result;
    } else {
      Vector<UChar> characters;
      stripped_token.AppendTo(characters);
      const UChar* position = characters.data();
      UChar* end = characters.data() + characters.size();
      SkipWhile<UChar, IsASCIIAlphaOrHyphen>(position, end);
      if (position != end)
        return false;
    }
  }

  if (referrer_policy == kReferrerPolicyDefault)
    return false;

  *result = referrer_policy;
  return true;
}

STATIC_ASSERT_ENUM(kWebReferrerPolicyAlways, kReferrerPolicyAlways);
STATIC_ASSERT_ENUM(kWebReferrerPolicyDefault, kReferrerPolicyDefault);
STATIC_ASSERT_ENUM(kWebReferrerPolicyNoReferrerWhenDowngrade,
                   kReferrerPolicyNoReferrerWhenDowngrade);
STATIC_ASSERT_ENUM(kWebReferrerPolicyNever, kReferrerPolicyNever);
STATIC_ASSERT_ENUM(kWebReferrerPolicyOrigin, kReferrerPolicyOrigin);
STATIC_ASSERT_ENUM(kWebReferrerPolicyOriginWhenCrossOrigin,
                   kReferrerPolicyOriginWhenCrossOrigin);
STATIC_ASSERT_ENUM(kWebReferrerPolicySameOrigin, kReferrerPolicySameOrigin);
STATIC_ASSERT_ENUM(kWebReferrerPolicyStrictOrigin, kReferrerPolicyStrictOrigin);
STATIC_ASSERT_ENUM(
    kWebReferrerPolicyNoReferrerWhenDowngradeOriginWhenCrossOrigin,
    kReferrerPolicyStrictOriginWhenCrossOrigin);

}  // namespace blink
