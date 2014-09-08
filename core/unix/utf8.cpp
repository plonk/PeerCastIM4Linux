/*
 * Copyright (C) 2001 Peter Harris <peter.harris@hummingbird.com>
 * Copyright (C) 2001 Edmund Grimley Evans <edmundo@rano.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iconv.h>

#include "common/utf8.h"

static int convert(iconv_t cd, const char *inbuf, char **to)
{
    size_t inlen = strlen(inbuf);
    size_t tolen = inlen * 3 + 1;
    char* tobuf = static_cast<char*>(malloc(tolen));
    char* head = tobuf;
    size_t ret = iconv(cd, const_cast<char**>(&inbuf), &inlen, &tobuf, &tolen);
    if (ret == (size_t) -1)
    {
        perror("iconv");
        return -1;
    }
    *tobuf = '\0';
    *to = static_cast<char*>( realloc(head, strlen(head) + 1) );

    return 0;
}

int utf8_encode(const char *inbuf, char **to)
{
    iconv_t cd = iconv_open("UTF-8", "CP932");

    if (cd == (iconv_t) -1)
    {
        perror("iconv_open");
        return -1;
    }
    int ret = convert(cd, inbuf, to);

    iconv_close(cd);

    return ret;
}

int utf8_decode(const char *inbuf, char **to)
{
    iconv_t cd = iconv_open("CP932", "UTF-8");

    if (cd == (iconv_t) -1)
    {
        perror("iconv_open");
        return -1;
    }
    int ret = convert(cd, inbuf, to);

    iconv_close(cd);

    return 0;
}
