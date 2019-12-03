#ifndef WANGSQL_BPLUSTREENODE_H
#define WANGSQL_BPLUSTREENODE_H

#include <vector>
#include <string>

using namespace std;

class BPTreeNode
{
public:
    BPTreeNode(const char *_filename, int _id, int _keyLength);
    BPTreeNode(const char *_filename, int _id, int _keyLength, bool _leaf, int firstPtr);

    ~BPTreeNode();

    // 获取节点大小
    int getSize() const;

    // 获取key长度
    int getKeyLength() const;

    // 判断节点是否叶子节点
    bool isLeaf() const;

    // 获取key
    const char *getKey(int pos) const;

    // 获取指针
    int getPointer(int pos) const;

    // 寻找key的位置
    int findPosition(const char *key) const;

    // 设置key的位置
    void setKey(int pos, const char *key);

    // 在pos处设置指针
    void setPointer(int pos, int ptr);

    // 设置一个块移除
    void setRemoved();

    // 在pos插入 key-pointer
    void insert(int pos, const char *key, int ptr);

    // 移除pos处的 key-pointer
    void remove(int pos);

    // 分隔成两个节点，返回新的哪一个
    BPTreeNode *split(int newId, char *newKey);

    // 从兄弟节点获取一个key，返回新的双亲节点
    const char *borrow(BPTreeNode *sib, bool leftSib, const char *parentKey);

    // 合并右边的兄弟节点
    void mergeRight(BPTreeNode *sib, const char *parentKey);

private:

    string filename;

    // 存放该节点的文件中的Block id
    int id;

    // 节点大小
    int size;

    // key 的长度
    int keyLength;

    // 该节点是否叶子标识
    bool leaf;

    // 该节点是否被修改
    bool dirty;

    // block 是否被移除标识
    bool blockRemoved;

    // 该节点含有的 keys
    vector<char*> keys;

    // 指向其他节点指针
    vector<int> ptrs;

};

#endif //WANGSQL_BPLUSTREENODE_H
