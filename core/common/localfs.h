#ifndef _LOCALFS_H
#define _LOCALFS_H

#include <string>
#include "socket.h"
#include "http.h"

//! ローカルファイルをサーブする。
class LocalFileServer
{
 public:
    LocalFileServer(const std::string& docroot) :
        documentRoot(docroot)
        {}
    HTTPResponse request(std::string path);

    std::string documentRoot;
};

#endif
