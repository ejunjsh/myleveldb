// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A database can be configured with a custom FilterPolicy object.
// This object is responsible for creating a small filter from a set
// of keys.  These filters are stored in leveldb and are consulted
// automatically by leveldb to decide whether or not to read some
// information from disk. In many cases, a filter can cut down the
// number of disk seeks form a handful to a single disk seek per
// DB::Get() call.
//
// Most people will want to use the builtin bloom filter support (see
// NewBloomFilterPolicy() below).
// 可以使用自定义FilterPolicy对象配置数据库。
// 此对象负责从一组关键点创建一个小过滤器。
// 这些过滤器存储在leveldb中，leveldb会自动查询这些过滤器，以决定是否从磁盘读取某些信息。
// 在许多情况下，筛选器可以将每次DB::Get()调用的磁盘寻道次数从少量减少到单个磁盘寻道。
//
// 大多数人都希望使用内置的bloom过滤器支持（请参见下面的NewBloomFilterPolicy()）。

#ifndef STORAGE_LEVELDB_INCLUDE_FILTER_POLICY_H_
#define STORAGE_LEVELDB_INCLUDE_FILTER_POLICY_H_

#include <string>

#include "leveldb/export.h"

namespace leveldb {

class Slice;

// 筛选器策略
class LEVELDB_EXPORT FilterPolicy {
 public:
  virtual ~FilterPolicy();

  // Return the name of this policy.  Note that if the filter encoding
  // changes in an incompatible way, the name returned by this method
  // must be changed.  Otherwise, old incompatible filters may be
  // passed to methods of this type.
  // 返回此策略的名称。请注意，如果筛选器编码以不兼容的方式更改，则必须更改此方法返回的名称。
  // 否则，旧的不兼容筛选器可能会传递给此类型的方法。
  virtual const char* Name() const = 0;

  // keys[0,n-1] contains a list of keys (potentially with duplicates)
  // that are ordered according to the user supplied comparator.
  // Append a filter that summarizes keys[0,n-1] to *dst.
  //
  // Warning: do not change the initial contents of *dst.  Instead,
  // append the newly constructed filter to *dst.
  // keys[0,n-1] 包含一个列表的键（可能有重复）
  // 他们是根据用户提供的比较器来排序
  // 将汇总keys[0，n-1]的筛选器附加到*dst。
  //
  // 警告：请勿更改*dst的初始内容。相反，将新构造的筛选器附加到*dst。
  virtual void CreateFilter(const Slice* keys, int n,
                            std::string* dst) const = 0;

  // "filter" contains the data appended by a preceding call to
  // CreateFilter() on this class.  This method must return true if
  // the key was in the list of keys passed to CreateFilter().
  // This method may return true or false if the key was not on the
  // list, but it should aim to return false with a high probability.
  // “filter”包含前面对此类的CreateFilter()调用所附加的数据。如果key位于传递给CreateFilter()的键列表中，则此方法必须返回true。 
  // 如果key不在列表中，此方法可能返回true或false，但它应该以高概率返回false为目标。
  virtual bool KeyMayMatch(const Slice& key, const Slice& filter) const = 0;
};

// Return a new filter policy that uses a bloom filter with approximately
// the specified number of bits per key.  A good value for bits_per_key
// is 10, which yields a filter with ~ 1% false positive rate.
//
// Callers must delete the result after any database that is using the
// result has been closed.
//
// Note: if you are using a custom comparator that ignores some parts
// of the keys being compared, you must not use NewBloomFilterPolicy()
// and must provide your own FilterPolicy that also ignores the
// corresponding parts of the keys.  For example, if the comparator
// ignores trailing spaces, it would be incorrect to use a
// FilterPolicy (like NewBloomFilterPolicy) that does not ignore
// trailing spaces in keys.
// 返回一个新的筛选器策略，它用一个bloom筛选器，你可以指定每个key的位数（bits_per_key）
// 推荐bits_per_key=10，这将产生一个误报率约为1%的过滤器 
//
// 调用方必须在使用结果的任何数据库关闭后删除该结果。
//
// 注意：如果使用的自定义比较器忽略了要比较的键的某些部分，则不能使用NewBloomFilterPolicy()，
// 并且必须提供自己的FilterPolicy，该策略也会忽略键的相应部分。例如，如果比较器忽略尾随空格，
// 则使用不忽略键中尾随空格的FilterPolicy（如NewBloomFilterPolicy）是不正确的。
LEVELDB_EXPORT const FilterPolicy* NewBloomFilterPolicy(int bits_per_key);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_FILTER_POLICY_H_