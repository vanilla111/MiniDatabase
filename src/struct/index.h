#ifndef WANGSQL_INDEX_H
#define WANGSQL_INDEX_H

#include <string>

using namespace std;

class Index
{
public:

    Index(const char *data);

    const char* getName() const;

    const char* getTableName() const;

    const char* getColName() const;

#ifdef DEBUG
    void debugPrint() const;
#endif

private:

    // 索引名
    string name;

    // 表名
    string tableName;

    // 字段名
    string colName;
};

#endif //WANGSQL_INDEX_H
