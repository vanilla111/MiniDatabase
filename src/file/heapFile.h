#ifndef WANGSQL_HEAPFILE_H
#define WANGSQL_HEAPFILE_H

#include <string>
#include "global.h"

using namespace std;

class HeapFile
{
public:

    // 文件初始指示器
    static const int FILE_BEGIN;

    // 创建heap file
    static void createFile(const char *_filename, int _recordLength);

    HeapFile(const char *_filename);

    int getRecordCount() const;

    // 读下一个记录，返回该记录的ID
    int getNextRecord(char *data);

    // 找第id的一个记录，返回该记录在内存中的起始位置
    const char *getRecordById(int id);

    // 增加一个记录到文件，返回该记录的id
    int addRecord(const char *data);

    // 删除第id的一个记录，返回成功与否
    bool deleteRecord(int id);

    // 移动指针到第id个记录
    void moveTo(int id);

private:

    string filename;

    // 记录长度，即数据表所有属性类型的长度之和
    int recordLength;

    // 记录个数
    int recordCount;

    // 第一个为空的记录
    int firstEmpty;

    // 一个块中记录的个数
    int recordBlockCount;

    // 指向当前数据块的指针
    Block *block;

    // 记录当前记录的指针
    int ptr;

    // 块内偏移
    int bias;

    // 更新文件的头
    void updateHeader();

    // 加载第id个记录到一个块
    void loadRecord(int id);
};

#endif //WANGSQL_HEAPFILE_H
