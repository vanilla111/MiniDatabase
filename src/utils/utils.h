#ifndef WANGSQL_UTILS_H
#define WANGSQL_UTILS_H

using namespace std;

class Utils
{
public:

    // 获得一个类型的大小
    static int getTypeSize(short type);

    // 检查一个文件是否存在
    static bool fileExists(const char *filename);

    // 删除一个文件
    static void deleteFile(const char *filename);

    // 根据类型解析 string -> binary data
    static char *getDataFromStr(const char *s, int type);

    // 创建存放数据的文件夹
    static bool createDirectory(string dirname);
};

#endif //WANGSQL_UTILS_H
