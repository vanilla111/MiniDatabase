#ifndef WANGSQL_API_H
#define WANGSQL_API_H
#include <string>
#include <vector>

using namespace std;

class Api
{
public:

    // 搜索记录，返回搜索到的记录数量
    int select(
            const char *tableName, const vector<string> *colName, const vector<string> *selectColName,
            const vector<int> *cond, const vector<string> *operand, int limit
    );

    // 插入一条记录
    bool insert(const char* tableName, const vector<string>* value);

    // 删除记录，返回删除记录的数量
    int remove(
            const char *tableName, const vector<string> *colName,
            const vector<int> *cond, const vector<string> *operand
    );

    // 创建表
    bool createTable(
            const char *tableName, const char *primary,
            const vector<string> *colName, const vector<short> *colType, vector<char> *colUnique
    );

    // 删除表
    bool dropTable(const char *tableName);

    // 创建索引
    bool createIndex(const char *indexName, const char *tableName, const char *colName);

    // 删除索引
    bool dropIndex(const char *indexName);

private:

    // 条件过滤器，返回满足条件的记录的数量
    int filter(
            const char *tableName, const vector<string> *colName,
            const vector<int> *cond, const vector<string> *operand,
            vector<char*> *record, vector<int> *ids
    );
};

#endif //WANGSQL_API_H
