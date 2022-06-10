leveldb
=======

_Jeff Dean, Sanjay Ghemawat_

The leveldb library provides a persistent key value store. Keys and values are
arbitrary byte arrays.  The keys are ordered within the key value store
according to a user-specified comparator function.

leveldb库提供了一个持久的键值存储。键和值是任意字节数组。根据用户指定的比较器功能，在键值存储中对键进行排序。

## Opening A Database

A leveldb database has a name which corresponds to a file system directory. All
of the contents of database are stored in this directory. The following example
shows how to open a database, creating it if necessary:

leveldb数据库的名称与文件系统目录相对应。数据库的所有内容都存储在此目录中。以下示例显示了如何打开数据库，并在必要时创建数据库：

```c++
#include <cassert>
#include "leveldb/db.h"

leveldb::DB* db;
leveldb::Options options;
options.create_if_missing = true;
leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
assert(status.ok());
...
```

If you want to raise an error if the database already exists, add the following
line before the `leveldb::DB::Open` call:

如果要在数据库已经存在的情况下引发错误，请在`leveldb::DB::Open`调用之前添加以下行：

```c++
options.error_if_exists = true;
```

## Status

You may have noticed the `leveldb::Status` type above. Values of this type are
returned by most functions in leveldb that may encounter an error. You can check
if such a result is ok, and also print an associated error message:

您可能已经注意到上面的`leveldb::Status`类型。leveldb中的大多数函数都会返回这种类型的值，
这些函数可能会遇到错误。您可以检查这样的结果是否正常，还可以打印相关的错误消息：

```c++
leveldb::Status s = ...;
if (!s.ok()) cerr << s.ToString() << endl;
```

## Closing A Database

When you are done with a database, just delete the database object. Example:

处理完数据库后，只需删除数据库对象。例如：

```c++
... open the db as described above ...
... do something with db ...
delete db;
```

## Reads And Writes

The database provides Put, Delete, and Get methods to modify/query the database.
For example, the following code moves the value stored under key1 to key2.

数据库提供Put、Delete和Get方法来修改/查询数据库。
例如，以下代码将键1下存储的值移动到键2。

```c++
std::string value;
leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
if (s.ok()) s = db->Put(leveldb::WriteOptions(), key2, value);
if (s.ok()) s = db->Delete(leveldb::WriteOptions(), key1);
```

## Atomic Updates

Note that if the process dies after the Put of key2 but before the delete of
key1, the same value may be left stored under multiple keys. Such problems can
be avoided by using the `WriteBatch` class to atomically apply a set of updates:

请注意，如果进程在输入键2之后但在删除键1之前终止，则相同的值可能会保留在多个键下。
通过使用“WriteBatch”类以原子方式应用一组更新，可以避免此类问题：

```c++
#include "leveldb/write_batch.h"
...
std::string value;
leveldb::Status s = db->Get(leveldb::ReadOptions(), key1, &value);
if (s.ok()) {
  leveldb::WriteBatch batch;
  batch.Delete(key1);
  batch.Put(key2, value);
  s = db->Write(leveldb::WriteOptions(), &batch);
}
```

The `WriteBatch` holds a sequence of edits to be made to the database, and these
edits within the batch are applied in order. Note that we called Delete before
Put so that if key1 is identical to key2, we do not end up erroneously dropping
the value entirely.

`WriteBatch`保存了要对数据库进行的一系列编辑，批中的这些编辑按顺序应用。
请注意，我们在Put之前调用了Delete，这样，如果key1与key2相同，我们就不会错误地完全删除该值。

Apart from its atomicity benefits, `WriteBatch` may also be used to speed up
bulk updates by placing lots of individual mutations into the same batch.

除了原子性的好处之外，“WriteBatch”还可以通过将大量的单个操作放入同一批中来加速批量更新。

## Synchronous Writes

By default, each write to leveldb is asynchronous: it returns after pushing the
write from the process into the operating system. The transfer from operating
system memory to the underlying persistent storage happens asynchronously. The
sync flag can be turned on for a particular write to make the write operation
not return until the data being written has been pushed all the way to
persistent storage. (On Posix systems, this is implemented by calling either
`fsync(...)` or `fdatasync(...)` or `msync(..., MS_SYNC)` before the write
operation returns.)

默认情况下，对leveldb的每次写入都是异步的：它在将写入从进程推送到操作系统后返回。
从操作系统内存到底层持久存储的传输是异步进行的。可以为特定写入打开同步标志，以使写入操作在被写入的数据被完全推送到持久存储之前不会返回。
（在Posix系统上，这是通过在写入操作返回之前调用`fsync(...)` ， `fdatasync(...)` 或者 `msync(..., MS_SYNC)`。）

```c++
leveldb::WriteOptions write_options;
write_options.sync = true;
db->Put(write_options, ...);
```

Asynchronous writes are often more than a thousand times as fast as synchronous
writes. The downside of asynchronous writes is that a crash of the machine may
cause the last few updates to be lost. Note that a crash of just the writing
process (i.e., not a reboot) will not cause any loss since even when sync is
false, an update is pushed from the process memory into the operating system
before it is considered done.

异步写入的速度通常是同步写入速度的1000倍以上。异步写入的缺点是，机器崩溃可能会导致最后几次更新丢失。
请注意，仅写入进程崩溃（不是操作系统重启）不会导致任何损失，因为即使同步为false，更新也会在认为完成之前从进程内存推送到操作系统中。

Asynchronous writes can often be used safely. For example, when loading a large
amount of data into the database you can handle lost updates by restarting the
bulk load after a crash. A hybrid scheme is also possible where every Nth write
is synchronous, and in the event of a crash, the bulk load is restarted just
after the last synchronous write finished by the previous run. (The synchronous
write can update a marker that describes where to restart on a crash.)

异步写入通常可以安全使用。例如，在将大量数据加载到数据库中时，可以通过在崩溃后重新启动批量加载来处理丢失的更新。
一个混合方案就是每个第n次写入都是同步的，并且在崩溃的情况下，大容量加载将在上一次运行完成最后一次同步写入后重新启动。（同步写入可以更新描述崩溃时在何处重新启动的标记。）

`WriteBatch` provides an alternative to asynchronous writes. Multiple updates
may be placed in the same WriteBatch and applied together using a synchronous
write (i.e., `write_options.sync` is set to true). The extra cost of the
synchronous write will be amortized across all of the writes in the batch.

`WriteBatch`提供异步写入的替代方法。可以将多个更新放在同一个WriteBatch中，
并使用同步写入一起应用（即，`write_options.sync`设置为true）。
同步写入的额外成本将在批处理中的所有写入中摊销。

## Concurrency

A database may only be opened by one process at a time. The leveldb
implementation acquires a lock from the operating system to prevent misuse.
Within a single process, the same `leveldb::DB` object may be safely shared by
multiple concurrent threads. I.e., different threads may write into or fetch
iterators or call Get on the same database without any external synchronization
(the leveldb implementation will automatically do the required synchronization).
However other objects (like Iterator and `WriteBatch`) may require external
synchronization. If two threads share such an object, they must protect access
to it using their own locking protocol. More details are available in the public
header files.

一次只能由一个进程打开数据库。leveldb实现从操作系统获取锁以防止误用。在单个进程中，同一个`leveldb::DB`对象可以由多个并发线程安全共享。
例如，不同的线程可以写入或获取迭代器，或者在同一数据库上调用Get，而无需任何外部同步（leveldb实现将自动执行所需的同步）。
但是，其他对象（如迭代器和`WriteBatch`）可能需要外部同步。如果两个线程共享这样一个对象，那么它们必须使用自己的锁定协议来保护对该对象的访问。公共头文件中提供了更多详细信息。

## Iteration

The following example demonstrates how to print all key,value pairs in a
database.

下面的示例演示如何打印数据库中的所有键、值对。

```c++
leveldb::Iterator* it = db->NewIterator(leveldb::ReadOptions());
for (it->SeekToFirst(); it->Valid(); it->Next()) {
  cout << it->key().ToString() << ": "  << it->value().ToString() << endl;
}
assert(it->status().ok());  // Check for any errors found during the scan
delete it;
```

The following variation shows how to process just the keys in the range
[start,limit):

```c++
for (it->Seek(start);
   it->Valid() && it->key().ToString() < limit;
   it->Next()) {
  ...
}
```

You can also process entries in reverse order. (Caveat: reverse iteration may be
somewhat slower than forward iteration.)

您还可以按相反顺序处理条目。（注意：反向迭代可能比正向迭代慢一些。）

```c++
for (it->SeekToLast(); it->Valid(); it->Prev()) {
  ...
}
```

## Snapshots

Snapshots provide consistent read-only views over the entire state of the
key-value store.  `ReadOptions::snapshot` may be non-NULL to indicate that a
read should operate on a particular version of the DB state. If
`ReadOptions::snapshot` is NULL, the read will operate on an implicit snapshot
of the current state.

快照为键值存储的整个状态提供一致的只读视图 `ReadOptions::snapshot`可以为非NULL，以指示读取操作应在DB状态的特定版本上进行。
如果`ReadOptions::snapshot`为空，则读取操作将在当前状态的隐式快照上进行。

Snapshots are created by the `DB::GetSnapshot()` method:

快照由`DB::GetSnapshot()` 方法创建：

```c++
leveldb::ReadOptions options;
options.snapshot = db->GetSnapshot();
... apply some updates to db ...
leveldb::Iterator* iter = db->NewIterator(options);
... read using iter to view the state when the snapshot was created ...
delete iter;
db->ReleaseSnapshot(options.snapshot);
```

Note that when a snapshot is no longer needed, it should be released using the
`DB::ReleaseSnapshot` interface. This allows the implementation to get rid of
state that was being maintained just to support reading as of that snapshot.

请注意，当不再需要快照时，应使用`DB::ReleaseSnapshot`接口释放快照。
这允许数据库的实现丢弃仅为支持从该快照开始读取而维护的状态。

## Slice

The return value of the `it->key()` and `it->value()` calls above are instances
of the `leveldb::Slice` type. Slice is a simple structure that contains a length
and a pointer to an external byte array. Returning a Slice is a cheaper
alternative to returning a `std::string` since we do not need to copy
potentially large keys and values. In addition, leveldb methods do not return
null-terminated C-style strings since leveldb keys and values are allowed to
contain `'\0'` bytes.

上面的`it->key()`和`it->value()`用的返回值是`leveldb::Slice`类型的实例。
切片是一个简单的结构，它包含一个长度和一个指向外部字节数组的指针。
返回切片比返回`std::string`更便宜，因为我们不需要复制可能较大的键和值。
此外，leveldb方法不返回以null结尾的C样式字符串，因为leveldb键和值允许包含`'\0'`字节。

C++ strings and null-terminated C-style strings can be easily converted to a
Slice:

C++字符串和以null结尾的C样式字符串可以轻松转换为切片：

```c++
leveldb::Slice s1 = "hello";

std::string str("world");
leveldb::Slice s2 = str;
```

A Slice can be easily converted back to a C++ string:

切片可以轻松转换回C++字符串：

```c++
std::string str = s1.ToString();
assert(str == std::string("hello"));
```

Be careful when using Slices since it is up to the caller to ensure that the
external byte array into which the Slice points remains live while the Slice is
in use. For example, the following is buggy:

使用切片时要小心，因为要确保切片所在的外部字节数组在使用切片时保持活动状态，这取决于调用方。例如，以下是错误的：

```c++
leveldb::Slice slice;
if (...) {
  std::string str = ...;
  slice = str;
}
Use(slice);
```

When the if statement goes out of scope, str will be destroyed and the backing
storage for slice will disappear.

当if语句超出范围时，str将被销毁，slice的后台存储将消失。

## Comparators

The preceding examples used the default ordering function for key, which orders
bytes lexicographically. You can however supply a custom comparator when opening
a database.  For example, suppose each database key consists of two numbers and
we should sort by the first number, breaking ties by the second number. First,
define a proper subclass of `leveldb::Comparator` that expresses these rules:

前面的示例使用了键的默认排序函数，该函数按字典顺序对字节排序。
但是，您可以在打开数据库时提供自定义比较器。
例如，假设每个数据库键由两个数字组成，我们应该按第一个数字排序，按第二个数字打破联系。
首先，定义一个适当的`leveldb::Comparator`子类，它表示以下规则：

```c++
class TwoPartComparator : public leveldb::Comparator {
 public:
  // Three-way comparison function:
  //   if a < b: negative result
  //   if a > b: positive result
  //   else: zero result
  int Compare(const leveldb::Slice& a, const leveldb::Slice& b) const {
    int a1, a2, b1, b2;
    ParseKey(a, &a1, &a2);
    ParseKey(b, &b1, &b2);
    if (a1 < b1) return -1;
    if (a1 > b1) return +1;
    if (a2 < b2) return -1;
    if (a2 > b2) return +1;
    return 0;
  }

  // Ignore the following methods for now:
  const char* Name() const { return "TwoPartComparator"; }
  void FindShortestSeparator(std::string*, const leveldb::Slice&) const {}
  void FindShortSuccessor(std::string*) const {}
};
```

Now create a database using this custom comparator:

现在使用此自定义比较器创建数据库：

```c++
TwoPartComparator cmp;
leveldb::DB* db;
leveldb::Options options;
options.create_if_missing = true;
options.comparator = &cmp;
leveldb::Status status = leveldb::DB::Open(options, "/tmp/testdb", &db);
...
```

### Backwards compatibility

The result of the comparator's Name method is attached to the database when it
is created, and is checked on every subsequent database open. If the name
changes, the `leveldb::DB::Open` call will fail. Therefore, change the name if
and only if the new key format and comparison function are incompatible with
existing databases, and it is ok to discard the contents of all existing
databases.

比较器名称方法的结果在创建时附加到数据库，并在随后打开的每个数据库中进行检查。
如果名称更改，`leveldb::DB::Open`调用将失败。
因此，如果且仅当新的键格式和比较函数与现有数据库不兼容，并且可以放弃所有现有数据库的内容时，才更改名称。

You can however still gradually evolve your key format over time with a little
bit of pre-planning. For example, you could store a version number at the end of
each key (one byte should suffice for most uses). When you wish to switch to a
new key format (e.g., adding an optional third part to the keys processed by
`TwoPartComparator`), (a) keep the same comparator name (b) increment the
version number for new keys (c) change the comparator function so it uses the
version numbers found in the keys to decide how to interpret them.

然而，您仍然可以通过一点预先规划，随着时间的推移逐渐发展您的键格式。
例如，您可以在每个键的末尾存储一个版本号（对于大多数使用，一个字节应该足够了）。
当您希望切换到新的键格式（例如，向`TwoPartComparator`处理的键添加可选的第三部分）时，
（a） 保持相同的比较器名称（b）增加新键的版本号（c）更改比较器功能，以便它使用键中找到的版本号来决定如何解释它们。

## Performance

Performance can be tuned by changing the default values of the types defined in
`include/options.h`.

可以通过更改`include/options.h`中定义的类型的默认值来调整性能。

### Block size

leveldb groups adjacent keys together into the same block and such a block is
the unit of transfer to and from persistent storage. The default block size is
approximately 4096 uncompressed bytes.  Applications that mostly do bulk scans
over the contents of the database may wish to increase this size. Applications
that do a lot of point reads of small values may wish to switch to a smaller
block size if performance measurements indicate an improvement. There isn't much
benefit in using blocks smaller than one kilobyte, or larger than a few
megabytes. Also note that compression will be more effective with larger block
sizes.

leveldb将相邻的密钥分组到同一个块中，这样的块是向持久性存储进行传输和从持久性存储进行传输的单元。
默认块大小约为4096个未压缩字节。主要对数据库内容进行批量扫描的应用程序可能希望增加此大小。
如果性能测量结果表明性能有所改善，那么对小值进行大量点读取的应用程序可能希望切换到较小的块大小。
使用小于1 KB或大于数MB的块没有多大好处。还要注意，块大小越大，压缩就越有效。

### Compression

Each block is individually compressed before being written to persistent
storage. Compression is on by default since the default compression method is
very fast, and is automatically disabled for uncompressible data. In rare cases,
applications may want to disable compression entirely, but should only do so if
benchmarks show a performance improvement:

每个块在写入持久存储之前都会被单独压缩。
默认情况下，压缩处于启用状态，因为默认的压缩方法非常快，并且对于不可压缩的数据自动禁用。
在极少数情况下，应用程序可能希望完全禁用压缩，但只有在基准测试显示性能改善时才应该这样做：

```c++
leveldb::Options options;
options.compression = leveldb::kNoCompression;
... leveldb::DB::Open(options, name, ...) ....
```

### Cache

The contents of the database are stored in a set of files in the filesystem and
each file stores a sequence of compressed blocks. If options.block_cache is
non-NULL, it is used to cache frequently used uncompressed block contents.

数据库的内容存储在文件系统中的一组文件中，每个文件存储一系列压缩块。如果选项`options.block_cache`为非NULL，用于缓存常用的未压缩块内容。

```c++
#include "leveldb/cache.h"

leveldb::Options options;
options.block_cache = leveldb::NewLRUCache(100 * 1048576);  // 100MB cache
leveldb::DB* db;
leveldb::DB::Open(options, name, &db);
... use the db ...
delete db
delete options.block_cache;
```

Note that the cache holds uncompressed data, and therefore it should be sized
according to application level data sizes, without any reduction from
compression. (Caching of compressed blocks is left to the operating system
buffer cache, or any custom Env implementation provided by the client.)

注意 缓存保存的是未压缩的数据，因此应该根据应用程序所需的数据大小来设置它的大小。
（已压缩数据的缓存工作交给操作系统的 buffer cache 或者用户自定义的 Env 实现去干。）

When performing a bulk read, the application may wish to disable caching so that
the data processed by the bulk read does not end up displacing most of the
cached contents. A per-iterator option can be used to achieve this:

当执行一个大块数据读操作时，应用程序可能想要取消缓存功能，
这样读进来的大块数据就不会导致当前缓存中的大部分数据被置换出去，我们可以为它提供一个单独的迭代器来达到该目的：

```c++
leveldb::ReadOptions options;
options.fill_cache = false;
leveldb::Iterator* it = db->NewIterator(options);
for (it->SeekToFirst(); it->Valid(); it->Next()) {
  ...
}
delete it;
```

### Key Layout

Note that the unit of disk transfer and caching is a block. Adjacent keys
(according to the database sort order) will usually be placed in the same block.
Therefore the application can improve its performance by placing keys that are
accessed together near each other and placing infrequently used keys in a
separate region of the key space.

注意，磁盘传输和缓存的单位都是一个 block，相邻的 keys（已排序）总在同一个 block 中，
因此应用可以通过把需要一起访问的 keys 放在一起，同时把不经常使用的 keys 放到一个独立的键空间区域来提升性能。

For example, suppose we are implementing a simple file system on top of leveldb.
The types of entries we might wish to store are:

举个例子，假设我们正基于 leveldb 实现一个简单的文件系统。我们打算存储到这个文件系统的数据类型如下：

    filename -> permission-bits, length, list of file_block_ids
    file_block_id -> data

We might want to prefix filename keys with one letter (say '/') and the
`file_block_id` keys with a different letter (say '0') so that scans over just
the metadata do not force us to fetch and cache bulky file contents.

我们可以给上面表示 filename 的 key 增加一个字符前缀，例如 '/'，
然后给表示 `file_block_id`  的 key 增加另一个不同的前缀，例如 '0'，
这样这些不同用途的 key 就具有了各自独立的键空间区域，扫描元数据的时候我们就不用读取和缓存大块文件内容数据了。

### Filters

Because of the way leveldb data is organized on disk, a single `Get()` call may
involve multiple reads from disk. The optional FilterPolicy mechanism can be
used to reduce the number of disk reads substantially.

鉴于 leveldb 数据在磁盘上的组织形式，一次`Get()` 调用可能涉及多次磁盘读操作，
可配置的 FilterPolicy 机制可以用来大幅减少磁盘读次数。

```c++
leveldb::Options options;
options.filter_policy = NewBloomFilterPolicy(10);
leveldb::DB* db;
leveldb::DB::Open(options, "/tmp/testdb", &db);
... use the database ...
delete db;
delete options.filter_policy;
```

The preceding code associates a Bloom filter based filtering policy with the
database.  Bloom filter based filtering relies on keeping some number of bits of
data in memory per key (in this case 10 bits per key since that is the argument
we passed to `NewBloomFilterPolicy`). This filter will reduce the number of
unnecessary disk reads needed for Get() calls by a factor of approximately
a 100. Increasing the bits per key will lead to a larger reduction at the cost
of more memory usage. We recommend that applications whose working set does not
fit in memory and that do a lot of random reads set a filter policy.

上述代码将一个基于布隆过滤器的过滤策略与数据库进行了关联，基于布隆过滤器的过滤方式依赖于如下事实，
在内存中保存每个 key 的部分位（在上面例子中是 10 位，因为我们传给`NewBloomFilterPolicy`的参数是 10），
这个过滤器将会使得 Get() 调用中非必须的磁盘读操作大约减少 100 倍，每个 key 用于过滤器的位数增加将会进一步减少读磁盘次数，
当然也会占用更多内存空间。我们推荐数据集无法全部放入内存同时又存在大量随机读的应用设置一个过滤器策略。

If you are using a custom comparator, you should ensure that the filter policy
you are using is compatible with your comparator. For example, consider a
comparator that ignores trailing spaces when comparing keys.
`NewBloomFilterPolicy` must not be used with such a comparator. Instead, the
application should provide a custom filter policy that also ignores trailing
spaces. For example:


如果你在使用自定义的比较器，应该确保你在用的过滤器策略与你的比较器兼容。
举个例子，如果一个比较器在比较 key 的时候忽略结尾的空格，那么`NewBloomFilterPolicy`一定不能与此比较器共存。
相反，应用应该提供一个自定义的过滤器策略，而且它也应该忽略 key 的尾部空格，示例如下：

```c++
class CustomFilterPolicy : public leveldb::FilterPolicy {
 private:
  leveldb::FilterPolicy* builtin_policy_;

 public:
  CustomFilterPolicy() : builtin_policy_(leveldb::NewBloomFilterPolicy(10)) {}
  ~CustomFilterPolicy() { delete builtin_policy_; }

  const char* Name() const { return "IgnoreTrailingSpacesFilter"; }

  void CreateFilter(const leveldb::Slice* keys, int n, std::string* dst) const {
    // Use builtin bloom filter code after removing trailing spaces
    std::vector<leveldb::Slice> trimmed(n);
    for (int i = 0; i < n; i++) {
      trimmed[i] = RemoveTrailingSpaces(keys[i]);
    }
    builtin_policy_->CreateFilter(trimmed.data(), n, dst);
  }
};
```

Advanced applications may provide a filter policy that does not use a bloom
filter but uses some other mechanism for summarizing a set of keys. See
`leveldb/filter_policy.h` for detail.

当然也可以自己提供非基于布隆过滤器的过滤器策略，具体见`leveldb/filter_policy.h`。

## Checksums

leveldb associates checksums with all data it stores in the file system. There
are two separate controls provided over how aggressively these checksums are
verified:

leveldb 将校验和与它存储在文件系统中的所有数据进行关联，对于这些校验和，有两个独立的控制：

`ReadOptions::verify_checksums` may be set to true to force checksum
verification of all data that is read from the file system on behalf of a
particular read.  By default, no such verification is done.

`ReadOptions::verify_checksums`可以设置为 true，以强制对所有从文件系统读取的数据进行校验。默认为 false，即，不会进行这样的校验。

`Options::paranoid_checks` may be set to true before opening a database to make
the database implementation raise an error as soon as it detects an internal
corruption. Depending on which portion of the database has been corrupted, the
error may be raised when the database is opened, or later by another database
operation. By default, paranoid checking is off so that the database can be used
even if parts of its persistent storage have been corrupted.

`Options::paranoid_checks`在数据库打开之前设置为 true ，以使得数据库一旦检测到数据损毁立即报错。
根据数据库损坏的部位，报错可能是在打开数据库后，也可能是在后续执行某个操作的时候。该配置默认是关闭状态，即，持久化存储部分损坏数据库也能继续使用。

If a database is corrupted (perhaps it cannot be opened when paranoid checking
is turned on), the `leveldb::RepairDB` function may be used to recover as much
of the data as possible

如果数据库损坏了(当开启 Options::paranoid_checks 的时候可能就打不开了)，`leveldb::RepairDB`函数可以用于对尽可能多的数据进行修复。

## Approximate Sizes

The `GetApproximateSizes` method can used to get the approximate number of bytes
of file system space used by one or more key ranges.

`GetApproximateSizes`方法用于获取一个或多个键区间占据的文件系统近似大小(单位, 字节)

```c++
leveldb::Range ranges[2];
ranges[0] = leveldb::Range("a", "c");
ranges[1] = leveldb::Range("x", "z");
uint64_t sizes[2];
db->GetApproximateSizes(ranges, 2, sizes);
```

The preceding call will set `sizes[0]` to the approximate number of bytes of
file system space used by the key range `[a..c)` and `sizes[1]` to the
approximate number of bytes used by the key range `[x..z)`.

上述代码结果是，`sizes[0]`保存`[a..c)`区间对应的文件系统大致字节数。`sizes[1]`保存 `[x..z)`键区间对应的文件系统大致字节数。

## Environment

All file operations (and other operating system calls) issued by the leveldb
implementation are routed through a `leveldb::Env` object. Sophisticated clients
may wish to provide their own Env implementation to get better control.
For example, an application may introduce artificial delays in the file IO
paths to limit the impact of leveldb on other activities in the system.

由 leveldb 发起的全部文件操作以及其它的操作系统调用最后都会被路由给一个`leveldb::Env` 对象。
用户也可以提供自己的 Env 实现以达到更好的控制。
比如，如果应用程序想要针对 leveldb 的文件 IO 引入一个人工延迟以限制 leveldb 对同一个系统中其它应用的影响：

```c++
class SlowEnv : public leveldb::Env {
  ... implementation of the Env interface ...
};

SlowEnv env;
leveldb::Options options;
options.env = &env;
Status s = leveldb::DB::Open(options, ...);
```

## Porting

leveldb may be ported to a new platform by providing platform specific
implementations of the types/methods/functions exported by
`leveldb/port/port.h`.  See `leveldb/port/port_example.h` for more details.

如果某个特定平台提供`leveldb/port/port.h`导出的类型/方法/函数实现，那么 leveldb 可以被移植到该平台上，更多细节见 `leveldb/port/port_example.h`。

In addition, the new platform may need a new default `leveldb::Env`
implementation.  See `leveldb/util/env_posix.h` for an example.

另外，新平台可能还需要一个新的默认的`leveldb::Env`实现。具体可参考`leveldb/util/env_posix.h`实现。

## Other Information

Details about the leveldb implementation may be found in the following
documents:

有关leveldb实现的详细信息，请参见以下文档：

1. [Implementation notes 实现笔记](https://github.com/ejunjsh/myleveldb/blob/main/doc/impl.md)
2. [Format of an immutable Table file 不可变表文件的格式](https://github.com/ejunjsh/myleveldb/blob/main/doc/table_format.md)
3. [Format of a log file 日志文件格式](https://github.com/ejunjsh/myleveldb/blob/main/doc/log_format.md)
