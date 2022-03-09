// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// File names used by DB code
// DB代码使用的文件名

#ifndef STORAGE_LEVELDB_DB_FILENAME_H_
#define STORAGE_LEVELDB_DB_FILENAME_H_

#include <cstdint>
#include <string>

#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "port/port.h"

namespace leveldb {

class Env;

// 文件类型
enum FileType {
  kLogFile,
  kDBLockFile,
  kTableFile,
  kDescriptorFile,
  kCurrentFile,
  kTempFile,
  kInfoLogFile  // Either the current one, or an old one 要么是当前的，要么是旧的
};

// Return the name of the log file with the specified number
// in the db named by "dbname".  The result will be prefixed with
// "dbname".
// 返回日志文件的名称，该文件在数据库中以“dbname”命名，并带有指定的编号。结果将以“dbname”作为前缀。
std::string LogFileName(const std::string& dbname, uint64_t number);

// Return the name of the sstable with the specified number
// in the db named by "dbname".  The result will be prefixed with
// "dbname".
// 返回sstable的名称，该名称在数据库中以“dbname”命名，并带有指定的数字。结果将以“dbname”作为前缀。
std::string TableFileName(const std::string& dbname, uint64_t number);

// Return the legacy file name for an sstable with the specified number
// in the db named by "dbname". The result will be prefixed with
// "dbname".
// 返回sstable的旧文件名，该名称在数据库中以“dbname”命名，并带有指定的数字。结果将以“dbname”作为前缀。
std::string SSTTableFileName(const std::string& dbname, uint64_t number);

// Return the name of the descriptor file for the db named by
// "dbname" and the specified incarnation number.  The result will be
// prefixed with "dbname".
// 返回描述符文件名，该名称在数据库中以“dbname”命名，并带有指定的数字。结果将以“dbname”作为前缀。
std::string DescriptorFileName(const std::string& dbname, uint64_t number);

// Return the name of the current file.  This file contains the name
// of the current manifest file.  The result will be prefixed with
// "dbname".
// 返回一个CURRENT文件的文件名，这个文件包含了当前清单文件的名字
// 结果将以“dbname”作为前缀。
std::string CurrentFileName(const std::string& dbname);

// Return the name of the lock file for the db named by
// "dbname".  The result will be prefixed with "dbname".
// 返回锁文件的文件名。该名称在数据库中以“dbname”命名，结果将以“dbname”作为前缀。
std::string LockFileName(const std::string& dbname);

// Return the name of a temporary file owned by the db named "dbname".
// The result will be prefixed with "dbname".
// 返回临时文件的文件名，由以“dbname”命名的数据块所有。结果将以“dbname”作为前缀。
std::string TempFileName(const std::string& dbname, uint64_t number);

// Return the name of the info log file for "dbname".
// 返回数据库"dbname"的信息日志文件的文件名
std::string InfoLogFileName(const std::string& dbname);

// Return the name of the old info log file for "dbname".
// 返回数据库"dbname"的旧信息日志文件的文件名
std::string OldInfoLogFileName(const std::string& dbname);

// If filename is a leveldb file, store the type of the file in *type.
// The number encoded in the filename is stored in *number.  If the
// filename was successfully parsed, returns true.  Else return false.
// 如果文件名是leveldb文件，将文件类型存储在*type中。
// 文件名中编码的数字存储在*number中。
// 如果文件名已成功解析，则返回true。否则返回false。
bool ParseFileName(const std::string& filename, uint64_t* number,
                   FileType* type);

// Make the CURRENT file point to the descriptor file with the
// specified number.
// 使CURRENT文件指向具有指定编号的描述符文件。
Status SetCurrentFile(Env* env, const std::string& dbname,
                      uint64_t descriptor_number);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_DB_FILENAME_H_
