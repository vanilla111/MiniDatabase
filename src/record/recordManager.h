#ifndef WANGSQL_RECORDMANAGER_H
#define WANGSQL_RECORDMANAGER_H

#include <vector>
#include <string>

using namespace std;

class RecordManager
{
public:

    // 从表中搜索符合条件的记录，返回搜索到的记录的条数
    int select(
            const char *tableName, const vector<string> *colName,
            const vector<int> *cond, const vector<string> *operand,
            vector<char*> *record, vector<int> *ids
    );

    // 插入一条新的记录，返回新记录的ID
    int insert(const char *tableName, const char *data);

    // 从表中删除第id条记录，返回是否成功
    bool remove(const char *tableName, const vector<int> *ids);

    // 创建一张表，返回是否成功
    bool createTable(const char *tableName);

    // 删除一张表，返回是否成功
    bool dropTable(const char *tableName);

    // 检查记录是否符合所有条件
    bool checkRecord(
            const char *record, const char *tableName,
            const vector<string> *colName, const vector<int> *cond,
            const vector<string> *operand
    );

private:

    // 比较string类型
    bool charCmp(const char *a, const char *b, int op);

    // 比较int类型
    bool intCmp(const char *a, const char *b, int op);

    // 比较float类型
    bool floatCmp(const char *a, const char *b, int op);
};

#endif //WANGSQL_RECORDMANAGER_H
