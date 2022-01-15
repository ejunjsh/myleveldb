// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A Cache is an interface that maps keys to values.  It has internal
// synchronization and may be safely accessed concurrently from
// multiple threads.  It may automatically evict entries to make room
// for new entries.  Values have a specified charge against the cache
// capacity.  For example, a cache where the values are variable
// length strings, may use the length of the string as the charge for
// the string.
// 缓存是将键映射到值的接口。它具有内部同步，
// 可以从多个线程同时安全地访问。它可能会自动逐出条目，为新条目腾出空间。
// value具有针对缓存容量的指定charge，
// 例如缓存里面的value是一个可变长度的字符串，可以用这个字符串的长度作为charge
//
// A builtin cache implementation with a least-recently-used eviction
// policy is provided.  Clients may use their own implementations if
// they want something more sophisticated (like scan-resistance, a
// custom eviction policy, variable cache sizing, etc.)
// 提供了一个具有最近最少使用的逐出策略的内置缓存实现。
// 如果客户想要更复杂的东西（如扫描阻力、自定义逐出策略、可变缓存大小等），他们可以使用自己的实现

#ifndef STORAGE_LEVELDB_INCLUDE_CACHE_H_
#define STORAGE_LEVELDB_INCLUDE_CACHE_H_

#include <cstdint>

#include "leveldb/export.h"
#include "leveldb/slice.h"

namespace leveldb {

class LEVELDB_EXPORT Cache;

// Create a new cache with a fixed size capacity.  This implementation
// of Cache uses a least-recently-used eviction policy.
// 创建一个固定大小的缓存，缓存实现使用最近最少使用的逐出策略
LEVELDB_EXPORT Cache* NewLRUCache(size_t capacity);

class LEVELDB_EXPORT Cache {
 public:
  Cache() = default;

  Cache(const Cache&) = delete;
  Cache& operator=(const Cache&) = delete;

  // Destroys all existing entries by calling the "deleter"
  // function that was passed to the constructor.
  // 通过调用“deleter”来释放所有存在的条目，
  // 这个“deleter”函数通过构造函数传进去
  virtual ~Cache();

  // Opaque handle to an entry stored in the cache.
  // 缓存里面的条目是以handle来存储的
  struct Handle {};

  // Insert a mapping from key->value into the cache and assign it
  // the specified charge against the total cache capacity.
  //
  // Returns a handle that corresponds to the mapping.  The caller
  // must call this->Release(handle) when the returned mapping is no
  // longer needed.
  //
  // When the inserted entry is no longer needed, the key and
  // value will be passed to "deleter".
  // 插入一个key->value的映射到缓存，同时也要传入一个charge来计算当前缓存容量，好删除不不用的缓存项
  // 返回这个映射的handle回来，如果调用者不需要这个handle，必须this->Release(handle)来释放它
  // 当这个新插入的条目不再需要，它会被传入到“deleter”函数里面被删除掉
  virtual Handle* Insert(const Slice& key, void* value, size_t charge,
                         void (*deleter)(const Slice& key, void* value)) = 0;

  // If the cache has no mapping for "key", returns nullptr.
  //
  // Else return a handle that corresponds to the mapping.  The caller
  // must call this->Release(handle) when the returned mapping is no
  // longer needed.
  // 如果缓存没有这个key的映射，返回nullptr
  // 否则返回一个映射对应的handle
  // 如果调用者不需要这个handle了，
  // 就必须调用this->Release(handle)来释放它
  virtual Handle* Lookup(const Slice& key) = 0;

  // Release a mapping returned by a previous Lookup().
  // REQUIRES: handle must not have been released yet.
  // REQUIRES: handle must have been returned by a method on *this.
  // 
  // 释放一个handle，这个handle来自于Lookup()的返回值
  // 必须：handle必须没有被释放
  // 必须：handle必须是*this的某个方法调用的返回值
  virtual void Release(Handle* handle) = 0;

  // Return the value encapsulated in a handle returned by a
  // successful Lookup().
  // REQUIRES: handle must not have been released yet.
  // REQUIRES: handle must have been returned by a method on *this.
  // 返回handle里面的值，这个handle来自于调用Lookup()的返回值
  // 必须：handle必须没有被释放
  // 必须：handle必须是*this的某个方法调用的返回值
  virtual void* Value(Handle* handle) = 0;

  // If the cache contains entry for key, erase it.  Note that the
  // underlying entry will be kept around until all existing handles
  // to it have been released.
  // 如果缓存中包含这个key，删之。请注意，底层条目将一直保留，直到它的所有现有handle被释放。
  virtual void Erase(const Slice& key) = 0;

  // Return a new numeric id.  May be used by multiple clients who are
  // sharing the same cache to partition the key space.  Typically the
  // client will allocate a new id at startup and prepend the id to
  // its cache keys.
  // 返回一个新的数字id。共享同一缓存的多个客户端可以使用该id对key的空间进行分区。
  // 通常，客户端将在启动时分配一个新id，并将该id前置到其缓存键。
  virtual uint64_t NewId() = 0;

  // Remove all cache entries that are not actively in use.  Memory-constrained
  // applications may wish to call this method to reduce memory usage.
  // Default implementation of Prune() does nothing.  Subclasses are strongly
  // encouraged to override the default implementation.  A future release of
  // leveldb may change Prune() to a pure abstract method.
  // 删除所有未在使用中的缓存项。内存受限的应用程序可能希望调用此方法以减少内存使用。
  // Prune()的默认实现不起任何作用。强烈建议子类重写默认实现。
  // leveldb的未来版本可能会将Prune()更改为纯抽象方法。
  virtual void Prune() {}

  // Return an estimate of the combined charges of all elements stored in the
  // cache.
  // 返回缓存中存储的所有元素的组合charge的估计值。
  virtual size_t TotalCharge() const = 0;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_CACHE_H_