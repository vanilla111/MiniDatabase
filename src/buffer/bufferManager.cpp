#include <cstdio>
#include <iostream>
#include "buffer/bufferManager.h"

using namespace std;

const int BufferManager::MAX_BLOCK_COUNT = MAX_BLOCK;

BufferManager::BufferManager() {
    blockCnt = 0;
    lruHead = new BlockNode(nullptr);
    lruTail = new BlockNode(nullptr);
    lruHead->nxt = lruHead->pre = lruTail;
    lruTail->pre = lruTail->nxt = lruHead;
}

BufferManager::~BufferManager() {
    // 指针的移动交给node的析构函数
    while (lruHead->nxt != lruTail) {
        deleteNodeBlock(lruHead->nxt);
    }
    delete lruHead;
    delete lruTail;
}

Block* BufferManager::getBlock(const char *filename, int id) {
    string blockName = string(filename) + "`" + to_string(id);
    // end()方法永远返回最后一个元素之后的位置，find()方法找不到元素时返回的就是这个
    if (nodeMap.find(blockName) != nodeMap.end()) {
        // 将寻找的这个节点设置为最近使用过
        BlockNode *node = nodeMap[blockName];
        node->remove();
        node->add(lruHead);
        return node->block;
    }

    if (blockCnt == MAX_BLOCK_COUNT) {
        // 内存中存放的块达到最大数量限制
        // 将最久未使用的块移除链表
        BlockNode *node = lruTail->pre;
        while (node->block->pin) {
            node = node->pre;
        }
        deleteNodeBlock(node);
    }

    return loadBlock(filename, id);
}

void BufferManager::removeBlockByFilename(const char *filename) {
    BlockNode *nxtNode;
    for (BlockNode *node = lruHead->nxt; node != lruTail; node = nxtNode) {
        nxtNode = node->nxt;
        if (node->block->filename == filename) {
            deleteNodeBlock(node, false);
        }
    }

}

Block* BufferManager::loadBlock(const char *filename, int id) {
    Block *block = new Block(filename, id);
    FILE *file = fopen((DATA_STORE_DIR + string(filename) + DATA_FILE_TYPE).c_str(), "rb");
    fseek(file, BLOCK_SIZE * id, SEEK_SET);
    fread(block->content, BLOCK_SIZE, 1, file);
    fclose(file);

    // 将新读入的块加入链表中与映射中
    BlockNode *node = new BlockNode(block);
    node->add(lruHead);
    nodeMap[string(filename) + "`" + to_string(id)] = node;
    blockCnt++;

    return block;
}

void BufferManager::deleteNodeBlock(BlockNode *node, bool write) {
    Block *block = node->block;
    if (write)
        writeBlock(block->filename.c_str(), block->id);
    nodeMap.erase(block->filename + "`" + to_string(block->id));
    // 从链表中删除的操作在Node的析构函数中完成
    delete node;
    delete block;
    blockCnt--;
}

void BufferManager::writeBlock(const char *filename, int id) {
    Block *block = nodeMap[string(filename) + '`' + to_string(id)]->block;
    // 如果块不是脏的，直接返回
    if (!block->dirty)
        return;
    FILE *file = fopen((DATA_STORE_DIR + string(filename) + DATA_FILE_TYPE).c_str(), "rb+");
    fseek(file, id * BLOCK_SIZE, SEEK_SET);
    fwrite(block->content, BLOCK_SIZE, 1, file);
    fclose(file);
}

#ifdef DEBUG
void BufferManager::debugPrint() const
{
    BlockNode* node = lruHead->nxt;
    cerr << "DEBUG: [BufferManager::debugPrint]" << endl;
    for (; node != lruTail; node = node->nxt)
        cerr << "Block filename = " << node->block->filename << ", id = " << node->block->id << endl;
    cerr << "----------------------------------------" << endl;
}
#endif