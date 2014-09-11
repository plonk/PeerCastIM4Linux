#ifndef _STR_H
#define _STR_H

#include <string>
#include <string.h>

extern char *stristr(const char *s1, const char *s2);
extern char *trimstr(char *s);

// ------------------------------------
//! String class that gets allocated on the stack.
class String
{
public:
	enum {
		MAX_LEN = 256 //!< 最大バイト数
	};

	enum TYPE
	{
		T_UNKNOWN,     //!< 不明。
		T_ASCII,       //!< ASCII.
		T_HTML,        //!< HTML文字実体参照を含む。
		T_ESC,         //!< URL encoded.
		T_ESCSAFE,
		T_META,
		T_METASAFE,
		T_BASE64,      //!< Base64 encoded.
		T_UNICODE,     //!< UTF-8.
		T_UNICODESAFE,
        //JP-EX
		T_SJIS,        //!< Shift-JIS
	};

    //! デフォルトコンストラクタ。
	String() {clear();}
    //! C文字列からのコンストラクタ。
	String(const char *p, TYPE t=T_ASCII)
	{
		set(p,t);
	}

	//! Set from straight null terminated string
	void set(const char *p, TYPE t=T_ASCII)
	{
        strncpy(data,p,MAX_LEN-1);
		data[MAX_LEN-1] = 0;
		type = t;
	}

	//! Set from quoted or unquoted null terminated string
	void setFromString(const char *str, TYPE t=T_ASCII);

	//! Set from stopwatch
	void setFromStopwatch(unsigned int t);

	//! Set from time
	void setFromTime(unsigned int t);


	//! Set from single word (end at whitespace)
	void setFromWord(const char *str)
	{
		int i;
		for(i=0; i<MAX_LEN-1; i++)
		{
			data[i] = *str++;
			if ((data[i]==0) || (data[i]==' '))
				break;
		}
		data[i]=0;
	}


	//! Set from null terminated string, remove first/last chars
	void setUnquote(const char *p, TYPE t=T_ASCII)
	{
		size_t slen = strlen(p);
		if (slen > 2)
		{
			if (slen >= MAX_LEN) slen = MAX_LEN;
			strncpy(data,p+1,slen-2);
			data[slen-2]=0;
		}else
			clear();
		type = t;
	}

    //! 内容を空にする。
	void clear()
	{
		memset(data, 0, MAX_LEN);
		data[0]=0;
		type = T_UNKNOWN;
	}
    //! URLエスケープする。
	void ASCII2ESC(const char *,bool);
    //! HTMLエスケープする。
	void ASCII2HTML(const char *);
	void ASCII2META(const char *,bool);
    //! URLアンエスケープする。
	void ESC2ASCII(const char *);
	void HTML2ASCII(const char *);
	void HTML2UNICODE(const char *);
    //! Base64デコードする。
	void BASE642ASCII(const char *);
    //! 文字コードを自動判別して UTF-8 にする。
	void UNKNOWN2UNICODE(const char *,bool);
	void ASCII2SJIS(const char *); //JP-EX

	static	int	base64WordToChars(char *,const char *);

    //! 文字列比較。
	static bool isSame(const char *s1, const char *s2) {return strcmp(s1,s2)==0;}

    //! 先頭が s に等しい。
	bool startsWith(const char *s) const {return strncmp(data,s,strlen(s))==0;}
    //! URLである。
	bool isValidURL();
    //! 空である。
	bool isEmpty() {return data[0]==0;}
    //! 比較。
	bool isSame(::String &s) const {return strcmp(data,s.data)==0;}
    //! 比較。
	bool isSame(const char *s) const {return strcmp(data,s)==0;}
    //! 部分一致。大文字小文字の違いは無視される。
	bool contains(::String &s) {return stristr(data,s.data)!=NULL;}
    //! 部分一致。大文字小文字の違いは無視される。
	bool contains(const char *s) {return stristr(data,s)!=NULL;}
    //! s を自身の最後に追加する。連結すると MAX_LEN - 2 文字を超える場合は何も行わない。
	void append(const char *s)
	{
		if ((strlen(s)+strlen(data) < (MAX_LEN-1)))
			strcat(data,s);
	}
    //! c を自身の最後に追加する。連結すると MAX_LEN - 2 文字を超える場合は何も行わない。
	void append(char c)
	{
		char tmp[2];
		tmp[0]=c;
		tmp[1]=0;
		append(tmp);
	}

    //! s を自身の先頭に追加する。連結すると MAX_LEN - 2 文字を超える場合は s にセットされる。
	void prepend(const char *s)
	{
		::String tmp;
		tmp.set(s);
		tmp.append(data);
		tmp.type = type;
		*this = tmp;
	}

    //! 等しい。
	bool operator == (const char *s) const {return isSame(s);}
    //! 等しくない。
	bool operator != (const char *s) const {return !isSame(s);}

    //! const char * にキャストする。
	operator const char *() const {return data;}
    operator std::string () const { return std::string(data); }

    // String& operator = (const std::string &rhs) { set(rhs.c_str()); return *this; }

    //! タイプを変更する。
	void convertTo(TYPE t);

    //! C文字列で得る。
	char	*cstr() {return data;}

    //! 空白またはタブである。
	static bool isWhitespace(char c) {return c==' ' || c=='\t';}

    //! タイプ。
	TYPE	type;
    //! 内部表現。
	char	data[MAX_LEN];
};

#endif
