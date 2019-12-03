#include <cstdio>
#include <cstring>
#include <iostream>

#include "buffer/bufferManager.h"
#include "main.h"
#include "file/heapFile.h"

using namespace std;

const int HeapFile::FILE_BEGIN = -1;

void HeapFile::createFile(const char *_filename, int _recordLength) {
    _recordLength++;
    FILE *file = fopen((DATA_STORE_DIR + string(_filename) + DATA_FILE_TYPE).c_str(), "wb");
    char data[BLOCK_SIZE] = {0};
    // 第1块只存放描述信息
    memcpy(data, &_recordLength, 4);
    memset(data + 4, 0, 4);
    memset(data + 8, 0xFF, 4);
    fwrite(data, BLOCK_SIZE, 1, file);
    fclose(file);
}

HeapFile::HeapFile(const char *_filename):filename(_filename) {
    BufferManager *bufferManager = MiniSQL::getBufferManager();
    block = bufferManager->getBlock(_filename, 0);
    // 读文件头的描述信息，包括记录长度、记录数量、第一个为空的记录的位置
    recordLength = *(reinterpret_cast<int*>(block->content));
    recordCount = *(reinterpret_cast<int*>(block->content + 4));
    firstEmpty = *(reinterpret_cast<int*>(block->content + 8));

    recordBlockCount = BLOCK_SIZE / recordLength;
    ptr = -1;
}

int HeapFile::getRecordCount() const {
    return recordCount;
}

void HeapFile::moveTo(int id) {
    ptr = id;
}

int HeapFile::getNextRecord(char *data) {
    bool invalid = true;
    do {
        if (ptr + 1 >= recordCount) {
            memset(data, 0, sizeof(char) * (recordLength - 1));
            return -1;
        }
        loadRecord(ptr + 1);
        invalid = *(reinterpret_cast<char*>(block->content + bias + recordLength - 1));
    } while (invalid); // 直到删除标志为0
    memcpy(data, block->content + bias, recordLength - 1);
    return ptr;
}

void HeapFile::loadRecord(int id) {
    BufferManager *bufferManager = MiniSQL::getBufferManager();
    ptr = id;
    // 数据从第2块开始存放
    block = bufferManager->getBlock(filename.c_str(), ptr / recordBlockCount + 1);
    bias = ptr % recordBlockCount * recordLength;
}

const char* HeapFile::getRecordById(int id) {
    if (id >= recordCount)
        return nullptr;

    loadRecord(id);
    bool invalid = *(reinterpret_cast<char*>(block->content + bias + recordLength - 1));
    if (invalid)
        return nullptr;

    return block->content + bias;
}

int HeapFile::addRecord(const char *data) {
    loadRecord(firstEmpty >= 0 ? firstEmpty : recordCount);

    if (firstEmpty >= 0) {
        // 更新第一个为空的位置
        firstEmpty = *(reinterpret_cast<int *>(block->content + bias));
    } else {
        // 更新记录的数量
        recordCount++;
    }

    // 添加数据
    memcpy(block->content + bias, data, recordLength - 1);
    // 置删除标志为0，表示未被删除
    memset(block->content + bias + recordLength - 1, 0, 1);
    // 更改这一块之后即被认为是脏的
    block->dirty = true;

    updateHeader();
    return ptr;
}

bool HeapFile::deleteRecord(int id) {
    if (id >= recordCount) {
        cerr << "ERROR: [HeapFile::deleteRecord] Index out of range!" << endl;
        return false;
    }

    // 检查记录的有效性
    loadRecord(id);
    // 读删除标志，如果是1就表示已经被删除了
    bool invalid = *(reinterpret_cast<char*>(block->content + bias + recordLength - 1));
    if (invalid) {
        cerr << "ERROR: [HeapFile::deleteRecord] Record already deleted!" << endl;
        return false;
    }

    // 删除数据
    memcpy(block->content + bias, &firstEmpty, 4);
    // 置删除标志为1
    memset(block->content + bias + recordLength - 1, 1, 1);
    block->dirty = true;

    firstEmpty = ptr;
    updateHeader();
    return true;
}

void HeapFile::updateHeader() {
    BufferManager *manager = MiniSQL::getBufferManager();
    Block *header = manager->getBlock(filename.c_str(), 0);
    memcpy(header->content + 4, &recordCount, 4);
    memcpy(header->content + 8, &firstEmpty, 4);
    header->dirty = true;
}