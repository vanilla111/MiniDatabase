#include <cstring>
#include <iostream>

#include "global.h"
#include "struct/table.h"
#include "file/heapFile.h"
#include "utils/utils.h"

#include "main.h"
#include "catalog/catalogManager.h"
#include "record/recordManager.h"

using namespace std;

int RecordManager::select(
        const char *tableName, const vector<string> *colName,
        const vector<int> *cond, const vector<string> *operand,
        vector<char*> *record, vector<int> *ids
) {
    CatalogManager *manager = MiniSQL::getCatalogManager();
    Table *table = manager->getTable(tableName);
    if (table == nullptr)
        return -1;

    HeapFile *file = new HeapFile((RECORD_STORE_DIR + string(tableName)).c_str());
    int recordLength = table->getRecordLength();

    // 遍历记录文件
    int id, hitCount = 0;
    char *dataIn = new char[recordLength];

    while ((id = file->getNextRecord(dataIn)) >= 0) {
        // 检查所有条件
        if (checkRecord(dataIn, tableName, colName, cond, operand)) {
            char *hit = new char[recordLength];
            memcpy(hit, dataIn, recordLength);
            record->push_back(hit);
            ids->push_back(id);
            hitCount++;
        }
    }

    delete[] dataIn;
    delete file;
    return hitCount;
}

int RecordManager::insert(const char *tableName, const char *data) {
    CatalogManager *manager = MiniSQL::getCatalogManager();
    Table *table = manager->getTable(tableName);
    if (table == nullptr) return -1;

    HeapFile *file = new HeapFile((RECORD_STORE_DIR + string(tableName)).c_str());
    // 检查唯一性
    char *exist = new char[table->getRecordLength()];
    while (file->getNextRecord(exist) >= 0) {
        int colId;
        if ((colId = table->checkConsistency(data, exist)) >= 0) {
            // 冲突，放弃插入
            cerr << "ERROR: [RecordManager::insert] Duplicate values in unique column `"
            << table->getColName(colId) << "` of table `" << tableName << "`!" << endl;
            delete[] exist;
            delete file;
            return -1;
        }
    }

    int ret = file->addRecord(data);

    delete[] exist;
    delete file;
    return ret;
}

bool RecordManager::remove(const char *tableName, const vector<int> *ids) {
    HeapFile *file = new HeapFile((RECORD_STORE_DIR + string(tableName)).c_str());
    for (int i = 0; i < (int)ids->size(); i++) {
        file->deleteRecord(ids->at(i));
    }
    delete file;
    return true;
}

bool RecordManager::createTable(const char *tableName) {
    CatalogManager *manager = MiniSQL::getCatalogManager();
    Table *table = manager->getTable(tableName);
    if (table == nullptr)
        return false;
    HeapFile::createFile((RECORD_STORE_DIR + string(tableName)).c_str(), table->getRecordLength());
    return true;
}

bool RecordManager::dropTable(const char *tableName) {
    Utils::deleteFile((RECORD_STORE_DIR + string(tableName)).c_str());
    return true;
}

bool RecordManager::checkRecord(
        const char *record, const char *tableName,
        const vector<string> *colName, const vector<int> *cond,
        const vector<string> *operand){
    CatalogManager *manager = MiniSQL::getCatalogManager();
    Table *table = manager->getTable(tableName);
    if (table == nullptr)
        return false;

    int condCount = colName->size();

    for (int i = 0; i < condCount; i++) {
        char dataOut[MAX_VALUE_LENGTH];
        short type = table->getValue(colName->at(i).c_str(), record, dataOut);

        if (type <= TYPE_CHAR) {
            if (!charCmp(dataOut, operand->at(i).c_str(), cond->at(i)))
                return false;
        } else if (type == TYPE_INT) {
            if (!intCmp(dataOut, operand->at(i).c_str(), cond->at(i)))
                return false;
        } else if (type == TYPE_FLOAT) {
            if (!floatCmp(dataOut, operand->at(i).c_str(), cond->at(i)))
                return false;
        }
    }

    return true;
}

bool RecordManager::intCmp(const char *a, const char *b, int op) {
    const int left = *(reinterpret_cast<const int*>(a));
    int right;
    sscanf(b, "%d", &right);

    if (op == COND_EQ)
        return left == right;
    else if (op == COND_NE)
        return left != right;
    else if (op == COND_LT)
        return left < right;
    else if (op == COND_GT)
        return left > right;
    else if (op == COND_LE)
        return left <= right;
    else if (op == COND_GE)
        return left >= right;
    else
        return false;
}

bool RecordManager::floatCmp(const char *a, const char *b, int op) {
    const float left = *(reinterpret_cast<const float*>(a));
    float right;
    sscanf(b, "%f", &right);

    if (op == COND_EQ)
        return left == right;
    else if (op == COND_NE)
        return left != right;
    else if (op == COND_LT)
        return left < right;
    else if (op == COND_GT)
        return left > right;
    else if (op == COND_LE)
        return left <= right;
    else if (op == COND_GE)
        return left >= right;
    else
        return false;
}

bool RecordManager::charCmp(const char *a, const char *b, int op) {
    if (op == COND_EQ)
        return strcmp(a, b) == 0;
    else if (op == COND_NE)
        return strcmp(a, b) != 0;
    else if (op == COND_LT)
        return strcmp(a, b) < 0;
    else if (op == COND_GT)
        return strcmp(a, b) > 0;
    else if (op == COND_LE)
        return strcmp(a, b) <= 0;
    else if (op == COND_GE)
        return strcmp(a, b) >= 0;
    else
        return false;
}