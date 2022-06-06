## Files

The implementation of leveldb is similar in spirit to the representation of a
single [Bigtable tablet (section 5.3)](https://research.google/pubs/pub27898/).
However the organization of the files that make up the representation is
somewhat different and is explained below.

leveldb的实现在精神上类似于单个[Bigtable tablet（第5.3节）]的表示(https://research.google/pubs/pub27898/).
但是，构成表示的文件的组织有所不同，下面将对此进行解释。

Each database is represented by a set of files stored in a directory. There are
several different types of files as documented below:

每个数据库由存储在目录中的一组文件表示。有以下几种不同类型的文件：

### Log files

A log file (*.log) stores a sequence of recent updates. Each update is appended
to the current log file. When the log file reaches a pre-determined size
(approximately 4MB by default), it is converted to a sorted table (see below)
and a new log file is created for future updates.

日志文件（*.log）存储最近更新的序列。每次更新都会附加到当前日志文件中。
当日志文件达到预先确定的大小（默认情况下约为4MB）时，它将转换为已排序的表（请参见下文），并创建一个新的日志文件以供将来更新。

A copy of the current log file is kept in an in-memory structure (the
`memtable`). This copy is consulted on every read so that read operations
reflect all logged updates.

当前日志文件的副本保存在内存结构（`memtable`）中。每次读取时都会查阅此副本，以便读取操作反映所有记录的更新。

## Sorted tables

A sorted table (*.ldb) stores a sequence of entries sorted by key. Each entry is
either a value for the key, or a deletion marker for the key. (Deletion markers
are kept around to hide obsolete values present in older sorted tables).

排序表（*.ldb）存储按键排序的条目序列。每个条目要么是键的值，要么是键的删除标记。（保留删除标记以隐藏旧排序表中存在的过时值）。

The set of sorted tables are organized into a sequence of levels. The sorted
table generated from a log file is placed in a special **young** level (also
called level-0). When the number of young files exceeds a certain threshold
(currently four), all of the young files are merged together with all of the
overlapping level-1 files to produce a sequence of new level-1 files (we create
a new level-1 file for every 2MB of data.)

这组已排序的表被组织成一系列层。从日志文件生成的排序表被置于一个特殊的**年轻**层（也称为0层）。
当年轻文件的数量超过某个阈值（当前为四个）时，所有年轻文件将与所有重叠的1层文件合并在一起，以生成一系列新的1层文件（我们为每2MB的数据创建一个新的1层文件）

Files in the young level may contain overlapping keys. However files in other
levels have distinct non-overlapping key ranges. Consider level number L where
L >= 1. When the combined size of files in level-L exceeds (10^L) MB (i.e., 10MB
for level-1, 100MB for level-2, ...), one file in level-L, and all of the
overlapping files in level-(L+1) are merged to form a set of new files for
level-(L+1). These merges have the effect of gradually migrating new updates
from the young level to the largest level using only bulk reads and writes
(i.e., minimizing expensive seeks).

年轻层的文件可能包含重叠键。但是，其他层的文件具有不同的非重叠键范围（即不重叠）。
假设有个层L而且L>=1.当L层的文件总大小超过(10^L) MB（例如层1是10MB，则层2就是100MB。。。）
一个文件在层L与L+1层重叠的文件会合并在一起放到L+1层。这些合并的效果是，
仅使用大容量读写操作将新更新从年轻级别逐渐迁移到最大级别（即尽量减少昂贵的搜索）。

### Manifest

A MANIFEST file lists the set of sorted tables that make up each level, the
corresponding key ranges, and other important metadata. A new MANIFEST file
(with a new number embedded in the file name) is created whenever the database
is reopened. The MANIFEST file is formatted as a log, and changes made to the
serving state (as files are added or removed) are appended to this log.

清单文件列出了组成每个级别的一组排序表、相应的键范围和其他重要元数据。
每当重新打开数据库时，都会创建一个新的清单文件（文件名中嵌入了一个新的编号）。
清单文件的格式为日志，对服务状态所做的更改（添加或删除文件时）将附加到此日志。

### Current

CURRENT is a simple text file that contains the name of the latest MANIFEST
file.

CURRENT是一个简单的文本文件，其中包含最新MANIFEST文件的名称。

### Info logs

Informational messages are printed to files named LOG and LOG.old.

信息消息将打印到名为LOG和LOG.old的文件中

### Others

Other files used for miscellaneous purposes may also be present (LOCK, *.dbtmp).

也可能存在其他用于其他目的的文件（LOCK，*.dbtmp）。

## Level 0

When the log file grows above a certain size (4MB by default):
Create a brand new memtable and log file and direct future updates here.

当日志文件增长到一定大小（默认为4MB）以上时：
创建一个全新的memtable和日志文件，并把将来的更新放到这里。

In the background:

1. Write the contents of the previous memtable to an sstable.
2. Discard the memtable.
3. Delete the old log file and the old memtable.
4. Add the new sstable to the young (level-0) level.

在后台：

1. 将上一个memtable的内容写入sstable。
2. 丢弃memtable。
3. 删除旧日志文件和旧memtable。
4. 将新的sstable添加到年轻层（0层）。

## Compactions

When the size of level L exceeds its limit, we compact it in a background
thread. The compaction picks a file from level L and all overlapping files from
the next level L+1. Note that if a level-L file overlaps only part of a
level-(L+1) file, the entire file at level-(L+1) is used as an input to the
compaction and will be discarded after the compaction.  Aside: because level-0
is special (files in it may overlap each other), we treat compactions from
level-0 to level-1 specially: a level-0 compaction may pick more than one
level-0 file in case some of these files overlap each other.

当级别L的大小超过其限制时，我们将其压缩在后台线程中。
压缩从层L中拾取一个文件，并从下一个层L+1中拾取所有重叠文件。请注意，如果层L文件仅与层（L+1）文件的一部分重叠，则层（L+1）的整个文件将用作压缩的输入，并将在压缩后丢弃。
旁白：由于0层是特殊的（其中的文件可能相互重叠），我们特别处理从0层到1层的压缩：如果其中一些文件相互重叠，0层压缩可能会拾取多个0层文件。

A compaction merges the contents of the picked files to produce a sequence of
level-(L+1) files. We switch to producing a new level-(L+1) file after the
current output file has reached the target file size (2MB). We also switch to a
new output file when the key range of the current output file has grown enough
to overlap more than ten level-(L+2) files.  This last rule ensures that a later
compaction of a level-(L+1) file will not pick up too much data from
level-(L+2).

压缩将合并拾取的文件的内容，以生成一系列层（L+1）的文件。
在当前输出文件达到目标文件大小（2MB）后，我们切换到生成一个新的层（L+1）文件。
当当前输出文件的键范围增长到足以重叠10多个层（L+2）文件时，我们还会切换到新的输出文件。
最后一条规则确保以后对层（L+1）文件的压缩不会从层（L+2）提取太多数据。

The old files are discarded and the new files are added to the serving state.

旧文件将被丢弃，新文件将添加到服务状态。

Compactions for a particular level rotate through the key space. In more detail,
for each level L, we remember the ending key of the last compaction at level L.
The next compaction for level L will pick the first file that starts after this
key (wrapping around to the beginning of the key space if there is no such
file).

特定级别的压缩在键空间中轮询。更详细地说，
对于每个层L，我们记住层L最后一次压缩的结束键。
层L的下一次压缩将拾取在此键之后开始的第一个文件（如果没有这样的文件，则环绕到键空间的开头）。

Compactions drop overwritten values. They also drop deletion markers if there
are no higher numbered levels that contain a file whose range overlaps the
current key.

压缩会删除覆盖的值。如果没有更高编号的层包含范围与当前键重叠的文件，它们也会删除删除标记。

### Timing

Level-0 compactions will read up to four 1MB files from level-0, and at worst
all the level-1 files (10MB). I.e., we will read 14MB and write 14MB.

0层压缩将从0层读取多达4个1MB的文件，最坏的情况是读取所有1级文件（10MB）。 例如，我们将读14MB，写14MB。

Other than the special level-0 compactions, we will pick one 2MB file from level
L. In the worst case, this will overlap ~ 12 files from level L+1 (10 because
level-(L+1) is ten times the size of level-L, and another two at the boundaries
since the file ranges at level-L will usually not be aligned with the file
ranges at level-L+1). The compaction will therefore read 26MB and write 26MB.
Assuming a disk IO rate of 100MB/s (ballpark range for modern drives), the worst
compaction cost will be approximately 0.5 second.

除了特殊的0层压缩之外，我们将从L层中选择一个2MB文件。
在最坏的情况下，这将与层L+1的约12个文件重叠（10个，因为层（L+1）的大小是层L的十倍，另外两个位于边界，因为层L的文件范围通常不会与层L+1的文件范围对齐）。
因此，压缩将读取26MB，写入26MB。
假设磁盘IO速率为100MB/s（现代驱动器的大致范围）
压缩成本约为0.5秒。

If we throttle the background writing to something small, say 10% of the full
100MB/s speed, a compaction may take up to 5 seconds. If the user is writing at
10MB/s, we might build up lots of level-0 files (~50 to hold the 5*10MB). This
may significantly increase the cost of reads due to the overhead of merging more
files together on every read.

如果我们将后台写入限制在较小的范围内，例如100MB/s全速度的10%，则压缩可能需要5秒。
如果用户以10MB/s的速度写入，我们可能会构建大量的0级文件（大约50个以容纳5*10MB）。
由于每次读取时将更多文件合并在一起的开销，这可能会显著增加读取成本。

Solution 1: To reduce this problem, we might want to increase the log switching
threshold when the number of level-0 files is large. Though the downside is that
the larger this threshold, the more memory we will need to hold the
corresponding memtable.

解决方案1：为了减少此问题，我们可能希望在0层文件数量较大时增加日志切换阈值。尽管缺点是这个阈值越大，我们需要保存相应memtable的内存就越多。

Solution 2: We might want to decrease write rate artificially when the number of
level-0 files goes up.

解决方案2：当0层文件数量增加时，我们可能希望人为地降低写入速率。

Solution 3: We work on reducing the cost of very wide merges. Perhaps most of
the level-0 files will have their blocks sitting uncompressed in the cache and
we will only need to worry about the O(N) complexity in the merging iterator.

解决方案3：我们致力于降低大规模合并的成本。也许大多数0层文件的块都未压缩地放在缓存中，我们只需要担心合并迭代器中的O(N)复杂性。

### Number of files

Instead of always making 2MB files, we could make larger files for larger levels
to reduce the total file count, though at the expense of more bursty
compactions.  Alternatively, we could shard the set of files into multiple
directories.

我们可以为更大的层创建更大的文件，而不是总是生成2MB文件，
以减少总的文件数，尽管这是以牺牲更多的突发压缩为代价的。
或者，我们可以将文件集分成多个目录。

An experiment on an ext3 filesystem on Feb 04, 2011 shows the following timings
to do 100K file opens in directories with varying number of files:

2011年2月4日在ext3文件系统上进行的一次实验显示，在不同文件数的目录中打开100K文件的时间如下：

| Files in directory | Microseconds to open a file |
|-------------------:|----------------------------:|
|               1000 |                           9 |
|              10000 |                          10 |
|             100000 |                          16 |

So maybe even the sharding is not necessary on modern filesystems?

那么，在现代文件系统上，甚至分片都不是必需的？

## Recovery

* Read CURRENT to find name of the latest committed MANIFEST
* Read the named MANIFEST file
* Clean up stale files
* We could open all sstables here, but it is probably better to be lazy...
* Convert log chunk to a new level-0 sstable
* Start directing new writes to a new log file with recovered sequence#

* 读取CURRENT以查找最新提交的MANIFEST的名称
* 读取命名MANIFEST文件
* 清理陈旧文件
* 我们可以在这里打开所有sstables，但最好是延迟加载。。。
* 将日志区块转换为新的0层sstable
* 开始将新写操作定向到具有恢复序列的新日志文件上

## Garbage collection of files

`RemoveObsoleteFiles()` is called at the end of every compaction and at the end
of recovery. It finds the names of all files in the database. It deletes all log
files that are not the current log file. It deletes all table files that are not
referenced from some level and are not the output of an active compaction.

`RemoveObsoleteFiles()`在每次压缩结束和恢复结束时调用。
它查找数据库中所有文件的名称。它删除所有不是当前日志文件的日志文件。
它删除所有未从某个层引用且不是活动压缩输出的表文件。
