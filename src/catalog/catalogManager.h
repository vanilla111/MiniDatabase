#ifndef WANGSQL_CATALOGMANAGER_H
#define WANGSQL_CATALOGMANAGER_H
#include <vector>
#include <string>
#include <unordered_map>

#include "file/heapFile.h"
#include "struct/table.h"
#include "struct/index.h"

using namespace std;

class CatalogManager
{
public:

    CatalogManager();

    ~CatalogManager();

    // 根据名字获得一张表的指针
    Table *getTable(const char *tableName) const;

    // 创建一张表，返回是否成功
    bool createTable(
            const char *tableName, const char *primary,
            const vector<string> *colName, const vector<short> *colType, vector<char> *colUnique
    );

    // 删除一张表，返回是否成功
    bool dropTable(const char *tableName);

    // 根据索引名获得一个索引的指针
    Index *getIndex(const char *indexName) const;

    // 获取表中所有索引
    void getIndexByTable(const char *tableName, vector<Index*> *vec);

    // 通过表名和字段名获得索引
    Index *getIndexByTableCol(const char *tableName, const char *colName);

    // 创建一个索引
    bool createIndex(const char *indexName, const char *tableName, const char *colName);

    // 删除索引
    bool dropIndex(const char *indexName);

    // 加载表的列信息，返回列的数量
    int loadTableColInfo(
            const char *tableName, vector<string> *colNameData,
            vector<short> *colType, vector<char> *colUnique
    );

#ifdef DEBUG
    // 输出所有表和字段的信息
    void debugPrint() const;
#endif

private:

    // Table map
    unordered_map<string, Table*> tableMap;
    unordered_map<string, int> tableIdMap;

    // Index name map
    unordered_map<string, Index*> indexMap;
    unordered_map<string, int> indexIdMap;

    // Table name 文件
    HeapFile *tableNameFile;

    // Index metadata 文件
    HeapFile *indexMetaFile;
};

#endif //WANGSQL_CATALOGMANAGER_H
