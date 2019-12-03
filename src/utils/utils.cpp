
#include <cstring>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "global.h"
#include "main.h"
#include "utils/utils.h"

using namespace std;

int Utils::getTypeSize(short type) {
    if (type == TYPE_NULL)
        return 0;
    else if (type < TYPE_CHAR)
        return type + 1;
    else if (type == TYPE_INT)
        return 4;
    else if (type == TYPE_FLOAT)
        return 4;
    else {
        cerr << "ERROR: [Utils::getTypeSize] Unknown type " << type << "!" << endl;
        return 0;
    }
}

bool Utils::fileExists(const char *filename) {
    FILE *file = fopen((DATA_STORE_DIR + string(filename) + DATA_FILE_TYPE).c_str(), "rb");
    if (file) {
        fclose(file);
        return true;
    }
    return false;
}

void Utils::deleteFile(const char *filename) {
    remove((DATA_STORE_DIR + string(filename) + DATA_FILE_TYPE).c_str());
    MiniSQL::getBufferManager()->removeBlockByFilename(filename);
}

char* Utils::getDataFromStr(const char *s, int type) {
    char *key = nullptr;
    int size = getTypeSize(type);

    if (type <= TYPE_CHAR) {
        int len = strlen(s);
        if (len > type) {
            cerr << "ERROR: [Utils::getDataFromStr] Expecting char(" << type << "), but found char(" << len << ")." << endl;
            return nullptr;
        }
        key = new char[size];
        memcpy(key, s, min(size, len + 1));
    } else if (type == TYPE_INT) {
        int value;
        if (sscanf(s, "%d", &value) < 1) {
            cerr << "ERROR: [Utils::getDataFromStr] Expecting int, but found '" << s << "'." << endl;
            return nullptr;
        }
        key = new char[size];
        memcpy(key, &value, size);
    } else if (type == TYPE_FLOAT) {
        float value;
        if (sscanf(s, "%f", &value) < 1) {
            cerr << "ERROR: [Utils::getDataFromStr] Expecting float, but found '" << s << "'." << endl;
            return nullptr;
        }
        key = new char[size];
        memcpy(key, &value, size);
    }
    return key;
}

bool Utils::createDirectory(string dirname) {
    int nameLength = dirname.length();
    char tempDirPath[256] = {0};
    for (int i = 0; i < nameLength; ++i) {
        tempDirPath[i] = dirname[i];
        if (tempDirPath[i] == '\\' || tempDirPath[i] == '/') {
            if (access(tempDirPath, 0) != 0) {
                int res = mkdir(tempDirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
                if (res)
                    return false;
            }
        }
    }
    return true;
}