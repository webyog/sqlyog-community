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
  | Authors:                                                             |
  +----------------------------------------------------------------------+
*/

#ifndef MYSQL_IO_H
#define MYSQL_IO_H

#ifdef _WIN32
void mysql_io_win_init(void);
#endif

MYSQL_STREAM * mysql_io_open(const char *name, size_t namelen);  
size_t	mysql_io_read(MYSQL_STREAM *stream, char *buf, size_t size);
size_t	mysql_io_write(MYSQL_STREAM *stream, const char *buf, size_t count);
void	mysql_io_close(MYSQL_STREAM *stream);

#endif /* MYSQLND_IO_H */
