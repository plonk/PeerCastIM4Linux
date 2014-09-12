// ------------------------------------------------
// File : http.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//		HTTP protocol handling
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------


#include <stdlib.h>
#include "common/http.h"
#include "common/sys.h"
#include "common/common.h"
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif
using namespace std;

//-----------------------------------------
bool HTTP::checkResponse(int r)
{
	if (readResponse()!=r)
	{
		LOG_ERROR("Unexpected HTTP: %s",cmdLine);
		throw StreamException("Unexpected HTTP response");
		return false;
	}

	return true;
}
//-----------------------------------------
void HTTP::readRequest()
{
	readLine(cmdLine,sizeof(cmdLine));
}
//-----------------------------------------
int HTTP::readResponse()
{
	readLine(cmdLine,sizeof(cmdLine));

	char *cp = cmdLine;

	while (*cp)	if (*++cp == ' ') break;
	while (*cp) if (*++cp != ' ') break;

	char *scp = cp;

	while (*cp)	if (*++cp == ' ') break;
	*cp = 0;

	return atoi(scp);
}

//-----------------------------------------
bool	HTTP::nextHeader()
{
	if (readLine(cmdLine,sizeof(cmdLine)))
	{
		char *ap = strstr(cmdLine,":");
		if (ap)
			while (*++ap)
				if (*ap!=' ')
					break;
		arg = ap;
		return true;
	}else
	{
		arg = NULL;
		return false;
	}

}
//-----------------------------------------
bool	HTTP::isHeader(const char *hs)
{
	return stristr(cmdLine,hs) != NULL;
}
//-----------------------------------------
bool	HTTP::isRequest(const char *rq)
{
	return strncmp(cmdLine,rq,strlen(rq)) == 0;
}
//-----------------------------------------
char *HTTP::getArgStr()
{
	return arg;
}
//-----------------------------------------
int	HTTP::getArgInt()
{
	if (arg)
		return atoi(arg);
	else
		return 0;
}
//-----------------------------------------
void HTTP::getAuthUserPass(char *user, char *pass, size_t szUser, size_t szPass)
{
	if (arg)
	{
		char *s = stristr(arg,"Basic");
		if (s)
		{
			while (*s)
				if (*s++ == ' ')
					break;
			String str;
			str.set(s,String::T_BASE64);
			str.convertTo(String::T_ASCII);
			s = strstr(str.cstr(),":");
			if (s)
			{
				*s = 0;
				if (user)
				{
					strncpy(user, str.c_str(), szUser);
					user[szUser-1] = '\0';
				}
				if (pass)
				{
					strncpy(pass, s+1, szPass);
					pass[szPass-1] = '\0';
				}
			}
		}
	}
}
// -----------------------------------
void	CookieList::init()
{
	for(int i=0; i<MAX_COOKIES; i++)
		list[i].clear();

	neverExpire = false;
}

// -----------------------------------
bool	CookieList::contains(Cookie &c)
{
	if ((c.id[0]) && (c.ip))
		for(int i=0; i<MAX_COOKIES; i++)
			if (list[i].compare(c))
				return true;

	return false;
}
// -----------------------------------
void	Cookie::logDebug(const char *str, int ind)
{
	char ipstr[64];
	Host h;
	h.ip = ip;
	h.IPtoStr(ipstr);

	LOG_DEBUG("%s %d: %s - %s",str,ind,ipstr,id);
}

// -----------------------------------
bool	CookieList::add(Cookie &c)
{
	if (contains(c))
		return false;

	unsigned int oldestTime=(unsigned int)-1;
	int oldestIndex=0;

	for(int i=0; i<MAX_COOKIES; i++)
		if (list[i].time <= oldestTime)
		{
			oldestIndex = i;
			oldestTime = list[i].time;
		}

	c.logDebug("Added cookie",oldestIndex);
	c.time = sys->getTime();
	list[oldestIndex]=c;
	return true;
}
// -----------------------------------
void	CookieList::remove(Cookie &c)
{
	for(int i=0; i<MAX_COOKIES; i++)
		if (list[i].compare(c))
			list[i].clear();
}
// -----------------------------------
map<int,string> HTTPResponse::messages = {
    { 100, "Continue" },
    { 101, "Switching Protocols" },
    { 200, "OK" },
    { 201, "Created" },
    { 202, "Accepted" },
    { 203, "Non-Authoritative Information" },
    { 204, "No Content" },
    { 205, "Reset Content" },
    { 206, "Partial Content" },
    { 300, "Multiple Choices" },
    { 301, "Moved Permanently" },
    { 302, "Found" },
    { 303, "See Other" },
    { 304, "Not Modified" },
    { 305, "Use Proxy" },
    { 307, "Temporary Redirect" },
    { 400, "Bad Request" },
    { 401, "Unauthorized" },
    { 402, "Payment Required" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
    { 405, "Method Not Allowed" },
    { 406, "Not Acceptable" },
    { 407, "Proxy Authentication Required" },
    { 408, "Request Time-out" },
    { 409, "Conflict" },
    { 410, "Gone" },
    { 411, "Length Required" },
    { 412, "Precondition Failed" },
    { 413, "Request Entity Too Large" },
    { 414, "Request-URI Too Large" },
    { 415, "Unsupported Media Type" },
    { 416, "Requested range not satisfiable" },
    { 417, "Expectation Failed" },
    { 500, "Internal Server Error" },
    { 501, "Not Implemented" },
    { 502, "Bad Gateway" },
    { 503, "Service Unavailable" },
    { 504, "Gateway Time-out" },
    { 505, "HTTP Version not supported" },
};

// ---------------------------------------
void HTTPResponse::writeToStream(Stream &os)
{
    const char *SP = " ", *CRLF = "\r\n";

    os.writeString(util::format("%s %d %s\r\n", "HTTP/1.0", status, messages[status]).c_str());
    for (std::pair<std::string,std::string> header : headers)
    {
        os.writeString(header.first.c_str())
            .writeString(":")
            .writeString(SP)
            .writeString(header.second.c_str())
            .writeString(CRLF);
    }
    os.writeString("Connection: close").writeString(CRLF);
    os.writeString(CRLF);
    body(os);
}

