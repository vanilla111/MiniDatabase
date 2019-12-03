#include <iostream>
#include <string>

#include "struct/index.h"
#include "struct/table.h"
#include "index/bPlusTree.h"
#include "utils/utils.h"

#include "main.h"
#include "catalog/catalogManager.h"
#include "index/indexManager.h"

int IndexManager::find(const char *indexName, const char *key) {
    BPTree *tree = new BPTree((INDEX_STORE_DIR + string(indexName)).c_str());
    int ret = tree->find(key);
    delete tree;
    return ret;
}

bool IndexManager::insert(const char *indexName, const char *key, int value) {
    BPTree *tree = new BPTree((INDEX_STORE_DIR + string(indexName)).c_str());
    if (!tree->add(key, value)) {
        cerr << "ERROR: [IndexManager::insert] Duplicate key in index `" << indexName << "`." << endl;
        delete tree;
        return false;
    }
    delete tree;
    return true;
}

bool IndexManager::remove(const char *indexName, const char *key) {
    BPTree *tree = new BPTree((INDEX_STORE_DIR + string(indexName)).c_str());
    if (!tree->remove(key)) {
        cerr << "ERROR: [IndexManager::remove] Cannot find key in index `" << indexName << "`." << endl;
        delete tree;
        return false;
    }
    delete tree;
    return true;
}

bool IndexManager::createIndex(const char *indexName) {
    CatalogManager *manager = MiniSQL::getCatalogManager();
    Index *index = manager->getIndex(indexName);
    if (index == nullptr)
        return false;
    Table *table = manager->getTable(index->getTableName());
    if (table == nullptr)
        return false;
    int keyLength = Utils::getTypeSize(table->getType(index->getColName()));

    BPTree::createFile((INDEX_STORE_DIR + string(indexName)).c_str(), keyLength);
    return true;
}

bool IndexManager::dropIndex(const char *indexName) {
    Utils::deleteFile((INDEX_STORE_DIR + string(indexName)).c_str());
    return true;
}