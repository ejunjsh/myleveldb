// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_LOG_READER_H_
#define STORAGE_LEVELDB_DB_LOG_READER_H_

#include <cstdint>

#include "db/log_format.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"

namespace leveldb {

class SequentialFile;

namespace log {

class Reader {
 public:
  // Interface for reporting errors.
  // 用来报告错误的接口
  class Reporter {
   public:
    virtual ~Reporter();

    // Some corruption was detected.  "bytes" is the approximate number
    // of bytes dropped due to the corruption.
    // 发现了一些文件损坏。“bytes”是由于损坏而丢弃的大致字节数。
    virtual void Corruption(size_t bytes, const Status& status) = 0;
  };

  // Create a reader that will return log records from "*file".
  // "*file" must remain live while this Reader is in use.
  //
  // If "reporter" is non-null, it is notified whenever some data is
  // dropped due to a detected corruption.  "*reporter" must remain
  // live while this Reader is in use.
  //
  // If "checksum" is true, verify checksums if available.
  //
  // The Reader will start reading at the first record located at physical
  // position >= initial_offset within the file.
  //
  // 创建一个将从“*file”返回日志记录的读取器。
  // 使用此读取器时，“*file”必须保持活动状态。
  //
  // 如果“reporter”不为空，则每当由于检测到损坏而删除某些数据时，都会通知它。
  // “*reporter”在该读取器使用期间必须保持活动状态。
  //
  // 如果“checksum”为真，则验证校验和（如果可用）。
  //
  // 读取器将从位于文件内物理位置>= initial_offset的第一条记录开始读取。
  Reader(SequentialFile* file, Reporter* reporter, bool checksum,
         uint64_t initial_offset);

  Reader(const Reader&) = delete;
  Reader& operator=(const Reader&) = delete;

  ~Reader();

  // Read the next record into *record.  Returns true if read
  // successfully, false if we hit end of the input.  May use
  // "*scratch" as temporary storage.  The contents filled in *record
  // will only be valid until the next mutating operation on this
  // reader or the next mutation to *scratch.
  // 将下一条记录读入*record。如果读取成功，则返回true；
  // 如果到达输入的尾部，则返回false。
  // 可使用“*scratch”作为临时存储。
  // *record中填写的内容只有在该读取器上的下一次读取操作之前或下一次*scratch修改之前有效。
  bool ReadRecord(Slice* record, std::string* scratch);

  // Returns the physical offset of the last record returned by ReadRecord.
  //
  // Undefined before the first call to ReadRecord.
  // 返回ReadRecord返回的最后一条记录的物理偏移量。
  //
  // 在第一次调用ReadRecord之前未定义。
  uint64_t LastRecordOffset();

 private:
  // Extend record types with the following special values
  // 使用以下特殊值扩展记录类型
  enum {
    kEof = kMaxRecordType + 1,
    // Returned whenever we find an invalid physical record.
    // Currently there are three situations in which this happens:
    // * The record has an invalid CRC (ReadPhysicalRecord reports a drop)
    // * The record is a 0-length record (No drop is reported)
    // * The record is below constructor's initial_offset (No drop is reported)
    // 当我们发现无效的物理记录时返回。
    // 目前有三种情况会发生这种情况：
    // * 记录的CRC无效（ReadPhysicalRecord报告文件损坏）
    // * 记录长度为0（未报告文件损坏）
    // * 记录低于构造器的initial_offset（未报告文件损坏）
    kBadRecord = kMaxRecordType + 2
  };

  // Skips all blocks that are completely before "initial_offset_".
  //
  // Returns true on success. Handles reporting.
  // 跳过完全在“initial_offset”之前的所有块。
  //
  // 成功时返回true。处理报告。
  bool SkipToInitialBlock();

  // Return type, or one of the preceding special values
  // 返回类型，或前面的一个特殊值
  unsigned int ReadPhysicalRecord(Slice* result);

  // Reports dropped bytes to the reporter.
  // buffer_ must be updated to remove the dropped bytes prior to invocation.
  // 报告向记者发送了字节。
  // 在调用之前，必须更新buffer_以删除删除的字节。
  void ReportCorruption(uint64_t bytes, const char* reason);
  void ReportDrop(uint64_t bytes, const Status& reason);

  SequentialFile* const file_;
  Reporter* const reporter_;
  bool const checksum_;
  char* const backing_store_; // buffer_后面的真正的缓存
  Slice buffer_;
  bool eof_;  // Last Read() indicated EOF by returning < kBlockSize
              // 在上一次的调用Read()返回的值< kBlockSize 则是EOF，

  // Offset of the last record returned by ReadRecord.
  // ReadRecord返回的上一条记录的偏移量。
  uint64_t last_record_offset_;
  // Offset of the first location past the end of buffer_.
  // 第一个位置超过buffer_结尾的偏移量。
  uint64_t end_of_buffer_offset_;

  // Offset at which to start looking for the first record to return
  // 开始查找要返回的第一条记录的偏移量
  uint64_t const initial_offset_;

  // True if we are resynchronizing after a seek (initial_offset_ > 0). In
  // particular, a run of kMiddleType and kLastType records can be silently
  // skipped in this mode
  // 在一次查找后（initial_offset_ > 0），如果我们正在重新同步，则是true
  // 特别的，在这个模式下，一些kMiddleType和kLastType记录可能悄悄的被跳过
  bool resyncing_;
};

}  // namespace log
}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_LOG_READER_H_
