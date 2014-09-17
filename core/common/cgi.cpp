#include <string.h>
#include "common/str.h"
#include "common/cgi.h"
#include "common/util.h"

using namespace std;
using namespace util;

// -----------------------------------
std::string getCGIarg_s(std::string query, std::string name)
{
    for (string def : split(query, "&"))
    {
        auto equation = split(def, "=");

        if (equation.size() == 1)
            equation.push_back("");

        if (equation.size() == 2)
        {
            if (equation[0] == name)
                return equation[1];
        } else {
            throw runtime_error(format("format error: %s", def));
        }
    }
    return "";
}

// -----------------------------------
const char *getCGIarg(const char *str, const char *arg)
{
	if (!str)
		return NULL;

	const char *s = strstr(str,arg);

	if (!s)
		return NULL;

	s += strlen(arg);

	return s;
}

// -----------------------------------
char *getCGIarg(char *str, const char *arg)
{
    return const_cast<char *>(getCGIarg(const_cast<const char*>(str), arg));
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

// -----------------------------------
bool getCGIargBOOL(char *a)
{
	return (strcmp(a,"1")==0);
}
// -----------------------------------
int getCGIargINT(char *a)
{
	return atoi(a);
}

// -----------------------------------
char *nextCGIarg(char *cp, char *cmd, char *arg)
{
	if (!*cp)
		return NULL;

	int cnt=0;

	// fetch command
	while (*cp)
	{
		char c = *cp++;
		if (c == '=')
			break;
		else
			*cmd++ = c;

		cnt++;
		if (cnt >= (MAX_CGI_LEN-1))
			break;
	}
	*cmd = 0;

	cnt=0;
	// fetch arg
	while (*cp)
	{
		char c = *cp++;
		if (c == '&')
			break;
		else
			*arg++ = c;

		cnt++;
		if (cnt >= (MAX_CGI_LEN-1))
			break;
	}
	*arg = 0;

	return cp;
}
