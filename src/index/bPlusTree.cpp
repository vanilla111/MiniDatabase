#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

#include "global.h"
#include "main.h"
#include "buffer/bufferManager.h"
#include "index/bPlusTreeNode.h"
#include "index/bPlusTree.h"

using namespace std;

const int BPTree::BPTREE_FAILED = -1;
const int BPTree::BPTREE_NORMAL = 0;
const int BPTree::BPTREE_ADD = 1;
const int BPTree::BPTREE_REMOVE = 2;
const int BPTree::BPTREE_CHANGE = 3;

// 创建 B+ 树的文件
void BPTree::createFile(const char *_filename, int _keyLength, int _order) {
    // 若不指定一个节点存放键值的容量，就自行计算一个
    if (_order < 0)
        _order = (BLOCK_SIZE - 8) / (_keyLength + 4) + 1;

    FILE *file = fopen((DATA_STORE_DIR + string(_filename) + DATA_FILE_TYPE).c_str(), "wb");
    int header[] = {_order, _keyLength, 0, -1, -1};
    fwrite(header, 4, 5, file);
    fclose(file);
}

BPTree::BPTree(const char *_filename): filename(_filename) {
    BufferManager *manager = MiniSQL::getBufferManager();
    Block *header = manager->getBlock(_filename, 0);

    // 文件头的信息
    order = *(reinterpret_cast<int*>(header->content));
    keyLength = *(reinterpret_cast<int*>(header->content + 4));
    nodeCount = *(reinterpret_cast<int*>(header->content + 8));
    root = *(reinterpret_cast<int*>(header->content + 12));
    firstEmpty = *(reinterpret_cast<int*>(header->content + 16));

    key = new char[keyLength];
}

BPTree::~BPTree() {
    delete[] key;
}

int BPTree::find(const char *_key) {
    memcpy(key, _key, keyLength);
    return root < 0 ? BPTREE_FAILED : find(root);
}

bool BPTree::add(const char *_key, int _value) {
    memcpy(key, _key, keyLength);
    value = _value;
    int res = root < 0 ? BPTREE_ADD : add(root);

    if (res == BPTREE_ADD) {
        // 生成新的根
        int newRoot = getFirstEmpty();
        BPTreeNode *node = new BPTreeNode(filename.c_str(), newRoot, keyLength, root < 0, root < 0 ? -1 : root);
        node->insert(0, key, value);
        delete node;
        root = newRoot;
    }
    updateHeader();

    return res != BPTREE_FAILED;
}

bool BPTree::remove(const char *_key) {
    memcpy(key, _key, keyLength);
    int res = root < 0 ? false : remove(root, 0, true, nullptr);
    updateHeader();
    return res != BPTREE_FAILED;
}

#ifdef DEBUG
void BPTree::debugPrint() {
    cerr << "DEBUG: [BPTree::debugPrint] Debug print start." << endl;
    cerr << "Node number = " << nodeCount << ", first empty = " << firstEmpty << endl;
    if (root >= 0) {
        cerr << "Root = " << root << endl;
        debugPrint(root);
    }
    else
        cerr << "Empty tree." << endl;
    cerr << "DEBUG: [BPTree::debugPrint] Debug print end." << endl;
}
#endif

// 递归查找值
int BPTree::find(int id) {
    BPTreeNode *node = new BPTreeNode(filename.c_str(), id, keyLength);
    int pos = node->findPosition(key);

    int ret = BPTREE_FAILED;
    if (node->isLeaf()) {
        // 如果发现页节点
        if (pos > 0) {
            const char *k = node->getKey(pos);
            if (memcmp(key, k, keyLength) == 0)
                ret = node->getPointer(pos);
        }
    } else
        ret = find(node->getPointer(pos));

    delete node;
    return ret;
}

// 递归地将key-value对存放到合适的位置
int BPTree::add(int id) {
    BPTreeNode *node = new BPTreeNode(filename.c_str(), id, keyLength);
    int pos = node->findPosition(key);

    int res = node->isLeaf() ? BPTREE_ADD : add(node->getPointer(pos));
    int ret = BPTREE_NORMAL;

    if (node->isLeaf() && pos > 0) {
        // 检查是否重复
        const char* k = node->getKey(pos);
        if (memcmp(key, k, keyLength) == 0)
            res = BPTREE_FAILED;
    }

    if (res == BPTREE_FAILED) {
        // 重复
        ret = BPTREE_FAILED;
    } else if (res == BPTREE_ADD) {
        // 添加 key-value
        node->insert(pos, key, value);
        if (node->getSize() >= order) {
            // 节点存放键值对已满，拆分成两个
            int newId = getFirstEmpty();
            BPTreeNode *newNode = node->split(newId, key);
            value = newId;

            delete newNode;
            ret = BPTREE_ADD;
        }
    }

    delete node;
    return ret;
}

// 递归地将指定键值对删除
int BPTree::remove(int id, int sibId, bool leftSib, const char *parentKey) {
    BPTreeNode *node = new BPTreeNode(filename.c_str(), id, keyLength);
    BPTreeNode *sib = nullptr;
    if (id != root)
        sib = new BPTreeNode(filename.c_str(), sibId, keyLength);
    int pos = node->findPosition(key);

    int res;
    if (node->isLeaf())
        res = BPTREE_FAILED;
    else {
        int nxtId = node->getPointer(pos);
        int nxtSib = node->getPointer(pos > 0 ? pos - 1 : pos + 1);
        const char *nxtParentKey = node->getKey(pos > 0 ? pos : pos + 1);
        res = remove(nxtId, nxtSib, pos > 0, nxtParentKey);
    }

    if (node->isLeaf()) {
        // 如果找到
        if (pos > 0) {
            const char *k = node->getKey(pos);
            if (memcmp(key, k, keyLength) == 0)
                res = BPTREE_REMOVE;
        }
    }

    int ret = BPTREE_NORMAL;
    if (res == BPTREE_FAILED)
        // 未找到key
        ret = BPTREE_FAILED;
    else if (res == BPTREE_CHANGE)
        // 更改key
        node->setKey(pos > 0 ? pos : pos + 1, key);
    else if (res == BPTREE_REMOVE) {
        // 删除key
        node->remove(pos > 0 ? pos : pos + 1);

        if (id == root) {
            if (node->getSize() == 0) {
                root = node->getPointer(0);
                removeBlock(id);
                node->setRemoved();
            }
        } else {
            int lim = (order + 1) / 2 - 1;
            if (node->getSize() < lim) {
                if (sib->getSize() > lim) {
                    // 向兄弟节点借一个键值对
                    const char *k = node->borrow(sib, leftSib, parentKey);
                    memcpy(key, k, keyLength);
                    ret = BPTREE_CHANGE;
                } else {
                    // 合并两个节点
                    if (leftSib) {
                        sib->mergeRight(node, parentKey);
                        removeBlock(id);
                        node->setRemoved();
                    } else {
                        node->mergeRight(sib, parentKey);
                        removeBlock(sibId);
                        sib->setRemoved();
                    }
                    ret = BPTREE_REMOVE;
                }
            }
        }
    }

    delete node;
    if (sib != nullptr)
        delete sib;
    return ret;
}

int BPTree::getFirstEmpty() {
    if (firstEmpty < 0)
        return ++nodeCount;

    int ret = firstEmpty;
    BufferManager *manager = MiniSQL::getBufferManager();
    Block *block = manager->getBlock(filename.c_str(), firstEmpty);
    firstEmpty = *(reinterpret_cast<int*>(block->content));
    return ret;
}

void BPTree::removeBlock(int id) {
    BufferManager *manager = MiniSQL::getBufferManager();
    Block *block = manager->getBlock(filename.c_str(), id);
    memcpy(block->content, &firstEmpty, 4);
    firstEmpty = id;
}

void BPTree::updateHeader() {
    BufferManager *manager = MiniSQL::getBufferManager();
    Block *block = manager->getBlock(filename.c_str(), 0);

    memcpy(block->content + 8, &nodeCount, 4);
    memcpy(block->content + 12, &root, 4);
    memcpy(block->content + 16, &firstEmpty, 4);

    block->dirty = true;
}

#ifdef DEBUG
void BPTree::debugPrint(int id) {
    BPTreeNode *node = new BPTreeNode(filename.c_str(), id, keyLength);

    cerr << "Block id = " << id << ", isLeaf = " << node->isLeaf() << endl;
    cerr << "Keys:";
    for (int i = 1; i <= node->getSize(); i++) {
        cerr << " ";
        const char *k = node->getKey(i);
        for (int j = 0; j < keyLength; j++) {
            cerr << (int)k[j];
            if (j < keyLength - 1)
                cerr << "~";
        }
    }
    cerr << endl;
    cerr << "Pointers: ";
    for (int i = 0; i <= node->getSize(); i++)
        cerr << " " << node->getPointer(i);
    cerr << endl;

    if (!node->isLeaf())
        for (int i = 0; i <= node->getSize(); i++)
            debugPrint(node->getPointer(i));

    delete node;
}
#endif
