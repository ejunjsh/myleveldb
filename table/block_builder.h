// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_
#define STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_

#include <cstdint>
#include <vector>

#include "leveldb/slice.h"

namespace leveldb {

struct Options;

class BlockBuilder {
 public:
  explicit BlockBuilder(const Options* options);

  BlockBuilder(const BlockBuilder&) = delete;
  BlockBuilder& operator=(const BlockBuilder&) = delete;

  // Reset the contents as if the BlockBuilder was just constructed.
  // 重置内容，就像块构建器刚刚构建一样。
  void Reset();

  // REQUIRES: Finish() has not been called since the last call to Reset().
  // REQUIRES: key is larger than any previously added key
  // 要求：自上次调用Reset()以来，尚未调用Finish()。
  // 要求：键比以前添加的任何键都大
  void Add(const Slice& key, const Slice& value);

  // Finish building the block and return a slice that refers to the
  // block contents.  The returned slice will remain valid for the
  // lifetime of this builder or until Reset() is called.
  // 完成构建块并返回一个引用块内容的切片。
  // 返回的切片将在此构建器的生存期内或在调用Reset()之前保持有效。
  Slice Finish();

  // Returns an estimate of the current (uncompressed) size of the block
  // we are building.
  // 返回我们正在构建的块的当前（未压缩）大小的估计值。
  size_t CurrentSizeEstimate() const;

  // Return true iff no entries have been added since the last Reset()
  // 自上次Reset()以来未添加任何条目就返回true
  bool empty() const { return buffer_.empty(); }

 private:
  const Options* options_;
  std::string buffer_;              // Destination buffer 目标缓存区
  std::vector<uint32_t> restarts_;  // Restart points 重启点
  int counter_;                     // Number of entries emitted since restart 自重启后发出的条目数
  bool finished_;                   // Has Finish() been called? Finish()调用后设置为true
  std::string last_key_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_BLOCK_BUILDER_H_
