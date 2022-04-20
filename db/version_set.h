// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// The representation of a DBImpl consists of a set of Versions.  The
// newest version is called "current".  Older versions may be kept
// around to provide a consistent view to live iterators.
//
// Each Version keeps track of a set of Table files per level.  The
// entire set of versions is maintained in a VersionSet.
//
// Version,VersionSet are thread-compatible, but require external
// synchronization on all accesses.
// 
// DBImpl的表示由一组版本组成。最新版本被称为“当前”。
// 旧版本可能会保留下来，以便为实时迭代器提供一致的视图。
// 
// 每个版本跟踪每个层级的一组表文件。整个版本集在VersionSet中维护。
//
// Version、VersionSet与线程兼容，但需要在所有访问上进行外部同步。

#ifndef STORAGE_LEVELDB_DB_VERSION_SET_H_
#define STORAGE_LEVELDB_DB_VERSION_SET_H_

#include <map>
#include <set>
#include <vector>

#include "db/dbformat.h"
#include "db/version_edit.h"
#include "port/port.h"
#include "port/thread_annotations.h"

namespace leveldb {

namespace log {
class Writer;
}

class Compaction;
class Iterator;
class MemTable;
class TableBuilder;
class TableCache;
class Version;
class VersionSet;
class WritableFile;

// Return the smallest index i such that files[i]->largest >= key.
// Return files.size() if there is no such file.
// REQUIRES: "files" contains a sorted list of non-overlapping files.
// 返回files[i]->largest >= key 时的最小索引i
// 如果没有这样的文件，则返回files.size()
// 要求："files"包含一个排序的非重叠的文件列表
int FindFile(const InternalKeyComparator& icmp,
             const std::vector<FileMetaData*>& files, const Slice& key);

// Returns true iff some file in "files" overlaps the user key range
// [*smallest,*largest].
// smallest==nullptr represents a key smaller than all keys in the DB.
// largest==nullptr represents a key largest than all keys in the DB.
// REQUIRES: If disjoint_sorted_files, files[] contains disjoint ranges
//           in sorted order.
// 如果“files”里的一些文件与用户键范围[*smallest,*largest]重叠，则返回true
// smallest_user_key==nullptr 表示键比所有在DB的键都要小
// largest_user_key==nullptr 表示键比所有在DB的键都要大
// 要求：如果disjoint_sorted_files为true，files[]在排序顺序下，包含不相交的范围
bool SomeFileOverlapsRange(const InternalKeyComparator& icmp,
                           bool disjoint_sorted_files,
                           const std::vector<FileMetaData*>& files,
                           const Slice* smallest_user_key,
                           const Slice* largest_user_key);

class Version {
 public:
  struct GetStats {
    FileMetaData* seek_file;
    int seek_file_level;
  };

  // Append to *iters a sequence of iterators that will
  // yield the contents of this Version when merged together.
  // REQUIRES: This version has been saved (see VersionSet::SaveTo)
  // 向*iters追加一系列迭代器，合并后将生成此版本的内容。
  // 要求：此版本已保存（请参阅VersionSet::SaveTo）
  void AddIterators(const ReadOptions&, std::vector<Iterator*>* iters);

  // Lookup the value for key.  If found, store it in *val and
  // return OK.  Else return a non-OK status.  Fills *stats.
  // REQUIRES: lock is not held
  // 查找键的值。如果找到，将其存储在*val中，然后
  // 返回OK。否则返回非OK状态。填充*stats。
  // 要求：未持有锁
  Status Get(const ReadOptions&, const LookupKey& key, std::string* val,
             GetStats* stats);

  // Adds "stats" into the current state.  Returns true if a new
  // compaction may need to be triggered, false otherwise.
  // REQUIRES: lock is held
  // 将“stats”添加到当前状态。如果可能需要触发新的压缩，则返回true，否则返回false。
  // 要求：持有锁
  bool UpdateStats(const GetStats& stats);

  // Record a sample of bytes read at the specified internal key.
  // Samples are taken approximately once every config::kReadBytesPeriod
  // bytes.  Returns true if a new compaction may need to be triggered.
  // REQUIRES: lock is held
  // 记录在指定的内部键处读取的字节样本。
  // 大约每个config::kReadBytesPeriod字节采集一次样本。
  // 如果可能需要触发新的压缩，则返回true。
  // 要求：持有锁
  bool RecordReadSample(Slice key);

  // Reference count management (so Versions do not disappear out from
  // under live iterators)
  // 引用计数管理（这样版本不会从活的迭代器中消失）
  void Ref();
  void Unref();

  void GetOverlappingInputs(
      int level,
      const InternalKey* begin,  // nullptr means before all keys  nullptr表示在所有键之前
      const InternalKey* end,    // nullptr means after all keys nullptr表示在所有键之后
      std::vector<FileMetaData*>* inputs);

  // Returns true iff some file in the specified level overlaps
  // some part of [*smallest_user_key,*largest_user_key].
  // smallest_user_key==nullptr represents a key smaller than all the DB's keys.
  // largest_user_key==nullptr represents a key largest than all the DB's keys.
  // 如果指定层级的某个文件与[*smallest_user_key,*largest_user_key]的某个部分重叠，则返回true。
  // smallest_user_key==nullptr 表示键比所有在DB的键都要小
  // largest_user_key==nullptr 表示键比所有在DB的键都要大
  bool OverlapInLevel(int level, const Slice* smallest_user_key,
                      const Slice* largest_user_key);

  // Return the level at which we should place a new memtable compaction
  // result that covers the range [smallest_user_key,largest_user_key].
  // 返回应该放置新memtable压缩结果的层级，该结果覆盖范围[smallest_user_key,largest_user_key]
  int PickLevelForMemTableOutput(const Slice& smallest_user_key,
                                 const Slice& largest_user_key);

  int NumFiles(int level) const { return files_[level].size(); }

  // Return a human readable string that describes this version's contents.
  // 返回描述此版本内容的可读字符串。
  std::string DebugString() const;

 private:
  friend class Compaction;
  friend class VersionSet;

  class LevelFileNumIterator;

  explicit Version(VersionSet* vset)
      : vset_(vset),
        next_(this),
        prev_(this),
        refs_(0),
        file_to_compact_(nullptr),
        file_to_compact_level_(-1),
        compaction_score_(-1),
        compaction_level_(-1) {}

  Version(const Version&) = delete;
  Version& operator=(const Version&) = delete;

  ~Version();

  Iterator* NewConcatenatingIterator(const ReadOptions&, int level) const;

  // Call func(arg, level, f) for every file that overlaps user_key in
  // order from newest to oldest.  If an invocation of func returns
  // false, makes no more calls.
  //
  // REQUIRES: user portion of internal_key == user_key.
  // 
  // 为每个跟user_key重叠的文件调用func(arg, level, f)，顺序是从最新到最旧
  // 如果func的调用返回false， 则不再调用了（退出）
  //
  // 要求：internal_key的用户部分 == user_key
  void ForEachOverlapping(Slice user_key, Slice internal_key, void* arg,
                          bool (*func)(void*, int, FileMetaData*));

  VersionSet* vset_;  // VersionSet to which this Version belongs 此版本所属的版本集
  Version* next_;     // Next version in linked list 链表的下个版本
  Version* prev_;     // Previous version in linked list  链表的上个版本 
  int refs_;          // Number of live refs to this version 这个版本的活的引用数

  // List of files per level
  // 每个层级的文件列表
  std::vector<FileMetaData*> files_[config::kNumLevels];

  // Next file to compact based on seek stats.
  // 基于查找的统计数据的下一个需要压缩的文件
  FileMetaData* file_to_compact_;
  int file_to_compact_level_;

  // Level that should be compacted next and its compaction score.
  // Score < 1 means compaction is not strictly needed.  These fields
  // are initialized by Finalize().
  // 接下来应该被压缩的层级和它的压缩分数
  // 分数 < 1 意味着压缩不是严格的需要
  // 这些字段在Finalize()初始化
  double compaction_score_;
  int compaction_level_;
};

class VersionSet {
 public:
  VersionSet(const std::string& dbname, const Options* options,
             TableCache* table_cache, const InternalKeyComparator*);
  VersionSet(const VersionSet&) = delete;
  VersionSet& operator=(const VersionSet&) = delete;

  ~VersionSet();

  // Apply *edit to the current version to form a new descriptor that
  // is both saved to persistent state and installed as the new
  // current version.  Will release *mu while actually writing to the file.
  // REQUIRES: *mu is held on entry.
  // REQUIRES: no other thread concurrently calls LogAndApply()
  // 应用*edit到当前版本，然后生成新的描述符，它被保存到一个持久化状态（落盘）和被安装成为一个新的当前版本
  // 要求：*mu在条目上被持有
  // 要求：没有其他线程同时访问LogAndApply()
  Status LogAndApply(VersionEdit* edit, port::Mutex* mu)
      EXCLUSIVE_LOCKS_REQUIRED(mu);

  // Recover the last saved descriptor from persistent storage.
  // 从持久化存储那里恢复上次保存的描述符
  Status Recover(bool* save_manifest);

  // Return the current version.
  // 返回当前版本
  Version* current() const { return current_; }

  // Return the current manifest file number
  // 返回当前清单文件号
  uint64_t ManifestFileNumber() const { return manifest_file_number_; }

  // Allocate and return a new file number
  // 分配和返回一个新的文件号
  uint64_t NewFileNumber() { return next_file_number_++; }

  // Arrange to reuse "file_number" unless a newer file number has
  // already been allocated.
  // REQUIRES: "file_number" was returned by a call to NewFileNumber().
  // 安排重复使用“文件号”，除非已经分配了新的文件号。
  // 要求:“file_number”是通过调用NewFileNumber()返回的。
  void ReuseFileNumber(uint64_t file_number) {
    if (next_file_number_ == file_number + 1) {
      next_file_number_ = file_number;
    }
  }

  // Return the number of Table files at the specified level.
  // 返回指定层的表文件数量
  int NumLevelFiles(int level) const;

  // Return the combined file size of all files at the specified level.
  // 返回指定层上所有文件的组合文件大小。
  int64_t NumLevelBytes(int level) const;

  // Return the last sequence number.
  // 返回上一个序列号
  uint64_t LastSequence() const { return last_sequence_; }

  // Set the last sequence number to s.
  // 把s设置为上一个序列号
  void SetLastSequence(uint64_t s) {
    assert(s >= last_sequence_);
    last_sequence_ = s;
  }

  // Mark the specified file number as used.
  // 将指定的文件号标记为已使用。
  void MarkFileNumberUsed(uint64_t number);

  // Return the current log file number.
  // 返回当前日志文件号。
  uint64_t LogNumber() const { return log_number_; }

  // Return the log file number for the log file that is currently
  // being compacted, or zero if there is no such log file.
  // 返回当前正在压缩的日志文件的日志文件号，如果没有此类日志文件，则返回零。
  uint64_t PrevLogNumber() const { return prev_log_number_; }

  // Pick level and inputs for a new compaction.
  // Returns nullptr if there is no compaction to be done.
  // Otherwise returns a pointer to a heap-allocated object that
  // describes the compaction.  Caller should delete the result.
  // 为一个压缩准备层和输入
  // 如果没有压缩需要完成，则返回nullptr
  // 否则返回一个指向堆分配对象的指针
  // 这个指针用来描述这个压缩。调用者应该负责删除这个压缩对象
  Compaction* PickCompaction();

  // Return a compaction object for compacting the range [begin,end] in
  // the specified level.  Returns nullptr if there is nothing in that
  // level that overlaps the specified range.  Caller should delete
  // the result.
  // 返回一个压缩对象，用于在指定级别中压缩范围[begin，end]。
  // 如果该层中没有与指定范围重叠的内容，则返回nullptr。
  // 调用者应该删除这个压缩对象。
  Compaction* CompactRange(int level, const InternalKey* begin,
                           const InternalKey* end);

  // Return the maximum overlapping data (in bytes) at next level for any
  // file at a level >= 1.
  // 对于层>=1的任何文件，返回下一层的最大重叠数据大小（字节大小）。
  int64_t MaxNextLevelOverlappingBytes();

  // Create an iterator that reads over the compaction inputs for "*c".
  // The caller should delete the iterator when no longer needed.
  // 创建一个迭代器，读取“*c”的压缩输入。
  // 当不再需要迭代器时，调用方应该删除迭代器。
  Iterator* MakeInputIterator(Compaction* c);

  // Returns true iff some level needs a compaction.
  // 如果某层需要压缩就返回true
  bool NeedsCompaction() const {
    Version* v = current_;
    return (v->compaction_score_ >= 1) || (v->file_to_compact_ != nullptr);
  }

  // Add all files listed in any live version to *live.
  // May also mutate some internal state.
  // 将任何live版本中列出的所有文件添加到*live。
  // 也可能会改变一些内部状态。
  void AddLiveFiles(std::set<uint64_t>* live);

  // Return the approximate offset in the database of the data for
  // "key" as of version "v".
  // 返回自版本“v”起“key”数据在数据库中的近似偏移量。
  uint64_t ApproximateOffsetOf(Version* v, const InternalKey& key);

  // Return a human-readable short (single-line) summary of the number
  // of files per level.  Uses *scratch as backing store.
  // 返回每个层的文件数的可读简短（单行）摘要。使用*scratch作为后台存储。
  struct LevelSummaryStorage {
    char buffer[100];
  };
  const char* LevelSummary(LevelSummaryStorage* scratch) const;

 private:
  class Builder;

  friend class Compaction;
  friend class Version;

  bool ReuseManifest(const std::string& dscname, const std::string& dscbase);

  void Finalize(Version* v);

  void GetRange(const std::vector<FileMetaData*>& inputs, InternalKey* smallest,
                InternalKey* largest);

  void GetRange2(const std::vector<FileMetaData*>& inputs1,
                 const std::vector<FileMetaData*>& inputs2,
                 InternalKey* smallest, InternalKey* largest);

  void SetupOtherInputs(Compaction* c);

  // Save current contents to *log
  // 保存当前内容到*log
  Status WriteSnapshot(log::Writer* log);

  void AppendVersion(Version* v);

  Env* const env_;
  const std::string dbname_;
  const Options* const options_;
  TableCache* const table_cache_;
  const InternalKeyComparator icmp_;
  uint64_t next_file_number_;
  uint64_t manifest_file_number_;
  uint64_t last_sequence_;
  uint64_t log_number_;
  uint64_t prev_log_number_;  // 0 or backing store for memtable being compacted 0或正在压缩的memtable的后台存储

  // Opened lazily
  // 延迟打开
  WritableFile* descriptor_file_;
  log::Writer* descriptor_log_;
  Version dummy_versions_;  // Head of circular doubly-linked list of versions. 循环双链接版本列表的头
  Version* current_;        // == dummy_versions_.prev_

  // Per-level key at which the next compaction at that level should start.
  // Either an empty string, or a valid InternalKey.
  std::string compact_pointer_[config::kNumLevels];
};

// A Compaction encapsulates information about a compaction.
// 一个Compaction 包装一个压缩的所有信息
class Compaction {
 public:
  ~Compaction();

  // Return the level that is being compacted.  Inputs from "level"
  // and "level+1" will be merged to produce a set of "level+1" files.
  // 返回正在压缩的层级，来自"level"和"level+1"的输入将被合并产生一个"level+1"的文件集合
  int level() const { return level_; }

  // Return the object that holds the edits to the descriptor done
  // by this compaction.
  // 返回一个对象，它持有这个压缩对描述符的修改
  VersionEdit* edit() { return &edit_; }

  // "which" must be either 0 or 1
  // “which”必须0或者1
  int num_input_files(int which) const { return inputs_[which].size(); }

  // Return the ith input file at "level()+which" ("which" must be 0 or 1).
  // 返回在"level()+which"的第i个输入文件（“which”必须0活着1）
  FileMetaData* input(int which, int i) const { return inputs_[which][i]; }

  // Maximum size of files to build during this compaction.
  // 在此压缩过程中要生成的最大文件大小。
  uint64_t MaxOutputFileSize() const { return max_output_file_size_; }

  // Is this a trivial compaction that can be implemented by just
  // moving a single input file to the next level (no merging or splitting)
  // 这是否是一个简单的压缩，只需将单个输入文件移动到下一个层（无合并或拆分）即可实现
  bool IsTrivialMove() const;

  // Add all inputs to this compaction as delete operations to *edit.
  // 将此压缩的所有输入添加为*edit的删除操作。
  void AddInputDeletions(VersionEdit* edit);

  // Returns true if the information we have available guarantees that
  // the compaction is producing data in "level+1" for which no data exists
  // in levels greater than "level+1".
  // 如果我们现有的信息保证这次在“level+1”生成的数据在大于"level+1"的层中不存在，则返回true
  bool IsBaseLevelForKey(const Slice& user_key);

  // Returns true iff we should stop building the current output
  // before processing "internal_key".
  // 如果在处理“internal_key”之前我们应该停止构建当前输出，则返回true。
  bool ShouldStopBefore(const Slice& internal_key);

  // Release the input version for the compaction, once the compaction
  // is successful.
  // 一旦压缩成功，则释放压缩关联的输入版本
  void ReleaseInputs();

 private:
  friend class Version;
  friend class VersionSet;

  Compaction(const Options* options, int level);

  int level_;  // 要压缩的层
  uint64_t max_output_file_size_;  // 最大文件大小
  Version* input_version_;    // 当前压缩的版本
  VersionEdit edit_;   // 压缩成功生成的VersionEdit

  // Each compaction reads inputs from "level_" and "level_+1"
  // 每个压缩都是从"level_" 和 "level_+1" 里的文件作为输入 
  std::vector<FileMetaData*> inputs_[2];  // The two sets of inputs 两组输入，分别对应"level_" 和 "level_+1"

  // State used to check for number of overlapping grandparent files
  // (parent == level_ + 1, grandparent == level_ + 2)
  // 用于检查重叠祖父母文件数量的状态
  std::vector<FileMetaData*> grandparents_; // 当前压缩跟祖父母的重叠文件
  size_t grandparent_index_;  // Index in grandparent_starts_ 在grandparent_starts_的索引
  bool seen_key_;             // Some output key has been seen  已看到一些输出键
  int64_t overlapped_bytes_;  // Bytes of overlap between current output
                              // and grandparent files 当前输出和祖辈文件之间的重叠字节数

  // State for implementing IsBaseLevelForKey
  // 状态用于实现IsBaseLevelForKey

  // level_ptrs_ holds indices into input_version_->levels_: our state
  // is that we are positioned at one of the file ranges for each
  // higher level than the ones involved in this compaction (i.e. for
  // all L >= level_ + 2).
  // level_ptrs_持有的是每个层在IsBaseLevelForKey函数调用后的索引，这个状态能够加快调用IsBaseLevelForKey这个函数，避免重复计算没意义的数据
  // 上面英文注释不知道怎么翻译，具体看看这个version_set.cc 文件中 IsBaseLevelForKey 函数的中文注释，应该就好理解这个东西的作用了
  size_t level_ptrs_[config::kNumLevels];
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_VERSION_SET_H_
