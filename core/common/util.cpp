// ---------------------------
#include <iostream>
#include <algorithm>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/regex.hpp>

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

    static const char* daysOfWeek[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char* monthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Oct", "Dec" };

    string rfc1123Time(time_t t)
    {
        tm tm;
        char fmt[30], buf[30];

        gmtime_r(&t, &tm);
        strftime(fmt, sizeof(fmt), "%%s, %d %%s %Y %H:%M:%S GMT", &tm);
        snprintf(buf, sizeof(buf), fmt, daysOfWeek[tm.tm_wday], monthNames[tm.tm_mon]);

        return buf;
    }

    time_t parseRfc1123Time(string str)
    {
        using namespace boost::gregorian;
        using namespace boost::posix_time;
        using namespace boost;

        static const regex e("..., (\\d+) (...) (\\d{4}) (\\d{2}):(\\d{2}):(\\d{2}) GMT");
        cmatch m;

        regex_match(str.c_str(), m, e);
        const string& day = m[1], mon = m[2], year = m[3], hour = m[4], min = m[5], sec = m[6];

        auto it = find_if(begin(monthNames), end(monthNames), [&] (const char* x) { return mon == x; });
        if (it == end(monthNames))
            throw runtime_error("format error");

        int monthOrd = it - begin(monthNames) + 1;

        ptime t( date(atoi(year.c_str()), monthOrd, atoi(day.c_str())),
                 time_duration(atoi(hour.c_str()),
                               atoi(min.c_str()),
                               atoi(sec.c_str()),
                               0) );

        return (t - from_time_t(0)).total_seconds();
    }

    string getMimeType(string ext)
    {
        return (MIMETypes.find(ext)==MIMETypes.end()) ? "application/octet-stream" : MIMETypes[ext];
    }

    const char* colorcode(bool firewalled, bool relay, int numRelays)
    {
        if (firewalled)
        {
            return (numRelays==0) ? "red" : "orange";
        }else
        {
            if (!relay)
            {
                return (numRelays==0) ? "purple" : "blue";
            }else
            {
                return "green";
            }
        }
    }
};
