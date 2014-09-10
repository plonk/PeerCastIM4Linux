#include <string>
#include "common/localfs.h"
#include "common/html.h"
#include "common/http.h"
#include <boost/filesystem.hpp> 
#include "common/version2.h"
#include "util.h"

using namespace std;
using namespace boost::filesystem;
using namespace util;

static HTTPResponse handleLocalFileRequest(const char *path, const char *args);
static HTTPResponse serveLocalFile(const char *fileName);
static void errorPage(Stream& os, string title, string heading, string msg);
static void page404(Stream& os, string msg);
static void page403(Stream& os, string msg);

// -----------------------------------
static HTTPResponse handleLocalFileRequest(const char *path, const char *args)
{
    if (!exists(path))
        return HTTPResponse(404, { {"Content-Type", "text/html"}, },
                            [=](Stream& os)
                            {
                                page404(os, "The requested URL " + string(path) + " was not found on this server.");
                            });

    if (is_directory(path))
        return HTTPResponse(403, { {"Content-Type", "text/html"}, },
                            [=](Stream& os)
                            {
                                page403(os, "You don't have permission to access " + string(path) + " on this server.");
                            });

    string ext = util::extension(path);
	if (ext==".html" || ext==".htm")
	{
        try {
            Template *templ = new Template(path, args);
            return HTTPResponse(200, { { "Content-Type", "text/html" }, },
                                [=](Stream& os)
                                {
                                    templ->writeToStream(os);
                                    delete templ;
                                });
        } catch (StreamException &e)
        {
            return HTTPResponse(404, { { "Content-Type", "text/html" }, },
                                [=](Stream& os)
                                {
                                    page404(os, string(e.msg) + ": " + path);
                                });
        }
	}else {
        return serveLocalFile(path);
    }
}

// -----------------------------------
static void errorPage(Stream& os, string title, string heading, string msg)
{
    char ip[80];
    Host(ClientSocket::getIP(NULL), 0).IPtoStr(ip);

    HTMLBuilder hb(os);
    hb.doctype();
    hb.startHTML();
    hb.startTag("head");
    hb.startTagEnd("title", title.c_str());
    hb.end();
    hb.startBody();
    hb.startTagEnd("h1", heading.c_str());
    hb.startTagEnd("p", msg.c_str());
    hb.startSingleTagEnd("hr");
    hb.startTagEnd("address", "%s at %s Port unknown", PCX_AGENTEX, ip);
    hb.end();
    hb.end();
}

// -----------------------------------
static void page404(Stream& os, string msg)
{
    errorPage(os, "404 Not Found", "Not Found", msg);
}

// -----------------------------------
static void page403(Stream& os, string msg)
{
    errorPage(os, "403 Forbidden", "Forbidden", msg);
}

// -----------------------------------
static HTTPResponse serveLocalFile(const char *fileName)
{
    string ext = util::extension(fileName);

    string type = (MIMETypes.find(ext)==MIMETypes.end()) ? "application/octet-stream" : MIMETypes[ext];

    try {
        FileStream *file = new FileStream();
        file->openReadOnly(fileName);
        return HTTPResponse(200,
                            {
                                { "Content-Type", type },
                            },
                            [=](Stream& os) {
                                while (!file->eof())
                                    os.writeChar(file->readChar());
                                delete file;
                            });
    }catch (StreamException &e)
    {
        string msg = string(e.msg) + ": " + fileName;
        return HTTPResponse(404,
                            {
                                {"Content-Type", "text/html"},
                            }, [=](Stream& os) { page404(os, msg); });
    }
}

// -----------------------------------
HTTPResponse LocalFileServer::request(std::string path)
{
    vector<string> vs = split(path, "?");
    string fsPath = documentRoot + vs[0];
    const char *args = (vs.size() < 2) ? NULL : vs[1].c_str();

	LOG_DEBUG("Writing local file: %s", fsPath.c_str());

    return handleLocalFileRequest(fsPath.c_str(), args);
}
