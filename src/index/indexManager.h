#ifndef WANGSQL_INDEXMANAGER_H
#define WANGSQL_INDEXMANAGER_H

class IndexManager
{
public:

    // 在索引中寻找 key，返回记录的id
    int find(const char *indexName, const char *key);

    // 插入 key 到索引中
    bool insert(const char *indexName, const char *key, int value);

    // 删除 key 在索引中
    bool remove(const char *indexName, const char *key);

    // 创建索引
    bool createIndex(const char *indexName);

    // 删除索引
    bool dropIndex(const char *indexName);
};

#endif //WANGSQL_INDEXMANAGER_H
