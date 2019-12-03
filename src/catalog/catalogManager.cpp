#include <cstring>
#include <iostream>
#include <unordered_set>

#include "global.h"
#include "utils/utils.h"
#include "catalog/catalogManager.h"

using namespace std;

CatalogManager::CatalogManager() {
    // 载入目录文件
    tableNameFile = new HeapFile(CATALOG_TABLES_FILE);
    indexMetaFile = new HeapFile(CATALOG_INDICES_FILE);

    int id;
    char tableData[TABLE_RECORD_LENGTH];
    char indexData[INDEX_RECORD_LENGTH];
    while ((id = tableNameFile->getNextRecord(tableData)) >= 0) {
        Table *table = new Table(tableData);
        tableIdMap[table->getName()] = id;
        tableMap[table->getName()] = table;
    }
    while ((id = indexMetaFile->getNextRecord(indexData)) >= 0) {
        Index *index = new Index(indexData);
        indexIdMap[index->getName()] = id;
        indexMap[index->getName()] = index;
    }
}

CatalogManager::~CatalogManager() {
    for (auto table : tableMap)
        delete table.second;
    for (auto index : indexMap)
        delete index.second;

    delete tableNameFile;
    delete indexMetaFile;
}

Table *CatalogManager::getTable(const char *tableName) const {
    if (tableMap.find(tableName) == tableMap.end()) {
        cerr << "ERROR: [CatalogManager::getTable] Table `" << tableName << "` does not exist!" << endl;
        return nullptr;
    }
    return tableMap.at(tableName);
}


bool CatalogManager::createTable(
        const char *tableName,
        const char *primary,
        const vector<string> *colName,
        const vector<short> *colType,
        vector<char> *colUnique) {
    if (tableMap.find(tableName) != tableMap.end()) {
        cerr << "ERROR: [CatalogManager::createTable] Table `" << tableName << "` already exists!" << endl;
        return false;
    }

    int colCount = colName->size();

    // 确保每一列的名字都是唯一的
    unordered_set<string> colNameSet;
    for (auto name : *colName) {
        if (colNameSet.find(name) != colNameSet.end()) {
            cerr << "ERROR: [CatalogManager::createTable] Duplicate column name `" << name << "`!" << endl;
            return false;
        }
        colNameSet.insert(name);
    }

    // 确保主键在这些列中
    if (colNameSet.find(primary) == colNameSet.end()) {
        cerr << "ERROR: [CatalogManager::createTable] Cannot find primary key name `" << primary << "` in column names!" << endl;
        return false;
    }

    // colUnique 指定哪些列的值需要满足唯一性
    // 主键的值必须是唯一的，所以将对应位置置为1
    for (int i = 0; i < colCount; i++) {
        if (primary == colName->at(i)) {
            colUnique->at(i) = 1;
        }
    }

    // 表的名字、主键写入目录文件
    char tableData[MAX_NAME_LENGTH * 2] = {0};
    memcpy(tableData, tableName, MAX_NAME_LENGTH);
    memcpy(tableData + MAX_NAME_LENGTH, primary, MAX_NAME_LENGTH);
    int id = tableNameFile->addRecord(tableData);

    // 表的记录加入 map
    tableIdMap[tableName] = id;
    tableMap[tableName] = new Table(tableData);

    // 初始化存储表的列信息的文件
    HeapFile::createFile((CATALOG_RECORD_FILE_PREFIX + string(tableName)).c_str(), MAX_NAME_LENGTH + 3);
    auto *tableDataFile = new HeapFile((CATALOG_RECORD_FILE_PREFIX + string(tableName)).c_str());

    for (int i = 0; i < colCount; i++) {
        char colData[MAX_NAME_LENGTH + 3] = {0};
        // 前 MAX_NAME_LENGTH 记录列名，之后2个字节记录类型，最后1个字节记录是否需要满足唯一性
        memcpy(colData, colName->at(i).c_str(), MAX_NAME_LENGTH);
        memcpy(colData + MAX_NAME_LENGTH, &(colType->at(i)), 2);
        memcpy(colData + MAX_NAME_LENGTH + 2, &(colUnique->at(i)), 1);
        tableDataFile->addRecord(colData);
    }

    return true;
}

bool CatalogManager::dropTable(const char *tableName) {
    if (getTable(tableName) == nullptr) return false;

    // 删除表
    delete tableMap[tableName];
    tableNameFile->deleteRecord(tableIdMap[tableName]);
    tableIdMap.erase(tableName);
    tableMap.erase(tableName);

    // 删除文件与内存块中的对应的记录
    Utils::deleteFile((CATALOG_RECORD_FILE_PREFIX + string(tableName)).c_str());

    return true;
}

Index *CatalogManager::getIndex(const char *indexName) const {
    if (indexMap.find(indexName) == indexMap.end()) {
        cerr << "ERROR: [CatalogManager::getIndex] Index `" << indexName << "` does not exist!" << endl;
        return nullptr;
    }
    return indexMap.at(indexName);
}

bool CatalogManager::createIndex(const char *indexName, const char *tableName, const char *colName) {
    if (getTable(tableName) == nullptr) return false;
    if (indexMap.find(indexName) != indexMap.end()) {
        cerr << "ERROR: [CatalogManager::createIndex] Index `" << indexName << "` already exists!" << endl;
        return false;
    }

    // 检查该列是否存在，并且有唯一性要求
    Table *table = tableMap[tableName];
    char isUnique = table->getUnique(colName);
    if (isUnique == 0) {
        cerr << "ERROR: [CatalogManager::createIndex] Column `" << colName << "` is not unique!" << endl;
    }
    if (isUnique != 1) {
        return false;
    }

    // 检查该表中同一列是否已经指定成为了索引
    Index *exist = getIndexByTableCol(tableName, colName);
    if (exist != nullptr) {
        cerr << "ERROR: [CatalogManager::createIndex] Index with table name `" << tableName << "` and column name `" << colName << "` already exists(Index name `" << exist->getName() << "`)!" << endl;
        return false;
    }

    // 索引信息写入文件，索引名、表名、列名
    char indexData[MAX_NAME_LENGTH * 3];
    memcpy(indexData, indexName, MAX_NAME_LENGTH);
    memcpy(indexData + MAX_NAME_LENGTH, tableName, MAX_NAME_LENGTH);
    memcpy(indexData + MAX_NAME_LENGTH * 2, colName, MAX_NAME_LENGTH);
    int id = indexMetaFile->addRecord(indexData);

    indexIdMap[indexName] = id;
    indexMap[indexName] = new Index(indexData);

    return true;
}

void CatalogManager::getIndexByTable(const char *tableName, vector<Index*> *vec) {
    for (auto item : indexMap) {
        Index *index = item.second;
        if (strcmp(index->getTableName(), tableName) == 0)
            vec->push_back(index);
    }
}

Index *CatalogManager::getIndexByTableCol(const char *tableName, const char *colName) {
    for (auto item : indexMap) {
        Index *index = item.second;
        if (strcmp(index->getTableName(), tableName) == 0 && strcmp(index->getColName(), colName) == 0)
            return index;
    }
    return nullptr;
}

bool CatalogManager::dropIndex(const char *indexName) {
    if (indexMap.find(indexName) == indexMap.end()) {
        cerr << "ERROR: [CatalogManager::dropIndex] Index `" << indexName << "` does not exist!" << endl;
        return false;
    }

    delete indexMap[indexName];
    indexMetaFile->deleteRecord(indexIdMap[indexName]);
    indexIdMap.erase(indexName);
    indexMap.erase(indexName);

    return true;
}

int CatalogManager::loadTableColInfo(
        const char *tableName, vector<string> *colName,
        vector<short> *colType, vector<char> *colUnique) {
    HeapFile *colFile = new HeapFile((CATALOG_RECORD_FILE_PREFIX + string(tableName)).c_str());

    int colCount = colFile->getRecordCount();
    char colData[MAX_NAME_LENGTH + 3];
    for (int i = 0; i < colCount; i++) {
        colFile->getNextRecord(colData);

        char nameData[MAX_NAME_LENGTH];
        memcpy(nameData, colData, MAX_NAME_LENGTH);
        colName->push_back(nameData);

        short type = *(reinterpret_cast<short*>(colData + MAX_NAME_LENGTH));
        colType->push_back(type);

        char unique = colData[MAX_NAME_LENGTH + 2];
        colUnique->push_back(unique);
    }

    delete colFile;
    return colCount;
}

#ifdef DEBUG
// 输出所有表和索引的信息
void CatalogManager::debugPrint() const {
    cerr << "DEBUG: [CatalogManager::debugPrint] debugPrint begin" << endl;
    for (auto table : tableMap)
        table.second->debugPrint();
    for (auto index : indexMap)
        index.second->debugPrint();
    cerr << "DEBUG: [CatalogManager::debugPrint] debugPrint end" << endl;
}
#endif