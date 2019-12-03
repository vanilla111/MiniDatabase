#include <cstring>
#include <iostream>
#include <string>
#include <file/heapFile.h>

#include "global.h"
#include "utils/utils.h"
#include "interpreter/interpreter.h"
#include "index/bPlusTree.h"
#include "server/server.h"
#include "main.h"

using namespace std;

BufferManager* MiniSQL::bufferManager =  nullptr;
CatalogManager* MiniSQL::catalogManager =  nullptr;
RecordManager* MiniSQL::recordManager =  nullptr;
IndexManager* MiniSQL::indexManager =  nullptr;

// 初始化系统
void MiniSQL::init() {
    // 生成工作目录
    if (!(
            Utils::createDirectory(DATA_STORE_DIR) &&
            Utils::createDirectory(DATA_STORE_DIR + string(CATALOG_STORE_DIR)) &&
            Utils::createDirectory(DATA_STORE_DIR + string(INDEX_STORE_DIR)) &&
            Utils::createDirectory(DATA_STORE_DIR + string(RECORD_STORE_DIR))
            )) {
        cerr << "ERROR: [Main::init] Create work directories failed!" << endl;
        exit(10);
    }
    // 检查存放表信息的文件是否存在
    if (!Utils::fileExists("catalog/tables"))
        HeapFile::createFile("catalog/tables", TABLE_RECORD_LENGTH);

    // 检查存放索引信息的文件是否存在
    if (!Utils::fileExists("catalog/indices"))
        HeapFile::createFile("catalog/indices", INDEX_RECORD_LENGTH);

    // Init managers
    bufferManager = new BufferManager();
    catalogManager = new CatalogManager();
    recordManager = new RecordManager();
    indexManager = new IndexManager();
}

void MiniSQL::cleanUp() {
    delete bufferManager;
    delete catalogManager;
    delete recordManager;
    delete indexManager;
}

BufferManager* MiniSQL::getBufferManager() {
    return bufferManager;
}

CatalogManager* MiniSQL::getCatalogManager() {
    return catalogManager;
}

RecordManager* MiniSQL::getRecordManager() {
    return recordManager;
}

IndexManager* MiniSQL::getIndexManager() {
    return indexManager;
}

void MiniSQL::startServer(Interpreter *interpreter) {
    auto server = new Server(interpreter);
    server->start();
}

int main(int argc, char const *argv[]) {
    MiniSQL::init();
    Interpreter *interpreter = new Interpreter();

    if (argc == 1) {
        string sql;
        while (!interpreter->isExiting()) {
            if (interpreter->tokenVecEmpty())
                cout << endl << "minisql> ";
            else
                cout << "    ...> ";

            getline(cin, sql);
            interpreter->execute(sql.c_str());
        }
    } else if (argc == 2) {
        const char *opt = argv[1];
        if (strcmp(opt, "-d") != 0) {
            cerr << "Usage: minisql -d to start server." << endl;
        } else {
            MiniSQL::startServer(interpreter);
        }
    }

    delete interpreter;
    MiniSQL::cleanUp();
    return 0;
}
