// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_TABLE_FORMAT_H_
#define STORAGE_LEVELDB_TABLE_FORMAT_H_

#include <cstdint>
#include <string>

#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "leveldb/table_builder.h"

namespace leveldb {

class Block;
class RandomAccessFile;
struct ReadOptions;

// BlockHandle is a pointer to the extent of a file that stores a data
// block or a meta block.
// 块句柄是指向存储数据块或元块的文件范围的指针。
class BlockHandle {
 public:
  // Maximum encoding length of a BlockHandle
  // 块句柄最大编码长度
  enum { kMaxEncodedLength = 10 + 10 };

  BlockHandle();

  // The offset of the block in the file.
  // 文件中块的偏移量。
  uint64_t offset() const { return offset_; }
  void set_offset(uint64_t offset) { offset_ = offset; }

  // The size of the stored block
  // 存储块的大小
  uint64_t size() const { return size_; }
  void set_size(uint64_t size) { size_ = size; }

  void EncodeTo(std::string* dst) const;
  Status DecodeFrom(Slice* input);

 private:
  uint64_t offset_;
  uint64_t size_;
};

// Footer encapsulates the fixed information stored at the tail
// end of every table file.
// 页脚封装了存储在每个表文件末尾的固定信息。
class Footer {
 public:
  // Encoded length of a Footer.  Note that the serialization of a
  // Footer will always occupy exactly this many bytes.  It consists
  // of two block handles and a magic number.
  // 页脚的编码长度。请注意，页脚的序列化总是占用这么多字节。它由两个块句柄和一个魔数组成。
  enum { kEncodedLength = 2 * BlockHandle::kMaxEncodedLength + 8 };

  Footer() = default;

  // The block handle for the metaindex block of the table
  // 表的元索引块的块句柄
  const BlockHandle& metaindex_handle() const { return metaindex_handle_; }
  void set_metaindex_handle(const BlockHandle& h) { metaindex_handle_ = h; }

  // The block handle for the index block of the table
  // 表的索引块的块句柄
  const BlockHandle& index_handle() const { return index_handle_; }
  void set_index_handle(const BlockHandle& h) { index_handle_ = h; }

  void EncodeTo(std::string* dst) const;
  Status DecodeFrom(Slice* input);

 private:
  BlockHandle metaindex_handle_;
  BlockHandle index_handle_;
};

// kTableMagicNumber was picked by running
//    echo http://code.google.com/p/leveldb/ | sha1sum
// and taking the leading 64 bits.
// kTableMagicNumber是由运行命令：
//    echo http://code.google.com/p/leveldb/ | sha1sum
// 并取前64位获得
static const uint64_t kTableMagicNumber = 0xdb4775248b80fb57ull;

// 1-byte type + 32-bit crc
static const size_t kBlockTrailerSize = 5;

struct BlockContents {
  Slice data;           // Actual contents of data 数据的实际内容
  bool cachable;        // True iff data can be cached 如果被缓存了，返回true
  bool heap_allocated;  // True iff caller should delete[] data.data()  如果是堆分配，调用者应该delete[] data.data()，返回true
};

// Read the block identified by "handle" from "file".  On failure
// return non-OK.  On success fill *result and return OK.
// 从“file”中读取由“handle”标识的块。失败时返回非OK。成功后填充*result并返回OK。
Status ReadBlock(RandomAccessFile* file, const ReadOptions& options,
                 const BlockHandle& handle, BlockContents* result);

// Implementation details follow.  Clients should ignore,
// 具体实现细节如下。客户应该忽略，（啥意思啊？）

inline BlockHandle::BlockHandle()
    : offset_(~static_cast<uint64_t>(0)), size_(~static_cast<uint64_t>(0)) {}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_TABLE_FORMAT_H_
