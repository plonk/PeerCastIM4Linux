#include <string>
#include "common/localfs.h"
#include "common/html.h"
#include "common/http.h"
#include <boost/filesystem.hpp>
#include "common/version2.h"
#include "util.h"
#include "common/template.h"

using namespace std;
using namespace boost::filesystem;
using namespace util;

static HTTPResponse handleLocalFileRequest(string, string);
static HTTPResponse serveLocalFile(string);

// -----------------------------------
static HTTPResponse handleLocalFileRequest(string path, string args)
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
	}else {
        return serveLocalFile(path);
    }
}

// -----------------------------------
static HTTPResponse serveLocalFile(string fileName)
{
    string ext = extension(fileName);

    string type = (MIMETypes.find(ext)==MIMETypes.end()) ? "application/octet-stream" : MIMETypes[ext];

    try {
        return HTTPResponse(200,
                            {
                                { "Content-Type", type },
                            },
                            [=] (Stream& os) mutable
                            {
                                FileStream file;

                                file.openReadOnly(fileName.c_str());
                                while (!file.eof())
                                    os.writeChar(file.readChar());
                            });
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
HTTPResponse LocalFileServer::request(std::string path)
{
    vector<string> vs = split(path, "?");
    string fsPath = documentRoot + vs[0];
    string args = (vs.size() < 2) ? "" : vs[1];

	LOG_DEBUG("Writing local file: %s", fsPath.c_str());

    return handleLocalFileRequest(fsPath.c_str(), args.c_str());
}
