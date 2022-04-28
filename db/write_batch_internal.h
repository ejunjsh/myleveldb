// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_
#define STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_

#include "db/dbformat.h"
#include "leveldb/write_batch.h"

namespace leveldb {

class MemTable;

// WriteBatchInternal provides static methods for manipulating a
// WriteBatch that we don't want in the public WriteBatch interface.
// WriteBatchInternal提供静态方法用来操作一个WriteBatch，不像把这些方法放到WriteBatch的公共接口
class WriteBatchInternal {
 public:
  // Return the number of entries in the batch.
  // 返回批有多少条目
  static int Count(const WriteBatch* batch);

  // Set the count for the number of entries in the batch.
  // 设置批的条目数量
  static void SetCount(WriteBatch* batch, int n);

  // Return the sequence number for the start of this batch.
  // 返回批开始时的序列号
  static SequenceNumber Sequence(const WriteBatch* batch);

  // Store the specified number as the sequence number for the start of
  // this batch.
  // 设置批开始时的序列号
  static void SetSequence(WriteBatch* batch, SequenceNumber seq);

  static Slice Contents(const WriteBatch* batch) { return Slice(batch->rep_); }

  static size_t ByteSize(const WriteBatch* batch) { return batch->rep_.size(); }

  static void SetContents(WriteBatch* batch, const Slice& contents);

  static Status InsertInto(const WriteBatch* batch, MemTable* memtable);

  static void Append(WriteBatch* dst, const WriteBatch* src);
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_WRITE_BATCH_INTERNAL_H_
