#include <iostream>

#include "global.h"
#include "struct/index.h"

using namespace std;

Index::Index(const char *data) {
    name = data;
    tableName = data + MAX_NAME_LENGTH;
    colName = data + MAX_NAME_LENGTH * 2;
}

const char* Index::getName() const {
    return name.c_str();
}

const char* Index::getTableName() const {
    return tableName.c_str();
}

const char* Index::getColName() const {
    return colName.c_str();
}

#ifdef DEBUG
void Index::debugPrint() const {
    cerr << "DEBUG: [Index::debugPrint]" << endl;
    cerr << "Index name = " << name << ", table name = " << tableName << ", column name = " << colName << endl;
    cerr << "----------------------------------------" << endl;
}
#endif