// ------------------------------------------------
// File : sys.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//		Sys is a base class for all things systemy, like starting threads, creating sockets etc..
//		Lock is a very basic cross platform CriticalSection class
//		SJIS-UTF8 conversion by ????
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

#include <string>
#include "common/util.h"
#include "common/common.h"
#include "common/sys.h"
#include "common/socket.h"
#include "common/gnutella.h"
#include <stdlib.h>
#include <time.h>
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

// -----------------------------------
const char *LogBuffer::logTypes[]=
{
	"",
	"DBUG",
	"EROR",
	"GNET",
	"CHAN",
};

// ------------------------------------------
Sys::Sys()
{
	idleSleepTime = 10;
	logBuf = new LogBuffer(1000,100);
	numThreads=0;
}

// ------------------------------------------
void Sys::sleepIdle()
{
	sleep(idleSleepTime);
}

// ------------------------------------------
bool Host::isLocalhost()
{
	return loopbackIP() || (ip == ClientSocket::getIP(NULL));
}
// ------------------------------------------
void Host::fromStrName(const char *str, int p)
{
	if (!strlen(str))
	{
		port = 0;
		ip = 0;
		return;
	}

	char name[128];
	strncpy(name,str,sizeof(name)-1);
	name[127] = '\0';
	port = p;
	char *pp = strstr(name,":");
	if (pp)
	{
		port = atoi(pp+1);
		pp[0] = 0;
	}

	ip = ClientSocket::getIP(name);
}
// ------------------------------------------
void Host::fromStrIP(const char *str, int p)
{
	unsigned int ipb[4];
	unsigned int ipp;


	if (strstr(str,":"))
	{
		if (sscanf(str,"%03d.%03d.%03d.%03d:%d",&ipb[0],&ipb[1],&ipb[2],&ipb[3],&ipp) == 5)
		{
			ip = ((ipb[0]&0xff) << 24) | ((ipb[1]&0xff) << 16) | ((ipb[2]&0xff) << 8) | ((ipb[3]&0xff));
			port = ipp;
		}else
		{
			ip = 0;
			port = 0;
		}
	}else{
		port = p;
		if (sscanf(str,"%03d.%03d.%03d.%03d",&ipb[0],&ipb[1],&ipb[2],&ipb[3]) == 4)
			ip = ((ipb[0]&0xff) << 24) | ((ipb[1]&0xff) << 16) | ((ipb[2]&0xff) << 8) | ((ipb[3]&0xff));
		else
			ip = 0;
	}
}
// -----------------------------------
bool Host::isMemberOf(Host &h)
{
	if (h.ip==0)
		return false;

    if( h.ip0() != 255 && ip0() != h.ip0() )
        return false;
    if( h.ip1() != 255 && ip1() != h.ip1() )
        return false;
    if( h.ip2() != 255 && ip2() != h.ip2() )
        return false;
    if( h.ip3() != 255 && ip3() != h.ip3() )
        return false;

/* removed for endieness compatibility
	for(int i=0; i<4; i++)
		if (h.ipByte[i] != 255)
			if (ipByte[i] != h.ipByte[i])
				return false;
*/
	return true;
}

// -----------------------------------
char *trimstr(char *s1)
{
	while (*s1)
	{
		if ((*s1 == ' ') || (*s1 == '\t'))
			s1++;
		else
			break;

	}

	char *s = s1;

	if(strlen(s1) > 0) {
/*	s1 = s1+strlen(s1);

	while (*--s1)
		if ((*s1 != ' ') && (*s1 != '\t'))
			break;*/

		s1 = s1+strlen(s1);

//	s1[1] = 0;

		while (*--s1)
			if ((*s1 != ' ') && (*s1 != '\t'))
				break;

		s1[1] = 0;
	}
	return s;
}

// -----------------------------------
char *stristr(const char *s1, const char *s2)
{
	while (*s1)
	{
		if (TOUPPER(*s1) == TOUPPER(*s2))
		{
			const char *c1 = s1;
			const char *c2 = s2;

			while (*c1 && *c2)
			{
				if (TOUPPER(*c1) != TOUPPER(*c2))
					break;
				c1++;
				c2++;
			}
			if (*c2==0)
				return (char *)s1;
		}

		s1++;
	}
	return NULL;
}
// -----------------------------------
void LogBuffer::write(const char *str, TYPE t)
{
	lock.on();

	size_t len = strlen(str);
	int cnt=0;
	while (len)
	{
		size_t rlen = len;
		if (rlen > (lineLen-1))
			rlen = lineLen-1;

		int i = currLine % maxLines;
		int bp = i*lineLen;
		strncpy(&buf[bp],str,rlen);
		buf[bp+rlen] = 0;
		if (cnt==0)
		{
			times[i] = sys->getTime();
			types[i] = t;
		}else
		{
			times[i] = 0;
			types[i] = T_NONE;
		}
		currLine++;

		str += rlen;
		len -= rlen;
		cnt++;
	}

	lock.off();
}

// -----------------------------------
char *getCGIarg(const char *str, const char *arg)
{
	if (!str)
		return NULL;

	const char *s = strstr(str,arg);

	if (!s)
		return NULL;

	s += strlen(arg);

	return const_cast<char*>(s);
}

// -----------------------------------
bool cmpCGIarg(char *str, const char *arg, const char *value)
{
	if ((!str) || (!strlen(value)))
		return false;

	if (strnicmp(str,arg,strlen(arg)) == 0)
	{

		str += strlen(arg);

		return strncmp(str,value,strlen(value))==0;
	}else
		return false;
}
// -----------------------------------
bool hasCGIarg(const char *str, const char *arg)
{
	if (!str)
		return false;

	const char *s = strstr(str,arg);

	if (!s)
		return false;

	return true;
}


// ---------------------------
void GnuID::encode(Host *h, const char *salt1, const char *salt2, unsigned char salt3)
{
	int s1=0,s2=0;
	for(int i=0; i<16; i++)
	{
		unsigned char ipb = id[i];

		// encode with IP address
		if (h)
			ipb ^= ((unsigned char *)&h->ip)[i&3];

		// add a bit of salt
		if (salt1)
		{
			if (salt1[s1])
				ipb ^= salt1[s1++];
			else
				s1=0;
		}

		// and some more
		if (salt2)
		{
			if (salt2[s2])
				ipb ^= salt2[s2++];
			else
				s2=0;
		}

		// plus some pepper
		ipb ^= salt3;

		id[i] = ipb;
	}

}
// ---------------------------
void GnuID::toStr(char *str)
{

	str[0] = 0;
	for(int i=0; i<16; i++)
	{
		char tmp[8];
		unsigned char ipb = id[i];

		sprintf(tmp,"%02X",ipb);
		strcat(str,tmp);
	}
}
// ---------------------------
std::string GnuID::str()
{
    std::string ret;

    for (int i = 0; i < 16; i++)
        ret += util::format("%02X", id[i]);

    return ret;
}
// ---------------------------
void GnuID::fromStr(const char *str)
{
	clear();

	if (strlen(str) < 32)
		return;

	char buf[8];

	buf[2] = 0;

	for(int i=0; i<16; i++)
	{
		buf[0] = str[i*2];
		buf[1] = str[i*2+1];
		id[i] = (unsigned char)strtoul(buf,NULL,16);
	}

}

// ---------------------------
GnuID& GnuID::generate(unsigned char flags)
{
	clear();

	for(int i=0; i<16; i++)
		id[i] = sys->rnd();

	id[0] = flags;

    return *this;
}

// ---------------------------
unsigned char GnuID::getFlags()
{
	return id[0];
}

// ---------------------------
GnuIDList::GnuIDList(int max)
:ids(new GnuID[max])
{
	maxID = max;
	for(int i=0; i<maxID; i++)
		ids[i].clear();
}
// ---------------------------
GnuIDList::~GnuIDList()
{
	delete [] ids;
}
// ---------------------------
bool GnuIDList::contains(GnuID &id)
{
	for(int i=0; i<maxID; i++)
		if (ids[i].isSame(id))
			return true;
	return false;
}
// ---------------------------
int GnuIDList::numUsed()
{
	int cnt=0;
	for(int i=0; i<maxID; i++)
		if (ids[i].storeTime)
			cnt++;
	return cnt;
}
// ---------------------------
unsigned int GnuIDList::getOldest()
{
	unsigned int t=(unsigned int)-1;
	for(int i=0; i<maxID; i++)
		if (ids[i].storeTime)
			if (ids[i].storeTime < t)
				t = ids[i].storeTime;
	return t;
}
// ---------------------------
void GnuIDList::add(GnuID &id)
{
	unsigned int minTime = (unsigned int) -1;
	int minIndex = 0;

	// find same or oldest
	for(int i=0; i<maxID; i++)
	{
		if (ids[i].isSame(id))
		{
			ids[i].storeTime = sys->getTime();
			return;
		}
		if (ids[i].storeTime <= minTime)
		{
			minTime = ids[i].storeTime;
			minIndex = i;
		}
	}

	ids[minIndex] = id;
	ids[minIndex].storeTime = sys->getTime();
}
// ---------------------------
void GnuIDList::clear()
{
	for(int i=0; i<maxID; i++)
		ids[i].clear();
}

// ---------------------------
void LogBuffer::dumpHTML(Stream &out)
{
	WLockBlock lb(&lock);
	lb.on();

	unsigned int nl = currLine;
	unsigned int sp = 0;
	if (nl > maxLines)
	{
		nl = maxLines-1;
		sp = (currLine+1)%maxLines;
	}

	String tim,str;
	if (nl)
	{
		for(unsigned int i=0; i<nl; i++)
		{
			unsigned int bp = sp*lineLen;

			if (types[sp])
			{
				tim.setFromTime(times[sp]);

				out.writeString(tim.cstr());
				out.writeString(" <b>[");
				out.writeString(getTypeStr(types[sp]));
				out.writeString("]</b> ");
			}
			str.set(&buf[bp]);
			str.convertTo(String::T_HTML);

			out.writeString(str.cstr());
			out.writeString("<br>");

			sp++;
			sp %= maxLines;
		}
	}

	lb.off();

}

// ---------------------------
void	ThreadInfo::shutdown()
{
	active = false;
	//sys->waitThread(this);
}
