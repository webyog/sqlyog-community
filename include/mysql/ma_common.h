/* Copyright (C) 2013 by MontyProgram AB

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

/* defines for the libmariadb library */

#ifndef _ma_common_h
#define _ma_common_h

#include <mysql.h>
#include <hash.h>


typedef struct st_mariadb_db_driver
{
  struct st_mariadb_client_plugin_DB *plugin;
  char *name;
  void *buffer;
} MARIADB_DB_DRIVER;

struct mysql_async_context;

struct st_mysql_options_extension {
  char *plugin_dir;
  char *default_auth;
  char *ssl_crl;
  char *ssl_crlpath;
  char *server_public_key_path;
  struct mysql_async_context *async_context;
  HASH connect_attrs;
  size_t connect_attrs_len;
  void (*report_progress)(const MYSQL *mysql,
                          unsigned int stage,
                          unsigned int max_stage,
                          double progress,
                          const char *proc_info,
                          unsigned int proc_info_length);
  MARIADB_DB_DRIVER       *db_driver;
};


#endif
