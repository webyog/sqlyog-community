/* Copyright (C) 2000 MySQL AB & MySQL Finland AB & TCX DataKonsult AB
   
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA */

/*
  A better inplementation of the UNIX ctype(3) library.
  Notes:   my_global.h should be included before ctype.h
*/

#ifndef _m_ctype_h
#define _m_ctype_h

#include <ctype.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define CHARSET_DIR	"charsets/"
#define MY_CS_NAME_SIZE 32

#define MADB_DEFAULT_CHARSET_NAME "latin1"
#define MADB_DEFAULT_COLLATION_NAME "latin1_swedish_ci"

/* we use the mysqlnd implementation */
typedef struct charset_info_st
{
  unsigned int	nr; /* so far only 1 byte for charset */
  unsigned int  state;
  char	*csname;
  char	*name;
  char  *dir;
  unsigned int codepage;
  char  *encoding;
  unsigned int	char_minlen;
  unsigned int	char_maxlen;
  unsigned int 	(*mb_charlen)(unsigned int c);
  unsigned int 	(*mb_valid)(const char *start, const char *end);
} CHARSET_INFO;

extern const CHARSET_INFO  compiled_charsets[];
extern CHARSET_INFO *default_charset_info;
extern CHARSET_INFO *my_charset_bin;
extern CHARSET_INFO *my_charset_latin1;
extern CHARSET_INFO *my_charset_utf8_general_ci;

CHARSET_INFO *find_compiled_charset(unsigned int cs_number);
CHARSET_INFO *find_compiled_charset_by_name(const char *name);

size_t mysql_cset_escape_quotes(const CHARSET_INFO *cset, char *newstr,  const char *escapestr, size_t escapestr_len);
size_t mysql_cset_escape_slashes(const CHARSET_INFO *cset, char *newstr, const char *escapestr, size_t escapestr_len);
char* madb_get_os_character_set(void);
#ifdef _WIN32
int madb_get_windows_cp(const char *charset);
#endif

#ifdef	__cplusplus
}
#endif

#endif
