#ifndef _TEMPLATE_H
#define _TEMPLATE_H

#include "common/stream.h"

// ---------------------------------------
//! Template engine.
class Template
{
public:
	enum
	{
		TMPL_UNKNOWN,
		TMPL_LOOP,
		TMPL_IF,
		TMPL_ELSE,
		TMPL_END
	};

	Template(const char *fileName, const char *args = NULL);
	~Template();

	void	writeVariable(Stream &,const String &,int);
	int		getIntVariable(const String &,int);
	bool	getBoolVariable(const String &,int);

	void	readIf(Stream &,Stream *,int);
	void	readLoop(Stream &,Stream *,int);
	void	readVariable(Stream &,Stream *,int);
	bool	readTemplate(Stream &,Stream *,int);
	int		readCmd(Stream &,Stream *,int);
    void	writeToStream(Stream &os);

	const char *tmplArgs;
    FileStream file;
	Stream *out;
};

#endif
