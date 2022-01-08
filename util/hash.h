// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Simple hash function used for internal data structures
// 用于内部数据结构的简单哈希函数

#ifndef STORAGE_LEVELDB_UTIL_HASH_H_
#define STORAGE_LEVELDB_UTIL_HASH_H_

#include <cstddef>
#include <cstdint>

namespace leveldb {

uint32_t Hash(const char* data, size_t n, uint32_t seed);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_HASH_H_