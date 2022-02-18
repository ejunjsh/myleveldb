// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_TABLE_TWO_LEVEL_ITERATOR_H_
#define STORAGE_LEVELDB_TABLE_TWO_LEVEL_ITERATOR_H_

#include "leveldb/iterator.h"

namespace leveldb {

struct ReadOptions;

// Return a new two level iterator.  A two-level iterator contains an
// index iterator whose values point to a sequence of blocks where
// each block is itself a sequence of key,value pairs.  The returned
// two-level iterator yields the concatenation of all key/value pairs
// in the sequence of blocks.  Takes ownership of "index_iter" and
// will delete it when no longer needed.
//
// Uses a supplied function to convert an index_iter value into
// an iterator over the contents of the corresponding block.
//
// 返回一个新的两级迭代器。一个两级迭代器包含了一个索引迭代器，它的值指向一系列的块，这里的每个块又包含了一系列的键值对
// 这个两级迭代器会产出在一系列块中所有键值对的合集
// 获取"index_iter"的所有权，并在不需要的时候删掉
// 
// 使用提供的函数将index_iter的值转换为对应块内容的迭代器。
Iterator* NewTwoLevelIterator(
    Iterator* index_iter,
    Iterator* (*block_function)(void* arg, const ReadOptions& options,
                                const Slice& index_value),
    void* arg, const ReadOptions& options);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_TWO_LEVEL_ITERATOR_H_
