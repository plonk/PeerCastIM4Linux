// ---------------------------
#include <iostream>
#include <boost/format.hpp>

#include "common/http.h"
#include "util.h"

namespace util
{
    using namespace std;

    map<string,string> MIMETypes = {
        { ".css", MIME_CSS },
        { ".jpg", MIME_JPEG },
        { ".gif", MIME_GIF },
        { ".png", MIME_PNG },
        { ".js", "application/javascript" },
        { ".txt", MIME_TEXT },
        { ".html", MIME_HTML },
        { ".htm", MIME_HTML },
    };

    vector<string> split(const string& str, const string& delimiter)
    {
        vector<string> ret;
        size_t start = 0, index;

        while ( (index = str.find(delimiter, start)) != string::npos)
        {
            ret.push_back( string(str.begin()+start, str.begin()+index) );
            start = index + delimiter.size();
        }
        ret.push_back( string(str.begin()+start, str.end()) );

        return ret;
    }

    string readFile(string filename)
    {
        ifstream fs(filename);

        if (!fs)
            throw runtime_error("Could not open file");

        fs.seekg(0, ios::end);
        size_t len = fs.tellg();
        char *buf = new char[len + 1];

        fs.seekg(0, ios::beg);
        fs.read(buf, len);

        buf[len] = '\0';
        string ret(buf, len);
        delete[] buf;

        return ret;
    }

    // ベースケース。
    string format(boost::format& fmt)
    {
        return str(fmt);
    }
};
