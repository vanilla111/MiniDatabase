#include <cstring>
#include <ctime>
#include <iostream>

#include "global.h"
#include "struct/table.h"
#include "file/heapFile.h"
#include "utils/utils.h"

#include "main.h"
#include "catalog/catalogManager.h"
#include "record/recordManager.h"
#include "api/api.h"

using namespace std;

int Api::select(
        const char *tableName, const vector<string> *colName, const vector<string> *selectColName,
        const vector<int> *cond, const vector<string> *operand, int limit) {
    CatalogManager *catalogManager = MiniSQL::getCatalogManager();
    Table *table = catalogManager->getTable(tableName);
    if (table == nullptr)
        return -1;

    int condCount = (int)cond->size();

    // 检查所有条件都是有效的
    for (int i = 0; i < condCount; i++) {
        if (
                cond->at(i) != COND_EQ &&
                cond->at(i) != COND_NE &&
                cond->at(i) != COND_LT &&
                cond->at(i) != COND_GT &&
                cond->at(i) != COND_LE &&
                cond->at(i) != COND_GE) {
            cerr << "ERROR: [Api::select] Unknown condition `" << cond->at(i) << "`!" << endl;
            return -1;
        }
    }

    // 检查指定的列名是否存在
    for (int i = 0; i < condCount; i++) {
        short type = table->getType(colName->at(i).c_str());
        if (type == TYPE_NULL)
            return -1;
    }

    // 获取结果
    vector<char*> record;
    vector<int> _;

    int colCount = table->getColCount();
    int isSelect[colCount];
    if (selectColName->size() == 0) {
        for (int i = 0; i < colCount; ++i) {
            isSelect[i] = 1;
        }
    } else {
        for (int i = 0; i < colCount; ++i) {
            isSelect[i] = 0;
        }
        for (string selectName : *selectColName) {
            int colId = table->getId(selectName.c_str());
            if (colId == -1) {
                cerr << "ERROR: [API::select] Column `" << selectName << "` does not exist!" << endl;
                return -1;
            } else {
                isSelect[colId] = 1;
            }
        }
    }

    int selectCount = filter(tableName, colName, cond, operand, &record, &_);
    if (selectCount < 0)
        return -1;

    // 打印列名
    cout << endl;
    for (int i = 0; i < colCount; i++) {
        if (isSelect[i] == 1) {
            string colName = string(table->getColName(i));
            cout << colName << "\t";
        }
    }
    cout << endl << "----------------------------------------" << endl;

    // 打印搜出的每一行记录
    vector<char*> parsed;
    int j = 0;
    for (auto data : record) {
        table->recordToVec(data, &parsed);
        for (int i = 0; i < colCount; i++) {
            if (isSelect[i] == 1) {
                string colName = string(table->getColName(i));
                short type = table->getType(colName.c_str());

                if (type <= TYPE_CHAR)
                    cout << parsed[i] << "\t";
                else if (type == TYPE_INT)
                    cout << *(reinterpret_cast<int*>(parsed[i])) << "\t";
                else if (type == TYPE_FLOAT)
                    cout << *(reinterpret_cast<float*>(parsed[i])) << "\t";
            }
        }
        cout << endl;

        // 清理已打印的记录
        for (int i = 0; i < colCount; i++)
            delete[] parsed[i];
        parsed.clear();
        if (j >= limit - 1 && limit > 0) break;
        j++;
    }
    cout << endl;

    return selectCount;
}

bool Api::insert(const char *tableName, const vector<string> *value) {
    CatalogManager *catalogManager = MiniSQL::getCatalogManager();
    RecordManager *recordManager = MiniSQL::getRecordManager();
    IndexManager *indexManager = MiniSQL::getIndexManager();

    Table *table = catalogManager->getTable(tableName);
    if (table == nullptr)
        return false;

    // 解析数据
    char *data = new char[table->getRecordLength()];
    if (!table->vecToRecord(value, data)) {
        // 不符合格式，或者不满足唯一性要求
        delete[] data;
        return false;
    }

    // 执行插入
    int res = recordManager->insert(tableName, data);
    if (res < 0) {
        // 插入失败
        delete[] data;
        return false;
    }

    // 更新索引信息
    vector<char*> vec;
    table->recordToVec(data, &vec);
    vector<Index*> indices;
    catalogManager->getIndexByTable(tableName, &indices);

    for (auto index : indices) {
        indexManager->insert(
                index->getName(), vec[table->getId(index->getColName())], res
        );
    }

    for (auto t : vec)
        delete[] t;
    delete[] data;
    return true;
}

int Api::remove(
        const char *tableName, const vector<string> *colName,
        const vector<int> *cond, const vector<string> *operand){
    CatalogManager *catalogManager = MiniSQL::getCatalogManager();
    RecordManager *recordManager = MiniSQL::getRecordManager();
    IndexManager *indexManager = MiniSQL::getIndexManager();

    Table *table = catalogManager->getTable(tableName);
    if (table == nullptr)
        return false;

    vector<char*> record;
    vector<int> ids;

    int removeCount = filter(
            tableName, colName, cond, operand, &record, &ids
    );
    if (removeCount < 0)
        return -1;

    // 执行删除
    bool res = recordManager->remove(tableName, &ids);
    if (!res)
        return -1;

    // 更新索引信息
    vector<char*> vec;
    vector<Index*> indices;
    catalogManager->getIndexByTable(tableName, &indices);

    for (auto data : record) {
        vec.clear();
        table->recordToVec(data, &vec);

        for (auto index : indices) {
            indexManager->remove(
                    index->getName(), vec[table->getId(index->getColName())]
            );
        }

        for (auto t : vec)
            delete[] t;
    }

    return removeCount;
}

bool Api::createTable(
        const char *tableName, const char *primary,
        const vector<string> *colName, const vector<short> *colType, vector<char> *colUnique
) {
    CatalogManager *catalogManager = MiniSQL::getCatalogManager();
    RecordManager *recordManager = MiniSQL::getRecordManager();

    if (catalogManager->createTable(tableName, primary, colName, colType, colUnique)) {
        recordManager->createTable(tableName);
        createIndex(to_string(time(0)).c_str(), tableName, primary);
        return true;
    }
    else
        return false;
}

bool Api::dropTable(const char *tableName)
{
    CatalogManager *catalogManager = MiniSQL::getCatalogManager();
    RecordManager *recordManager = MiniSQL::getRecordManager();

    if (catalogManager->dropTable(tableName)) {
        recordManager->dropTable(tableName);
        // 删除所有相关的索引
        vector<Index*> indices;
        catalogManager->getIndexByTable(tableName, &indices);
        for (auto index : indices)
            dropIndex(index->getName());

        return true;
    }
    else
        return false;
}

bool Api::createIndex(const char *indexName, const char *tableName, const char *colName) {
    CatalogManager *catalogManager = MiniSQL::getCatalogManager();
    IndexManager *indexManager = MiniSQL::getIndexManager();

    if (catalogManager->createIndex(indexName, tableName, colName)) {
        indexManager->createIndex(indexName);

        HeapFile *file = new HeapFile((RECORD_STORE_DIR + string(tableName)).c_str());
        Table *table = catalogManager->getTable(tableName);
        char *data = new char[table->getRecordLength()];
        int id;
        while ((id = file->getNextRecord(data)) >= 0) {
            char dataOut[MAX_VALUE_LENGTH];
            table->getValue(colName, data, dataOut);
            indexManager->insert(indexName, dataOut, id);
        }

        return true;
    }
    else
        return false;
}

bool Api::dropIndex(const char *indexName) {
    CatalogManager *catalogManager = MiniSQL::getCatalogManager();
    IndexManager *indexManager = MiniSQL::getIndexManager();

    if (catalogManager->dropIndex(indexName)) {
        indexManager->dropIndex(indexName);
        return true;
    }
    else
        return false;
}

int Api::filter(
        const char *tableName, const vector<string> *colName,
        const vector<int> *cond, const vector<string> *operand,
        vector<char*> *record, vector<int> *ids) {
    CatalogManager *catalogManager = MiniSQL::getCatalogManager();
    RecordManager *recordManager = MiniSQL::getRecordManager();
    IndexManager *indexManager = MiniSQL::getIndexManager();

    Table *table = catalogManager->getTable(tableName);
    int condCount = (int)cond->size();

    // 尝试使用索引过滤
    for (int i = 0; i < condCount; i++) {
        if (cond->at(i) != COND_EQ)
            continue;
        Index *index = catalogManager->getIndexByTableCol(
                tableName, colName->at(i).c_str());
        if (index == nullptr)
            continue;

        short type = table->getType(colName->at(i).c_str());
        char *key = Utils::getDataFromStr(operand->at(i).c_str(), type);
        if (key == nullptr)
            return 0;
        int id = indexManager->find(index->getName(), key);

        int ret;
        if (id < 0)
            ret = 0;
        else {
            // 记录已找到，检查其他条件
            HeapFile *file = new HeapFile((RECORD_STORE_DIR + string(tableName)).c_str());
            const char *data = file->getRecordById(id);
            delete file;

            if (recordManager->checkRecord(
                    data, tableName, colName, cond, operand)) {
                int recordLength = table->getRecordLength();
                char *hit = new char[recordLength];
                memcpy(hit, data, recordLength);

                record->push_back(hit);
                ids->push_back(id);
                ret = 1;
            } else
                ret = 0;
        }

        delete[] key;
        return ret;
    }

    // 使用暴力方法
    return recordManager->select(
            tableName, colName, cond, operand, record, ids
    );
}