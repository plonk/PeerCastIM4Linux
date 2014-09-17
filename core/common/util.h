#ifndef _UTIL_H
#define _UTIL_H

#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include <boost/format.hpp>
#include <time.h>

namespace util
{
    using namespace std;

    extern map<string,string> MIMETypes;

    vector<string> split(const string& str, const string& delimiter);

    string readFile(string filename);

    string format(boost::format& fmt);

    // 再帰。
    template<typename T, typename... Args>
    string format(boost::format& fmt, T value, Args... args)
    {
        return format(fmt % value, args...);
    }

    // 文字列から boost::format に変換する。
    template<typename... Args>
    string format(const char *sfmt, Args... args)
    {
        boost::format fmt(sfmt);
        return format(fmt, args...);
    }

    string rfc1123Time(time_t t);
    time_t parseRfc1123Time(string str);

    string getMimeType(string ext);
    const char* colorcode(bool firewalled, bool relay, int numRelays);
};

#endif
