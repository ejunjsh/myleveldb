# myleveldb

看leveldb，加中文注释，顺便学c++

## 看代码路径

-> include/leveldb/status -> util/status -> util/arena -> include/leveldb/slice -> include/leveldb/filter_policy

-> util/hash -> util/filter_policy -> util/bloom -> util/logging -> util/mutexlock -> include/leveldb/cache

-> util/cache -> util/coding -> util/no_destructor -> include/leveldb/comparator/

-> util/comparator -> util/crc32c -> util/histogram -> include/leveldb/options -> util/options -> util/posix_logger

-> util/random -> include/leveldb/env -> util/env_posix -> table/format -> table/block_builder 

-> table/block -> table/filter_block -> include/leveldb/iterator -> table/iterator -> table/iterator_wrapper

-> table/merger -> table/two_level_iterator -> include/leveldb/table -> table/table

-> include/leveldb/table_builder -> table/table_builder

## 参考

[leveldb源码](https://github.com/google/leveldb)

[sstable 格式](https://www.cnblogs.com/cobbliu/p/6194072.html)
