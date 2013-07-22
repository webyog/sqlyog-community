/*
  +----------------------------------------------------------------------+
  | PHP Version 6                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 2006-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Authors: Georg Richter <georg@mysql.com>                             |
  |          Andrey Hristov <andrey@mysql.com>                           |
  |          Ulf Wendel <uwendel@mysql.com>                              |
  +----------------------------------------------------------------------+
*/

#ifndef MYSQLND_MM_H
#define MYSQLND_MM_H
#include <stdlib.h>


char * mnd_strndup(const char *s, size_t length);
char * mnd_strdup(const char *src);


#define mnd_malloc(size)				malloc((size))
#define mnd_calloc(nmemb, size)			calloc((nmemb), (size))
#define mnd_realloc(ptr, new_size)		realloc((ptr), (new_size))
#define mnd_free(ptr)					free((ptr))

#endif /* MYSQLND_MM_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
