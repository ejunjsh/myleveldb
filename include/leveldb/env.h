// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// An Env is an interface used by the leveldb implementation to access
// operating system functionality like the filesystem etc.  Callers
// may wish to provide a custom Env object when opening a database to
// get fine gain control; e.g., to rate limit file system operations.
//
// All Env implementations are safe for concurrent access from
// multiple threads without any external synchronization.
// Env是leveldb实现用来访问操作系统功能（如文件系统等）的接口。
// 调用方可能希望在打开数据库时提供自定义Env对象，以获得良好的增益控制；
// 例如，对文件系统操作进行速率限制。
//
// 所有Env实现都可以安全地从多个线程进行并发访问，而无需任何外部同步。

#ifndef STORAGE_LEVELDB_INCLUDE_ENV_H_
#define STORAGE_LEVELDB_INCLUDE_ENV_H_

#include <cstdarg>
#include <cstdint>
#include <string>
#include <vector>

#include "leveldb/export.h"
#include "leveldb/status.h"

// This workaround can be removed when leveldb::Env::DeleteFile is removed.
// 删除leveldb::Env::DeleteFile时，可以删除此变通方法。
#if defined(_WIN32)
// On Windows, the method name DeleteFile (below) introduces the risk of
// triggering undefined behavior by exposing the compiler to different
// declarations of the Env class in different translation units.
//
// This is because <windows.h>, a fairly popular header file for Windows
// applications, defines a DeleteFile macro. So, files that include the Windows
// header before this header will contain an altered Env declaration.
//
// This workaround ensures that the compiler sees the same Env declaration,
// independently of whether <windows.h> was included.
//
// 在Windows上，方法名DeleteFile（如下）通过将编译器暴露于不同翻译单元中的Env类的不同声明，引入了触发未定义行为的风险。
// 
// 这是因为 <windows.h> 是Windows应用程序中相当流行的头文件，它定义了DeleteFile宏
// 所以文件包含这个头文件之前包含了windows的头文件，将会包含一个修改的Env定义
// 
// 此解决方案确保编译器看到相同的Env声明，而与是否包含<windows.h>无关
#if defined(DeleteFile)
#undef DeleteFile
#define LEVELDB_DELETEFILE_UNDEFINED
#endif  // defined(DeleteFile)
#endif  // defined(_WIN32)

namespace leveldb {

class FileLock;
class Logger;
class RandomAccessFile;
class SequentialFile;
class Slice;
class WritableFile;

class LEVELDB_EXPORT Env {
 public:
  Env();

  Env(const Env&) = delete;
  Env& operator=(const Env&) = delete;

  virtual ~Env();

  // Return a default environment suitable for the current operating
  // system.  Sophisticated users may wish to provide their own Env
  // implementation instead of relying on this default environment.
  //
  // The result of Default() belongs to leveldb and must never be deleted.
  //
  // 返回适合当前操作系统的默认环境。成熟的用户可能希望提供自己的Env实现，而不是依赖于此默认环境。
  // Default()的结果属于leveldb，永远不能删除。
  static Env* Default();

  // Create an object that sequentially reads the file with the specified name.
  // On success, stores a pointer to the new file in *result and returns OK.
  // On failure stores nullptr in *result and returns non-OK.  If the file does
  // not exist, returns a non-OK status.  Implementations should return a
  // NotFound status when the file does not exist.
  //
  // The returned file will only be accessed by one thread at a time.
  //
  // 创建一个对象，该对象按顺序读取具有指定名称的文件。
  // 成功后，将指向新文件的指针存储在*result中，并返回OK。
  // 失败时，将nullptr存储在*result中，并返回非OK状态。
  // 如果文件不存在，则返回非OK状态。
  // 实现应该返回一个文件不存在时的NotFound状态。
  //
  // 返回的文件一次只能由一个线程访问。
  virtual Status NewSequentialFile(const std::string& fname,
                                   SequentialFile** result) = 0;

  // Create an object supporting random-access reads from the file with the
  // specified name.  On success, stores a pointer to the new file in
  // *result and returns OK.  On failure stores nullptr in *result and
  // returns non-OK.  If the file does not exist, returns a non-OK
  // status.  Implementations should return a NotFound status when the file does
  // not exist.
  //
  // The returned file may be concurrently accessed by multiple threads.
  //
  // 创建一个对象支持随机读取给定名字的文件
  // 成功后，将指向新文件的指针存储在*result中，并返回OK。
  // 失败时，将nullptr存储在*result中，并返回非OK状态。
  // 如果文件不存在，则返回非OK状态。
  // 实现应该返回一个文件不存在时的NotFound状态。
  //
  // 多个线程可以同时访问返回的文件。
  virtual Status NewRandomAccessFile(const std::string& fname,
                                     RandomAccessFile** result) = 0;

  // Create an object that writes to a new file with the specified
  // name.  Deletes any existing file with the same name and creates a
  // new file.  On success, stores a pointer to the new file in
  // *result and returns OK.  On failure stores nullptr in *result and
  // returns non-OK.
  //
  // The returned file will only be accessed by one thread at a time.
  //
  // 创建一个对象可以写入到指定名字的新文件
  // 如果文件存在，则删除原文件，并创建一个新的
  // 成功后，将指向新文件的指针存储在*result中，并返回OK。
  // 失败时，将nullptr存储在*result中，并返回非OK状态。
  //
  // 返回的文件一次只能由一个线程访问。
  virtual Status NewWritableFile(const std::string& fname,
                                 WritableFile** result) = 0;

  // Create an object that either appends to an existing file, or
  // writes to a new file (if the file does not exist to begin with).
  // On success, stores a pointer to the new file in *result and
  // returns OK.  On failure stores nullptr in *result and returns
  // non-OK.
  //
  // The returned file will only be accessed by one thread at a time.
  //
  // May return an IsNotSupportedError error if this Env does
  // not allow appending to an existing file.  Users of Env (including
  // the leveldb implementation) must be prepared to deal with
  // an Env that does not support appending.
  //
  // 创建一个对象，该对象要么附加到现有文件，要么写入新文件（如果该文件不存在）。
  // 成功后，将指向新文件的指针存储在*result中，并返回OK。
  // 失败时，将nullptr存储在*result中，并返回非OK状态。
  //
  // 返回的文件一次只能由一个线程访问。
  // 如果此Env不允许附加到现有文件，则可能会返回IsNotSupportedError错误。
  // Env（包括leveldb实现）的用户必须准备好处理不支持附加的Env。
  virtual Status NewAppendableFile(const std::string& fname,
                                   WritableFile** result);

  // Returns true iff the named file exists.
  // 如果文件存在，返回true
  virtual bool FileExists(const std::string& fname) = 0;

  // Store in *result the names of the children of the specified directory.
  // The names are relative to "dir".
  // Original contents of *results are dropped.
  // 将指定目录的子目录的名称存储在*result中。
  // 这些名称是相对于“dir”的。
  // *result的原始内容将被删除。
  virtual Status GetChildren(const std::string& dir,
                             std::vector<std::string>* result) = 0;
  // Delete the named file.
  //
  // The default implementation calls DeleteFile, to support legacy Env
  // implementations. Updated Env implementations must override RemoveFile and
  // ignore the existence of DeleteFile. Updated code calling into the Env API
  // must call RemoveFile instead of DeleteFile.
  //
  // A future release will remove DeleteDir and the default implementation of
  // RemoveDir.
  // 删除命名文件。
  //
  // 默认实现调用DeleteFile，以支持旧版Env实现。
  // 更新的Env实现必须覆盖RemoveFile并忽略DeleteFile的存在。
  // 调用Env API的更新代码必须调用RemoveFile，而不是DeleteFile。
  //
  // 未来的版本将删除DeleteDir和RemoveDir的默认实现。
  virtual Status RemoveFile(const std::string& fname);

  // DEPRECATED: Modern Env implementations should override RemoveFile instead.
  //
  // The default implementation calls RemoveFile, to support legacy Env user
  // code that calls this method on modern Env implementations. Modern Env user
  // code should call RemoveFile.
  //
  // A future release will remove this method.
  // 不推荐使用：现代Env实现应该覆盖RemoveFile。
  //
  // 默认实现调用RemoveFile，以支持在现代Env实现中调用此方法的旧版Env用户代码。
  // 现代Env用户代码应该调用RemoveFile。
  //
  // 未来的版本将删除此方法。
  virtual Status DeleteFile(const std::string& fname);

  // Create the specified directory.
  // 创建一个指定的目录
  virtual Status CreateDir(const std::string& dirname) = 0;

  // Delete the specified directory.
  //
  // The default implementation calls DeleteDir, to support legacy Env
  // implementations. Updated Env implementations must override RemoveDir and
  // ignore the existence of DeleteDir. Modern code calling into the Env API
  // must call RemoveDir instead of DeleteDir.
  //
  // A future release will remove DeleteDir and the default implementation of
  // RemoveDir.
  // 删除指定的目录。
  //
  // 默认实现调用DeleteDir，以支持遗留环境
  // 实现。更新的Env实现必须覆盖RemoveDir和
  // 忽略DeleteDir的存在。调用Env API的现代代码
  // 必须调用RemoveDir而不是DeleteDir。
  //
  // 未来的版本将删除DeleteDir和的默认实现RemoveDir。
  virtual Status RemoveDir(const std::string& dirname);

  // DEPRECATED: Modern Env implementations should override RemoveDir instead.
  //
  // The default implementation calls RemoveDir, to support legacy Env user
  // code that calls this method on modern Env implementations. Modern Env user
  // code should call RemoveDir.
  //
  // A future release will remove this method.
  // 不推荐使用：现代Env实现应该覆盖RemoveDir。
  //
  // 默认实现调用RemoveDir，以支持在现代Env实现中调用此方法的遗留Env用户代码。现代Env用户代码应该调用RemoveDir。
  //
  // 未来的版本将删除此方法。
  virtual Status DeleteDir(const std::string& dirname);

  // Store the size of fname in *file_size.
  // 存储文件fname的大小到*file_size
  virtual Status GetFileSize(const std::string& fname, uint64_t* file_size) = 0;

  // Rename file src to target.
  // 重命名文件
  virtual Status RenameFile(const std::string& src,
                            const std::string& target) = 0;

  // Lock the specified file.  Used to prevent concurrent access to
  // the same db by multiple processes.  On failure, stores nullptr in
  // *lock and returns non-OK.
  //
  // On success, stores a pointer to the object that represents the
  // acquired lock in *lock and returns OK.  The caller should call
  // UnlockFile(*lock) to release the lock.  If the process exits,
  // the lock will be automatically released.
  //
  // If somebody else already holds the lock, finishes immediately
  // with a failure.  I.e., this call does not wait for existing locks
  // to go away.
  //
  // May create the named file if it does not already exist.
  // 锁定指定的文件。用于防止多个进程同时访问同一数据库。
  // 失败时，将nullptr存储在*lock中，并返回非OK。
  //
  // 成功后，在*lock中存储一个指向表示获取的锁的对象的指针，并返回OK。
  // 调用者应该调用UnlockFile（*lock）来释放锁。
  // 如果进程退出，锁将自动释放。
  // 如果其他人已经持有锁，则会立即以失败告终。
  // 也就是说，此调用不会等待现有锁释放。
  //
  // 如果命名文件不存在，则可以创建该文件。
  virtual Status LockFile(const std::string& fname, FileLock** lock) = 0;

  // Release the lock acquired by a previous successful call to LockFile.
  // REQUIRES: lock was returned by a successful LockFile() call
  // REQUIRES: lock has not already been unlocked.
  // 释放上次成功调用LockFile获得的锁。
  // 要求：锁是成功调用LockFile()返回的
  // 要求：锁尚未解锁。
  virtual Status UnlockFile(FileLock* lock) = 0;

  // Arrange to run "(*function)(arg)" once in a background thread.
  //
  // "function" may run in an unspecified thread.  Multiple functions
  // added to the same Env may run concurrently in different threads.
  // I.e., the caller may not assume that background work items are
  // serialized.
  // 安排在后台线程中运行一次“(*function)(arg)”。
  //
  // “function”可能在未指定的线程中运行。添加到同一个Env的多个函数可以在不同的线程中并发运行。
  // 也就是说，调用方可能不会假定后台工作项是序列化的。
  virtual void Schedule(void (*function)(void* arg), void* arg) = 0;

  // Start a new thread, invoking "function(arg)" within the new thread.
  // When "function(arg)" returns, the thread will be destroyed.
  // 启动一个新线程，在新线程中调用“函数（arg）”。当“函数（arg）”返回时，线程将被销毁。
  virtual void StartThread(void (*function)(void* arg), void* arg) = 0;

  // *path is set to a temporary directory that can be used for testing. It may
  // or may not have just been created. The directory may or may not differ
  // between runs of the same process, but subsequent calls will return the
  // same directory.
  // *path设置为可用于测试的临时目录。
  // 它可能是也可能不是刚刚创建的。
  // 同一进程的不同运行的目录可能不同，也可能相同，但后续调用将返回相同的目录。
  virtual Status GetTestDirectory(std::string* path) = 0;

  // Create and return a log file for storing informational messages.
  // 创建并返回用于存储信息性消息的日志文件。
  virtual Status NewLogger(const std::string& fname, Logger** result) = 0;

  // Returns the number of micro-seconds since some fixed point in time. Only
  // useful for computing deltas of time.
  // 返回自某个固定时间点以来的微秒数。仅适用于计算时间增量。
  virtual uint64_t NowMicros() = 0;

  // Sleep/delay the thread for the prescribed number of micro-seconds.
  // 将线程休眠/延迟指定的微秒数。
  virtual void SleepForMicroseconds(int micros) = 0;
};

// A file abstraction for reading sequentially through a file
// 用于顺序读取文件的文件抽象
class LEVELDB_EXPORT SequentialFile {
 public:
  SequentialFile() = default;

  SequentialFile(const SequentialFile&) = delete;
  SequentialFile& operator=(const SequentialFile&) = delete;

  virtual ~SequentialFile();

  // Read up to "n" bytes from the file.  "scratch[0..n-1]" may be
  // written by this routine.  Sets "*result" to the data that was
  // read (including if fewer than "n" bytes were successfully read).
  // May set "*result" to point at data in "scratch[0..n-1]", so
  // "scratch[0..n-1]" must be live when "*result" is used.
  // If an error was encountered, returns a non-OK status.
  //
  // REQUIRES: External synchronization
  // 从文件中最多读取“n”个字节。“scratch[0..n-1]”可以由该例程编写。将“*result”设置为已读取的数据（包括成功读取的字节数是否少于“n”）。
  // 可以将“*result”设置为指向“scratch[0..n-1]”中的数据，因此使用“*result”时，“scratch[0..n-1]”必须处于活动状态。
  // 如果遇到错误，则返回非OK状态。
  //
  // 要求：外部同步
  virtual Status Read(size_t n, Slice* result, char* scratch) = 0;

  // Skip "n" bytes from the file. This is guaranteed to be no
  // slower that reading the same data, but may be faster.
  //
  // If end of file is reached, skipping will stop at the end of the
  // file, and Skip will return OK.
  //
  // REQUIRES: External synchronization
  // 跳过文件中的“n”字节。这保证不会比读取相同数据慢，但可能会更快。
  //
  // 如果到达文件末尾，Skip将在文件末尾停止，Skip将返回OK。
  //
  // 要求：外部同步
  virtual Status Skip(uint64_t n) = 0;
};

// A file abstraction for randomly reading the contents of a file.
// 随机读取文件内容的文件抽象。
class LEVELDB_EXPORT RandomAccessFile {
 public:
  RandomAccessFile() = default;

  RandomAccessFile(const RandomAccessFile&) = delete;
  RandomAccessFile& operator=(const RandomAccessFile&) = delete;

  virtual ~RandomAccessFile();

  // Read up to "n" bytes from the file starting at "offset".
  // "scratch[0..n-1]" may be written by this routine.  Sets "*result"
  // to the data that was read (including if fewer than "n" bytes were
  // successfully read).  May set "*result" to point at data in
  // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
  // "*result" is used.  If an error was encountered, returns a non-OK
  // status.
  //
  // Safe for concurrent use by multiple threads.
  // 从“偏移量”开始从文件中读取最多“n”个字节。
  // “scratch[0..n-1]”可以由该例程编写。
  // 将“*result”设置为已读取的数据（包括成功读取的字节数是否少于“n”）。
  // 可以将“*result”设置为指向“scratch[0..n-1]”中的数据，因此使用“*result”时，“scratch[0..n-1]”必须处于活动状态。
  // 如果遇到错误，则返回非OK状态。
  //
  // 多线程并发使用安全。
  virtual Status Read(uint64_t offset, size_t n, Slice* result,
                      char* scratch) const = 0;
};

// A file abstraction for sequential writing.  The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
// 用于顺序写入的文件抽象。实现必须提供缓冲，因为调用者可能会一次向文件中附加小片段。
class LEVELDB_EXPORT WritableFile {
 public:
  WritableFile() = default;

  WritableFile(const WritableFile&) = delete;
  WritableFile& operator=(const WritableFile&) = delete;

  virtual ~WritableFile();

  virtual Status Append(const Slice& data) = 0;
  virtual Status Close() = 0;
  virtual Status Flush() = 0;
  virtual Status Sync() = 0;
};

// An interface for writing log messages.
// 用于写入日志消息的接口。
class LEVELDB_EXPORT Logger {
 public:
  Logger() = default;

  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;

  virtual ~Logger();

  // Write an entry to the log file with the specified format.
  // 以指定的格式将条目写入日志文件。
  virtual void Logv(const char* format, std::va_list ap) = 0;
};

// Identifies a locked file.
// 标识锁定的文件。
class LEVELDB_EXPORT FileLock {
 public:
  FileLock() = default;

  FileLock(const FileLock&) = delete;
  FileLock& operator=(const FileLock&) = delete;

  virtual ~FileLock();
};

// Log the specified data to *info_log if info_log is non-null.
// 如果*info_Log为非空，则将指定的数据记录到*info_Log。
void Log(Logger* info_log, const char* format, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((__format__(__printf__, 2, 3)))
#endif
    ;

// A utility routine: write "data" to the named file.
// 实用程序例程：将“data”写入指定文件。
LEVELDB_EXPORT Status WriteStringToFile(Env* env, const Slice& data,
                                        const std::string& fname);

// A utility routine: read contents of named file into *data
// 实用程序例程：将命名文件的内容读入*data
LEVELDB_EXPORT Status ReadFileToString(Env* env, const std::string& fname,
                                       std::string* data);

// An implementation of Env that forwards all calls to another Env.
// May be useful to clients who wish to override just part of the
// functionality of another Env.
// Env的一种实现，它将所有调用转发给另一个Env。
// 对于希望只覆盖另一个Env的部分功能的客户来说可能很有用。
class LEVELDB_EXPORT EnvWrapper : public Env {
 public:
  // Initialize an EnvWrapper that delegates all calls to *t.
  // 初始化EnvWrapper，并将所有调用委托给*t的。
  explicit EnvWrapper(Env* t) : target_(t) {}
  virtual ~EnvWrapper();

  // Return the target to which this Env forwards all calls.
  // 返回目标env，这个env会接收转发过来的所有调用
  Env* target() const { return target_; }

  // The following text is boilerplate that forwards all methods to target().
  // 以下文本是将所有方法转发到target()的样板文件。
  Status NewSequentialFile(const std::string& f, SequentialFile** r) override {
    return target_->NewSequentialFile(f, r);
  }
  Status NewRandomAccessFile(const std::string& f,
                             RandomAccessFile** r) override {
    return target_->NewRandomAccessFile(f, r);
  }
  Status NewWritableFile(const std::string& f, WritableFile** r) override {
    return target_->NewWritableFile(f, r);
  }
  Status NewAppendableFile(const std::string& f, WritableFile** r) override {
    return target_->NewAppendableFile(f, r);
  }
  bool FileExists(const std::string& f) override {
    return target_->FileExists(f);
  }
  Status GetChildren(const std::string& dir,
                     std::vector<std::string>* r) override {
    return target_->GetChildren(dir, r);
  }
  Status RemoveFile(const std::string& f) override {
    return target_->RemoveFile(f);
  }
  Status CreateDir(const std::string& d) override {
    return target_->CreateDir(d);
  }
  Status RemoveDir(const std::string& d) override {
    return target_->RemoveDir(d);
  }
  Status GetFileSize(const std::string& f, uint64_t* s) override {
    return target_->GetFileSize(f, s);
  }
  Status RenameFile(const std::string& s, const std::string& t) override {
    return target_->RenameFile(s, t);
  }
  Status LockFile(const std::string& f, FileLock** l) override {
    return target_->LockFile(f, l);
  }
  Status UnlockFile(FileLock* l) override { return target_->UnlockFile(l); }
  void Schedule(void (*f)(void*), void* a) override {
    return target_->Schedule(f, a);
  }
  void StartThread(void (*f)(void*), void* a) override {
    return target_->StartThread(f, a);
  }
  Status GetTestDirectory(std::string* path) override {
    return target_->GetTestDirectory(path);
  }
  Status NewLogger(const std::string& fname, Logger** result) override {
    return target_->NewLogger(fname, result);
  }
  uint64_t NowMicros() override { return target_->NowMicros(); }
  void SleepForMicroseconds(int micros) override {
    target_->SleepForMicroseconds(micros);
  }

 private:
  Env* target_;
};

}  // namespace leveldb

// This workaround can be removed when leveldb::Env::DeleteFile is removed.
// Redefine DeleteFile if it was undefined earlier.
// 删除leveldb::Env::DeleteFile时，可以删除此变通方法。
// 如果先前未定义DeleteFile，重新定义它。
#if defined(_WIN32) && defined(LEVELDB_DELETEFILE_UNDEFINED)
#if defined(UNICODE)
#define DeleteFile DeleteFileW
#else
#define DeleteFile DeleteFileA
#endif  // defined(UNICODE)
#endif  // defined(_WIN32) && defined(LEVELDB_DELETEFILE_UNDEFINED)

#endif  // STORAGE_LEVELDB_INCLUDE_ENV_H_
