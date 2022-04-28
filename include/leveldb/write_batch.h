// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// WriteBatch holds a collection of updates to apply atomically to a DB.
// WriteBatch保存一组更新，以原子方式应用于数据库。
//
// The updates are applied in the order in which they are added
// to the WriteBatch.  For example, the value of "key" will be "v3"
// after the following batch is written:
// 更新将按其添加到WriteBatch的顺序应用。例如，写入以下批次后，“key”的值将为“v3”：
//
//    batch.Put("key", "v1");
//    batch.Delete("key");
//    batch.Put("key", "v2");
//    batch.Put("key", "v3");
//
// Multiple threads can invoke const methods on a WriteBatch without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same WriteBatch must use
// external synchronization.
// 多个线程可以在没有外部同步的情况下调用WriteBatch上的const方法，
// 但如果任何线程都可以调用非const方法，那么访问同一WriteBatch的所有线程都必须使用外部同步。

#ifndef STORAGE_LEVELDB_INCLUDE_WRITE_BATCH_H_
#define STORAGE_LEVELDB_INCLUDE_WRITE_BATCH_H_

#include <string>

#include "leveldb/export.h"
#include "leveldb/status.h"

namespace leveldb {

class Slice;

class LEVELDB_EXPORT WriteBatch {
 public:
  class LEVELDB_EXPORT Handler {
   public:
    virtual ~Handler();
    virtual void Put(const Slice& key, const Slice& value) = 0;
    virtual void Delete(const Slice& key) = 0;
  };

  WriteBatch();

  // Intentionally copyable.
  // 可拷贝
  WriteBatch(const WriteBatch&) = default;
  WriteBatch& operator=(const WriteBatch&) = default;

  ~WriteBatch();

  // Store the mapping "key->value" in the database.
  // 在数据库里面存储映射 "key->value"
  void Put(const Slice& key, const Slice& value);

  // If the database contains a mapping for "key", erase it.  Else do nothing.
  // 如果数据库包含“key”的映射，请将其删除。否则什么也不做。
  void Delete(const Slice& key);

  // Clear all updates buffered in this batch.
  // 清除此批中缓冲的所有更新。
  void Clear();

  // The size of the database changes caused by this batch.
  //
  // This number is tied to implementation details, and may change across
  // releases. It is intended for LevelDB usage metrics.
  // 此批处理导致的数据库大小更改。
  //
  // 这个数字与实现细节有关，可能会随着版本的不同而变化。它用于LevelDB使用指标。
  size_t ApproximateSize() const;

  // Copies the operations in "source" to this batch.
  //
  // This runs in O(source size) time. However, the constant factor is better
  // than calling Iterate() over the source batch with a Handler that replicates
  // the operations into this batch.
  // 从source拷贝到当前的批
  //
  // 这个是个常量级别的复杂度，好过用Iterate()在source迭代，然后每次迭代通过Handler来复制操作到当前批，这样的复杂度是线性级别
  void Append(const WriteBatch& source);

  // Support for iterating over the contents of a batch.
  // 支持迭代批的内容。
  Status Iterate(Handler* handler) const;

 private:
  friend class WriteBatchInternal;

  std::string rep_;  // See comment in write_batch.cc for the format of rep_  请参阅write_batch.cc中关于rep_格式的注释
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_WRITE_BATCH_H_
