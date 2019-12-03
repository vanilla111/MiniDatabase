#ifndef WANGSQL_TOKENIZER_H
#define WANGSQL_TOKENIZER_H

#include <vector>
#include <string>

using namespace std;

class Tokenizer
{
public:

    // Token type
    static const int TOKEN_INVALID;
    static const int TOKEN_IDLE;
    static const int TOKEN_END;
    static const int TOKEN_IDENTIFIER;
    static const int TOKEN_NUMBER;
    static const int TOKEN_STRING_SINGLE;
    static const int TOKEN_STRING_DOUBLE;
    static const int TOKEN_SYMBOL;
    static const int TOKEN_OPERATOR;

    // 从SQL语句中读取所有的token，返回分号的数量，即SQL语句的数量
    int getTokens(const char* sql, vector<string>* tokens, vector<int>* type);
};

#endif //WANGSQL_TOKENIZER_H
