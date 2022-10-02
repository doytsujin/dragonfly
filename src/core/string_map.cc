// Copyright 2022, DragonflyDB authors.  All rights reserved.
// See LICENSE for licensing terms.
//

#include "core/string_map.h"

#include "base/endian.h"
#include "base/logging.h"
#include "core/compact_object.h"
#include "core/sds_utils.h"

extern "C" {
#include "redis/zmalloc.h"
}

using namespace std;

namespace dfly {

namespace {

sds GetValue(sds key) {
  char* from = key + sdslen(key) + 1;
  return (char*)absl::little_endian::Load64(from);
}

}  // namespace

StringMap::~StringMap() {
  Clear();
}

bool StringMap::AddOrSet(string_view field, string_view value, uint32_t ttl_sec) {
  CHECK_EQ(ttl_sec, UINT32_MAX);  // TBD

  // 8 additional bytes for a pointer to value.
  sds newkey = AllocSdsWithSpace(field.size(), 8);
  if (!field.empty()) {
    memcpy(newkey, field.data(), field.size());
  }

  sds val = sdsnewlen(value.data(), value.size());
  absl::little_endian::Store64(newkey + field.size() + 1, uint64_t(val));
  bool has_ttl = false;

  if (!AddInternal(newkey, has_ttl)) {
    ObjDelete(newkey, has_ttl);
    LOG(FATAL) << "TBD:ORSET";

    return false;
  }

  return true;
}

bool StringMap::Erase(string_view field) {
  return false;
}

bool StringMap::Contains(string_view field) const {
  // 1 - means it's string_view. See ObjEqual for details.
  return FindInternal(&field, 1) != nullptr;
}

void StringMap::Clear() {
  ClearInternal();
}

sds StringMap::Find(std::string_view key) {
  sds str = (sds)FindInternal(&key, 1);
  if (!str)
    return nullptr;

  char* valptr = str + sdslen(str) + 1;
  sds res = (sds)absl::little_endian::Load64(valptr);
  return res;
}

uint64_t StringMap::Hash(const void* obj, uint32_t cookie) const {
  DCHECK_LT(cookie, 2u);

  if (cookie == 0) {
    sds s = (sds)obj;
    return CompactObj::HashCode(string_view{s, sdslen(s)});
  }

  const string_view* sv = (const string_view*)obj;
  return CompactObj::HashCode(*sv);
}

bool StringMap::ObjEqual(const void* left, const void* right, uint32_t right_cookie) const {
  DCHECK_LT(right_cookie, 2u);

  sds s1 = (sds)left;
  if (right_cookie == 0) {
    sds s2 = (sds)right;

    if (sdslen(s1) != sdslen(s2)) {
      return false;
    }

    return sdslen(s1) == 0 || memcmp(s1, s2, sdslen(s1)) == 0;
  }

  const string_view* right_sv = (const string_view*)right;
  string_view left_sv{s1, sdslen(s1)};
  return left_sv == (*right_sv);
}

size_t StringMap::ObjectAllocSize(const void* obj) const {
  sds s1 = (sds)obj;
  size_t res = zmalloc_usable_size(sdsAllocPtr(s1));
  sds val = GetValue(s1);
  res += zmalloc_usable_size(sdsAllocPtr(val));

  return res;
}

uint32_t StringMap::ObjExpireTime(const void* obj) const {
  LOG(FATAL) << "TBD";
  return 0;
}

void StringMap::ObjDelete(void* obj, bool has_ttl) const {
  sds s1 = (sds)obj;
  sds value = GetValue(s1);
  sdsfree(value);
  sdsfree(s1);
}

}  // namespace dfly