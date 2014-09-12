// ---------------------------
#include <iostream>
#include <algorithm>
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
        size_t head = 0, tail;

        while ( (tail = str.find(delimiter, head)) != string::npos)
        {
            ret.push_back( string(str.begin()+head, str.begin()+tail) );
            head = tail + delimiter.size();
        }
        ret.push_back( string(str.begin()+head, str.end()) );

        auto newEnd = remove(ret.begin(), ret.end(), "");
        ret.resize(newEnd - ret.begin());

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

    string rfc1123Time(time_t t)
    {
        static const char* dow[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
        static const char* mon[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Oct", "Dec" };
        tm tm;
        char fmt[30], buf[30];

        gmtime_r(&t, &tm);
        strftime(fmt, sizeof(fmt), "%%s, %d %%s %Y %H:%M:%S GMT", &tm);
        snprintf(buf, sizeof(buf), fmt, dow[tm.tm_wday], mon[tm.tm_mon]);

        return buf;
    }
};
