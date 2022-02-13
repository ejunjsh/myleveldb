leveldb File format
===================

    <beginning_of_file>        // 文件开始
    [data block 1]
    [data block 2]
    ...
    [data block N]
    [meta block 1]
    ...
    [meta block K]
    [metaindex block]
    [index block]
    [Footer]        (fixed size; starts at file_size - sizeof(Footer))  // 固定大小，开始于（文件大小 - sizeof(Footer)）的位置
    <end_of_file>      // 文件结束

The file contains internal pointers.  Each such pointer is called
a BlockHandle and contains the following information:

该文件包含内部指针。每个这样的指针称为块句柄，包含以下信息：

    offset:   varint64             // 块在文件里的位移
    size:     varint64             // 块大小

See [varints](https://developers.google.com/protocol-buffers/docs/encoding#varints)
for an explanation of varint64 format.
varint64格式是什么，可以看这个解释 [varints](https://developers.google.com/protocol-buffers/docs/encoding#varints)

1.  The sequence of key/value pairs in the file are stored in sorted
order and partitioned into a sequence of data blocks.  These blocks
come one after another at the beginning of the file.  Each data block
is formatted according to the code in `block_builder.cc`, and then
optionally compressed.

文件中的键/值对序列按排序顺序存储，并划分为一系列数据块。这些块在文件的开头一个接一个地出现。
每个数据块都根据`block_builder.cc`中的代码进行格式化，然后可选地进行压缩。

2. After the data blocks we store a bunch of meta blocks.  The
supported meta block types are described below.  More meta block types
may be added in the future.  Each meta block is again formatted using
`block_builder.cc` and then optionally compressed.

在数据块之后，我们存储了一堆元块。
支持的元块类型如下所述。将来可能会添加更多元块类型。
每个元块都再次使用`block_builder.cc`进行格式化，然后可选地压缩。

3. A "metaindex" block.  It contains one entry for every other meta
block where the key is the name of the meta block and the value is a
BlockHandle pointing to that meta block.

“元索引”块。它为每个其他元块包含一个条目，其中键是元块的名称，值是指向该元块的块句柄。

4. An "index" block.  This block contains one entry per data block,
where the key is a string >= last key in that data block and before
the first key in the successive data block.  The value is the
BlockHandle for the data block.

“索引”块。此块包含每个数据块的一个条目，
其中键是一个字符串>=该数据块中的最后一个键，位于下一个数据块中的第一个键之前。
其中值是数据块的块句柄。

5. At the very end of the file is a fixed length footer that contains
the BlockHandle of the metaindex and index blocks as well as a magic number.

在文件的最后是一个固定长度的页脚，其中包含元索引和索引块的块句柄以及一个幻数。

        metaindex_handle: char[p];     // Block handle for metaindex  元索引块的块句柄
        index_handle:     char[q];     // Block handle for index     索引块的块句柄
        padding:          char[40-p-q];// zeroed bytes to make fixed length   将字节归零，使其成为固定长度
                                       // (40==2*BlockHandle::kMaxEncodedLength)
        magic:            fixed64;     // == 0xdb4775248b80fb57 (little-endian) 小端

## "filter" Meta Block  “筛选器”元块

If a `FilterPolicy` was specified when the database was opened, a
filter block is stored in each table.  The "metaindex" block contains
an entry that maps from `filter.<N>` to the BlockHandle for the filter
block where `<N>` is the string returned by the filter policy's
`Name()` method.

如果一个`FilterPolicy`在数据库打开时被指定，一个筛选器块被存储在每张表。
"元索引"块包含一个条目，它从`filter.<N>`映射到一个块句柄，“元索引”指向到一个筛选器块
而`<N>`就是筛选器策略`Name()`方法的返回值，它是个字符串

The filter block stores a sequence of filters, where filter i contains
the output of `FilterPolicy::CreateFilter()` on all keys that are stored
in a block whose file offset falls within the range

筛选器块存储一系列的筛选器，其中筛选器i包含`FilterPolicy::CreateFilter()`对所有键的输出
这些被存储在一个块，而块的偏移量就落在下面这个范围

    [ i*base ... (i+1)*base-1 ]

Currently, "base" is 2KB.  So for example, if blocks X and Y start in
the range `[ 0KB .. 2KB-1 ]`, all of the keys in X and Y will be
converted to a filter by calling `FilterPolicy::CreateFilter()`, and the
resulting filter will be stored as the first filter in the filter
block.

当前，"base"是2KB, 举个例，如果块X和Y开始在范围`[ 0KB .. 2KB-1 ]`，
所有在X和Y的键通过调用`FilterPolicy::CreateFilter()`转换成一个筛选器，
而这个筛选器将存到筛选器块的第一个筛选器

The filter block is formatted as follows:

筛选器块被格式化为下面这样：

    [filter 0]
    [filter 1]
    [filter 2]
    ...
    [filter N-1]

    [offset of filter 0]                  : 4 bytes
    [offset of filter 1]                  : 4 bytes
    [offset of filter 2]                  : 4 bytes
    ...
    [offset of filter N-1]                : 4 bytes

    [offset of beginning of offset array] : 4 bytes
    lg(base)                              : 1 byte

The offset array at the end of the filter block allows efficient
mapping from a data block offset to the corresponding filter.

筛选器块的尾部的偏移数组可以有效的从一个数据块的偏移量映射到对应的筛选器

## "stats" Meta Block  “统计信息”元块

This meta block contains a bunch of stats.  The key is the name
of the statistic.  The value contains the statistic.

这个元块包含一系列统计信息。键是统计信息的名称。该值包含统计信息。

TODO(postrelease): record following stats.  记录下面这些统计信息

    data size                   // 数据大小
    index size                  // 索引大小
    key size (uncompressed)     // 未压缩时的键大小
    value size (uncompressed)   // 未压缩时的值大小
    number of entries           // 条目数量
    number of data blocks       // 数据块数量
