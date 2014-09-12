#include "common/template.h"
#include "common/servmgr.h"
#include "common/channel.h"
#include "common/stats.h"
#include "common/cgi.h"

// --------------------------------------
Template::Template(const char *fileName, const char *args)
    : out(NULL), tmplArgs(args)
{
    file.openReadOnly(fileName);
}
// --------------------------------------
Template::~Template()
{
	file.close();
}

void Template::writeToStream(Stream &output)
{
    out = &output;
    readTemplate(file,out,0);
}

// --------------------------------------
void Template::writeVariable(Stream &s,const String &varName, int loop)
{
	bool r=false;
	if (varName.startsWith("servMgr."))
		r=servMgr->writeVariable(s,varName+8);
	else if (varName.startsWith("chanMgr."))
		r=chanMgr->writeVariable(s,varName+8,loop);
	else if (varName.startsWith("stats."))
		r=stats.writeVariable(s,varName+6);
	else if (varName.startsWith("sys."))
	{
		if (varName == "sys.log.dumpHTML")
		{
			sys->logBuf->dumpHTML(s);
			r=true;
		}
	}
	else if (varName.startsWith("loop."))
	{
		if (varName.startsWith("loop.channel."))
		{
			Channel *ch = chanMgr->findChannelByIndex(loop);
			if (ch)
				r = ch->writeVariable(s,varName+13,loop);
		}else if (varName.startsWith("loop.servent."))
		{
			Servent *sv = servMgr->findServentByIndex(loop);
			if (sv)
				r = sv->writeVariable(s,varName+13);
		}else if (varName.startsWith("loop.filter."))
		{
			ServFilter *sf = &servMgr->filters[loop];
			r = sf->writeVariable(s,varName+12);

		}else if (varName.startsWith("loop.bcid."))
		{
			BCID *bcid = servMgr->findValidBCID(loop);
			if (bcid)
				r = bcid->writeVariable(s,varName+10);

		}else if (varName == "loop.indexEven")
		{
			s.writeStringF("%d",(loop&1)==0);
			r = true;
		}else if (varName == "loop.index")
		{
			s.writeStringF("%d",loop);
			r = true;
		}else if (varName.startsWith("loop.hit."))
		{
			const char *idstr = getCGIarg(tmplArgs,"id=");
			if (idstr)
			{
				GnuID id;
				id.fromStr(idstr);
				ChanHitList *chl = chanMgr->findHitListByID(id);
				if (chl)
				{
					int cnt=0;
					ChanHit *ch = chl->hit;
					while (ch)
					{
						if (ch->host.ip && !ch->dead)
						{
							if (cnt == loop)
							{
								r = ch->writeVariable(s,varName+9);
								break;
							}
							cnt++;
						}
						ch=ch->next;
					}

				}
			}
		}

	}
	else if (varName.startsWith("page."))
	{
		if (varName.startsWith("page.channel."))
		{
			const char *idstr = getCGIarg(tmplArgs,"id=");
			if (idstr)
			{
				GnuID id;
				id.fromStr(idstr);
				Channel *ch = chanMgr->findChannelByID(id);
				if (ch)
					r = ch->writeVariable(s,varName+13,loop);
			}
		}else
		{

			String v = varName+5;
			v.append('=');
			const char *a = getCGIarg(tmplArgs,v);
			if (a)
			{
				s.writeString(a);
				r=true;
			}
		}
	}


	if (!r)
		s.writeString(varName);
}
// --------------------------------------
int Template::getIntVariable(const String &varName,int loop)
{
	String val;
	LOG_DEBUG("AAA %d %d %d %d", val[0], val[1], val[2], val[3]);
	MemoryStream mem(val.cstr(),String::MAX_LEN);

	writeVariable(mem,varName,loop);

	LOG_DEBUG("AAA %d %d %d %d", val[0], val[1], val[2], val[3]);
	return atoi(val.c_str());
}
// --------------------------------------
bool Template::getBoolVariable(const String &varName,int loop)
{
	String val;
	MemoryStream mem(val.cstr(),String::MAX_LEN);

	writeVariable(mem,varName,loop);

	String tmp;
	tmp = varName;
	LOG_DEBUG("*** %s : %c", tmp.c_str(), val[0]);

	// integer
	if ((val[0] >= '0') && (val[0] <= '9'))
		return atoi(val.c_str()) != 0;

	// string
	if (val[0]!=0)
		return true;

	return false;
}

// --------------------------------------
void	Template::readIf(Stream &in,Stream *outp,int loop)
{
	String var;
	bool varCond=true;

	while (!in.eof())
	{
		char c = in.readChar();

		if (c == '}')
		{
			if (getBoolVariable(var,loop)==varCond)
			{
				LOG_DEBUG("==varCond, loop = %d", loop);
				if (readTemplate(in,outp,loop))
					readTemplate(in,NULL,loop);
			}else{
				LOG_DEBUG("!=varCond, loop = %d", loop);
				if (readTemplate(in,NULL,loop))
					readTemplate(in,outp,loop);
			}
			return;
		}else if (c == '!')
		{
			varCond = !varCond;
		}else
		{
			var.append(c);
		}
	}

}

// --------------------------------------
void	Template::readLoop(Stream &in,Stream *outp,int loop)
{
	String var;
	while (!in.eof())
	{
		char c = in.readChar();

		if (c == '}')
		{
			int cnt = getIntVariable(var,loop);

			LOG_DEBUG("loop_cnt : %s = %d", var.c_str(), cnt);

			if (cnt)
			{
				int spos = in.getPosition();
				for(int i=0; i<cnt; i++)
				{
					in.seekTo(spos);
					readTemplate(in,outp,i);
				}
			}else
			{
				readTemplate(in,NULL,0);
			}
			return;

		}else
		{
			var.append(c);
		}
	}

}

// --------------------------------------
int Template::readCmd(Stream &in,Stream *outp,int loop)
{
	String cmd;

	int tmpl = TMPL_UNKNOWN;

	while (!in.eof())
	{
		char c = in.readChar();

		if (String::isWhitespace(c) || (c=='}'))
		{
			if (cmd == "loop")
			{
				readLoop(in,outp,loop);
				tmpl = TMPL_LOOP;
			}else if (cmd == "if")
			{
				readIf(in,outp,loop);
				tmpl = TMPL_IF;
			}else if (cmd == "end")
			{
				tmpl = TMPL_END;
			}
			else if (cmd == "else")
			{
				tmpl = TMPL_ELSE;
			}
			break;
		}else
		{
			cmd.append(c);
		}
	}
	return tmpl;
}

// --------------------------------------
void Template::readVariable(Stream &in,Stream *outp,int loop)
{
	String var;
	while (!in.eof())
	{
		char c = in.readChar();
		if (c == '}')
		{
			if (outp)
				writeVariable(*outp,var,loop);
			return;
		}else
		{
			var.append(c);
		}
	}

}
// --------------------------------------
bool Template::readTemplate(Stream &in,Stream *outp,int loop)
{
	while (!in.eof())
	{
		char c = in.readChar();

		if (c == '{')
		{
			c = in.readChar();
			if (c == '$')
			{
				readVariable(in,outp,loop);
			}
			else if (c == '@')
			{
				int t = readCmd(in,outp,loop);
				if (t == TMPL_END)
					return false;
				else if (t == TMPL_ELSE)
					return true;
			}
			else if (c == '{')
			{
				if (outp)
					outp->writeChar(c);
			}
			else
				throw StreamException("Unknown template escape");
		}else
		{
			if (outp)
				outp->writeChar(c);
		}
	}
	return false;
}
