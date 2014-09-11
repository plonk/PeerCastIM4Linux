// ------------------------------------------------
// File : servhs.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//		Servent handshaking, TODO: should be in its own class
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
#include "common/servent.h"
#include "common/servmgr.h"
#include "common/html.h"
#include "common/stats.h"
#include "common/peercast.h"
#include "common/pcp.h"
#include "common/version2.h"
#include "common/util.h"
#include "common/admin.h"
#include "common/localfs.h"
#include "common/template.h"
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

using namespace std;
using namespace util;

// -----------------------------------
static bool beginWith(const char* str, const char* prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}
// -----------------------------------
void Servent::handleGetMethod(HTTP &http, char *in)
{
    char *fn = in+4;

    char *pt = strstr(fn,HTTP_PROTO1);
    if (pt)
        pt[-1] = 0;

    if (beginWith(fn,"/http/"))
    {
        String dirName = fn+6;

        if (!isAllowed(ALLOW_HTML))
             throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        if (!handshakeAuth(http,fn,false))
            throw HTTPException(HTTP_SC_UNAUTHORIZED,401);

        handshakeRemoteFile(dirName);
    }else if (beginWith(fn,"/html/"))
    {
        String dirName = fn+1;

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        if (handshakeAuth(http,fn,true))
            handshakeLocalFile(dirName);
    }else if (beginWith(fn,"/admin/?") || beginWith(fn,"/admin?"))
    {
        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        LOG_DEBUG("Admin client");
        handshakeCMD(strchr(fn, '?') + 1);
    }else if (beginWith(fn,"/admin.cgi"))
    {
        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        char *pwdArg = getCGIarg(fn,"pass=");
        char *songArg = getCGIarg(fn,"song=");
        char *mountArg = getCGIarg(fn,"mount=");
        char *urlArg = getCGIarg(fn,"url=");

        if (pwdArg && songArg)
        {
            size_t i;
            size_t slen = strlen(fn);
            for(i=0; i<slen; i++)
                if (fn[i]=='&') fn[i] = 0;

            Channel *c=chanMgr->channel;
            while (c)
            {
                if ((c->status == Channel::S_BROADCASTING) &&
                    (c->info.contentType == ChanInfo::T_MP3) )
                {
                    // if we have a mount point then check for it, otherwise update all channels.
                    bool match=true;

                    if (mountArg)
                        match = strcmp(c->mount,mountArg)==0;

                    if (match)
                    {
                        ChanInfo newInfo = c->info;
                        newInfo.track.title.set(songArg,String::T_ESC);
                        newInfo.track.title.convertTo(String::T_UNICODE);

                        if (urlArg)
                            if (urlArg[0])
                                newInfo.track.contact.set(urlArg,String::T_ESC);
                        LOG_CHANNEL("Channel Shoutcast update: %s",songArg);
                        c->updateInfo(newInfo);
                    }
                }
                c=c->next;
            }
        }
    }else if (beginWith(fn,"/pls/"))
    {

        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        ChanInfo info;
        if (servMgr->getChannel(fn+5,info,isPrivate()))
            handshakePLS(info,false);
        else
            throw HTTPException(HTTP_SC_NOTFOUND,404);
    }else if (beginWith(fn,"/stream/"))
    {
        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        triggerChannel(fn+8,ChanInfo::SP_HTTP,isPrivate());
    }else if (beginWith(fn,"/channel/"))
    {
        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
                throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        triggerChannel(fn+9,ChanInfo::SP_PCP,false);
    }else
    {
        while (http.nextHeader());
        http.writeLine(HTTP_SC_FOUND)
            .writeLineF("Location: /%s/index.html",servMgr->htmlPath)
            .writeLine("");
    }
}
// -----------------------------------
void Servent::handlePostMethod(HTTP &http, char *in)
{
    char *fn = in+5;

    char *pt = strstr(fn,HTTP_PROTO1);
    if (pt)
        pt[-1] = 0;

    if (beginWith(fn,"/pls/"))
    {

        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_UNAVAILABLE,503);


        ChanInfo info;
        if (servMgr->getChannel(fn+5,info,isPrivate()))
            handshakePLS(info,false);
        else
            throw HTTPException(HTTP_SC_NOTFOUND,404);

    }else if (beginWith(fn,"/stream/"))
    {

        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        triggerChannel(fn+8,ChanInfo::SP_HTTP,isPrivate());

    }else if (beginWith(fn,"/admin"))
    {
        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE,503);


        LOG_DEBUG("Admin client");
        while(http.nextHeader()){
            LOG_DEBUG("%s",http.cmdLine);
        }
        char buf[8192];
        if (sock->readLine(buf, sizeof(buf)) != 0){
            handshakeCMD(buf);
        }

    }else
    {
        while (http.nextHeader());
        http.writeLine(HTTP_SC_FOUND)
            .writeLineF("Location: /%s/index.html",servMgr->htmlPath)
            .writeLine("");
    }
}
// -----------------------------------
void Servent::handleHeadMethod(HTTP &http, char *in, bool isHTTP)
{
    char *str = in + 4;

    if (str = stristr(str, "/stream/"))
    {
        int cnt = 0;

        str += 8;
        while (*str && (('0' <= *str && *str <= '9') || ('A' <= *str && *str <= 'F') || ('a' <= *str && *str <= 'f')))
            ++cnt, ++str;

        if (cnt == 32 && beginWith(str, ".wmv"))
        {
            // interpret "HEAD /stream/[0-9a-fA-F]{32}.wmv" as GET
            LOG_DEBUG("INFO: interpret as GET");

            char *fn = in+5;

            char *pt = strstr(fn,HTTP_PROTO1);
            if (pt)
                pt[-1] = 0;

            if (!sock->host.isLocalhost())
                if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                    throw HTTPException(HTTP_SC_UNAVAILABLE,503);

            triggerChannel(fn+8,ChanInfo::SP_HTTP,isPrivate());

            return;
        }
    }

    if (http.isRequest(servMgr->password))
    {
        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_UNAVAILABLE,503);

        loginPassword.set(servMgr->password);	// pwd already checked

        sock->writeLine("OK2")
            .writeLine("icy-caps:11")
            .writeLine("");
        LOG_DEBUG("ShoutCast client");

        handshakeICY(Channel::SRC_SHOUTCAST,isHTTP);
        sock = NULL;	// socket is taken over by channel, so don`t close it

    }else
    {
        throw HTTPException(HTTP_SC_BADREQUEST,400);
    }
}
// -----------------------------------
void Servent::handshakeHTTP(HTTP &http, bool isHTTP)
{
	char *in = http.cmdLine;

	if (http.isRequest("GET /"))
	{
        handleGetMethod(http, in);
	}
	else if (http.isRequest("POST /"))
	{
        handlePostMethod(http, in);
	}else if (http.isRequest("GIV"))
	{
		HTTP http(*sock);

		while (http.nextHeader()) ;

		if (!isAllowed(ALLOW_NETWORK))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		GnuID id;
		id.clear();

		char *idstr = strstr(in,"/");
		if (idstr)
			id.fromStr(idstr+1);

		char ipstr[64];
		sock->host.toStr(ipstr);

		if (id.isSet())
		{
			// at the moment we don`t really care where the GIV came from, so just give to chan. no. if its waiting.
			Channel *ch = chanMgr->findChannelByID(id);

			if (!ch)
				throw HTTPException(HTTP_SC_NOTFOUND,404);

			if (!ch->acceptGIV(sock))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);


			LOG_DEBUG("Accepted GIV channel %s from: %s",idstr,ipstr);

			sock=NULL;					// release this servent but dont close socket.
		}else
		{

			if (!servMgr->acceptGIV(sock))
				throw HTTPException(HTTP_SC_UNAVAILABLE,503);

			LOG_DEBUG("Accepted GIV PCP from: %s",ipstr);
			sock=NULL;					// release this servent but dont close socket.
		}
	}else if (http.isRequest(PCX_PCP_CONNECT))
	{

		if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		processIncomingPCP(true);

	}else if (http.isRequest("PEERCAST CONNECT"))
	{
		if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		LOG_DEBUG("PEERCAST client");
		processServent();

	}else if (http.isRequest("SOURCE"))
	{
		if (!isAllowed(ALLOW_BROADCAST))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		char *mount = NULL;

		char *ps;
		if (ps=strstr(in,"ICE/1.0"))
		{
			mount = in+7;
			*ps = 0;
			LOG_DEBUG("ICE 1.0 client to %s",mount?mount:"unknown");
		}else{
			mount = in+strlen(in);
			while (*--mount)
				if (*mount == '/')
				{
					mount[-1] = 0; // password preceeds
					break;
				}
			loginPassword.set(in+7);

			LOG_DEBUG("ICY client: %s %s",loginPassword.c_str(),mount?mount:"unknown");
		}

		if (mount)
			loginMount.set(mount);

		handshakeICY(Channel::SRC_ICECAST,isHTTP);
		sock = NULL;	// socket is taken over by channel, so don`t close it

	} else if (http.isRequest("HEAD")) // for android client
	{
        handleHeadMethod(http, in, isHTTP);
	} else if (http.isRequest(servMgr->password))
	{
		if (!isAllowed(ALLOW_BROADCAST))
			throw HTTPException(HTTP_SC_UNAVAILABLE,503);

		loginPassword.set(servMgr->password);	// pwd already checked

		sock->writeLine("OK2")
            .writeLine("icy-caps:11")
            .writeLine("");
		LOG_DEBUG("ShoutCast client");

		handshakeICY(Channel::SRC_SHOUTCAST,isHTTP);
		sock = NULL;	// socket is taken over by channel, so don`t close it
	}else
	{
		throw HTTPException(HTTP_SC_BADREQUEST,400);
	}

}
// -----------------------------------
bool Servent::canStream(Channel *ch)
{
	if (ch==NULL)
		return false;

	if (servMgr->isDisabled)
		return false;

	if (!isPrivate())
	{
		if  (!ch->isPlaying() || ch->isFull() || ((type == T_DIRECT) && servMgr->directFull()))
			return false;

		if (!isIndexTxt(ch) && (type == T_RELAY) && (servMgr->relaysFull()))
			return false;

		Channel *c = chanMgr->channel;
		int noRelay = 0;
		unsigned int needRate = 0;
		unsigned int allRate = 0;
		while(c){
			if (c->isPlaying()){
				int nlr = c->localRelays();
				allRate += c->info.bitrate * nlr;
				if ((c != ch) && (nlr == 0)){
					if(!isIndexTxt(c))	// for PCRaw (relay)
						noRelay++;
					needRate+=c->info.bitrate;
				}
			}
			c = c->next;
		}
		unsigned int numRelay = servMgr->numStreams(Servent::T_RELAY,false);
        int diff = servMgr->maxRelays - numRelay;
		if (ch->localRelays()){
			if (noRelay > diff){
				noRelay = diff;
			}
		} else {
			noRelay = 0;
			needRate = 0;
		}

		LOG_DEBUG("Relay check: Max=%d Now=%d Need=%d ch=%d",
			servMgr->maxBitrateOut, allRate, needRate, ch->info.bitrate);
		//		if  (	!ch->isPlaying()
		//				|| ch->isFull()
		//				|| (allRate + needRate + ch->info.bitrate > servMgr->maxBitrateOut)
		//				|| ((type == T_RELAY) && servMgr->relaysFull() && force_off)	// for PCRaw (relay) (force_off)
		//				|| ((type == T_RELAY) && (((servMgr->numStreams(Servent::T_RELAY,false) + noRelay) >= servMgr->maxRelays)) && force_off)	// for PCRaw (relay) (force_off)
		//				|| ((type == T_DIRECT) && servMgr->directFull())
		//		){

		if (allRate + needRate + ch->info.bitrate > servMgr->maxBitrateOut)
		{
			LOG_DEBUG("Relay check: NG");
			return false;
		}

		if (!isIndexTxt(ch) && (type == T_RELAY) && (numRelay + noRelay >= servMgr->maxRelays))
		{
			LOG_DEBUG("Relay check: NG");
			return false;
		}
	}

	LOG_DEBUG("Relay check: OK");
	return true;
}
// -----------------------------------
void Servent::handshakeIncoming()
{
	setStatus(S_HANDSHAKE);

	char buf[2048];
	sock->readLine(buf,sizeof(buf));

	char sb[64];
	sock->host.toStr(sb);

	if (stristr(buf,RTSP_PROTO1))
	{
		LOG_DEBUG("RTSP from %s '%.100s'",sb,buf);
		RTSP rtsp(*sock);
		rtsp.initRequest(buf);
		handshakeRTSP(rtsp);
	}else if (stristr(buf,HTTP_PROTO1))
	{
		LOG_DEBUG("HTTP from %s '%.100s'",sb,buf);
		HTTP http(*sock);
		http.initRequest(buf);
		handshakeHTTP(http,true);
	}else
	{
		LOG_DEBUG("Connect from %s '%.100s'",sb,buf);
		HTTP http(*sock);
		http.initRequest(buf);
		handshakeHTTP(http,false);
	}

}
// -----------------------------------
void Servent::triggerChannel(char *str, ChanInfo::PROTOCOL proto,bool relay)
{

	ChanInfo info;

	servMgr->getChannel(str,info,relay);

	if (proto == ChanInfo::SP_PCP)
		type = T_RELAY;
	else
		type = T_DIRECT;

	outputProtocol = proto;

	processStream(false,info);

}
// -----------------------------------
void writePLSHeader(Stream &s, PlayList::TYPE type)
{
    s.writeLine(HTTP_SC_OK)
        .writeLineF("%s %s",HTTP_HS_SERVER,PCX_AGENT);

	const char *content;
	switch(type)
	{
		case PlayList::T_PLS:
			content = MIME_XM3U;
			break;
		case PlayList::T_ASX:
			content = MIME_ASX;
			break;
		case PlayList::T_RAM:
			content = MIME_RAM;
			break;
		default:
			content = MIME_TEXT;
			break;
	}
	s.writeLineF("%s %s",HTTP_HS_CONTENT,content)
        .writeLine("Content-Disposition: inline")
        .writeLine("Cache-Control: private" )
        .writeLineF("%s %s",HTTP_HS_CONNECTION,"close");

	s.writeLine("");
}
// -----------------------------------
void Servent::handshakePLS(ChanInfo &info, bool doneHandshake)
{
	char url[256];

	char in[128];

	if (!doneHandshake)
		while (sock->readLine(in,128));


	if (getLocalTypeURL(url,info.contentType))
	{

		PlayList::TYPE type;

		if ((info.contentType == ChanInfo::T_WMA) || (info.contentType == ChanInfo::T_WMV))
			type = PlayList::T_ASX;
		else if (info.contentType == ChanInfo::T_OGM)
			type = PlayList::T_RAM;
		else
			type = PlayList::T_PLS;

		writePLSHeader(*sock,type);

		PlayList *pls;

		pls = new PlayList(type,1);

		pls->addChannel(url,info);

		pls->write(*sock);

		delete pls;
	}
}
// -----------------------------------
void Servent::handshakePLS(ChanHitList **cl, int num, bool doneHandshake)
{
	char url[256];
	char in[128];

	if (!doneHandshake)
		while (sock->readLine(in,128));

	if (getLocalURL(url))
	{
		writePLSHeader(*sock,PlayList::T_SCPLS);

		PlayList *pls;

		pls = new PlayList(PlayList::T_SCPLS,num);

		for(int i=0; i<num; i++)
			pls->addChannel(url,cl[i]->info);

		pls->write(*sock);

		delete pls;
	}
}
// -----------------------------------
bool Servent::getLocalURL(char *str)
{
	if (!sock)
		throw StreamException("Not connected");


	char ipStr[64];

	Host h;

	if (sock->host.localIP())
		h = sock->getLocalHost();
	else
		h = servMgr->serverHost;

	h.port = servMgr->serverHost.port;

	h.toStr(ipStr);

	sprintf(str,"http://%s",ipStr);
	return true;
}
// -----------------------------------
bool Servent::getLocalTypeURL(char *str, ChanInfo::TYPE type)
{
	if (!sock)
		throw StreamException("Not connected");


	char ipStr[64];

	Host h;

	if (sock->host.localIP())
		h = sock->getLocalHost();
	else
		h = servMgr->serverHost;

	h.port = servMgr->serverHost.port;

	h.toStr(ipStr);
	switch(type) {
		case ChanInfo::T_WMA:
		case ChanInfo::T_WMV:
			sprintf(str,"mms://%s",ipStr);
			break;
		default:
			sprintf(str,"http://%s",ipStr);
	}
	return true;
}
// -----------------------------------
// Warning: testing RTSP/RTP stuff below.
// .. moved over to seperate app now.
// -----------------------------------
void Servent::handshakeRTSP(RTSP &rtsp)
{
	throw HTTPException(HTTP_SC_BADREQUEST,400);
}
// -----------------------------------
bool Servent::handshakeAuth(HTTP &http,const char *args,bool local)
{
	char user[1024],pass[1024];
	user[0] = pass[0] = 0;

	auto pwd = getCGIarg_s(args, "pass");

	if (pwd == "" && strlen(servMgr->password))
	{
		if (strcmp(pwd.c_str(), servMgr->password)==0)
		{
		    while (http.nextHeader());
			return true;
		}
	}

	Cookie gotCookie;
	cookie.clear();

    while (http.nextHeader())
	{
		char *arg = http.getArgStr();
		if (!arg)
			continue;

		switch (servMgr->authType)
		{
			case ServMgr::AUTH_HTTPBASIC:
				if (http.isHeader("Authorization"))
					http.getAuthUserPass(user, pass, sizeof(user), sizeof(pass));
				break;
			case ServMgr::AUTH_COOKIE:
				if (http.isHeader("Cookie"))
				{
					LOG_DEBUG("Got cookie: %s",arg);
					char *idp=arg;
					while ((idp = strstr(idp,"id=")))
					{
						idp+=3;
						gotCookie.set(idp,sock->host.ip);
						if (servMgr->cookieList.contains(gotCookie))
						{
							LOG_DEBUG("Cookie found");
							cookie = gotCookie;
							break;
						}

					}
				}
				break;
		}
	}

	if (sock->host.isLocalhost())
		return true;


	switch (servMgr->authType)
	{
		case ServMgr::AUTH_HTTPBASIC:

			if ((strcmp(pass,servMgr->password)==0) && strlen(servMgr->password))
				return true;
			break;
		case ServMgr::AUTH_COOKIE:
			if (servMgr->cookieList.contains(cookie))
				return true;
			break;
	}



	if (servMgr->authType == ServMgr::AUTH_HTTPBASIC)
	{
		http.writeLine(HTTP_SC_UNAUTHORIZED);
		http.writeLine("WWW-Authenticate: Basic realm=\"PeerCast Admin\"");
	}else if (servMgr->authType == ServMgr::AUTH_COOKIE)
	{
		String file = servMgr->htmlPath;
		file.append("/login.html");
		if (local)
			handshakeLocalFile(file);
		else
			handshakeRemoteFile(file);
	}


	return false;
}

// -----------------------------------
void Servent::handshakeCMD(char *cmd)
{
	HTTP http(*sock);
    AdminController ctl(*this);

	if (!handshakeAuth(http,cmd,true))
		return;

    ctl.send(cmd).writeToStream(*sock);
}
// -----------------------------------
void Servent::readICYHeader(HTTP &http, ChanInfo &info, char *pwd, size_t szPwd)
{
	char *arg = http.getArgStr();
	if (!arg) return;

	if (http.isHeader("x-audiocast-name") || http.isHeader("icy-name") || http.isHeader("ice-name"))
	{
		info.name.set(arg,String::T_ASCII);
		info.name.convertTo(String::T_UNICODE);

	}else if (http.isHeader("x-audiocast-url") || http.isHeader("icy-url") || http.isHeader("ice-url"))
		info.url.set(arg,String::T_ASCII);
	else if (http.isHeader("x-audiocast-bitrate") || (http.isHeader("icy-br")) || http.isHeader("ice-bitrate") || http.isHeader("icy-bitrate"))
		info.bitrate = atoi(arg);
	else if (http.isHeader("x-audiocast-genre") || http.isHeader("ice-genre") || http.isHeader("icy-genre"))
	{
		info.genre.set(arg,String::T_ASCII);
		info.genre.convertTo(String::T_UNICODE);

	}else if (http.isHeader("x-audiocast-description") || http.isHeader("ice-description"))
	{
		info.desc.set(arg,String::T_ASCII);
		info.desc.convertTo(String::T_UNICODE);

	}else if (http.isHeader("Authorization"))
		http.getAuthUserPass(NULL, pwd, 0, sizeof(pwd));
	else if (http.isHeader(PCX_HS_CHANNELID))
		info.id.fromStr(arg);
	else if (http.isHeader("ice-password"))
	{
		if (pwd)
			if (strlen(arg) < 64)
				strcpy(pwd,arg);
	}else if (http.isHeader("content-type"))
	{
		if (stristr(arg,MIME_OGG))
			info.contentType = ChanInfo::T_OGG;
		else if (stristr(arg,MIME_XOGG))
			info.contentType = ChanInfo::T_OGG;

		else if (stristr(arg,MIME_MP3))
			info.contentType = ChanInfo::T_MP3;
		else if (stristr(arg,MIME_XMP3))
			info.contentType = ChanInfo::T_MP3;

		else if (stristr(arg,MIME_WMA))
			info.contentType = ChanInfo::T_WMA;
		else if (stristr(arg,MIME_WMV))
			info.contentType = ChanInfo::T_WMV;
		else if (stristr(arg,MIME_ASX))
			info.contentType = ChanInfo::T_ASX;

		else if (stristr(arg,MIME_NSV))
			info.contentType = ChanInfo::T_NSV;
		else if (stristr(arg,MIME_RAW))
			info.contentType = ChanInfo::T_RAW;

		else if (stristr(arg,MIME_MMS))
			info.srcProtocol = ChanInfo::SP_MMS;
		else if (stristr(arg,MIME_XPCP))
			info.srcProtocol = ChanInfo::SP_PCP;
		else if (stristr(arg,MIME_XPEERCAST))
			info.srcProtocol = ChanInfo::SP_PEERCAST;

		else if (stristr(arg,MIME_XSCPLS))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_PLS))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_XPLS))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_M3U))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_MPEGURL))
			info.contentType = ChanInfo::T_PLS;
		else if (stristr(arg,MIME_TEXT))
			info.contentType = ChanInfo::T_PLS;


	}

}

// -----------------------------------
void Servent::handshakeICY(Channel::SRC_TYPE type, bool isHTTP)
{
	ChanInfo info;

	HTTP http(*sock);

	// default to mp3 for shoutcast DSP (doesn`t send content-type)
	if (type == Channel::SRC_SHOUTCAST)
		info.contentType = ChanInfo::T_MP3;

	while (http.nextHeader())
	{
		LOG_DEBUG("ICY %.100s",http.cmdLine);
		readICYHeader(http, info, loginPassword.cstr(), loginPassword.MAX_LEN);
	}



	// check password before anything else, if needed
	if (!loginPassword.isSame(servMgr->password))
	{
		if (!sock->host.isLocalhost() || !loginPassword.isEmpty())
			throw HTTPException(HTTP_SC_UNAUTHORIZED,401);
	}


	// we need a valid IP address before we start
	servMgr->checkFirewall();


	// attach channel ID to name, channel ID is also encoded with IP address
	// to help prevent channel hijacking.


	info.id = chanMgr->broadcastID;
	info.id.encode(NULL,info.name.c_str(),loginMount.c_str(),info.bitrate);

	LOG_DEBUG("Incoming source: %s : %s",info.name.c_str(),ChanInfo::getTypeStr(info.contentType));


	if (isHTTP)
		sock->writeStringF("%s\n\n",HTTP_SC_OK);
	else
		sock->writeLine("OK");

	Channel *c = chanMgr->findChannelByID(info.id);
	if (c)
	{
		LOG_CHANNEL("ICY channel already active, closing old one");
		c->thread.shutdown();
	}


	info.comment = chanMgr->broadcastMsg;
	info.bcID = chanMgr->broadcastID;

	c = chanMgr->createChannel(info,loginMount.c_str());
	if (!c)
		throw HTTPException(HTTP_SC_UNAVAILABLE,503);

	c->startICY(sock,type);
}

// -----------------------------------
static string getApplicationDirectory()
{
	if (servMgr->getModulePath) //JP-EX
	{
		peercastApp->getDirectory();
		return servMgr->modulePath;
	}else
    {
		return peercastApp->getPath();
    }
}
// -----------------------------------
void Servent::handshakeLocalFile(const char *absPath)
{
    LocalFileServer lfs(getApplicationDirectory());

    lfs.request(absPath).writeToStream(*sock);
}

// -----------------------------------
void Servent::handshakeRemoteFile(const char *dirName)
{
	ClientSocket *rsock = sys->createSocket();
	if (!rsock)
		throw HTTPException(HTTP_SC_UNAVAILABLE,503);


	const char *hostName = "www.peercast.org";	// hardwired for "security"

	Host host;
	host.fromStrName(hostName,80);


	rsock->open(host);
	rsock->connect();

	HTTP rhttp(*rsock);

	rhttp.writeLineF("GET /%s HTTP/1.0",dirName)
        .writeLineF("%s %s",HTTP_HS_HOST,hostName)
        .writeLineF("%s %s",HTTP_HS_CONNECTION,"close")
        .writeLineF("%s %s",HTTP_HS_ACCEPT,"*/*");
	rhttp.writeLine("");

	String contentType;
	bool isTemplate = false;
	while (rhttp.nextHeader())
	{
		char *arg = rhttp.getArgStr();
		if (arg)
		{
			if (rhttp.isHeader("content-type"))
				contentType = arg;
		}
	}

	MemoryStream mem(100*1024);
	while (!rsock->eof())
	{
		int len=0;
		char buf[4096];
		len = rsock->readUpto(buf,sizeof(buf));
		if (len==0)
			break;
		else
			mem.write(buf,len);

	}
	rsock->close();

	int fileLen = mem.getPosition();
	mem.len = fileLen;
	mem.rewind();


	if (contentType.contains(MIME_HTML))
		isTemplate = true;

	sock->writeLine(HTTP_SC_OK)
        .writeLineF("%s %s",HTTP_HS_SERVER,PCX_AGENT)
        .writeLineF("%s %s",HTTP_HS_CACHE,"no-cache")
        .writeLineF("%s %s",HTTP_HS_CONNECTION,"close")
        .writeLineF("%s %s",HTTP_HS_CONTENT,contentType.c_str());

	sock->writeLine("");

	if (isTemplate)
	{
        Template(std::string(mem.buf, fileLen).c_str()).writeToStream(*sock);
	}else
		sock->write(mem.buf,fileLen);

	mem.free2();
}
