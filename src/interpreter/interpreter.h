#ifndef WANGSQL_INTERPRETER_H
#define WANGSQL_INTERPRETER_H

#include <vector>
#include <string>

#include "interpreter/tokenizer.h"
#include "api/api.h"

using namespace std;

class Interpreter
{
public:

    Interpreter(bool _fromFile = false);

    ~Interpreter();

    // 获取已处理的请求的数量
    int getQueryCount() const;

    bool isExiting() const;

    // 向量中的token是否都已经指向
    bool tokenVecEmpty() const;

    // SQL的解析与执行
    void execute(const char *sql);

    void execute(const char *sql, const char *file);

private:

    // Token vector
    vector<string> tokens;

    // Token type vector
    vector<int> type;

    // 当前token所在下标
    int ptr;

    // 已处理请求数量
    int queryCount;

    // 用户活跃标志
    bool exiting;

    // 是否正在使用文件解析SQL语句
    bool fromFile;

    // Tokenizer
    Tokenizer* tokenizer;

    // Api
    Api* api;

    void select();

    void insert();

    void remove();

    // where
    bool where(vector<string> *colName, vector<int> *cond, vector<string> *operand);

    // create table/index
    void create();

    // create table
    void createTable();

    // create index
    void createIndex();

    // drop table/index
    void drop();

    // execfile
    void execfile();

    // 退出
    void exit();

    // 获取操作类型
    static int getOperatorType(const char *op);

    // 获取下一列的类型
    short getNextColType();

    // 报告错误
    void reportUnexpected(const char *position, const char *expecting);

    // 跳过当前语句
    void skipStatement();
};

#endif //WANGSQL_INTERPRETER_H
