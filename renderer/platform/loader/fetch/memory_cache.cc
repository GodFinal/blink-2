/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loading_log.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin_hash.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"
#include "third_party/blink/renderer/platform/wtf/auto_reset.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

static Persistent<MemoryCache>* g_memory_cache;

static const unsigned kCDefaultCacheCapacity = 8192 * 1024;
static const int kCMinDelayBeforeLiveDecodedPrune = 1;  // Seconds.
static const double kCMaxPruneDeferralDelay = 0.5;      // Seconds.

// Percentage of capacity toward which we prune, to avoid immediately pruning
// again.
static const float kCTargetPrunePercentage = .95f;

MemoryCache* GetMemoryCache() {
  DCHECK(WTF::IsMainThread());
  if (!g_memory_cache)
    g_memory_cache = new Persistent<MemoryCache>(MemoryCache::Create());
  return g_memory_cache->Get();
}

MemoryCache* ReplaceMemoryCacheForTesting(MemoryCache* cache) {
  GetMemoryCache();
  MemoryCache* old_cache = g_memory_cache->Release();
  *g_memory_cache = cache;
  MemoryCacheDumpProvider::Instance()->SetMemoryCache(cache);
  return old_cache;
}

void MemoryCacheEntry::Trace(blink::Visitor* visitor) {
  visitor->template RegisterWeakMembers<MemoryCacheEntry,
                                        &MemoryCacheEntry::ClearResourceWeak>(
      this);
}

void MemoryCacheEntry::ClearResourceWeak(Visitor* visitor) {
  if (!resource_ || ThreadHeap::IsHeapObjectAlive(resource_))
    return;
  GetMemoryCache()->Remove(resource_.Get());
  resource_.Clear();
}

inline MemoryCache::MemoryCache()
    : in_prune_resources_(false),
      prune_pending_(false),
      max_prune_deferral_delay_(kCMaxPruneDeferralDelay),
      prune_time_stamp_(0.0),
      prune_frame_time_stamp_(0.0),
      last_frame_paint_time_stamp_(0.0),
      capacity_(kCDefaultCacheCapacity),
      delay_before_live_decoded_prune_(kCMinDelayBeforeLiveDecodedPrune),
      size_(0) {
  MemoryCacheDumpProvider::Instance()->SetMemoryCache(this);
  if (MemoryCoordinator::IsLowEndDevice())
    MemoryCoordinator::Instance().RegisterClient(this);
}

MemoryCache* MemoryCache::Create() {
  return new MemoryCache;
}

MemoryCache::~MemoryCache() {
  if (prune_pending_)
    Platform::Current()->CurrentThread()->RemoveTaskObserver(this);
}

void MemoryCache::Trace(blink::Visitor* visitor) {
  visitor->Trace(resource_maps_);
  MemoryCacheDumpClient::Trace(visitor);
  MemoryCoordinatorClient::Trace(visitor);
}

KURL MemoryCache::RemoveFragmentIdentifierIfNeeded(const KURL& original_url) {
  if (!original_url.HasFragmentIdentifier())
    return original_url;
  // Strip away fragment identifier from HTTP URLs. Data URLs must be
  // unmodified. For file and custom URLs clients may expect resources to be
  // unique even when they differ by the fragment identifier only.
  if (!original_url.ProtocolIsInHTTPFamily())
    return original_url;
  KURL url = original_url;
  url.RemoveFragmentIdentifier();
  return url;
}

String MemoryCache::DefaultCacheIdentifier() {
  return g_empty_string;
}

MemoryCache::ResourceMap* MemoryCache::EnsureResourceMap(
    const String& cache_identifier) {
  if (!resource_maps_.Contains(cache_identifier)) {
    ResourceMapIndex::AddResult result =
        resource_maps_.insert(cache_identifier, new ResourceMap);
    CHECK(result.is_new_entry);
  }
  return resource_maps_.at(cache_identifier);
}

void MemoryCache::Add(Resource* resource) {
  DCHECK(resource);
  ResourceMap* resources = EnsureResourceMap(resource->CacheIdentifier());
  AddInternal(resources, MemoryCacheEntry::Create(resource));
  RESOURCE_LOADING_DVLOG(1)
      << "MemoryCache::add Added " << resource->Url().GetString()
      << ", resource " << resource;
}

void MemoryCache::AddInternal(ResourceMap* resource_map,
                              MemoryCacheEntry* entry) {
  DCHECK(WTF::IsMainThread());
  DCHECK(resource_map);

  Resource* resource = entry->GetResource();
  if (!resource)
    return;
  DCHECK(resource->Url().IsValid());

  KURL url = RemoveFragmentIdentifierIfNeeded(resource->Url());
  ResourceMap::iterator it = resource_map->find(url);
  if (it != resource_map->end()) {
    Resource* old_resource = it->value->GetResource();
    CHECK_NE(old_resource, resource);
    Update(old_resource, old_resource->size(), 0);
  }
  resource_map->Set(url, entry);
  Update(resource, 0, resource->size());
}

void MemoryCache::Remove(Resource* resource) {
  DCHECK(WTF::IsMainThread());
  DCHECK(resource);
  // Resources can be created with garbage urls in error cases. These Resources
  // should never be added to the cache (AddInternal() DCHECKs that the url is
  // valid). Null urls will crash if we attempt to hash them, so early exit.
  if (resource->Url().IsNull())
    return;

  RESOURCE_LOADING_DVLOG(1) << "Evicting resource " << resource << " for "
                            << resource->Url().GetString() << " from cache";
  TRACE_EVENT1("blink", "MemoryCache::evict", "resource",
               resource->Url().GetString().Utf8());

  ResourceMap* resources = resource_maps_.at(resource->CacheIdentifier());
  if (!resources)
    return;

  KURL url = RemoveFragmentIdentifierIfNeeded(resource->Url());
  ResourceMap::iterator it = resources->find(url);
  if (it == resources->end() || it->value->GetResource() != resource)
    return;
  RemoveInternal(resources, it);
}

void MemoryCache::RemoveInternal(ResourceMap* resource_map,
                                 const ResourceMap::iterator& it) {
  DCHECK(WTF::IsMainThread());
  DCHECK(resource_map);

  Resource* resource = it->value->GetResource();
  DCHECK(resource);

  Update(resource, resource->size(), 0);
  resource_map->erase(it);
}

bool MemoryCache::Contains(const Resource* resource) const {
  if (!resource || resource->Url().IsEmpty())
    return false;
  const ResourceMap* resources = resource_maps_.at(resource->CacheIdentifier());
  if (!resources)
    return false;
  KURL url = RemoveFragmentIdentifierIfNeeded(resource->Url());
  MemoryCacheEntry* entry = resources->at(url);
  return entry && resource == entry->GetResource();
}

Resource* MemoryCache::ResourceForURL(const KURL& resource_url) const {
  return ResourceForURL(resource_url, DefaultCacheIdentifier());
}

Resource* MemoryCache::ResourceForURL(const KURL& resource_url,
                                      const String& cache_identifier) const {
  DCHECK(WTF::IsMainThread());
  if (!resource_url.IsValid() || resource_url.IsNull())
    return nullptr;
  DCHECK(!cache_identifier.IsNull());
  const ResourceMap* resources = resource_maps_.at(cache_identifier);
  if (!resources)
    return nullptr;
  MemoryCacheEntry* entry =
      resources->at(RemoveFragmentIdentifierIfNeeded(resource_url));
  if (!entry)
    return nullptr;
  return entry->GetResource();
}

HeapVector<Member<Resource>> MemoryCache::ResourcesForURL(
    const KURL& resource_url) const {
  DCHECK(WTF::IsMainThread());
  KURL url = RemoveFragmentIdentifierIfNeeded(resource_url);
  HeapVector<Member<Resource>> results;
  for (const auto& resource_map_iter : resource_maps_) {
    if (MemoryCacheEntry* entry = resource_map_iter.value->at(url)) {
      Resource* resource = entry->GetResource();
      DCHECK(resource);
      results.push_back(resource);
    }
  }
  return results;
}

void MemoryCache::PruneResources(PruneStrategy strategy) {
  DCHECK(!prune_pending_);
  const size_t size_limit = (strategy == kMaximalPrune) ? 0 : Capacity();
  if (size_ <= size_limit)
    return;

  // Cut by a percentage to avoid immediately pruning again.
  size_t target_size =
      static_cast<size_t>(size_limit * kCTargetPrunePercentage);

  for (const auto& resource_map_iter : resource_maps_) {
    for (const auto& resource_iter : *resource_map_iter.value) {
      Resource* resource = resource_iter.value->GetResource();
      DCHECK(resource);
      if (resource->IsLoaded() && resource->DecodedSize()) {
        // Check to see if the remaining resources are too new to prune.
        double elapsed_time = prune_frame_time_stamp_ -
                              resource_iter.value->last_decoded_access_time_;
        if (strategy == kAutomaticPrune &&
            elapsed_time < delay_before_live_decoded_prune_)
          continue;
        resource->Prune();
        if (size_ <= target_size)
          return;
      }
    }
  }
}

void MemoryCache::SetCapacity(size_t total_bytes) {
  capacity_ = total_bytes;
  Prune();
}

void MemoryCache::Update(Resource* resource, size_t old_size, size_t new_size) {
  if (!Contains(resource))
    return;
  ptrdiff_t delta = new_size - old_size;
  DCHECK(delta >= 0 || size_ >= static_cast<size_t>(-delta));
  size_ += delta;
}

void MemoryCache::RemoveURLFromCache(const KURL& url) {
  HeapVector<Member<Resource>> resources = ResourcesForURL(url);
  for (Resource* resource : resources)
    Remove(resource);
}

void MemoryCache::TypeStatistic::AddResource(Resource* o) {
  count++;
  size += o->size();
  decoded_size += o->DecodedSize();
  encoded_size += o->EncodedSize();
  overhead_size += o->OverheadSize();
  encoded_size_duplicated_in_data_urls +=
      o->Url().ProtocolIsData() ? o->EncodedSize() : 0;
}

MemoryCache::Statistics MemoryCache::GetStatistics() const {
  Statistics stats;
  for (const auto& resource_map_iter : resource_maps_) {
    for (const auto& resource_iter : *resource_map_iter.value) {
      Resource* resource = resource_iter.value->GetResource();
      DCHECK(resource);
      switch (resource->GetType()) {
        case Resource::kImage:
          stats.images.AddResource(resource);
          break;
        case Resource::kCSSStyleSheet:
          stats.css_style_sheets.AddResource(resource);
          break;
        case Resource::kScript:
          stats.scripts.AddResource(resource);
          break;
        case Resource::kXSLStyleSheet:
          stats.xsl_style_sheets.AddResource(resource);
          break;
        case Resource::kFont:
          stats.fonts.AddResource(resource);
          break;
        default:
          stats.other.AddResource(resource);
          break;
      }
    }
  }
  return stats;
}

void MemoryCache::EvictResources(EvictResourcePolicy policy) {
  for (auto resource_map_iter = resource_maps_.begin();
       resource_map_iter != resource_maps_.end();) {
    ResourceMap* resources = resource_map_iter->value.Get();
    HeapVector<Member<MemoryCacheEntry>> unused_preloads;
    for (auto resource_iter = resources->begin();
         resource_iter != resources->end();
         resource_iter = resources->begin()) {
      DCHECK(resource_iter.Get());
      DCHECK(resource_iter->value.Get());
      DCHECK(resource_iter->value->GetResource());
      Resource* resource = resource_iter->value->GetResource();
      DCHECK(resource);
      if (policy != kEvictAllResources && resource->IsUnusedPreload()) {
        // Store unused preloads aside, so they could be added back later.
        // That is in order to avoid the performance impact of iterating over
        // the same resource multiple times.
        unused_preloads.push_back(resource_iter->value.Get());
      }
      RemoveInternal(resources, resource_iter);
    }
    for (const auto& unused_preload : unused_preloads) {
      AddInternal(resources, unused_preload);
    }
    // We may iterate multiple times over resourceMaps with unused preloads.
    // That's extremely unlikely to have any real-life performance impact.
    if (!resources->size()) {
      resource_maps_.erase(resource_map_iter);
      resource_map_iter = resource_maps_.begin();
    } else {
      ++resource_map_iter;
    }
  }
}

void MemoryCache::Prune() {
  TRACE_EVENT0("renderer", "MemoryCache::prune()");

  if (in_prune_resources_)
    return;
  if (size_ <= capacity_)  // Fast path.
    return;

  // To avoid burdening the current thread with repetitive pruning jobs, pruning
  // is postponed until the end of the current task. If it has been more than
  // m_maxPruneDeferralDelay since the last prune, then we prune immediately. If
  // the current thread's run loop is not active, then pruning will happen
  // immediately only if it has been over m_maxPruneDeferralDelay since the last
  // prune.
  double current_time = WTF::CurrentTime();
  if (prune_pending_) {
    if (current_time - prune_time_stamp_ >= max_prune_deferral_delay_) {
      PruneNow(current_time, kAutomaticPrune);
    }
  } else {
    if (current_time - prune_time_stamp_ >= max_prune_deferral_delay_) {
      PruneNow(current_time, kAutomaticPrune);  // Delay exceeded, prune now.
    } else {
      // Defer.
      Platform::Current()->CurrentThread()->AddTaskObserver(this);
      prune_pending_ = true;
    }
  }
}

void MemoryCache::WillProcessTask() {}

void MemoryCache::DidProcessTask() {
  // Perform deferred pruning
  DCHECK(prune_pending_);
  PruneNow(WTF::CurrentTime(), kAutomaticPrune);
}

void MemoryCache::PruneAll() {
  double current_time = WTF::CurrentTime();
  PruneNow(current_time, kMaximalPrune);
}

void MemoryCache::PruneNow(double current_time, PruneStrategy strategy) {
  if (prune_pending_) {
    prune_pending_ = false;
    Platform::Current()->CurrentThread()->RemoveTaskObserver(this);
  }

  AutoReset<bool> reentrancy_protector(&in_prune_resources_, true);

  PruneResources(strategy);
  prune_frame_time_stamp_ = last_frame_paint_time_stamp_;
  prune_time_stamp_ = current_time;
}

void MemoryCache::UpdateFramePaintTimestamp() {
  last_frame_paint_time_stamp_ = CurrentTime();
}

bool MemoryCache::OnMemoryDump(WebMemoryDumpLevelOfDetail level_of_detail,
                               WebProcessMemoryDump* memory_dump) {
  if (level_of_detail == WebMemoryDumpLevelOfDetail::kBackground) {
    Statistics stats = GetStatistics();
    WebMemoryAllocatorDump* dump1 =
        memory_dump->CreateMemoryAllocatorDump("web_cache/Image_resources");
    dump1->AddScalar("size", "bytes",
                     stats.images.encoded_size + stats.images.overhead_size);
    WebMemoryAllocatorDump* dump2 = memory_dump->CreateMemoryAllocatorDump(
        "web_cache/CSS stylesheet_resources");
    dump2->AddScalar("size", "bytes",
                     stats.css_style_sheets.encoded_size +
                         stats.css_style_sheets.overhead_size);
    WebMemoryAllocatorDump* dump3 =
        memory_dump->CreateMemoryAllocatorDump("web_cache/Script_resources");
    dump3->AddScalar("size", "bytes",
                     stats.scripts.encoded_size + stats.scripts.overhead_size);
    WebMemoryAllocatorDump* dump4 = memory_dump->CreateMemoryAllocatorDump(
        "web_cache/XSL stylesheet_resources");
    dump4->AddScalar("size", "bytes",
                     stats.xsl_style_sheets.encoded_size +
                         stats.xsl_style_sheets.overhead_size);
    WebMemoryAllocatorDump* dump5 =
        memory_dump->CreateMemoryAllocatorDump("web_cache/Font_resources");
    dump5->AddScalar("size", "bytes",
                     stats.fonts.encoded_size + stats.fonts.overhead_size);
    WebMemoryAllocatorDump* dump6 =
        memory_dump->CreateMemoryAllocatorDump("web_cache/Other_resources");
    dump6->AddScalar("size", "bytes",
                     stats.other.encoded_size + stats.other.overhead_size);
    return true;
  }

  for (const auto& resource_map_iter : resource_maps_) {
    for (const auto& resource_iter : *resource_map_iter.value) {
      Resource* resource = resource_iter.value->GetResource();
      resource->OnMemoryDump(level_of_detail, memory_dump);
    }
  }
  return true;
}

void MemoryCache::OnMemoryPressure(WebMemoryPressureLevel level) {
  PruneAll();
}

}  // namespace blink
