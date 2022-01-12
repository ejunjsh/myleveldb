// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_UTIL_MUTEXLOCK_H_
#define STORAGE_LEVELDB_UTIL_MUTEXLOCK_H_

#include "port/port.h"
#include "port/thread_annotations.h"

namespace leveldb {

// Helper class that locks a mutex on construction and unlocks the mutex when
// the destructor of the MutexLock object is invoked.
// 帮助类，在构造函数里面锁定一个mutex，调用析构函数时解锁这个mutex
//
// Typical usage: 经典用法：
//
//   void MyClass::MyMethod() {
//     MutexLock l(&mu_);       // mu_ is an instance variable 是一个实例变量
//     ... some complex code, possibly with multiple return paths ... 一些复杂的代码，可能有多个返回路径
//   }

class SCOPED_LOCKABLE MutexLock {
 public:
  explicit MutexLock(port::Mutex* mu) EXCLUSIVE_LOCK_FUNCTION(mu) : mu_(mu) {
    this->mu_->Lock();
  }
  ~MutexLock() UNLOCK_FUNCTION() { this->mu_->Unlock(); }

  MutexLock(const MutexLock&) = delete;
  MutexLock& operator=(const MutexLock&) = delete;

 private:
  port::Mutex* const mu_;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_MUTEXLOCK_H_