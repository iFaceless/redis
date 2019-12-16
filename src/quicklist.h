/* quicklist.h - A generic doubly linked quicklist implementation
 *
 * Copyright (c) 2014, Matt Stancliff <matt@genges.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this quicklist of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this quicklist of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QUICKLIST_H__
#define __QUICKLIST_H__

/* Node, quicklist, and Iterator are the only data structures used currently. */

/* quicklistNode is a 32 byte struct describing a ziplist for a quicklist.
 * We use bit fields keep the quicklistNode at 32 bytes.
 * count: 16 bits, max 65536 (max zl bytes is 65k, so max count actually < 32k).
 * encoding: 2 bits, RAW=1, LZF=2.
 * container: 2 bits, NONE=1, ZIPLIST=2.
 * recompress: 1 bit, bool, true if node is temporarry decompressed for usage.
 * attempted_compress: 1 bit, boolean, used for verifying during testing.
 * extra: 10 bits, free for future use; pads out the remainder of 32 bits */
// quicklistNode 实际上是对 ziplist 的描述，元素存储于 ziplist 中。
// 为什么需要在 quicklistNode 中位于 ziplist 的一些元信息呢？这主要是因为
// 节点指向的 ziplist 可以被压缩，这样就不能快速获取一些元信息了（元素个数等）。
typedef struct quicklistNode {
    // 双向链表，自然需要前后关联
    struct quicklistNode *prev;
    struct quicklistNode *next;
    // 指向 ziplist 指针（如果是压缩节点，实际指向的是 quicklistLZF）
    unsigned char *zl;
    // ziplist 的大小（bytes）
    unsigned int sz;             /* ziplist size in bytes */
    // ziplist 中存储的元素个数
    unsigned int count : 16;     /* count of items in ziplist */
    // 表示 ziplist 进行了压缩编码，RAW=1 表示没有压缩；2 表示使用了 LZF 算法压缩
    unsigned int encoding : 2;   /* RAW==1 or LZF==2 */
    // 容器类型
    unsigned int container : 2;  /* NONE==1 or ZIPLIST==2 */
    // 当前节点是否被压缩过？如果压缩过还需要在使用前进行解压
    unsigned int recompress : 1; /* was this node previous compressed? */
    // 当前节点太小了，无法压缩
    unsigned int attempted_compress : 1; /* node can't compress; too small */
    unsigned int extra : 10; /* more bits to steal for future usage */
} quicklistNode;

/* quicklistLZF is a 4+N byte struct holding 'sz' followed by 'compressed'.
 * 'sz' is byte length of 'compressed' field.
 * 'compressed' is LZF data with total (compressed) length 'sz'
 * NOTE: uncompressed length is stored in quicklistNode->sz.
 * When quicklistNode->zl is compressed, node->zl points to a quicklistLZF */
// quicklistLZF 表示 ziplist 压缩后的数据结构，它是一个 4+N 字节大小的结构体。
typedef struct quicklistLZF {
    // sz 表示 LZF 压缩后的数据大小
    unsigned int sz; /* LZF size in bytes*/
    // compressed 存储压缩后的数据
    char compressed[];
} quicklistLZF;

/* quicklist is a 40 byte struct (on 64-bit systems) describing a quicklist.
 * 'count' is the number of total entries.
 * 'len' is the number of quicklist nodes.
 * 'compress' is: -1 if compression disabled, otherwise it's the number
 *                of quicklistNodes to leave uncompressed at ends of quicklist.
 * 'fill' is the user-requested (or default) fill factor. */
// quicklist 是对快速链表这种数据结构的抽象，其中存储了一些元信息。
typedef struct quicklist {
    // head, tail 指向链表的头尾
    quicklistNode *head;
    quicklistNode *tail;
    // count 记录了所有 ziplists 的元素个数之和
    unsigned long count;        /* total count of all entries in all ziplists */
    // len 记录了有多少个 Nodes
    unsigned long len;          /* number of quicklistNodes */
    // fill 表示每个节点最多可以包含的数据项，正数表示最多可以含有的元素个数
    // 负数则表示每个节点 ziplist 的最大长度（字节数）：
    // -1, 4KB
    // -2, 8KB
    // -3, 16KB
    // -4, 32KB
    // -5, 64KB
    int fill : 16;              /* fill factor for individual nodes */
    // compress 表示两端不被压缩的节点个数。一般对于 list 的操作通常是在两端进行的
    // 所以，为了方便 LPUSH/LPOP/RPUSH/RPOP 命令，这里可以选择对两端不做压缩。
    // 但是为了节约内存，会对中间节点进行压缩（ziplist 已经够节约内存了，但是还是要压缩
    // 更进一步地节约内存），代价就是消耗点 CPU 时间用于压缩或者解压缩。
    unsigned int compress : 16; /* depth of end nodes not to compress;0=off */
} quicklist;

// quicklistIter 针对 quicklist 的迭代器，支持正向反向迭代。
typedef struct quicklistIter {
    const quicklist *quicklist;
    quicklistNode *current;
    unsigned char *zi; // 指向元素所在的 ziplist
    long offset; /* offset in current ziplist */
    int direction;
} quicklistIter;

// quicklistEntry 是对 quicklistNode 下 ziplist 某个元素的抽象，
// 方便我们获取元素的内容。
typedef struct quicklistEntry {
    // quicklist 指向 quicklist 的指针
    const quicklist *quicklist;
    // quicklistNode 当前 entry 关联的节点
    quicklistNode *node;
    // zi 关联的 ziplist
    unsigned char *zi;
    // value 指向 string 类型的元素位置
    unsigned char *value;
    // longvalue 元素转换为整数的值
    long long longval;
    // sz 表示 value 的有效长度（string 类型编码时，encoding 中的长度部分就是这个）
    unsigned int sz;
    // offset 表示在当前 ziplist 中的偏移量
    int offset;
} quicklistEntry;

#define QUICKLIST_HEAD 0
#define QUICKLIST_TAIL -1

/* quicklist node encodings */
#define QUICKLIST_NODE_ENCODING_RAW 1
#define QUICKLIST_NODE_ENCODING_LZF 2

/* quicklist compression disable */
#define QUICKLIST_NOCOMPRESS 0

/* quicklist container formats */
#define QUICKLIST_NODE_CONTAINER_NONE 1
#define QUICKLIST_NODE_CONTAINER_ZIPLIST 2

#define quicklistNodeIsCompressed(node)                                        \
    ((node)->encoding == QUICKLIST_NODE_ENCODING_LZF)

/* Prototypes */
quicklist *quicklistCreate(void);
quicklist *quicklistNew(int fill, int compress);
void quicklistSetCompressDepth(quicklist *quicklist, int depth);
void quicklistSetFill(quicklist *quicklist, int fill);
void quicklistSetOptions(quicklist *quicklist, int fill, int depth);
void quicklistRelease(quicklist *quicklist);
int quicklistPushHead(quicklist *quicklist, void *value, const size_t sz);
int quicklistPushTail(quicklist *quicklist, void *value, const size_t sz);
void quicklistPush(quicklist *quicklist, void *value, const size_t sz,
                   int where);
void quicklistAppendZiplist(quicklist *quicklist, unsigned char *zl);
quicklist *quicklistAppendValuesFromZiplist(quicklist *quicklist,
                                            unsigned char *zl);
quicklist *quicklistCreateFromZiplist(int fill, int compress,
                                      unsigned char *zl);
void quicklistInsertAfter(quicklist *quicklist, quicklistEntry *node,
                          void *value, const size_t sz);
void quicklistInsertBefore(quicklist *quicklist, quicklistEntry *node,
                           void *value, const size_t sz);
void quicklistDelEntry(quicklistIter *iter, quicklistEntry *entry);
int quicklistReplaceAtIndex(quicklist *quicklist, long index, void *data,
                            int sz);
int quicklistDelRange(quicklist *quicklist, const long start, const long stop);
quicklistIter *quicklistGetIterator(const quicklist *quicklist, int direction);
quicklistIter *quicklistGetIteratorAtIdx(const quicklist *quicklist,
                                         int direction, const long long idx);
int quicklistNext(quicklistIter *iter, quicklistEntry *node);
void quicklistReleaseIterator(quicklistIter *iter);
quicklist *quicklistDup(quicklist *orig);
int quicklistIndex(const quicklist *quicklist, const long long index,
                   quicklistEntry *entry);
void quicklistRewind(quicklist *quicklist, quicklistIter *li);
void quicklistRewindTail(quicklist *quicklist, quicklistIter *li);
void quicklistRotate(quicklist *quicklist);
int quicklistPopCustom(quicklist *quicklist, int where, unsigned char **data,
                       unsigned int *sz, long long *sval,
                       void *(*saver)(unsigned char *data, unsigned int sz));
int quicklistPop(quicklist *quicklist, int where, unsigned char **data,
                 unsigned int *sz, long long *slong);
unsigned long quicklistCount(const quicklist *ql);
int quicklistCompare(unsigned char *p1, unsigned char *p2, int p2_len);
size_t quicklistGetLzf(const quicklistNode *node, void **data);

#ifdef REDIS_TEST
int quicklistTest(int argc, char *argv[]);
#endif

/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

#endif /* __QUICKLIST_H__ */
