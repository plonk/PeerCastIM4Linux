// ------------------------------------------------
// File : html.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//		HTML protocol handling
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


#include <stdarg.h>
#include "common/html.h"
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

using namespace std;

// --------------------------------------
HTMLBuilder::~HTMLBuilder()
{
    // tag level check
}

// --------------------------------------
void HTMLBuilder::doctype()
{
    out.writeLine("<!DOCTYPE html>");
}
// --------------------------------------
void HTMLBuilder::startHTML()
{
	startNode("html");
}
// --------------------------------------
void HTMLBuilder::startBody()
{
	startNode("body");
}
// --------------------------------------
void HTMLBuilder::addHead()
{
	char buf[512];
    startNode("head");
    startTagEnd("title",title.c_str());
    startTagEnd("meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"");

    if (!refreshURL.isEmpty())
    {
        sprintf(buf,"meta http-equiv=\"refresh\" content=\"%d;URL=%s\"",refresh,refreshURL.c_str());
        startTagEnd(buf);
    }else if (refresh)
    {
        sprintf(buf,"meta http-equiv=\"refresh\" content=\"%d\"",refresh);
        startTagEnd(buf);
    }


    end();
}
// --------------------------------------
void HTMLBuilder::startNode(const char *tag, const char *data)
{
	const char *p = tag;
	char *o = &currTag[tagLevel][0];

	int i;
	for(i=0; i<MAX_TAGLEN-1; i++)
	{
		char c = *p++;
		if ((c==0) || (c==' '))
			break;
		else
			*o++ = c;
	}
	*o = 0;

    indent();
	out.writeString("<")
        .writeString(tag)
        .writeString(">")
        .writeString("\n");

	tagLevel++;
	if (tagLevel >= MAX_TAGLEVEL)
		throw StreamException("HTML too deep!");

	if (data)
    {
        indent();
		out.writeString(data)
            .writeString("\n");
    }
}
// --------------------------------------
void HTMLBuilder::end()
{
	tagLevel--;
	if (tagLevel < 0)
		throw StreamException("HTML premature end!");

    indent();
	out.writeString("</")
        .writeString(&currTag[tagLevel][0])
        .writeString(">")
        .writeString("\n");
}
// --------------------------------------
void HTMLBuilder::addLink(const char *url, const char *text, bool toblank)
{
	char buf[1024];

	sprintf(buf,"a href=\"%s\" %s",url,toblank?"target=\"_blank\"":"");
	startNode(buf,text);
	end();
}
// --------------------------------------
void HTMLBuilder::startTag(const char *tag, const char *fmt,...)
{
	if (fmt)
	{

		va_list ap;
        va_start(ap, fmt);

		char tmp[512];
		vsprintf(tmp,fmt,ap);
		startNode(tag,tmp);

        va_end(ap);
	}else{
		startNode(tag,NULL);
	}
}
// --------------------------------------
void HTMLBuilder::startTagEnd(const char *tag, const char *fmt,...)
{
	if (fmt)
	{

		va_list ap;
        va_start(ap, fmt);

		char tmp[512];
		vsprintf(tmp,fmt,ap);
		startNode(tag,tmp);

        va_end(ap);
	}else{
		startNode(tag,NULL);
	}
	end();
}
// --------------------------------------
void HTMLBuilder::startSingleTagEnd(const char *fmt,...)
{
	va_list ap;
	va_start(ap, fmt);

    indent();
	char tmp[512];
	vsprintf(tmp,fmt,ap);
	out.writeString("<")
        .writeString(tmp)
        .writeString(">")
        .writeString("\n");

	va_end(ap);
}

// --------------------------------------
void HTMLBuilder::startTableRow(int i)
{
	if (i & 1)
		startTag("tr bgcolor=\"#dddddd\" align=\"left\"");
	else
		startTag("tr bgcolor=\"#eeeeee\" align=\"left\"");
}

// --------------------------------------
void HTMLBuilder::indent()
{
    out.writeString(string(tagLevel * 4, ' ').c_str());
}
// -----------------------------------
#include "version2.h"
#include "common/socket.h"
void HTMLBuilder::errorPage(string title, string heading, string msg)
{
    char ip[80];
    Host(ClientSocket::getIP(NULL), 0).IPtoStr(ip);

    doctype();
    startHTML();
    startTag("head");
    startTagEnd("title", title.c_str());
    end();
    startBody();
    startTagEnd("h1", heading.c_str());
    startTagEnd("p", msg.c_str());
    startSingleTagEnd("hr");
    startTagEnd("address", "%s at %s Port unknown", PCX_AGENTEX, ip);
    end();
    end();
}

// -----------------------------------
void HTMLBuilder::page404(string msg)
{
    errorPage("404 Not Found", "Not Found", msg);
}

// -----------------------------------
void HTMLBuilder::page403(string msg)
{
    errorPage("403 Forbidden", "Forbidden", msg);
}

