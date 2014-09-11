#ifndef _ADMIN_H
#define _ADMIN_H
#include "common/servent.h"
#include "common/http.h"
#include "common/html.h"
#include "common/common.h"
#include "common/cgi.h"

class AdminController
{
public:
    AdminController(Servent& _servent) :
        servent(_servent) { }

    HTTPResponse send(char *cmd);

private:
    HTTPResponse redirect(char *cmd);
    HTTPResponse viewxml(char *cmd);
    HTTPResponse clearlog(char *cmd);
    HTTPResponse save(char *cmd);
    HTTPResponse reg(char *cmd);
    HTTPResponse edit_bcid(char *cmd);
    HTTPResponse add_bcid(char *cmd);
    HTTPResponse apply(char *cmd);
    HTTPResponse fetch(char *cmd);
    HTTPResponse stopserv(char *cmd);
    HTTPResponse hitlist(char *cmd);
    HTTPResponse clear(char *cmd);
    HTTPResponse upgrade(char *cmd);
    HTTPResponse connect(char *cmd);
    HTTPResponse shutdown(char *cmd);
    HTTPResponse stop(char *cmd);
    HTTPResponse bump(char *cmd);
    HTTPResponse keep(char *cmd);
    HTTPResponse relay(char *cmd);
    HTTPResponse net_add(char *cmd);
    HTTPResponse logout(char *cmd);
    HTTPResponse login(char *cmd);
    HTTPResponse setmeta(char *cmd);
    HTTPResponse unknown(char *cmd);

    Servent &servent;

    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];
};
#endif
