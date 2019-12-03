#ifndef WANGSQL_BPLUSTREE_H
#define WANGSQL_BPLUSTREE_H

class BPTree
{
public:

    // 创建B+树索引文件
    static void createFile(const char *_filename, int _keyLength, int _order = -1);

    BPTree(const char *_filename);

    ~BPTree();

    // 根据key找值
    int find(const char *_key);

    // 添加一个key-value对
    bool add(const char *_key, int _value);

    // 删除一个kye-value对
    bool remove(const char *_key);

#ifdef DEBUG
    // 输出树的结构
    void debugPrint();
#endif

private:

    static const int BPTREE_FAILED;
    static const int BPTREE_NORMAL;
    static const int BPTREE_ADD;
    static const int BPTREE_REMOVE;
    static const int BPTREE_CHANGE;

    // 排序
    int order;

    // key的长度
    int keyLength;

    // 节点数量
    int nodeCount;

    // 根节点的block id
    int root;

    // 第一个空的块
    int firstEmpty;

    // 文件名
    string filename;

    // 持有Key-value
    char *key;
    int value;

    // 以下函数递归完成相应功能
    int find(int id);

    int add(int id);

    int remove(int id, int sibId, bool leftSib, const char *parentKey);

    int getFirstEmpty();

    void removeBlock(int id);

    void updateHeader();

#ifdef DEBUG
    // 递归函数的结构树
    void debugPrint(int id);
#endif
};

#endif //WANGSQL_BPLUSTREE_H
