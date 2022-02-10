// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A filter block is stored near the end of a Table file.  It contains
// filters (e.g., bloom filters) for all data blocks in the table combined
// into a single filter block.
// 过滤器块存储在表文件的末尾附近。
// 它包含表中所有数据块的过滤器（例如bloom过滤器），这些过滤器块组合成一个更大的过滤器块。

#ifndef STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
#define STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "leveldb/slice.h"
#include "util/hash.h"

namespace leveldb {

class FilterPolicy;

// A FilterBlockBuilder is used to construct all of the filters for a
// particular Table.  It generates a single string which is stored as
// a special block in the Table.
//
// The sequence of calls to FilterBlockBuilder must match the regexp:
//      (StartBlock AddKey*)* Finish
// FilterBlockBuilder用于构造特定表的所有筛选器。它生成一个字符串，作为特殊块存储在表中。
//
// 对FilterBlockBuilder的调用顺序必须与regexp匹配：
//      (StartBlock AddKey*)* Finish
class FilterBlockBuilder {
 public:
  explicit FilterBlockBuilder(const FilterPolicy*);

  FilterBlockBuilder(const FilterBlockBuilder&) = delete;
  FilterBlockBuilder& operator=(const FilterBlockBuilder&) = delete;

  void StartBlock(uint64_t block_offset);
  void AddKey(const Slice& key);
  Slice Finish();

 private:
  void GenerateFilter();

  const FilterPolicy* policy_;
  std::string keys_;             // Flattened key contents 扁平化键内容
  std::vector<size_t> start_;    // Starting index in keys_ of each key 在每个键的key_中开始索引
  std::string result_;           // Filter data computed so far 筛选到目前为止计算的数据
  std::vector<Slice> tmp_keys_;  // policy_->CreateFilter() argument 参数
  std::vector<uint32_t> filter_offsets_;
};

class FilterBlockReader {
 public:
  // REQUIRES: "contents" and *policy must stay live while *this is live.
  // 要求："contents" 和 *policy必须有效
  FilterBlockReader(const FilterPolicy* policy, const Slice& contents);
  bool KeyMayMatch(uint64_t block_offset, const Slice& key);

 private:
  const FilterPolicy* policy_;
  const char* data_;    // Pointer to filter data (at block-start) 指向筛选器数据（在块开始位置）
  const char* offset_;  // Pointer to beginning of offset array (at block-end) 指向偏移数组的开始位置（块结束位置）
  size_t num_;          // Number of entries in offset array 偏移数组的条目数
  size_t base_lg_;      // Encoding parameter (see kFilterBaseLg in .cc file) 编码参数（详情看相应的.cc文件里面的kFilterBaseLg）
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_FILTER_BLOCK_H_
