// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A Status encapsulates the result of an operation.  It may indicate success,
// or it may indicate an error with an associated error message.
// Status 封装了一个操作的结果，它可能代表成功，或者一个相关连信息的错误
//
// Multiple threads can invoke const methods on a Status without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Status must use
// external synchronization.
// 多线程可以调用const方法而不需要外部同步（锁）
// 但是如果任意线程访问non-const方法，则所有线程访问这个Status就需要外部同步（锁）

#ifndef STORAGE_LEVELDB_INCLUDE_STATUS_H_
#define STORAGE_LEVELDB_INCLUDE_STATUS_H_

#include <algorithm>
#include <string>

#include "leveldb/export.h"
#include "leveldb/slice.h"

namespace leveldb {

class LEVELDB_EXPORT Status {
 public:
  // Create a success status.
  // 创建一个成功的status
  Status() noexcept : state_(nullptr) {}
  ~Status() { delete[] state_; }

  Status(const Status& rhs);
  Status& operator=(const Status& rhs);

  Status(Status&& rhs) noexcept : state_(rhs.state_) { rhs.state_ = nullptr; }
  Status& operator=(Status&& rhs) noexcept;

  // Return a success status.
  // 返回一个成功的status
  static Status OK() { return Status(); }

  // Return error status of an appropriate type.
  // 返回合适类型的错误status
  static Status NotFound(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kNotFound, msg, msg2);
  }
  static Status Corruption(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kCorruption, msg, msg2);
  }
  static Status NotSupported(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kNotSupported, msg, msg2);
  }
  static Status InvalidArgument(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kInvalidArgument, msg, msg2);
  }
  static Status IOError(const Slice& msg, const Slice& msg2 = Slice()) {
    return Status(kIOError, msg, msg2);
  }

  // Returns true iff the status indicates success.
  // 如果status表示成功，返回true
  bool ok() const { return (state_ == nullptr); }

  // Returns true iff the status indicates a NotFound error.
  // 如果status表示没找到错误，返回true
  bool IsNotFound() const { return code() == kNotFound; }

  // Returns true iff the status indicates a Corruption error.
  // 文件坏掉错误，返回true
  bool IsCorruption() const { return code() == kCorruption; }

  // Returns true iff the status indicates an IOError.
  // IO错误，返回true
  bool IsIOError() const { return code() == kIOError; }

  // Returns true iff the status indicates a NotSupportedError.
  // 不支持错误，返回status
  bool IsNotSupportedError() const { return code() == kNotSupported; }

  // Returns true iff the status indicates an InvalidArgument.
  // 无效参数错误，返回true
  bool IsInvalidArgument() const { return code() == kInvalidArgument; }

  // Return a string representation of this status suitable for printing.
  // Returns the string "OK" for success.
  // 返回一个方便打印的status字符串表示
  // 返回字符串"OK"代表成功
  std::string ToString() const;

 private:
  enum Code {
    kOk = 0,
    kNotFound = 1,
    kCorruption = 2,
    kNotSupported = 3,
    kInvalidArgument = 4,
    kIOError = 5
  };

  Code code() const {
    return (state_ == nullptr) ? kOk : static_cast<Code>(state_[4]);
  }

  Status(Code code, const Slice& msg, const Slice& msg2);
  static const char* CopyState(const char* s);

  // OK status has a null state_.  Otherwise, state_ is a new[] array
  // of the following form:
  // OK status 是个空的state_,否则state_是一个new[]数组
  // 由下面组成
  //    state_[0..3] == length of message 消息长度
  //    state_[4]    == code 消息类型
  //    state_[5..]  == message 消息
  const char* state_;
};

inline Status::Status(const Status& rhs) {
  state_ = (rhs.state_ == nullptr) ? nullptr : CopyState(rhs.state_);
}
inline Status& Status::operator=(const Status& rhs) {
  // The following condition catches both aliasing (when this == &rhs),
  // and the common case where both rhs and *this are ok.
  // 以下条件将捕获别名（当this==&rhs时），以及rhs和*this都正常的常见情况。
  if (state_ != rhs.state_) {
    delete[] state_;
    state_ = (rhs.state_ == nullptr) ? nullptr : CopyState(rhs.state_);
  }
  return *this;
}
inline Status& Status::operator=(Status&& rhs) noexcept {
  std::swap(state_, rhs.state_);
  return *this;
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_STATUS_H_