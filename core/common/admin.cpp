#include "common/admin.h"
#include "common/servmgr.h"
#include "common/peercast.h"
#include "common/version2.h"
#include "common/stats.h"
#include "common/sys.h"
#include "common/util.h"
#include <boost/format.hpp>

using namespace std;
using namespace util;

// -----------------------------------
HTTPResponse AdminController::redirect(char *cmd)
{
    String url(getCGIarg_s(cmd,"url").c_str(), String::T_ESC);
    url.convertTo(String::T_ASCII);

    if (url.isEmpty())
        return HTTPResponse(200, {{ "Content-Type", "text/html" }},
                        [=] (Stream& os)
                        {
                            HTMLBuilder(os).errorPage("Error", "Argument Format Error", "Empty URL");
                        });

    if (!url.contains("http://"))
        url.prepend("http://");

    return HTTPResponse(200, { { "Content-Type", "text/html" } },
                    [=](Stream &os)
                    {
                        HTMLBuilder hb(os);

                        hb.setRefreshURL(url.c_str());
                        hb.setTitle("Redirecting...");

                        hb.doctype();
                        hb.startHTML();
                        hb.addHead();
                        hb.startBody();
                        hb.startTagEnd("h3","Please wait...");
                        hb.end();
                        hb.end();
                    });
}

static HTTPResponse handshakeXML();

HTTPResponse AdminController::viewxml(char *cmd)
{
    return handshakeXML();
}

// -----------------------------------
static XML::Node *createChannelXML(ChanHitList *chl)
{
	XML::Node *n = chl->info.createChannelXML();
	n->add(chl->createXML());
	n->add(chl->info.createTrackXML());
	return n;
}
// -----------------------------------
static XML::Node *createChannelXML(Channel *c)
{
	XML::Node *n = c->info.createChannelXML();
	n->add(c->createRelayXML(true));
	n->add(c->info.createTrackXML());
	return n;
}
// -----------------------------------
static HTTPResponse handshakeXML()
{
	int i;
	XML *xml = new XML();

	XML::Node *rn = new XML::Node("peercast");
	xml->setRoot(rn);


	rn->add(new XML::Node("servent uptime=\"%d\"",servMgr->getUptime()));
	rn->add(new XML::Node("bandwidth out=\"%d\" in=\"%d\"",
                          stats.getPerSecond(Stats::BYTESOUT)-stats.getPerSecond(Stats::LOCALBYTESOUT),
                          stats.getPerSecond(Stats::BYTESIN)-stats.getPerSecond(Stats::LOCALBYTESIN)
                ));
	rn->add(new XML::Node("connections total=\"%d\" relays=\"%d\" direct=\"%d\"",servMgr->totalConnected(),servMgr->numStreams(Servent::T_RELAY,true),servMgr->numStreams(Servent::T_DIRECT,true)));

	XML::Node *an = new XML::Node("channels_relayed total=\"%d\"",chanMgr->numChannels());
	rn->add(an);

	Channel *c = chanMgr->channel;
	while (c)
	{
		if (c->isActive())
			an->add(createChannelXML(c));
		c=c->next;
	}

	// add public channels
	{
		XML::Node *fn = new XML::Node("channels_found total=\"%d\"",chanMgr->numHitLists());
		rn->add(fn);

		ChanHitList *chl = chanMgr->hitlist;
		while (chl)
		{
			if (chl->isUsed())
				fn->add(createChannelXML(chl));
			chl = chl->next;
		}
	}

	XML::Node *hc = new XML::Node("host_cache");
	for(i=0; i<ServMgr::MAX_HOSTCACHE; i++)
	{
		ServHost *sh = &servMgr->hostCache[i];
		if (sh->type != ServHost::T_NONE)
		{
			char ipstr[64];
			sh->host.toStr(ipstr);

			hc->add(new XML::Node("host ip=\"%s\" type=\"%s\" time=\"%d\"",ipstr,ServHost::getTypeStr(sh->type),sh->time));
		}
	}
	rn->add(hc);

	// calculate content-length
	DummyStream ds;
	xml->write(ds);

    return HTTPResponse(200,
                    {
                        { HTTP_HS_SERVER, PCX_AGENT },
                        { HTTP_HS_CONTENT, MIME_XML },
                        { HTTP_HS_LENGTH, to_string(ds.getLength()) }
                    },
                    [=](Stream &os)
                    {
                        // write HTTP body
                        xml->write(os);
                        delete xml;
                    });
}

HTTPResponse AdminController::clearlog(char *cmd)
{
    sys->logBuf->clear();

    return HTTPResponse::redirectF("/%s/viewlog.html", servMgr->htmlPath);
}

HTTPResponse AdminController::save(char *cmd)
{
    peercastInst->saveSettings();

    return HTTPResponse::redirectF("/%s/settings.html", servMgr->htmlPath);
}

HTTPResponse AdminController::reg(char *cmd)
{
    char idstr[128];
    chanMgr->broadcastID.toStr(idstr);

    return HTTPResponse::redirectF("http://www.peercast.org/register/?id=%s", idstr);
}

HTTPResponse AdminController::edit_bcid(char *cmd)
{
    char *cp = cmd;
    GnuID id;
    BCID *bcid;
    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"id")==0)
            id.fromStr(arg);
        else if (strcmp(curr,"del")==0)
            servMgr->removeValidBCID(id);
        else if (strcmp(curr,"valid")==0)
        {
            bcid = servMgr->findValidBCID(id);
            if (bcid)
                bcid->valid = getCGIargBOOL(arg);
        }
    }
    peercastInst->saveSettings();

    return HTTPResponse::redirectF("/%s/bcid.html", servMgr->htmlPath);
}

HTTPResponse AdminController::add_bcid(char *cmd)
{
    BCID *bcid = new BCID();

    char *cp = cmd;
    bool result=false;
    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"id")==0)
            bcid->id.fromStr(arg);
        else if (strcmp(curr,"name")==0)
            bcid->name.set(arg);
        else if (strcmp(curr,"email")==0)
            bcid->email.set(arg);
        else if (strcmp(curr,"url")==0)
            bcid->url.set(arg);
        else if (strcmp(curr,"valid")==0)
            bcid->valid = getCGIargBOOL(arg);
        else if (strcmp(curr,"result")==0)
            result = true;
    }

    LOG_DEBUG("Adding BCID : %s",bcid->name.c_str());
    servMgr->addValidBCID(bcid);
    peercastInst->saveSettings();
    if (result)
    {
        return HTTPResponse(200, {{ "Content-Type", "text/plain" }}, [](Stream& os) { os.writeString("OK"); });
    }else
    {
        return HTTPResponse::redirectF("/%s/bcid.html", servMgr->htmlPath);
    }
}

HTTPResponse AdminController::apply(char *cmd)
{
    HTTPResponse response = HTTPResponse::redirect("/");
    ServFilter *currFilter=servMgr->filters;
    bool beginfilt = false;

    bool brRoot=false;
    bool getUpd=false;
    int showLog=0;
    int allowServer1=0;
    int allowServer2=0;
    int newPort=servMgr->serverHost.port;
    int enableGetName = 0;
    int allowConnectPCST = 0;
    int disableAutoBumpIfDirect = 0; //JP-MOD
    int asxDetailedMode = 0; //JP-MOD

    char *cp = cmd;
    while (cp=nextCGIarg(cp,curr,arg))
    {
        LOG_DEBUG("ARG: %s = %s", curr, arg);

        // server
        if (strcmp(curr,"serveractive")==0)
            servMgr->autoServe = getCGIargBOOL(arg);
        else if (strcmp(curr,"port")==0)
            newPort = getCGIargINT(arg);
        else if (strcmp(curr,"icymeta")==0)
        {
            int iv = getCGIargINT(arg);
            if (iv < 0) iv = 0;
            else if (iv > 16384) iv = 16384;

            chanMgr->icyMetaInterval = iv;

        }else if (strcmp(curr,"passnew")==0)
            strcpy(servMgr->password,arg);
        else if (strcmp(curr,"root")==0)
            servMgr->isRoot = getCGIargBOOL(arg);
        else if (strcmp(curr,"brroot")==0)
            brRoot = getCGIargBOOL(arg);
        else if (strcmp(curr,"getupd")==0)
            getUpd = getCGIargBOOL(arg);
        else if (strcmp(curr,"huint")==0)
            chanMgr->setUpdateInterval(getCGIargINT(arg));
        else if (strcmp(curr,"forceip")==0)
            servMgr->forceIP = arg;
        else if (strcmp(curr,"htmlPath")==0)
        {
            strcpy(servMgr->htmlPath,"html/");
            strcat(servMgr->htmlPath,arg);
        }else if (strcmp(curr,"djmsg")==0)
        {
            String msg;
            msg.set(arg,String::T_ESC);
            msg.convertTo(String::T_UNICODE);
            chanMgr->setBroadcastMsg(msg);
        }
        else if (strcmp(curr,"pcmsg")==0)
        {
            servMgr->rootMsg.set(arg,String::T_ESC);
            servMgr->rootMsg.convertTo(String::T_UNICODE);
        }else if (strcmp(curr,"minpgnu")==0)
            servMgr->minGnuIncoming = atoi(arg);
        else if (strcmp(curr,"maxpgnu")==0)
            servMgr->maxGnuIncoming = atoi(arg);

        // connections
        else if (strcmp(curr,"maxcin")==0)
            servMgr->maxControl = getCGIargINT(arg);

        else if (strcmp(curr,"maxup")==0)
            servMgr->maxBitrateOut = getCGIargINT(arg);
        else if (strcmp(curr,"maxrelays")==0)
            servMgr->setMaxRelays(getCGIargINT(arg));
        else if (strcmp(curr,"maxdirect")==0)
            servMgr->maxDirect = getCGIargINT(arg);
        else if (strcmp(curr,"maxrelaypc")==0)
            chanMgr->maxRelaysPerChannel = getCGIargINT(arg);
        else if (strncmp(curr,"filt_",5)==0)
        {
            if (!beginfilt) {
                servMgr->numFilters = 0;
                beginfilt = true;
            }
            char *fs = curr+5;
            {
                if (strncmp(fs,"ip",2)==0)		// ip must be first
                {
                    currFilter = &servMgr->filters[servMgr->numFilters];
                    currFilter->init();
                    currFilter->host.fromStrIP(arg,DEFAULT_PORT);
                    if ((currFilter->host.ip) && (servMgr->numFilters < (ServMgr::MAX_FILTERS-1)))
                    {
                        servMgr->numFilters++;
                        servMgr->filters[servMgr->numFilters].init();	// clear new entry
                        LOG_DEBUG("numFilters = %d", servMgr->numFilters);
                    }

                }else if (strncmp(fs,"bn",2)==0)
                    currFilter->flags |= ServFilter::F_BAN;
                else if (strncmp(fs,"pr",2)==0)
                    currFilter->flags |= ServFilter::F_PRIVATE;
                else if (strncmp(fs,"nw",2)==0)
                    currFilter->flags |= ServFilter::F_NETWORK;
                else if (strncmp(fs,"di",2)==0)
                    currFilter->flags |= ServFilter::F_DIRECT;
            }
        }

        // client
        else if (strcmp(curr,"clientactive")==0)
            servMgr->autoConnect = getCGIargBOOL(arg);
        else if (strcmp(curr,"yp")==0)
        {
            if (!PCP_FORCE_YP)
            {
                String str(arg,String::T_ESC);
                str.convertTo(String::T_ASCII);
                servMgr->rootHost = str;
            }
        }
        else if (strcmp(curr,"yp2")==0)
        {
            if (!PCP_FORCE_YP)
            {
                String str(arg,String::T_ESC);
                str.convertTo(String::T_ASCII);
                servMgr->rootHost2 = str;
            }
        }
        else if (strcmp(curr,"deadhitage")==0)
            chanMgr->deadHitAge = getCGIargINT(arg);
        else if (strcmp(curr,"refresh")==0)
            servMgr->refreshHTML = getCGIargINT(arg);
        else if (strcmp(curr,"auth")==0)
        {
            if (strcmp(arg,"cookie")==0)
                servMgr->authType = ServMgr::AUTH_COOKIE;
            else if (strcmp(arg,"http")==0)
                servMgr->authType = ServMgr::AUTH_HTTPBASIC;

        }else if (strcmp(curr,"expire")==0)
        {
            if (strcmp(arg,"session")==0)
                servMgr->cookieList.neverExpire = false;
            else if (strcmp(arg,"never")==0)
                servMgr->cookieList.neverExpire = true;
        }

        else if (strcmp(curr,"logDebug")==0)
            showLog |= atoi(arg)?(1<<LogBuffer::T_DEBUG):0;
        else if (strcmp(curr,"logErrors")==0)
            showLog |= atoi(arg)?(1<<LogBuffer::T_ERROR):0;
        else if (strcmp(curr,"logNetwork")==0)
            showLog |= atoi(arg)?(1<<LogBuffer::T_NETWORK):0;
        else if (strcmp(curr,"logChannel")==0)
            showLog |= atoi(arg)?(1<<LogBuffer::T_CHANNEL):0;

        else if (strcmp(curr,"allowHTML1")==0)
            allowServer1 |= atoi(arg)?(Servent::ALLOW_HTML):0;
        else if (strcmp(curr,"allowNetwork1")==0)
            allowServer1 |= atoi(arg)?(Servent::ALLOW_NETWORK):0;
        else if (strcmp(curr,"allowBroadcast1")==0)
            allowServer1 |= atoi(arg)?(Servent::ALLOW_BROADCAST):0;
        else if (strcmp(curr,"allowDirect1")==0)
            allowServer1 |= atoi(arg)?(Servent::ALLOW_DIRECT):0;

        else if (strcmp(curr,"allowHTML2")==0)
            allowServer2 |= atoi(arg)?(Servent::ALLOW_HTML):0;
        else if (strcmp(curr,"allowBroadcast2")==0)
            allowServer2 |= atoi(arg)?(Servent::ALLOW_BROADCAST):0;

        // JP-EX
        else if (strcmp(curr, "autoRelayKeep") ==0)
            servMgr->autoRelayKeep = getCGIargINT(arg);
        else if (strcmp(curr, "autoMaxRelaySetting") ==0)
            servMgr->autoMaxRelaySetting = getCGIargINT(arg);
        else if (strcmp(curr, "autoBumpSkipCount") ==0)
            servMgr->autoBumpSkipCount = getCGIargINT(arg);
        else if (strcmp(curr, "kickPushStartRelays") ==0)
            servMgr->kickPushStartRelays = getCGIargINT(arg);
        else if (strcmp(curr, "kickPushInterval") ==0)
            servMgr->kickPushInterval = getCGIargINT(arg);
        else if (strcmp(curr, "allowConnectPCST") ==0)
            allowConnectPCST = atoi(arg) ? 1 : 0;
        else if (strcmp(curr, "enableGetName") ==0)
            enableGetName = atoi(arg)? 1 : 0;
        else if (strcmp(curr, "autoPort0Kick") ==0)
            servMgr->autoPort0Kick = getCGIargBOOL(arg);
        else if (strcmp(curr, "allowOnlyVP") ==0)
            servMgr->allowOnlyVP = getCGIargBOOL(arg);
        else if (strcmp(curr, "kickKeepTime") ==0)
            servMgr->kickKeepTime = getCGIargINT(arg);

        else if (strcmp(curr, "maxRelaysIndexTxt") ==0)		// for PCRaw (relay)
            servMgr->maxRelaysIndexTxt = getCGIargINT(arg);
        else if (strcmp(curr, "disableAutoBumpIfDirect") ==0) //JP-MOD
            disableAutoBumpIfDirect = atoi(arg) ? 1 : 0;
        else if (strcmp(curr, "asxDetailedMode") ==0) //JP-MOD
            asxDetailedMode = getCGIargINT(arg);
    } // while nextCGIarg

    servMgr->showLog = showLog;
    servMgr->allowServer1 = allowServer1;
    servMgr->allowServer2 = allowServer2;
    servMgr->enableGetName = enableGetName;
    servMgr->allowConnectPCST = allowConnectPCST;
    servMgr->disableAutoBumpIfDirect = disableAutoBumpIfDirect; //JP-MOD
    servMgr->asxDetailedMode = asxDetailedMode; //JP-MOD
    if (!(servMgr->allowServer1 & Servent::ALLOW_HTML) && !(servMgr->allowServer2 & Servent::ALLOW_HTML))
        servMgr->allowServer1 |= Servent::ALLOW_HTML;

    if (servMgr->serverHost.port != newPort)
    {
        Host lh(ClientSocket::getIP(NULL),newPort);
        char ipstr[64];
        lh.toStr(ipstr);

        response = HTTPResponse::redirectF("http://%s/%s/settings.html", ipstr, servMgr->htmlPath);

        servMgr->serverHost.port = newPort;
        servMgr->restartServer=true;
    }else
    {
        response = HTTPResponse::redirectF("/%s/settings.html", servMgr->htmlPath);
    }

    peercastInst->saveSettings();
    peercastApp->updateSettings();

    if ((servMgr->isRoot) && (brRoot))
        servMgr->broadcastRootSettings(getUpd);

    return response;
}

HTTPResponse AdminController::fetch(char *cmd)
{
    ChanInfo info;
    String curl;

    char *cp = cmd;
    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"url")==0)
        {
            curl.set(arg,String::T_ESC);
            curl.convertTo(String::T_UNICODE);
        }else if (strcmp(curr,"name")==0)
        {
            info.name.set(arg,String::T_ESC);
            info.name.convertTo(String::T_UNICODE);
        }else if (strcmp(curr,"desc")==0)
        {
            info.desc.set(arg,String::T_ESC);
            info.desc.convertTo(String::T_UNICODE);
        }else if (strcmp(curr,"genre")==0)
        {
            info.genre.set(arg,String::T_ESC);
            info.genre.convertTo(String::T_UNICODE);
        }else if (strcmp(curr,"contact")==0)
        {
            info.url.set(arg,String::T_ESC);
            info.url.convertTo(String::T_UNICODE);
        }else if (strcmp(curr,"bitrate")==0)
        {
            info.bitrate = atoi(arg);
        }else if (strcmp(curr,"type")==0)
        {
            info.contentType = ChanInfo::getTypeFromStr(arg);
        }else if (strcmp(curr,"bcstClap")==0) //JP-MOD
        {
            info.ppFlags |= ServMgr::bcstClap;
        }
    }

    info.bcID = chanMgr->broadcastID;

    Channel *c = chanMgr->createChannel(info,NULL);
    if (c)
        c->startURL(curl.c_str());

    return HTTPResponse::redirectF("/%s/relays.html", servMgr->htmlPath);
}

HTTPResponse AdminController::stopserv(char *cmd)
{
    char *cp = cmd;
    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"index")==0)
        {
            Servent *s = servMgr->findServentByIndex(atoi(arg));
            if (s)
                s->abort();
        }
    }
    return HTTPResponse::redirectF("/%s/connections.html", servMgr->htmlPath);
}

HTTPResponse AdminController::hitlist(char *cmd)
{

    bool stayConnected=hasCGIarg(cmd,"relay");

    int index = 0;
    ChanHitList *chl = chanMgr->hitlist;
    while (chl)
    {
        if (chl->isUsed())
        {
            if (cmpCGIarg(cmd,format("c%d=",index).c_str(),"1"))
            {
                Channel *c;
                if (!(c=chanMgr->findChannelByID(chl->info.id)))
                {
                    c = chanMgr->createChannel(chl->info,NULL);
                    if (!c)
                        throw StreamException("out of channels");
                    c->stayConnected = stayConnected;
                    c->startGet();
                }
            }
        }
        chl = chl->next;
        index++;
    }

    char *findArg = getCGIarg(cmd,"keywords="); // 使われていない

    if (hasCGIarg(cmd,"relay"))
    {
        sys->sleep(500);

        return HTTPResponse::redirectF("/%s/relays.html", servMgr->htmlPath);
    }
    return HTTPResponse::redirect("/"); // ここでいい？
}

HTTPResponse AdminController::clear(char *cmd)
{
    char *cp = cmd;

    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"hostcache")==0)
            servMgr->clearHostCache(ServHost::T_SERVENT);
        else if (strcmp(curr,"hitlists")==0)
            chanMgr->clearHitLists();
        else if (strcmp(curr,"packets")==0)
        {
            stats.clearRange(Stats::PACKETSSTART,Stats::PACKETSEND);
            servMgr->numVersions = 0;
        }
    }

    return HTTPResponse::redirectF("/%s/index.html", servMgr->htmlPath);
}

HTTPResponse AdminController::upgrade(char *cmd)
{
    if (servMgr->downloadURL[0])
    {
        return HTTPResponse::redirectF("/admin?cmd=redirect&url=%s", servMgr->downloadURL);
    }else {
        return HTTPResponse::redirect("/"); // 何かページを表示するべき？;
    }
}

HTTPResponse AdminController::connect(char *cmd)
{
    Servent *s = servMgr->servents;
    {
        if (cmpCGIarg(cmd,util::format("c%d=",s->serventIndex).c_str(),"1"))
        {
            if (hasCGIarg(cmd,"stop"))
                s->thread.active = false;
        }
        s=s->next;
    }

    return HTTPResponse::redirectF("/%s/connections.html", servMgr->htmlPath);
}

HTTPResponse AdminController::shutdown(char *cmd)
{
    servMgr->shutdownTimer = 1;
    return HTTPResponse::redirect("/"); // シャットダウンする前に何か表示するべき？
}

HTTPResponse AdminController::stop(char *cmd)
{
    GnuID id;
    char *cp = cmd;
    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"id")==0)
            id.fromStr(arg);
    }

    Channel *c = chanMgr->findChannelByID(id);
    if (c){
        c->thread.active = false;
        c->thread.finish = true;
    }

    sys->sleep(500);

    return HTTPResponse::redirectF("/%s/relays.html", servMgr->htmlPath);
}

HTTPResponse AdminController::bump(char *cmd)
{
    GnuID id;
    char *cp = cmd;
    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"id")==0)
            id.fromStr(arg);
    }

    Channel *c = chanMgr->findChannelByID(id);
    if (c)
        c->bump = true;

    return HTTPResponse::redirectF("/%s/relays.html", servMgr->htmlPath);
}

HTTPResponse AdminController::keep(char *cmd)
{
    GnuID id;
    char *cp = cmd;
    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"id")==0)
            id.fromStr(arg);
    }

    Channel *c = chanMgr->findChannelByID(id);
    if (c)
    { //JP-Patch
        c->stayConnected = !c->stayConnected;
    } //JP-Patch

    return HTTPResponse::redirectF("/%s/relays.html", servMgr->htmlPath);
}

HTTPResponse AdminController::relay(char *cmd)
{
    ChanInfo info;
    char *cp = cmd;

    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"id")==0)
            info.id.fromStr(arg);
    }

    if (!chanMgr->findChannelByID(info.id))
    {

        ChanHitList *chl = chanMgr->findHitList(info);
        if (!chl)
            throw StreamException("channel not found");


        Channel *c = chanMgr->createChannel(chl->info,NULL);
        if (!c)
            throw StreamException("out of channels");

        c->stayConnected = true;
        c->startGet();
    }

    return HTTPResponse::redirectF("/%s/relays.html", servMgr->htmlPath);
}

HTTPResponse AdminController::net_add(char *cmd)
{
    GnuID id;
    id.clear();

    while (cmd=nextCGIarg(cmd,curr,arg))
    {
        if (strcmp(curr,"ip")==0)
        {
            Host h;
            h.fromStrIP(arg,DEFAULT_PORT);
            if (servMgr->addOutgoing(h,id,true))
                LOG_NETWORK("Added connection: %s",arg);

        }else if (strcmp(curr,"id")==0)
        {
            id.fromStr(arg);
        }
    }
    return HTTPResponse::redirect("/"); // ???
}

HTTPResponse AdminController::logout(char *cmd)
{
    servMgr->cookieList.remove(servent.cookie);
    return HTTPResponse::redirect("/");
}

HTTPResponse AdminController::login(char *cmd)
{
    string idstr = GnuID().generate().str();

    servent.cookie.set(idstr.c_str(), servent.sock->host.ip);
    servMgr->cookieList.add(servent.cookie);

    string cookie;
    if (servMgr->cookieList.neverExpire)
        cookie = format("id=%s; path=/; expires=\"Mon, 01-Jan-3000 00:00:00 GMT\";", idstr);
    else
        cookie = format("id=%s; path=/;", idstr);

    return HTTPResponse(302,
                    { { HTTP_HS_SETCOOKIE, cookie }, { "Location", format("/%s/index.html", servMgr->htmlPath) } },
                    [](Stream&) {});
}

HTTPResponse AdminController::setmeta(char *cmd)
{
    string location = "/";
    char *cp = cmd;

    while (cp=nextCGIarg(cp,curr,arg))
    {
        if (strcmp(curr,"name")==0)
        {
            String chname;
            chname.set(arg,String::T_ESC);
            chname.convertTo(String::T_ASCII);

            Channel *c = chanMgr->findChannelByName(chname.c_str());
            if (c && (c->isActive()) && (c->status == Channel::S_BROADCASTING)){
                ChanInfo newInfo = c->info;
                newInfo.ppFlags = ServMgr::bcstNone; //JP-MOD
                while (cmd=nextCGIarg(cmd,curr,arg))
                {
                    String chmeta;
                    chmeta.set(arg,String::T_ESC);
                    chmeta.convertTo(String::T_ASCII);
                    if (strcmp(curr,"desc")==0)
                        newInfo.desc = chmeta.c_str();
                    else if (strcmp(curr,"url")==0)
                        newInfo.url = chmeta.c_str();
                    else if (strcmp(curr,"genre")==0)
                        newInfo.genre = chmeta.c_str();
                    else if (strcmp(curr,"comment")==0)
                        newInfo.comment = chmeta.c_str();
                    else if (strcmp(curr,"bcstClap")==0) //JP-MOD
                        newInfo.ppFlags |= ServMgr::bcstClap;
                    else if (strcmp(curr,"t_contact")==0)
                        newInfo.track.contact = chmeta.c_str();
                    else if (strcmp(curr,"t_title")==0)
                        newInfo.track.title = chmeta.c_str();
                    else if (strcmp(curr,"t_artist")==0)
                        newInfo.track.artist = chmeta.c_str();
                    else if (strcmp(curr,"t_album")==0)
                        newInfo.track.album = chmeta.c_str();
                    else if (strcmp(curr,"t_genre")==0)
                        newInfo.track.genre = chmeta.c_str();
                }
                c->updateInfo(newInfo);

                location = format("/%s/relayinfo.html?id=%s", servMgr->htmlPath, newInfo.id.str());
            }
        }
    }
    return HTTPResponse::redirect(location);
}

HTTPResponse AdminController::send(char *cmd)
{
    LOG_DEBUG("send: %s", cmd);
	try
	{
        if (cmpCGIarg(cmd,"cmd=","redirect"))
            return redirect(cmd);
        else if (cmpCGIarg(cmd,"cmd=","viewxml"))
            return viewxml(cmd);
        else if (cmpCGIarg(cmd,"cmd=","clearlog"))
            return clearlog(cmd);
        else if (cmpCGIarg(cmd,"cmd=","save"))
            return save(cmd);
        else if (cmpCGIarg(cmd,"cmd=","reg"))
            return reg(cmd);
        else if (cmpCGIarg(cmd,"cmd=","edit_bcid"))
            return edit_bcid(cmd);
        else if (cmpCGIarg(cmd,"cmd=","add_bcid"))
            return add_bcid(cmd);
        else if (cmpCGIarg(cmd,"cmd=","apply"))
            return apply(cmd);
        else if (cmpCGIarg(cmd,"cmd=","fetch"))
            return fetch(cmd);
        else if (cmpCGIarg(cmd,"cmd=","stopserv"))
            return stopserv(cmd);
        else if (cmpCGIarg(cmd,"cmd=","hitlist"))
            return hitlist(cmd);
        else if (cmpCGIarg(cmd,"cmd=","clear"))
            return clear(cmd);
        else if (cmpCGIarg(cmd,"cmd=","upgrade"))
            return upgrade(cmd);
        else if (cmpCGIarg(cmd,"cmd=","connect"))
            return connect(cmd);
        else if (cmpCGIarg(cmd,"cmd=","shutdown"))
            return shutdown(cmd);
        else if (cmpCGIarg(cmd,"cmd=","stop"))
            return stop(cmd);
        else if (cmpCGIarg(cmd,"cmd=","bump"))
            return bump(cmd);
        else if (cmpCGIarg(cmd,"cmd=","keep"))
            return keep(cmd);
        else if (cmpCGIarg(cmd,"cmd=","relay"))
            return relay(cmd);
        else if (cmpCGIarg(cmd,"net=","add"))
            return net_add(cmd);
        else if (cmpCGIarg(cmd,"cmd=","logout"))
            return logout(cmd);
        else if (cmpCGIarg(cmd,"cmd=","login"))
            return login(cmd);
        else if (cmpCGIarg(cmd,"cmd=","setmeta"))
            return setmeta(cmd);
        else
            return unknown(cmd);
	}catch(StreamException &e)
	{
		return HTTPResponse(200, {{ "Content-Type", "text/html" }},
                        [=](Stream &os)
                        {
                            HTMLBuilder hb(os);

                            hb.doctype();
                            hb.startTagEnd("title", "ERROR - %s",e.msg);
                            hb.startTagEnd("h1", "ERROR - %s",e.msg);
                            LOG_ERROR("admin: %s",e.msg);
                        });
	}
}

HTTPResponse AdminController::unknown(char *cmd)
{
    return HTTPResponse::redirectF("/%s/index.html", servMgr->htmlPath);
}
