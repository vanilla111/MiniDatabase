#ifndef WANGSQL_TABLE_H
#define WANGSQL_TABLE_H

#include <vector>
#include <string>

#include "global.h"

using namespace std;

class Table
{
public:

    Table(const char *data);

    // 获取表名
    const char* getName() const;

    // 获取主键名
    const char* getPrimary() const;

    // 获取列数量
    int getColCount();

    // 获取一条记录的长度
    int getRecordLength();

    // 根据id获取列名
    const char* getColName(int id);

    // 根据列名获取id
    int getId(const char *colName);

    // 根据列名获取其值，返回其类型
    short getValue(const char *colName, const char *dataIn, char *dataOut);

    // 根据列名获取其类型
    short getType(const char *colName);

    // 根据列名判断是否是需要维护唯一性的列
    char getUnique(const char *colName);

    // 检查新数据和已存在数据的一致性（唯一性）
    // 如果是一致的返回-1，否则返回非唯一列的id
    int checkConsistency(const char *data, const char *exist);

    // 将一条记录解析为向量，返回是否成功
    bool recordToVec(const char *data, vector<char*> *vec);

    // 讲一个向量解析为一条记录，返回是否成功
    bool vecToRecord(const vector<string> *vec, char *data);

    // 获取所有的列名
    vector<string> getColNameList();

#ifdef DEBUG
    // 输出表的信息
    void debugPrint();
#endif

private:

    // 表名
    string name;

    // 主键
    string primary;

    // 字段数量
    int colCount;

    // 一条记录的长度
    int recordLength;

    // 所有列的名字组成的向量
    vector<string> colNameList;

    // 列对应的类型组成的向量
    vector<short> colType;

    // 要求唯一的列组成的向量
    vector<char> colUnique;

    // 数据中每一列的起始位置
    vector<int> startPos;

    // 从catalog加载的列信息
    void loadColInfo();
};

#endif //WANGSQL_TABLE_H
