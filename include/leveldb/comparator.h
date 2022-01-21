// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_
#define STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_

#include <string>

#include "leveldb/export.h"

namespace leveldb {

class Slice;

// A Comparator object provides a total order across slices that are
// used as keys in an sstable or a database.  A Comparator implementation
// must be thread-safe since leveldb may invoke its methods concurrently
// from multiple threads.
// 比较器可以对一堆slice排序，这些slice都是作为sstable和数据库的key
// 比较器实现必须是线程安全的，因为leveldb可以从多个线程并发调用其方法。
class LEVELDB_EXPORT Comparator {
 public:
  virtual ~Comparator();

  // Three-way comparison.  Returns value:
  // 三路比较，返回值：
  //   < 0 iff "a" < "b",
  //   == 0 iff "a" == "b",
  //   > 0 iff "a" > "b"
  virtual int Compare(const Slice& a, const Slice& b) const = 0;

  // The name of the comparator.  Used to check for comparator
  // mismatches (i.e., a DB created with one comparator is
  // accessed using a different comparator.
  //
  // The client of this package should switch to a new name whenever
  // the comparator implementation changes in a way that will cause
  // the relative ordering of any two keys to change.
  //
  // Names starting with "leveldb." are reserved and should not be used
  // by any clients of this package.
  // 比较器名称，用来检查比较器是不是一致（例如一个数据库创建的时候用的比较器是不是跟访问这个数据库时用的比较器是不是一致）
  // 
  // 每当比较器实现发生变化，导致任意两个键的相对顺序发生变化时，该包的使用者应切换到新名称。
  // 
  // 以“leveldb”开头的名称是保留的，不应由此包使用者使用。
  virtual const char* Name() const = 0;

  // Advanced functions: these are used to reduce the space requirements
  // for internal data structures like index blocks.
  // 高级函数：这些函数用于减少内部数据结构（如索引块）的空间需求。

  // If *start < limit, changes *start to a short string in [start,limit).
  // Simple comparator implementations may return with *start unchanged,
  // i.e., an implementation of this method that does nothing is correct.
  // 如果*start<limit，则在[start，limit]中将*start更改为短字符串。(完全不知道什么意思)
  // 简单的比较器实现可能返回*start不变，
  // 也就是说，这种方法的实现不做任何事情是正确的。 
  virtual void FindShortestSeparator(std::string* start,
                                     const Slice& limit) const = 0;

  // Changes *key to a short string >= *key.
  // Simple comparator implementations may return with *key unchanged,
  // i.e., an implementation of this method that does nothing is correct.
  // 将*key更改为短字符串 >= *key。(完全不知道什么意思)
  // 简单的比较器实现可能返回*start不变，
  // 也就是说，这种方法的实现不做任何事情是正确的。
  virtual void FindShortSuccessor(std::string* key) const = 0;
};

// Return a builtin comparator that uses lexicographic byte-wise
// ordering.  The result remains the property of this module and
// must not be deleted.
// 返回使用字典字节顺序的内置比较器。结果仍然是此模块的属性，不能删除。
LEVELDB_EXPORT const Comparator* BytewiseComparator();

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_COMPARATOR_H_
