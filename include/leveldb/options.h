// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_INCLUDE_OPTIONS_H_
#define STORAGE_LEVELDB_INCLUDE_OPTIONS_H_

#include <cstddef>

#include "leveldb/export.h"

namespace leveldb {

class Cache;
class Comparator;
class Env;
class FilterPolicy;
class Logger;
class Snapshot;

// DB contents are stored in a set of blocks, each of which holds a
// sequence of key,value pairs.  Each block may be compressed before
// being stored in a file.  The following enum describes which
// compression method (if any) is used to compress a block.
// DB内容存储在一组块中，每个块都保存一系列键、值对。
// 在存储到文件中之前，可以对每个块进行压缩。
// 下面的枚举描述了用于压缩块的压缩方法（如果有）。
enum CompressionType {
  // NOTE: do not change the values of existing entries, as these are
  // part of the persistent format on disk.
  // 注意：不要更改现有条目的值，因为它们是磁盘上持久格式的一部分。
  kNoCompression = 0x0,
  kSnappyCompression = 0x1
};

// Options to control the behavior of a database (passed to DB::Open)
// Options是控制数据库行为的选项（传递给DB::Open）
struct LEVELDB_EXPORT Options {
  // Create an Options object with default values for all fields.
  // 使用所有字段的默认值创建Option对象。
  Options();

  // -------------------
  // Parameters that affect behavior

  // Comparator used to define the order of keys in the table.
  // Default: a comparator that uses lexicographic byte-wise ordering
  //
  // REQUIRES: The client must ensure that the comparator supplied
  // here has the same name and orders keys *exactly* the same as the
  // comparator provided to previous open calls on the same DB.
  //
  // 影响行为的参数
  // 
  // 用于定义表中键的顺序的比较器。
  // 默认值：使用字典字节顺序的比较器 
  // 要求：客户端调用必须确保此处提供的比较器具有相同的名称和顺序键*完全*与提供给同一数据库上以前打开调用的比较器相同。
  const Comparator* comparator;

  // If true, the database will be created if it is missing.
  // 如果为true，则将创建缺少的数据库。
  bool create_if_missing = false;

  // If true, an error is raised if the database already exists.
  // 如果为true，则在数据库已存在时引发错误。
  bool error_if_exists = false;

  // If true, the implementation will do aggressive checking of the
  // data it is processing and will stop early if it detects any
  // errors.  This may have unforeseen ramifications: for example, a
  // corruption of one DB entry may cause a large number of entries to
  // become unreadable or for the entire DB to become unopenable.
  // 如果为true，则实现将对正在处理的数据进行积极检查，
  // 如果检测到任何错误，将提前停止。
  // 这可能会产生不可预见的后果：
  // 例如，一个DB条目的损坏可能会导致大量条目无法读取或整个DB变得不可打开。
  bool paranoid_checks = false;

  // Use the specified object to interact with the environment,
  // e.g. to read/write files, schedule background work, etc.
  // Default: Env::Default()
  // 使用指定的对象与环境交互，例如读/写文件、安排后台工作等。
  // 默认值：Env::Default()
  Env* env;

  // Any internal progress/error information generated by the db will
  // be written to info_log if it is non-null, or to a file stored
  // in the same directory as the DB contents if info_log is null.
  // 如果info_log不为空任何由db生成内部进度/错误信息都会被写到info_log
  // 或者如果info_log为空时，存到一个文件里，文件放在DB内容所在的目录
  Logger* info_log = nullptr;

  // -------------------
  // Parameters that affect performance

  // Amount of data to build up in memory (backed by an unsorted log
  // on disk) before converting to a sorted on-disk file.
  //
  // Larger values increase performance, especially during bulk loads.
  // Up to two write buffers may be held in memory at the same time,
  // so you may wish to adjust this parameter to control memory usage.
  // Also, a larger write buffer will result in a longer recovery time
  // the next time the database is opened.
  // 
  // 参数影响性能
  // 
  // 在转换为已排序的磁盘文件之前，要在内存中建立的数据量（由未排序的磁盘日志支持）。
  //
  // 较大的值可提高性能，尤其是在批量加载期间。
  // 内存中最多可同时保存两个写缓冲区，
  // 因此，您可能希望调整此参数以控制内存使用。
  // 此外，更大的写入缓冲区将导致下次打开数据库时恢复时间更长。
  size_t write_buffer_size = 4 * 1024 * 1024;

  // Number of open files that can be used by the DB.  You may need to
  // increase this if your database has a large working set (budget
  // one open file per 2MB of working set).
  // 数据库可以使用的打开文件数。
  // 如果您的数据库有一个较大的工作集（预算每2MB工作集打开一个文件），则可能需要增加这个值。
  int max_open_files = 1000;

  // Control over blocks (user data is stored in a set of blocks, and
  // a block is the unit of reading from disk).

  // If non-null, use the specified cache for blocks.
  // If null, leveldb will automatically create and use an 8MB internal cache.
  // 对块的控制（用户数据存储在一组块中，块是从磁盘读取的单位）。
  //
  // 如果非空，请使用块的指定缓存。
  // 如果为空，leveldb将自动创建并使用8MB内部缓存。
  Cache* block_cache = nullptr;

  // Approximate size of user data packed per block.  Note that the
  // block size specified here corresponds to uncompressed data.  The
  // actual size of the unit read from disk may be smaller if
  // compression is enabled.  This parameter can be changed dynamically.
  // 每个块打包的用户数据的近似大小。请注意，此处指定的块大小对应于未压缩的数据。
  // 如果启用压缩，则从磁盘读取的单元的实际大小可能更小。此参数可以动态更改。
  size_t block_size = 4 * 1024;

  // Number of keys between restart points for delta encoding of keys.
  // This parameter can be changed dynamically.  Most clients should
  // leave this parameter alone.
  // 在重启点之间的键的数量，for delta encoding of keys.（不知道是什么？）
  // 此参数可以动态更改。大多数客户端应该不使用此参数。
  int block_restart_interval = 16;

  // Leveldb will write up to this amount of bytes to a file before
  // switching to a new one.
  // Most clients should leave this parameter alone.  However if your
  // filesystem is more efficient with larger files, you could
  // consider increasing the value.  The downside will be longer
  // compactions and hence longer latency/performance hiccups.
  // Another reason to increase this parameter might be when you are
  // initially populating a large database.
  // 在切换到新文件之前，Leveldb将向一个文件写入多达此数量的字节。
  // 大多数客户机应该不使用此参数。
  // 但是，如果文件系统更大，文件效率更高，则可以考虑增加值。
  // 缺点是压缩时间更长，因此延迟/性能问题更长。
  // 增加此参数的另一个原因可能是在最初填充大型数据库时。
  size_t max_file_size = 2 * 1024 * 1024;

  // Compress blocks using the specified compression algorithm.  This
  // parameter can be changed dynamically.
  //
  // Default: kSnappyCompression, which gives lightweight but fast
  // compression.
  //
  // Typical speeds of kSnappyCompression on an Intel(R) Core(TM)2 2.4GHz:
  //    ~200-500MB/s compression
  //    ~400-800MB/s decompression
  // Note that these speeds are significantly faster than most
  // persistent storage speeds, and therefore it is typically never
  // worth switching to kNoCompression.  Even if the input data is
  // incompressible, the kSnappyCompression implementation will
  // efficiently detect that and will switch to uncompressed mode.
  // 使用指定的压缩算法压缩块。此参数可以动态更改。
  //
  // 默认值：kSnappyCompression，它提供轻量级但快速的压缩。
  // 英特尔（R）Core（TM）2.4GHz上kSnappyCompression的典型速度：
  //    ~200-500MB/s压缩
  //    ~400-800MB/s解压
  // 请注意，这些速度明显快于大多数持久存储速度，因此通常不值得切换到kNoCompression。
  // 即使输入数据是不可压缩的，kSnappyCompression实现也会有效地检测到这一点，并切换到未压缩模式。
  CompressionType compression = kSnappyCompression;

  // EXPERIMENTAL: If true, append to existing MANIFEST and log files
  // when a database is opened.  This can significantly speed up open.
  //
  // Default: currently false, but may become true later.
  //实验性：如果为true，则在打开数据库时附加到现有MANIFEST和日志文件。这可以显著加快打开速度。
  //
  //默认值：当前为false，但稍后可能变为true。
  bool reuse_logs = false;

  // If non-null, use the specified filter policy to reduce disk reads.
  // Many applications will benefit from passing the result of
  // NewBloomFilterPolicy() here.
  // 如果非空，请使用指定的筛选器策略减少磁盘读取。
  // 许多应用程序将受益于在此处传递NewBloomFilterPolicy()的结果。
  const FilterPolicy* filter_policy = nullptr;
};

// Options that control read operations
// 控制读操作的选项
struct LEVELDB_EXPORT ReadOptions {
  ReadOptions() = default;

  // If true, all data read from underlying storage will be
  // verified against corresponding checksums.
  // 如果为true，则从底层存储读取的所有数据都将
  // 根据相应的校验和进行验证。
  bool verify_checksums = false;

  // Should the data read for this iteration be cached in memory?
  // Callers may wish to set this field to false for bulk scans.
  // 为该迭代读取的数据是否应该缓存在内存中？
  // 对于批量扫描，调用者可能希望将此字段设置为false。
  bool fill_cache = true;

  // If "snapshot" is non-null, read as of the supplied snapshot
  // (which must belong to the DB that is being read and which must
  // not have been released).  If "snapshot" is null, use an implicit
  // snapshot of the state at the beginning of this read operation.
  // 如果"snapshot"非空，则从提供的快照开始读取（该快照必须属于正在读取的数据库，并且还没有释放）。
  // 如果"snapshot"为空，则使用读操作开始时的状态隐式快照
  const Snapshot* snapshot = nullptr;
};

// Options that control write operations
// 控制写操作的选项
struct LEVELDB_EXPORT WriteOptions {
  WriteOptions() = default;

  // If true, the write will be flushed from the operating system
  // buffer cache (by calling WritableFile::Sync()) before the write
  // is considered complete.  If this flag is true, writes will be
  // slower.
  //
  // If this flag is false, and the machine crashes, some recent
  // writes may be lost.  Note that if it is just the process that
  // crashes (i.e., the machine does not reboot), no writes will be
  // lost even if sync==false.
  //
  // In other words, a DB write with sync==false has similar
  // crash semantics as the "write()" system call.  A DB write
  // with sync==true has similar crash semantics to a "write()"
  // system call followed by "fsync()".
  //
  // 如果为true，则在写入被视为完成之前，将从操作系统缓冲区缓存中刷新写入（通过调用WritableFile::Sync()）。
  // 如果此标志为true，则写入速度将变慢。
  // 
  // 如果此标志为false，并且计算机崩溃，则可能会丢失最近的一些写入。
  // 请注意，如果只是进程崩溃（即，机器没有重新启动），则即使sync==false，也不会丢失任何写操作。
  // 
  // 换句话说，sync==false的DB write具有与“write（）”系统调用类似的崩溃语义。
  // sync==true的DB write具有与“write（）”系统调用后跟“fsync（）”类似的崩溃语义。
  bool sync = false;
};

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_INCLUDE_OPTIONS_H_