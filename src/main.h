#ifndef WANGSQL_MAIN_H
#define WANGSQL_MAIN_H

#include "buffer/bufferManager.h"
#include "catalog/catalogManager.h"
#include "record/recordManager.h"
#include "index/indexManager.h"
#include "interpreter/interpreter.h"

class MiniSQL
{
public:

    // 初始化系统
    static void init();

    // 清理 managers
    static void cleanUp();

    static BufferManager* getBufferManager();

    static CatalogManager* getCatalogManager();

    static RecordManager* getRecordManager();

    static IndexManager* getIndexManager();

    static void startServer(Interpreter *interpreter);

private:

    static BufferManager* bufferManager;

    static CatalogManager* catalogManager;

    static RecordManager* recordManager;

    static IndexManager* indexManager;
};

#endif //WANGSQL_MAIN_H
