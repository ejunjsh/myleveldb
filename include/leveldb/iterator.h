// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// An iterator yields a sequence of key/value pairs from a source.
// The following class defines the interface.  Multiple implementations
// are provided by this library.  In particular, iterators are provided
// to access the contents of a Table or a DB.
//
// Multiple threads can invoke const methods on an Iterator without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Iterator must use
// external synchronization.
// 迭代器从源中生成一系列键/值对。
// 下面的类定义了接口。该库提供了多种实现。特别是，提供了迭代器来访问表或数据库的内容。
//
// 多个线程可以在没有外部同步的情况下调用迭代器上的const方法，
// 但是如果任何线程都可以调用非const方法，那么访问同一迭代器的所有线程都必须使用外部同步。

#ifndef STORAGE_LEVELDB_INCLUDE_ITERATOR_H_
#define STORAGE_LEVELDB_INCLUDE_ITERATOR_H_

#include "leveldb/export.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"

namespace leveldb {

class LEVELDB_EXPORT Iterator {
 public:
  Iterator();

  Iterator(const Iterator&) = delete;
  Iterator& operator=(const Iterator&) = delete;

  virtual ~Iterator();

  // An iterator is either positioned at a key/value pair, or
  // not valid.  This method returns true iff the iterator is valid.
  // 迭代器要么位于键/值对上，要么无效。如果迭代器有效，此方法返回true。
  virtual bool Valid() const = 0;

  // Position at the first key in the source.  The iterator is Valid()
  // after this call iff the source is not empty.
  // 定位源里面的第一个键。迭代器在这个函数调用后并且源不是空的时候变有效
  virtual void SeekToFirst() = 0;

  // Position at the last key in the source.  The iterator is
  // Valid() after this call iff the source is not empty.
  // 定位源里面的最后一个键。迭代器在这个函数调用后并且源不是空的时候变有效
  virtual void SeekToLast() = 0;

  // Position at the first key in the source that is at or past target.
  // The iterator is Valid() after this call iff the source contains
  // an entry that comes at or past target.
  // 定位在源中位于或超过target的第一个键。
  // 如果源包含或超过target的条目，则迭代器在此调用后有效。
  virtual void Seek(const Slice& target) = 0;

  // Moves to the next entry in the source.  After this call, Valid() is
  // true iff the iterator was not positioned at the last entry in the source.
  // REQUIRES: Valid()
  // 移动到源中的下一个条目。在此调用之后，如果迭代器未定位在源中的最后一个条目，则Valid（）为true。
  // 要求：Valid（）
  virtual void Next() = 0;

  // Moves to the previous entry in the source.  After this call, Valid() is
  // true iff the iterator was not positioned at the first entry in source.
  // REQUIRES: Valid()
  // 移动到源中的上一个条目。在此调用之后，如果迭代器未定位在源中的第一个条目，则Valid（）为true。
  // 要求：Valid（）
  virtual void Prev() = 0;

  // Return the key for the current entry.  The underlying storage for
  // the returned slice is valid only until the next modification of
  // the iterator.
  // REQUIRES: Valid()
  // 返回当前条目的键。返回切片的底层存储仅在迭代器的下一次修改之前有效。
  // 要求：Valid（）
  virtual Slice key() const = 0;

  // Return the value for the current entry.  The underlying storage for
  // the returned slice is valid only until the next modification of
  // the iterator.
  // REQUIRES: Valid()
  // 返回当前条目的值。返回切片的底层存储仅在迭代器的下一次修改之前有效。
  // 要求：Valid（）
  virtual Slice value() const = 0;

  // If an error has occurred, return it.  Else return an ok status.
  // 如果发生错误，请将其返回。否则返回ok状态。
  virtual Status status() const = 0;

  // Clients are allowed to register function/arg1/arg2 triples that
  // will be invoked when this iterator is destroyed.
  //
  // Note that unlike all of the preceding methods, this method is
  // not abstract and therefore clients should not override it.
  // 允许客户端注册function/arg1/arg2三元组，当该迭代器被销毁时将调用该函数。
  //
  // 请注意，与前面的所有方法不同，此方法不是抽象的，因此客户端不应重写它。
  using CleanupFunction = void (*)(void* arg1, void* arg2);
  void RegisterCleanup(CleanupFunction function, void* arg1, void* arg2);

 private:
  // Cleanup functions are stored in a single-linked list.
  // The list's head node is inlined in the iterator.
  // 清理函数存储在一个链接列表中。
  // 列表的头节点内联在迭代器中。
  struct CleanupNode {
    // True if the node is not used. Only head nodes might be unused.
    // 如果未使用该节点，则为True。只有头部节点可能未使用。
    bool IsEmpty() const { return function == nullptr; }
    // Invokes the cleanup function.
    // 调用清理函数
    void Run() {
      assert(function != nullptr);
      (*function)(arg1, arg2);
    }

    // The head node is used if the function pointer is not null.
    // 如果function指针不为null，则使用head节点。
    CleanupFunction function;
    void* arg1;
    void* arg2;
    CleanupNode* next;
  };
  CleanupNode cleanup_head_;
};

// Return an empty iterator (yields nothing).
// 返回一个空迭代器（不产生任何结果）。
LEVELDB_EXPORT Iterator* NewEmptyIterator();

// Return an empty iterator with the specified status.
// 返回具有指定状态的空迭代器。
LEVELDB_EXPORT Iterator* NewErrorIterator(const Status& status);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_ITERATOR_H_
