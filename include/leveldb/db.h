// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_DB_H_
#define STORAGE_LEVELDB_INCLUDE_DB_H_

#include <cstdint>
#include <cstdio>

#include "leveldb/export.h"
#include "leveldb/iterator.h"
#include "leveldb/options.h"

namespace leveldb {

// Update CMakeLists.txt if you change these
// 如果你改了这里，请同时更新CMakeLists.txt
static const int kMajorVersion = 1;
static const int kMinorVersion = 23;

struct Options;
struct ReadOptions;
struct WriteOptions;
class WriteBatch;

// Abstract handle to particular state of a DB.
// A Snapshot is an immutable object and can therefore be safely
// accessed from multiple threads without any external synchronization.
// 数据库特定状态的抽象句柄。
// 快照是一个不可变的对象，因此可以从多个线程安全地访问，而无需任何外部同步。
class LEVELDB_EXPORT Snapshot {
 protected:
  virtual ~Snapshot();
};

// A range of keys
// 一个范围的键
struct LEVELDB_EXPORT Range {
  Range() = default;
  Range(const Slice& s, const Slice& l) : start(s), limit(l) {}

  Slice start;  // Included in the range 包括在范围内
  Slice limit;  // Not included in the range 不包括在范围内
};

// A DB is a persistent ordered map from keys to values.
// A DB is safe for concurrent access from multiple threads without
// any external synchronization.
// DB是从键到值的持久有序映射。
// DB可以安全地从多个线程进行并发访问，而无需任何外部同步。
class LEVELDB_EXPORT DB {
 public:
  // Open the database with the specified "name".
  // Stores a pointer to a heap-allocated database in *dbptr and returns
  // OK on success.
  // Stores nullptr in *dbptr and returns a non-OK status on error.
  // Caller should delete *dbptr when it is no longer needed.
  // 用指定的“name”打开数据库。
  // 将指向堆分配数据库的指针存储在*dbptr中，并在成功时返回OK。
  // 将nullptr存储在*dbptr中，并在出错时返回非OK状态。
  // 当不再需要*dbptr时，调用者应该删除它。
  static Status Open(const Options& options, const std::string& name,
                     DB** dbptr);

  DB() = default;

  DB(const DB&) = delete;
  DB& operator=(const DB&) = delete;

  virtual ~DB();

  // Set the database entry for "key" to "value".  Returns OK on success,
  // and a non-OK status on error.
  // Note: consider setting options.sync = true.
  // 将“key”的数据库条目设置为“value”。成功时返回OK，错误时返回非OK状态。
  // 提醒：考虑设置选项。sync=true。
  virtual Status Put(const WriteOptions& options, const Slice& key,
                     const Slice& value) = 0;

  // Remove the database entry (if any) for "key".  Returns OK on
  // success, and a non-OK status on error.  It is not an error if "key"
  // did not exist in the database.
  // Note: consider setting options.sync = true.
  // 删除“key”的数据库条目（如果有）。成功时返回OK，错误时返回非OK状态。如果数据库中不存在“key”，则不是错误。
  // 提醒：考虑设置选项。sync=true。
  virtual Status Delete(const WriteOptions& options, const Slice& key) = 0;

  // Apply the specified updates to the database.
  // Returns OK on success, non-OK on failure.
  // Note: consider setting options.sync = true.
  // 将指定的更新应用于数据库。
  // 成功时返回OK，失败时返回非OK。
  // 提醒：考虑设置选项。sync=true。
  virtual Status Write(const WriteOptions& options, WriteBatch* updates) = 0;

  // If the database contains an entry for "key" store the
  // corresponding value in *value and return OK.
  //
  // If there is no entry for "key" leave *value unchanged and return
  // a status for which Status::IsNotFound() returns true.
  //
  // May return some other Status on an error.
  // 如果数据库包含“key”的条目，则将相应的值存储在*value中，并返回OK。
  //
  // 如果没有“key”的条目，则保持*value不变，并返回一个status::IsNotFound（）返回true的状态。
  //
  // 可能会在出现错误时返回其他状态。
  virtual Status Get(const ReadOptions& options, const Slice& key,
                     std::string* value) = 0;

  // Return a heap-allocated iterator over the contents of the database.
  // The result of NewIterator() is initially invalid (caller must
  // call one of the Seek methods on the iterator before using it).
  //
  // Caller should delete the iterator when it is no longer needed.
  // The returned iterator should be deleted before this db is deleted.
  // 在数据库内容上返回一个堆分配的迭代器。
  // NewIterator()的结果最初无效（调用方必须在使用迭代器之前调用迭代器上的一个Seek方法）。
  //
  // 当不再需要迭代器时，调用方应该删除它。
  // 在删除此数据库之前，应删除返回的迭代器。
  virtual Iterator* NewIterator(const ReadOptions& options) = 0;

  // Return a handle to the current DB state.  Iterators created with
  // this handle will all observe a stable snapshot of the current DB
  // state.  The caller must call ReleaseSnapshot(result) when the
  // snapshot is no longer needed.
  // 返回当前DB状态的一个句柄。用这个句柄创建的迭代器将观察当前数据库状态的稳定快照。
  // 当不再需要快照时，调用方必须用这个函数的返回值来调用ReleaseSnapshot()。
  virtual const Snapshot* GetSnapshot() = 0;

  // Release a previously acquired snapshot.  The caller must not
  // use "snapshot" after this call.
  // 释放之前获取的快照
  // 调用者在调用后不要使用“snapshot”
  virtual void ReleaseSnapshot(const Snapshot* snapshot) = 0;

  // DB implementations can export properties about their state
  // via this method.  If "property" is a valid property understood by this
  // DB implementation, fills "*value" with its current value and returns
  // true.  Otherwise returns false.
  // DB实现可以通过此方法导出有关其状态的属性。
  // 如果“property”是此DB实现可以理解的有效属性，则用其当前值填充“*value”，并返回true。否则返回false。
  //
  //
  // Valid property names include:
  // 有效的属性名字包括：
  //
  //  "leveldb.num-files-at-level<N>" - return the number of files at level <N>,
  //     where <N> is an ASCII representation of a level number (e.g. "0").
  // 返回级别<N>的文件数，其中<N>是级别号的ASCII表示（例如“0”）。
  //  "leveldb.stats" - returns a multi-line string that describes statistics
  //     about the internal operation of the DB.
  // 返回一个多行字符串，该字符串描述有关数据库内部操作的统计信息。
  //  "leveldb.sstables" - returns a multi-line string that describes all
  //     of the sstables that make up the db contents.
  // 返回一个多行字符串，该字符串描述构成db内容的所有sstables。
  //  "leveldb.approximate-memory-usage" - returns the approximate number of
  //     bytes of memory in use by the DB.
  // 返回数据库使用的内存字节数。
  virtual bool GetProperty(const Slice& property, std::string* value) = 0;

  // For each i in [0,n-1], store in "sizes[i]", the approximate
  // file system space used by keys in "[range[i].start .. range[i].limit)".
  //
  // Note that the returned sizes measure file system space usage, so
  // if the user data compresses by a factor of ten, the returned
  // sizes will be one-tenth the size of the corresponding user data size.
  //
  // The results may not include the sizes of recently written data.
  // 在范围[0,n-1]遍历每个i，这个范围存在 "sizes[i]"
  // “[range[i].start..range[i].limit”中的键使用的近似文件系统空间。
  // 
  // 请注意，返回的大小度量文件系统空间使用情况，因此如果用户数据压缩了10倍，则返回的大小将是相应用户数据大小的十分之一。
  //
  // 结果可能不包括最近写入的数据的大小。
  virtual void GetApproximateSizes(const Range* range, int n,
                                   uint64_t* sizes) = 0;

  // Compact the underlying storage for the key range [*begin,*end].
  // In particular, deleted and overwritten versions are discarded,
  // and the data is rearranged to reduce the cost of operations
  // needed to access the data.  This operation should typically only
  // be invoked by users who understand the underlying implementation.
  //
  // begin==nullptr is treated as a key before all keys in the database.
  // end==nullptr is treated as a key after all keys in the database.
  // Therefore the following call will compact the entire database:
  //    db->CompactRange(nullptr, nullptr);
  // 压缩键在范围[*begin，*end]里的底层存储。
  // 特别是，删除和覆盖的版本被丢弃，数据被重新排列，以减少访问数据所需的操作成本。
  // 此操作通常只应由了解底层实现的用户调用。
  //
  // begin==nullptr被视为数据库中所有键之前的键。
  // end==nullptr被视为数据库中所有键之后的键。
  // 因此，以下调用将压缩整个数据库：
  // db->CompactRange(nullptr，nullptr);
  virtual void CompactRange(const Slice* begin, const Slice* end) = 0;
};

// Destroy the contents of the specified database.
// Be very careful using this method.
//
// Note: For backwards compatibility, if DestroyDB is unable to list the
// database files, Status::OK() will still be returned masking this failure.
// 销毁指定数据库的内容。
// 使用这种方法要非常小心。
//
// 注意：为了向后兼容，如果DestroyDB无法列出数据库文件，仍然会返回Status::OK()，以掩盖此故障。
LEVELDB_EXPORT Status DestroyDB(const std::string& name,
                                const Options& options);

// If a DB cannot be opened, you may attempt to call this method to
// resurrect as much of the contents of the database as possible.
// Some data may be lost, so be careful when calling this function
// on a database that contains important information.
// 如果无法打开数据库，可以尝试调用此方法以尽可能多地恢复数据库的内容。
// 有些数据可能会丢失，因此在包含重要信息的数据库上调用此函数时要小心。
LEVELDB_EXPORT Status RepairDB(const std::string& dbname,
                               const Options& options);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_DB_H_
