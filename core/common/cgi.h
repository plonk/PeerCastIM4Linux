#ifndef _CGI_H
#define _GGI_H

#include <string>

#define MAX_CGI_LEN 1024

//! CGI クエリ文字列 query からキー name に対応する値を URL デコードせずに返す。キーが存在しない場合は空文字列を返す。
std::string getCGIarg_s(std::string query, std::string name);
const char *getCGIarg(const char *str, const char *arg);
char *getCGIarg(char *str, const char *arg);
bool cmpCGIarg(char *str, const char *arg, const char *value);
bool hasCGIarg(const char *str, const char *arg);
bool getCGIargBOOL(char *a);
int getCGIargINT(char *a);
char *nextCGIarg(char *cp, char *cmd, char *arg);


#endif
