// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/logging.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "leveldb/env.h"
#include "leveldb/slice.h"

namespace leveldb {

// 添加num到str
void AppendNumberTo(std::string* str, uint64_t num) {
  char buf[30];
  std::snprintf(buf, sizeof(buf), "%llu", static_cast<unsigned long long>(num));
  str->append(buf);
}

// 添加转义字符串
void AppendEscapedStringTo(std::string* str, const Slice& value) {
  for (size_t i = 0; i < value.size(); i++) {
    char c = value[i];
    if (c >= ' ' && c <= '~') {
      str->push_back(c);
    } else {
      char buf[10];
      std::snprintf(buf, sizeof(buf), "\\x%02x",
                    static_cast<unsigned int>(c) & 0xff);
      str->append(buf);
    }
  }
}

// 数字转字符串
std::string NumberToString(uint64_t num) {
  std::string r;
  AppendNumberTo(&r, num);
  return r;
}

// 返回转义字符串
std::string EscapeString(const Slice& value) {
  std::string r;
  AppendEscapedStringTo(&r, value);
  return r;
}

// 从in转换成数字放到val
bool ConsumeDecimalNumber(Slice* in, uint64_t* val) {
  // Constants that will be optimized away.
  // 将被优化掉的常数
  constexpr const uint64_t kMaxUint64 = std::numeric_limits<uint64_t>::max();
  constexpr const char kLastDigitOfMaxUint64 =
      '0' + static_cast<char>(kMaxUint64 % 10);

  uint64_t value = 0;

  // reinterpret_cast-ing from char* to uint8_t* to avoid signedness.
  // 从char*转换到uint8_t* 避免签名？

  const uint8_t* start = reinterpret_cast<const uint8_t*>(in->data());

  const uint8_t* end = start + in->size();
  const uint8_t* current = start;
  for (; current != end; ++current) {
    const uint8_t ch = *current;
    if (ch < '0' || ch > '9') break;

    // Overflow check.
    // kMaxUint64 / 10 is also constant and will be optimized away.
    // 溢出检查
    // kMaxUint64 / 10 也是个常量，要优化掉
    if (value > kMaxUint64 / 10 ||
        (value == kMaxUint64 / 10 && ch > kLastDigitOfMaxUint64)) {
      return false;
    }

    value = (value * 10) + (ch - '0');
  }

  *val = value;
  const size_t digits_consumed = current - start;
  in->remove_prefix(digits_consumed);
  return digits_consumed != 0;
}

}  // namespace leveldb