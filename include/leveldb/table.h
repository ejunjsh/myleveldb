// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_TABLE_H_
#define STORAGE_LEVELDB_INCLUDE_TABLE_H_

#include <cstdint>

#include "leveldb/export.h"
#include "leveldb/iterator.h"

namespace leveldb {

class Block;
class BlockHandle;
class Footer;
struct Options;
class RandomAccessFile;
struct ReadOptions;
class TableCache;

// A Table is a sorted map from strings to strings.  Tables are
// immutable and persistent.  A Table may be safely accessed from
// multiple threads without external synchronization.
// 表是从字符串到字符串的排序映射。表是不可变和持久的。可以从多个线程安全地访问一个表，而无需外部同步。
class LEVELDB_EXPORT Table {
 public:
  // Attempt to open the table that is stored in bytes [0..file_size)
  // of "file", and read the metadata entries necessary to allow
  // retrieving data from the table.
  //
  // If successful, returns ok and sets "*table" to the newly opened
  // table.  The client should delete "*table" when no longer needed.
  // If there was an error while initializing the table, sets "*table"
  // to nullptr and returns a non-ok status.  Does not take ownership of
  // "*source", but the client must ensure that "source" remains live
  // for the duration of the returned table's lifetime.
  //
  // *file must remain live while this Table is in use.
  //
  // 尝试打开存储在字节[0..file_size]为“file”的表，并读取允许从表中检索数据所需的元数据项。
  // 
  // 如果成功，返回ok同时设置"*table"，让它指向最新打开的表
  // 客户端应该在不需要"*table"时，删掉它
  // 如果初始化的时候发生错误，设置"*table"为空指针，并返回非ok状态
  // 不获取“*source”的所有权，但客户端必须确保“source”在返回表的生存期内保持活动状态。
  // 
  // 当表还在用的时候，*file必须保持活动状态，
  static Status Open(const Options& options, RandomAccessFile* file,
                     uint64_t file_size, Table** table);

  Table(const Table&) = delete;
  Table& operator=(const Table&) = delete;

  ~Table();

  // Returns a new iterator over the table contents.
  // The result of NewIterator() is initially invalid (caller must
  // call one of the Seek methods on the iterator before using it).
  // 返回表内容的新迭代器。
  // NewIterator()的结果最初无效（调用方必须在使用迭代器之前调用迭代器上的一个Seek方法）。
  Iterator* NewIterator(const ReadOptions&) const;

  // Given a key, return an approximate byte offset in the file where
  // the data for that key begins (or would begin if the key were
  // present in the file).  The returned value is in terms of file
  // bytes, and so includes effects like compression of the underlying data.
  // E.g., the approximate offset of the last key in the table will
  // be close to the file length.
  // 给定一个键，在该键的数据开始的文件中返回一个近似字节偏移量
  // （如果该键存在于文件中，则返回将开始的字节偏移量）。
  // 返回的值以文件字节为单位，因此包括底层数据的压缩等效果。
  // 例如，表中最后一个键的近似偏移量将接近文件长度。
  uint64_t ApproximateOffsetOf(const Slice& key) const;

 private:
  friend class TableCache;
  struct Rep;

  static Iterator* BlockReader(void*, const ReadOptions&, const Slice&);

  explicit Table(Rep* rep) : rep_(rep) {}

  // Calls (*handle_result)(arg, ...) with the entry found after a call
  // to Seek(key).  May not make such a call if filter policy says
  // that key is not present.
  // 在调用Seek(key)并找到一个条目的时候用这个条目作为参数调用 (*handle_result)(arg, ...)
  // 如果筛选器策略表示这个键不存在，则不调用这个函数(*handle_result)(arg, ...)
  Status InternalGet(const ReadOptions&, const Slice& key, void* arg,
                     void (*handle_result)(void* arg, const Slice& k,
                                           const Slice& v));

  void ReadMeta(const Footer& footer);
  void ReadFilter(const Slice& filter_handle_value);

  Rep* const rep_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_TABLE_H_
