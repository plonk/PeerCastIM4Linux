#include <string>
#include "common/localfs.h"
#include "common/html.h"
#include "common/http.h"
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include "common/version2.h"
#include "util.h"
#include "common/template.h"
#include <boost/optional.hpp>

using namespace std;
using namespace boost::filesystem;
using namespace util;

static HTTPResponse handleLocalFileRequest(string, string);
static HTTPResponse serveLocalFile(string, boost::optional<string> mtime);
static HTTPResponse serveTemplate(string path, string args);

// -----------------------------------
static HTTPResponse handleLocalFileRequest(string path, string args, const HTTPHeaders& headers)
{
    if (!exists(path))
        return HTTPResponse(404, { {"Content-Type", "text/html"}, },
                            [=](Stream& os)
                            {
                                HTMLBuilder(os).page404("The requested URL " + path + " was not found on this server.");
                            });

    if (is_directory(path))
        return HTTPResponse(403, { {"Content-Type", "text/html"}, },
                            [=](Stream& os)
                            {
                                HTMLBuilder(os).page403("You don't have permission to access " + path + " on this server.");
                            });

    string ext = extension(path);
	if (ext==".html" || ext==".htm")
	{
        return serveTemplate(path, args);
	}else {
        auto it = headers.find("If-Modified-Since");

        if (it == headers.end())
            return serveLocalFile(path, boost::none);
        else
            return serveLocalFile(path, headers.at("If-Modified-Since"));
    }
}

// -----------------------------------
static HTTPResponse serveTemplate(string path, string args)
{
    try {
        return HTTPResponse(200, { { "Content-Type", "text/html" }, },
                            [=](Stream& os)
                            {
                                Template templ(path.c_str(), args.c_str());
                                templ.writeToStream(os);
                            });
    } catch (StreamException &e)
    {
        return HTTPResponse(404, { { "Content-Type", "text/html" }, },
                            [=](Stream& os)
                            {
                                HTMLBuilder(os).page404(format("%s: %s", e.msg, path));
                            });
    }
}

// -----------------------------------
static HTTPResponse serveLocalFile(string fileName, boost::optional<string> lastModified)
{
    try {
        string type = getMimeType(extension(fileName));
        time_t fileMtime = last_write_time(fileName);

        if (lastModified && parseRfc1123Time(*lastModified) == fileMtime)
        {
            return HTTPResponse(304, {}, [=] (Stream& os) {});
        }else
        {
            return HTTPResponse(200,
                                {
                                    { "Content-Type", type },
                                    { "Content-Length", to_string( file_size(fileName) ) },
                                    { "Last-Modified", rfc1123Time(fileMtime) }
                                },
                                [=] (Stream& os)
                                {
                                    FileStream file;

                                    file.openReadOnly(fileName.c_str());
                                    while (true)
                                    {
                                        auto c = file.readChar();
                                        if (file.eof()) break;
                                        os.writeChar(c);
                                    }
                                    file.close();
                                });
        }
    }catch (StreamException &e)
    {
        string msg = string(e.msg) + ": " + fileName;
        return HTTPResponse(404,
                            {
                                {"Content-Type", "text/html"}
                            }, [=](Stream& os) { HTMLBuilder(os).page404(msg); });
    }
}

// -----------------------------------
HTTPResponse LocalFileServer::request(std::string path, const HTTPHeaders& headers)
{
    using namespace boost;

    static const regex e("([^?]*)(?:\\?)?(.*)");
    cmatch results;
    regex_match(path.c_str(), results, e);

    string fsPath = documentRoot + results[1];

	LOG_DEBUG("Writing local file: %s", fsPath.c_str());

    return handleLocalFileRequest(fsPath, results[2], headers);
}
