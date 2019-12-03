#ifndef WANGSQL_BUFFERMANAGER_H
#define WANGSQL_BUFFERMANAGER_H

#include <string>
#include <unordered_map>

#include "global.h"

using namespace std;

// 数据块双向循环链表的节点结构体. 使用 LRU 策略
struct BlockNode
{
    Block *block;
    BlockNode *pre;
    BlockNode *nxt;

    BlockNode(Block *_block): block(_block) {}

    // 将该节点加入到node之后
    void add(BlockNode *node)
    {
        pre = node;
        nxt = node->nxt;
        node->nxt->pre = this;
        node->nxt = this;
    }

    // 移除该节点
    void remove() { pre->nxt = nxt; nxt->pre = pre;}

    ~BlockNode() { remove(); }
};

class BufferManager
{
public:

    // 块的最大数量
    static const int MAX_BLOCK_COUNT;

    BufferManager();

    ~BufferManager();

    // 获得文件中id-th块，此id并非记录的id
    Block *getBlock(const char *filename, int id);

    // 删除文件中所有块
    void removeBlockByFilename(const char *filename);

#ifdef DEBUG
    // Print block filename and id
    void debugPrint() const;
#endif

private:

    // 当前块的数量
    int blockCnt;

    // 虚假的头节点和尾节点
    BlockNode *lruHead;
    BlockNode *lruTail;

    // "文件名+块编号" -> "块节点" 的映射
    unordered_map<string, BlockNode*> nodeMap;

    // 从内存中删除节点和它表示的块
    void deleteNodeBlock(BlockNode *node, bool write = true);

    // 从文件中加载id-th块
    Block *loadBlock(const char *filename, int id);

    // 修改文件中id-th块
    void writeBlock(const char *filename, int id);
};

#endif //WANGSQL_BUFFERMANAGER_H
