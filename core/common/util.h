#ifndef _UTIL_H
#define _UTIL_H

#include <sstream>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <tuple>
#include <vector>
#include "common/http.h"

namespace util
{
    using namespace std;

    extern map<string,string> MIMETypes;

    //! Returns file name extension, including the dot. "" if none.
    string extension(const string& filename);

    vector<string> split(const string& str, const string& delimiter);

    string readFile(string filename);
};

#endif
