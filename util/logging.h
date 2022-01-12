// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// Must not be included from any .h files to avoid polluting the namespace
// with macros.
// 不得包含在任何.h文件，以避免宏污染命名空间。

#ifndef STORAGE_LEVELDB_UTIL_LOGGING_H_
#define STORAGE_LEVELDB_UTIL_LOGGING_H_

#include <cstdint>
#include <cstdio>
#include <string>

#include "port/port.h"

namespace leveldb {

class Slice;
class WritableFile;

// Append a human-readable printout of "num" to *str
// 将“num”的可读打印输出附加到*str
void AppendNumberTo(std::string* str, uint64_t num);

// Append a human-readable printout of "value" to *str.
// Escapes any non-printable characters found in "value".
// 将“value”的可读打印输出附加到*str。
// 转义在“value”中找到的任何不可打印字符。
void AppendEscapedStringTo(std::string* str, const Slice& value);

// Return a human-readable printout of "num"
// 返回“num”的可读打印输出
std::string NumberToString(uint64_t num);

// Return a human-readable version of "value".
// Escapes any non-printable characters found in "value".
// 返回“value”的可读版本。
//转义在“value”中找到的任何不可打印字符。
std::string EscapeString(const Slice& value);

// Parse a human-readable number from "*in" into *val.  On success,
// advances "*in" past the consumed number and sets "*val" to the
// numeric value.  Otherwise, returns false and leaves *in in an
// unspecified state.
// 将人类可读的数字从“*in”解析为*val.
// 成功时，在*in中跳过已经消耗的数字，并设置*val为一个数字值
// 否则，返回false并使*in处于未指定状态。
bool ConsumeDecimalNumber(Slice* in, uint64_t* val);

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_LOGGING_H_