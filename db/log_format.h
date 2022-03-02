// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Log format information shared by reader and writer.
// See ../doc/log_format.md for more detail.
// 读写数据块共享的日志格式信息。
// 查看 ../doc/log_format.md了解更多细节。

#ifndef STORAGE_LEVELDB_DB_LOG_FORMAT_H_
#define STORAGE_LEVELDB_DB_LOG_FORMAT_H_

namespace leveldb {
namespace log {

enum RecordType {
  // Zero is reserved for preallocated files
  // 零是为预分配的文件保留的
  kZeroType = 0,

  kFullType = 1,

  // For fragments
  // 分片
  kFirstType = 2,
  kMiddleType = 3,
  kLastType = 4
};
static const int kMaxRecordType = kLastType;

static const int kBlockSize = 32768; // 32KB

// Header is checksum (4 bytes), length (2 bytes), type (1 byte).
// 头包含校验和（4个字节），长度（2个字节），类型（1个字节）
static const int kHeaderSize = 4 + 2 + 1;

}  // namespace log
}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_LOG_FORMAT_H_
