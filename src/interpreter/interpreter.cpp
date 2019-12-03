#include <ctime>
#include <iostream>
#include <fstream>

#include "global.h"
#include "record/recordManager.h"
#include "interpreter/interpreter.h"

using namespace std;

Interpreter::Interpreter(bool _fromFile): fromFile(_fromFile) {
    ptr = -1;
    queryCount = 0;
    exiting = false;
    tokenizer = new Tokenizer();
    api = new Api();
}

Interpreter::~Interpreter() {
    delete tokenizer;
    delete api;
}

int Interpreter::getQueryCount() const {
    return queryCount;
}

bool Interpreter::isExiting() const {
    return exiting;
}

bool Interpreter::tokenVecEmpty() const {
    return ptr == (int)tokens.size() - 1;
}

void Interpreter::execute(const char *sql, const char *file) {
    // 重定向标准输出到file
    ofstream newBuf(file);
    streambuf *oldCoutBuf = cout.rdbuf(newBuf.rdbuf());
    streambuf *oldCerrBuf = cerr.rdbuf(newBuf.rdbuf());
    // 执行
    execute(sql);
    // 还原
    cout.rdbuf(oldCoutBuf);
    cerr.rdbuf(oldCerrBuf);
}

void Interpreter::execute(const char* sql) {
    int endCount = tokenizer->getTokens(sql, &tokens, &type);
    queryCount += endCount;

    while (endCount--) {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER && type[ptr] != Tokenizer::TOKEN_END)
            reportUnexpected("execute", "instruction");
        else if (type[ptr] == Tokenizer::TOKEN_END) {}
        else if (tokens[ptr] == "select")
            select();
        else if (tokens[ptr] == "insert")
            insert();
        else if (tokens[ptr] == "delete")
            remove();
        else if (tokens[ptr] == "create")
            create();
        else if (tokens[ptr] == "drop")
            drop();
        else if (tokens[ptr] == "exec" || tokens[ptr] == "execfile")
            execfile();
        else if (tokens[ptr] == "exit" || tokens[ptr] == "quit")
            exit();
        else {
            cerr << "ERROR: [Interpreter::execute] Unknown instruction '" << tokens[ptr] << "'." << endl;
            skipStatement();
        }
    }
}

void Interpreter::select() {
    ptr++;
    // 解析select的列
    int i = 0;
    vector<string> selectColNames;
    do {
        if (tokens[ptr] == "*" && type[ptr] == Tokenizer::TOKEN_SYMBOL) {
            if (i == 0) {
                ptr++;
                break;
            } else {
                reportUnexpected("select", "Either '*' or the column name(Simultaneous input is not accepted)");
                return;
            }
        } else if (type[ptr] == Tokenizer::TOKEN_SYMBOL) {
            ptr++;
        } else if (type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
            selectColNames.push_back(tokens[ptr]);
            ptr++;
        } else {
            reportUnexpected("select", "column name");
            return;
        }
        i++;
    } while (tokens[ptr] != "from");

    if (tokens[ptr] != "from" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("select", "'from'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("select", "table name");
        return;
    }

    // 解析 select 信息
    const char *tableName = tokens[ptr].c_str();
    vector<string> colName;
    vector<int> cond;
    vector<string> operand;
    int limit = 0;
    bool flag = true;

    ptr++;
    if (tokens[ptr] == "where" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
        ptr--;
        flag = where(&colName, &cond, &operand);
    }

    if (tokens[ptr] == "limit" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_NUMBER) {
            reportUnexpected("select ... limit", "an integer number");
            return;
        }
        limit = atoi(tokens[ptr].c_str());
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_END) flag = false;
    }

    if (flag) {
        int tic, toc, selectCount;
        tic = clock();
        selectCount = api->select(tableName, &colName, &selectColNames, &cond, &operand, limit);
        toc = clock();

        // 输出执行时间
        if (selectCount >= 0 && !fromFile)
            cout << selectCount << " record(s) selected. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
    }
}

bool Interpreter::where(vector<string> *colName, vector<int> *cond, vector<string> *operand) {
    ptr++;
    if (type[ptr] == Tokenizer::TOKEN_END) // 没有条件直接返回
        return true;
    else if (tokens[ptr] != "where" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("select", "'where'");
        return false;
    }

    while (true) {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
            reportUnexpected("select", "column name");
            return false;
        }
        colName->push_back(tokens[ptr]);

        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_OPERATOR) {
            reportUnexpected("select", "operator");
            return false;
        }
        int op = getOperatorType(tokens[ptr].c_str());
        if (op < 0) {
            cerr << "ERROR: [Interpreter::select] Unknown operator '" << tokens[ptr] << "'." << endl;
            skipStatement();
            return false;
        }
        cond->push_back(op);

        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_NUMBER
            && type[ptr] != Tokenizer::TOKEN_STRING_SINGLE
            && type[ptr] != Tokenizer::TOKEN_STRING_DOUBLE) {
            reportUnexpected("select", "value");
            return false;
        }
        operand->push_back(tokens[ptr]);

        ptr++;
        if (type[ptr] == Tokenizer::TOKEN_END || tokens[ptr] == "limit")
            return true;
        else if (tokens[ptr] != "and" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
            reportUnexpected("select", "'and'(MiniSQL only supports conjunctive selection)");
            return false;
        }
    }
}

void Interpreter::insert() {
    ptr++;
    if (tokens[ptr] != "into" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("insert", "'into'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("insert", "table name");
        return;
    }

    // 解析 insert 信息
    const char* tableName = tokens[ptr].c_str();
    vector<string> value;

    ptr++;
    if (tokens[ptr] != "values" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("insert", "'values'");
        return;
    }

    ptr++;
    if (tokens[ptr] != "(" || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
        reportUnexpected("insert", "'('");
        return;
    }

    while (true) {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_NUMBER
        && type[ptr] != Tokenizer::TOKEN_STRING_SINGLE
        && type[ptr] != Tokenizer::TOKEN_STRING_DOUBLE) {
            reportUnexpected("insert", "value");
            return;
        }
        value.push_back(tokens[ptr]);

        ptr++;
        if (tokens[ptr] == ")" && type[ptr] == Tokenizer::TOKEN_SYMBOL)
            break;
        else if (tokens[ptr] != "," || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
            reportUnexpected("insert", "','");
            return;
        }
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END) {
        reportUnexpected("insert", "';'");
        return;
    }

    // 执行插入
    int tic, toc;
    bool res;
    tic = clock();
    res = api->insert(tableName, &value);
    toc = clock();

    if (res && !fromFile)
        cout << "1 record inserted. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
}

void Interpreter::remove() {
    ptr++;
    if (tokens[ptr] != "from" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("delete", "'from'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("delete", "table name");
        return;
    }

    // 解析 delete 信息
    const char* tableName = tokens[ptr].c_str();
    vector<string> colName;
    vector<int> cond;
    vector<string> operand;

    if (where(&colName, &cond, &operand)) {
        // 执行删除
        int tic, toc, removeCount;
        tic = clock();
        removeCount = api->remove(tableName, &colName, &cond, &operand);
        toc = clock();

        if (removeCount >= 0 && !fromFile)
            cout << removeCount << " record(s) deleted. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
    }
}

void Interpreter::create() {
    ptr++;
    if (tokens[ptr] == "table" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        createTable();
    else if (tokens[ptr] == "index" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        createIndex();
    else
        reportUnexpected("create", "'table' or 'index'");
}

void Interpreter::createTable() {
    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("createTable", "table name");
        return;
    }

    // 解析需要创建表的信息
    const char *tableName = tokens[ptr].c_str();
    const char *primary = nullptr;
    vector<string> colName;
    vector<short> colType;
    vector<char> colUnique;

    ptr++;
    if (tokens[ptr] != "(" || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
        reportUnexpected("createTable", "'('");
        return;
    }

    while (true) {
        ptr++;
        if (tokens[ptr] == "primary" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
            bool hasBracket = false;

            ptr++;
            if (tokens[ptr] != "key" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
                reportUnexpected("createTable", "'key'");
                return;
            }

            ptr++;
            if (tokens[ptr] == "(" && type[ptr] == Tokenizer::TOKEN_SYMBOL)
                hasBracket = true;
            else if (type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
                if (primary != nullptr) {
                    cerr << "ERROR: [Interpreter::createTable] Multiple primary key definition." << endl;
                    skipStatement();
                    return;
                }
                primary = tokens[ptr].c_str();
            } else {
                reportUnexpected("createTable", "primary key name or '('");
                return;
            }

            if (hasBracket) {
                ptr++;
                if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
                    reportUnexpected("createTable", "primary key name");
                    return;
                } else if (primary != nullptr) {
                    cerr << "ERROR: [Interpreter::createTable] Multiple primary key definition." << endl;
                    skipStatement();
                    return;
                }
                primary = tokens[ptr].c_str();

                ptr++;
                if (tokens[ptr] != ")" || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
                    reportUnexpected("createTable", "')'");
                    return;
                }
            }
        } else if (type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
            colName.push_back(tokens[ptr]);

            int t = getNextColType();
            if (t == TYPE_NULL)
                return;
            colType.push_back(t);

            if (tokens[ptr + 1] == "unique" && type[ptr + 1] == Tokenizer::TOKEN_IDENTIFIER) {
                ptr++;
                colUnique.push_back(1);
            } else
                colUnique.push_back(0);
        } else {
            reportUnexpected("createTable", "column name or 'primary'");
            return;
        }

        ptr++;
        if (tokens[ptr] == ")" && type[ptr] == Tokenizer::TOKEN_SYMBOL)
            break;
        else if (tokens[ptr] != "," || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
            reportUnexpected("createTable", "','");
            return;
        }
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END) {
        reportUnexpected("createTable", "';'");
        return;
    }

    if (primary == nullptr) {
        cerr << "ERROR: [Interpreter::createTable] No primary key definition!" << endl;
        return;
    }

    // 执行创建
    int tic, toc;
    bool res;
    tic = clock();
    res = api->createTable(tableName, primary, &colName, &colType, &colUnique);
    toc = clock();

    if (res && !fromFile)
        cout << "1 table created. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
}

void Interpreter::createIndex() {
    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("createIndex", "index name");
        return;
    }

    // 解析索引信息，索引名、表名、列名
    const char *indexName = tokens[ptr].c_str();
    const char *tableName;
    const char *colName;

    ptr++;
    if (tokens[ptr] != "on" || type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("createIndex", "'on'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("createIndex", "table name");
        return;
    }
    tableName = tokens[ptr].c_str();

    ptr++;
    if (tokens[ptr] != "(" || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
        reportUnexpected("createIndex", "'('");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
        reportUnexpected("createIndex", "column name");
        return;
    }
    colName = tokens[ptr].c_str();

    ptr++;
    if (tokens[ptr] != ")" || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
        reportUnexpected("createIndex", "')'");
        return;
    }

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END) {
        reportUnexpected("createIndex", "';'");
        return;
    }

    // 创建
    int tic, toc;
    bool res;
    tic = clock();
    res = api->createIndex(indexName, tableName, colName);
    toc = clock();

    if (res && !fromFile)
        cout << "1 index created. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
}

void Interpreter::drop() {
    ptr++;
    if (tokens[ptr] == "table" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
            reportUnexpected("drop", "table name");
            return;
        }
        const char* tableName = tokens[ptr].c_str();

        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_END) {
            reportUnexpected("drop", "';'");
            return;
        }

        // 执行删除表
        int tic, toc;
        bool res;
        tic = clock();
        res = api->dropTable(tableName);
        toc = clock();

        if (res && !fromFile)
            cout << "1 table dropped. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
    } else if (tokens[ptr] == "index" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_IDENTIFIER) {
            reportUnexpected("drop", "index name");
            return;
        }
        const char* indexName = tokens[ptr].c_str();

        ptr++;
        if (type[ptr] != Tokenizer::TOKEN_END) {
            reportUnexpected("drop", "';'");
            return;
        }

        // 执行删除索引
        int tic, toc;
        bool res;
        tic = clock();
        res = api->dropIndex(indexName);
        toc = clock();

        if (res && !fromFile)
            cout << "1 index dropped. Query done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;
    }
    else
        reportUnexpected("drop", "'table' or 'index'");
}

void Interpreter::execfile() {
    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_STRING_SINGLE && type[ptr] != Tokenizer::TOKEN_STRING_DOUBLE) {
        reportUnexpected("execfile", "a string as filename");
        return;
    }
    const char *filename = tokens[ptr].c_str();

    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END) {
        reportUnexpected("execfile", "';'");
        return;
    }

    if (fromFile) {
        cerr << "ERROR: [Interpreter::execfile] Cannot do 'execfile' instruction when executing from file." << endl;
        return;
    }

    // 从文件中读取命令
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "ERROR: [Interpreter::execfile] Cannot load file " << filename << "!" << endl;
        return;
    }
    string line, content = "";
    while (!file.eof()) {
        getline(file, line);
        content += line + '\n';
    }
    file.close();

    // 执行命令
    int tic, toc;
    Interpreter *interpreter = new Interpreter(true);
    tic = clock();
    interpreter->execute(content.c_str());
    toc = clock();

    // 输出执行时间
    cout << interpreter->getQueryCount() << " queries done in " << 1.0 * (toc-tic) / CLOCKS_PER_SEC << "s." << endl;

    delete interpreter;
}

void Interpreter::exit() {
    ptr++;
    if (type[ptr] != Tokenizer::TOKEN_END) {
        reportUnexpected("exit", "';'");
        return;
    }

    if (fromFile)
        cerr << "ERROR: [Interpreter::exit] Cannot do 'exit' instruction when executing from file." << endl;
    else {
        cout << "Bye~ :)" << endl;
        exiting = true;
    }
}

int Interpreter::getOperatorType(const char* op) {
    string s = op;
    if (s == "=")
        return COND_EQ;
    else if (s == "<>")
        return COND_NE;
    else if (s == "<")
        return COND_LT;
    else if (s == ">")
        return COND_GT;
    else if (s == "<=")
        return COND_LE;
    else if (s == ">=")
        return COND_GE;
    else
        return -1;
}

short Interpreter::getNextColType() {
    ptr++;
    if (tokens[ptr] == "char" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER) {
        ptr++;
        if (tokens[ptr] != "(" || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
            reportUnexpected("getNextColType", "'('");
            return TYPE_NULL;
        }

        ptr++;
        int len = stoi(tokens[ptr]);
        if (type[ptr] != Tokenizer::TOKEN_NUMBER || len <= 0 || len > TYPE_CHAR) {
            reportUnexpected("getNextColType", "1~255");
            return TYPE_NULL;
        }

        ptr++;
        if (tokens[ptr] != ")" || type[ptr] != Tokenizer::TOKEN_SYMBOL) {
            reportUnexpected("getNextColType", "')'");
            return TYPE_NULL;
        }

        return len;
    } else if (tokens[ptr] == "int" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        return TYPE_INT;
    else if (tokens[ptr] == "float" && type[ptr] == Tokenizer::TOKEN_IDENTIFIER)
        return TYPE_FLOAT;
    else {
        reportUnexpected("getNextColType", "'char', 'int' or 'float'(MiniSQL only supports these three data types)");
        return TYPE_NULL;
    }
}

void Interpreter::reportUnexpected(const char *position, const char *expecting) {
    cerr << "ERROR: [Interpreter::" << position << "] Expecting " << expecting << ", but found '" << tokens[ptr] << "'." << endl;
    skipStatement();
}

void Interpreter::skipStatement() {
    if (ptr < 0)
        ptr = 0;
    for (; type[ptr] != Tokenizer::TOKEN_END; ptr++);
}