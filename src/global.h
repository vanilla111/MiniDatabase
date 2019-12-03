#ifndef WANGSQL_GLOBAL_H
#define WANGSQL_GLOBAL_H

#include <string>

using namespace std;

#define DEBUG

#define SERVER_IP "192.168.33.1"
#define SERVER_PORT 8000

// 块大小 默认 1MB
#define BLOCK_SIZE 4096
// 缓冲区存放块数量限制
#define MAX_BLOCK 102400

// 一个记录中，一个值的最大长度
#define MAX_VALUE_LENGTH 256

// 命名最大长度
#define MAX_NAME_LENGTH 31

// 数据类型
#define TYPE_NULL 0
#define TYPE_CHAR 255
#define TYPE_INT 256
#define TYPE_FLOAT 257

// 数据库存放文件名,数据文件后缀
#define DATA_STORE_DIR "data/"
#define DATA_FILE_TYPE ".mdb"
#define CATALOG_TABLES_FILE "catalog/tables"
#define CATALOG_INDICES_FILE "catalog/indices"
#define CATALOG_RECORD_FILE_PREFIX "catalog/table_"
#define RECORD_STORE_DIR "record/"
#define INDEX_STORE_DIR "index/"
#define CATALOG_STORE_DIR "catalog/"

// 表目录记录长度 记录表名、主键
#define TABLE_RECORD_LENGTH (MAX_NAME_LENGTH * 2)
// 索目录引记录长度 记录 索引名、表名、列名
#define INDEX_RECORD_LENGTH (MAX_NAME_LENGTH * 3)
//

// 数值判断条件
#define COND_EQ 0
#define COND_NE 1
#define COND_LT 2
#define COND_GT 3
#define COND_LE 4
#define COND_GE 5

struct Block
{
    string filename;
    int id;

    bool dirty;
    bool pin;

    char content[BLOCK_SIZE];

    Block(const char *_filename, int _id): filename(_filename), id(_id)
    {
        dirty = false;
        pin = false;
    }
};

#endif //WANGSQL_GLOBAL_H
