#include <cstdio>
#include <iostream>

#include "utils/utils.h"
#include "main.h"
#include "struct/table.h"

Table::Table(const char *data) {
    name = data;
    primary = data + MAX_NAME_LENGTH;
    colCount = 0;
    recordLength = 0;
}

const char* Table::getName() const {
    return name.c_str();
}

const char* Table::getPrimary() const {
    return primary.c_str();
}

int Table::getColCount() {
    if (colCount == 0) loadColInfo();
    return colCount;
}

vector<string> Table::getColNameList() {
    return colNameList;
}

int Table::getRecordLength() {
    if (colCount == 0) loadColInfo();
    return recordLength;
}

const char* Table::getColName(int id) {
    if (colCount == 0) loadColInfo();
    if (id >= colCount) {
        cerr << "ERROR: [Table::getColName] Column id " << id << " too large!" << endl;
        return nullptr;
    }
    return colNameList[id].c_str();
}

int Table::getId(const char *colName) {
    for (int i = 0; i < getColCount(); ++i) {
        if (colNameList[i] == colName) {
            return i;
        }
    }
    return -1;
}

short Table::getValue(const char *colName, const char *dataIn, char *dataOut) {
    int id = getId(colName);
    if (id < 0) {
        cerr << "ERROR: [Table::getValue] Table `" << name << "` has no column named `" << colName << "`!" << endl;
        return TYPE_NULL;
    }
    memcpy(dataOut, dataIn + startPos[id], Utils::getTypeSize(colType[id]));
    return colType[id];
}

short Table::getType(const char *colName) {
    int id = getId(colName);
    if (id < 0) {
        cerr << "ERROR: [Table::getValue] Table `" << name << "` has no column named `" << colName << "`!" << endl;
        return TYPE_NULL;
    }
    return colType[id];
}

char Table::getUnique(const char *colName) {
    int id = getId(colName);
    if (id < 0) {
        cerr << "ERROR: [Table::getValue] Table `" << name << "` has no column named `" << colName << "`!" << endl;
        return -1;
    }
    return colUnique[id];
}

int Table::checkConsistency(const char *data, const char *exist) {
    for (int i = 0; i < getColCount(); ++i) {
        if (!colUnique[i]) continue;
        int j;
        for (j = 0; j < Utils::getTypeSize(colType[i]); j++) {
            if (data[startPos[i] + j] != exist[startPos[i] + j])
                break;
        }
        if (j >= Utils::getTypeSize(colType[i])) {
            return i;
        }
    }
    return -1;
}

bool Table::recordToVec(const char *data, vector<char *> *vec) {
    for (int i = 0; i < getColCount(); ++i) {
        int size = Utils::getTypeSize(colType[i]);
        char *value = new char[size];
        memcpy(value, data + startPos[i], size);
        vec->push_back(value);
    }
    return true;
}

bool Table::vecToRecord(const vector<string> *vec, char *data) {
    if (colCount == 0) loadColInfo();

    if ((int)vec->size() != colCount) {
        cerr << "ERROR: [Table::vecToRecord] Value number mismatch. Expecting " << colCount << " values, but found " << vec->size() << " values." << endl;
        return false;
    }

    for (int i = 0; i < colCount; i++) {
        char *key = Utils::getDataFromStr(vec->at(i).c_str(), colType[i]);
        int size = Utils::getTypeSize(colType[i]);
        if (key == nullptr) return false;
        memcpy(data + startPos[i], key, size);
        delete[] key;
    }
    return true;
}

void Table::loadColInfo() {
    colCount = MiniSQL::getCatalogManager()->loadTableColInfo(name.c_str(), &colNameList, &colType, &colUnique);
    for (int i = 0; i < colCount; ++i) {
        if (i > 0) {
            startPos.push_back(startPos[i - 1] + Utils::getTypeSize(colType[i - 1]));
        } else {
            startPos.push_back(0);
        }
    }
    recordLength = startPos[colCount - 1] + Utils::getTypeSize(colType[colCount - 1]);
}

#ifdef DEBUG
void Table::debugPrint()
{
    if (colCount == 0) loadColInfo();

    cerr << "DEBUG: [Table::debugPrint]" << endl;
    cerr << "Table name = " << name << ", column count = " << colCount << ", primary = " << primary << ", record length = " << recordLength << endl;
    for (int i = 0; i < colCount; i++)
        cerr << "Column = " << colNameList[i] << ", type = " << colType[i] << ", unique = " << (colUnique[i] ? '1' : '0') << endl;
    cerr << "----------------------------------------" << endl;
}
#endif