leveldb Log format 日志格式
==================
The log file contents are a sequence of 32KB blocks.  The only exception is that
the tail of the file may contain a partial block.

日志文件内容是一系列大小为32KB的块。唯一的例外是文件的尾部可能包含一个部分块。

Each block consists of a sequence of records:

每个块由一系列记录组成：

    block := record* trailer?   // 块由多个记录和一个trailer组成，trailer可以没有
    record :=                   // 记录由下面几个组成：记录头（checksum，length, type）大小为：4 + 2 + 1 = 7 个字节
      checksum: uint32     // crc32c of type and data[] ; little-endian  对type和data[]做校验和；小端
      length: uint16       // little-endian 记录长度 小端
      type: uint8          // One of FULL, FIRST, MIDDLE, LAST   记录类型
      data: uint8[length]  // 记录数据

A record never starts within the last six bytes of a block (since it won't fit).
Any leftover bytes here form the trailer, which must consist entirely of zero
bytes and must be skipped by readers.

一条记录不会从块的最后六个字节开始，因为不适合（记录头就是七个字节了）。所以块剩下的字节就形成了trailer（这词不知怎么翻译中文）
它被设置成全部都是0，然后读者会跳过它

Aside: if exactly seven bytes are left in the current block, and a new non-zero
length record is added, the writer must emit a FIRST record (which contains zero
bytes of user data) to fill up the trailing seven bytes of the block and then
emit all of the user data in subsequent blocks.

旁白：如果正好当前块剩下七个字节（记录头刚好七个字节），一个新的零长度的记录会添加进来，
写者必须写入一个FIRST记录（包含0个字节的用户数据）去填充块剩下的这七个字节，然后把全部用户数据写入到接下来的块里面

More types may be added in the future.  Some Readers may skip record types they
do not understand, others may report that some data was skipped.

未来会加入更多类型。一些读者会跳过它们不认识的记录类型，其他的会报告某些数据被跳过了

    FULL == 1
    FIRST == 2
    MIDDLE == 3
    LAST == 4

The FULL record contains the contents of an entire user record.

FULL类型的记录包含完整的用户记录内容

FIRST, MIDDLE, LAST are types used for user records that have been split into
multiple fragments (typically because of block boundaries).  FIRST is the type
of the first fragment of a user record, LAST is the type of the last fragment of
a user record, and MIDDLE is the type of all interior fragments of a user
record.

FIRST, MIDDLE, LAST 这三种用户记录类型把用户记录切分成多个片段（通常是因为块边界）。
FIRST是用户记录的第一个片段类型，LAST是用户记录的最后一个片段类型，MIDDLE是用户记录的所有内部片段的类型。

Example: consider a sequence of user records:

例子：考虑这个用户记录序列：

    A: length 1000
    B: length 97270
    C: length 8000

**A** will be stored as a FULL record in the first block.

      将以FULL类型的记录存到第一个块

**B** will be split into three fragments: first fragment occupies the rest of
the first block, second fragment occupies the entirety of the second block, and
the third fragment occupies a prefix of the third block.  This will leave six
bytes free in the third block, which will be left empty as the trailer.

      将切分为三个片段：第一片段占据第一个块剩下的区域，第二个片段占据整个第二个块，第三个片段占据第三个块的前面一部分
      这样将在第三个块上留下6个字节，这6个字节会留空，作为trailer

**C** will be stored as a FULL record in the fourth block.

      将以FULL类型的记录存到第四个块

----

## Some benefits over the recordio format: 与recordio格式相比，它有一些优点：

1. We do not need any heuristics for resyncing - just go to next block boundary
   and scan.  If there is a corruption, skip to the next block.  As a
   side-benefit, we do not get confused when part of the contents of one log
   file are embedded as a record inside another log file.

   我们不需要任何用于重新同步的启发式方法——只需转到下一个块边界并扫描即可。如果存在损坏，请跳到下一个块。另一个好处是，当一个日志文件的部分内容作为记录嵌入到另一个日志文件中时，我们不会感到困惑。

2. Splitting at approximate boundaries (e.g., for mapreduce) is simple: find the
   next block boundary and skip records until we hit a FULL or FIRST record.

   在近似边界处拆分（例如，对于mapreduce）很简单：找到下一个块边界并跳过记录，直到找到FULL或FIRST记录。

3. We do not need extra buffering for large records.

   对于大型记录，我们不需要额外的缓冲。

## Some downsides compared to recordio format: 与recordio格式相比，它有一些缺点：

1. No packing of tiny records.  This could be fixed by adding a new record type,
   so it is a shortcoming of the current implementation, not necessarily the
   format.

   没有小记录的包装。这可以通过添加新的记录类型来解决，因此这是当前实现的一个缺点，但不是必须的

2. No compression.  Again, this could be fixed by adding new record types.

   没有压缩。同样，这可以通过添加新的记录类型来解决。
