/* Copyright (C) 2010 - 2012 Sergei Golubchik and Monty Program Ab
                 2014 MariaDB Corporation AB

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
   
   You should have received a copy of the GNU Library General Public
   License along with this library; if not see <http://www.gnu.org/licenses>
   or write to the Free Software Foundation, Inc., 
   51 Franklin St., Fifth Floor, Boston, MA 02110, USA */

/**
  @file

  MySQL Client Plugin API

  This file defines the API for plugins that work on the client side
*/
#ifndef MYSQL_CLIENT_PLUGIN_INCLUDED
#define MYSQL_CLIENT_PLUGIN_INCLUDED

#ifndef MYSQL_ABI_CHECK
#include <stdarg.h>
#include <stdlib.h>
#endif

#ifndef PLUGINDIR
#define PLUGINDIR "lib/plugin"
#endif

/* known plugin types */
#define MYSQL_CLIENT_DB_PLUGIN               0
#define MYSQL_CLIENT_reserved                1
#define MYSQL_CLIENT_AUTHENTICATION_PLUGIN   2

#define MYSQL_CLIENT_AUTHENTICATION_PLUGIN_INTERFACE_VERSION  0x0100
#define MYSQL_CLIENT_DB_PLUGIN_INTERFACE_VERSION  0x0100

#define MYSQL_CLIENT_MAX_PLUGINS             3

#define mysql_declare_client_plugin(X)          \
     struct st_mysql_client_plugin_ ## X        \
        _mysql_client_plugin_declaration_ = {   \
          MYSQL_CLIENT_ ## X ## _PLUGIN,        \
          MYSQL_CLIENT_ ## X ## _PLUGIN_INTERFACE_VERSION,
#define mysql_end_client_plugin             }

/* generic plugin header structure */
#define MYSQL_CLIENT_PLUGIN_HEADER                      \
  int type;                                             \
  unsigned int interface_version;                       \
  const char *name;                                     \
  const char *author;                                   \
  const char *desc;                                     \
  unsigned int version[3];                              \
  int (*init)(char *, size_t, int, va_list);            \
  int (*deinit)(void);

struct st_mysql_client_plugin
{
  MYSQL_CLIENT_PLUGIN_HEADER
};

struct st_mysql;

/********* database api plugin specific declarations **********/
typedef struct st_mariadb_client_plugin_DB
{
  MYSQL_CLIENT_PLUGIN_HEADER
  /* functions */
  struct st_mysql_methods *methods;
  /*
  MYSQL * (*db_connect)(MYSQL *mysql,const char *host, const char *user,
		                 const char *passwd, const char *db, uint port,
                     const char *unix_socket,unsigned long client_flag);
  void (*db_close)(MYSQL *mysql);
  int (*db_query)(MYSQL *mysql, const char *query, size_t query_len);
  int (*db_read_one_row)(MYSQL *mysql, uint fields, MYSQL_ROW row, 
                         ulong *lengths);
  MYSQL_DATA *(*db_read_all_rows)(MYSQL *mysql, 
                                  MYSQL_FIELD *mysql_fields, uint fields);
  void (*db_query_end)(MYSQL *mysql);
  int (*db_stmt_prepare)(MYSQL_STMT *stmt, const char *stmt_str, ulong length);
  my_bool (*db_stmt_close)(MYSQL_STMT *stmt);
  my_bool (*is_supported_buffer_type)(enum enum_field_types type);
  int (*db_stmt_fetch)(MYSQL_STMT *stmt);
  int (*db_stmt_execute)(MYSQL_STMT *stmt); */
} MARIADB_DB_PLUGIN;

#define MARIADB_DB_DRIVER(a) ((a)->ext_db)

/******** authentication plugin specific declarations *********/
#include <mysql/plugin_auth_common.h>

struct st_mysql_client_plugin_AUTHENTICATION
{
  MYSQL_CLIENT_PLUGIN_HEADER
  int (*authenticate_user)(MYSQL_PLUGIN_VIO *vio, struct st_mysql *mysql);
};

/**
  type of the mysql_authentication_dialog_ask function

  @param mysql          mysql
  @param type           type of the input
                        1 - ordinary string input
                        2 - password string
  @param prompt         prompt
  @param buf            a buffer to store the use input
  @param buf_len        the length of the buffer

  @retval               a pointer to the user input string.
                        It may be equal to 'buf' or to 'mysql->password'.
                        In all other cases it is assumed to be an allocated
                        string, and the "dialog" plugin will free() it.
*/
typedef char *(*mysql_authentication_dialog_ask_t)(struct st_mysql *mysql,
                      int type, const char *prompt, char *buf, int buf_len);
/******** using plugins ************/

/**
  loads a plugin and initializes it

  @param mysql  MYSQL structure. only MYSQL_PLUGIN_DIR option value is used,
                and last_errno/last_error, for error reporting
  @param name   a name of the plugin to load
  @param type   type of plugin that should be loaded, -1 to disable type check
  @param argc   number of arguments to pass to the plugin initialization
                function
  @param ...    arguments for the plugin initialization function

  @retval
  a pointer to the loaded plugin, or NULL in case of a failure
*/
struct st_mysql_client_plugin * STDCALL
mysql_load_plugin(struct st_mysql *mysql, const char *name, int type,
                  int argc, ...);

/**
  loads a plugin and initializes it, taking va_list as an argument

  This is the same as mysql_load_plugin, but take va_list instead of
  a list of arguments.

  @param mysql  MYSQL structure. only MYSQL_PLUGIN_DIR option value is used,
                and last_errno/last_error, for error reporting
  @param name   a name of the plugin to load
  @param type   type of plugin that should be loaded, -1 to disable type check
  @param argc   number of arguments to pass to the plugin initialization
                function
  @param args   arguments for the plugin initialization function

  @retval
  a pointer to the loaded plugin, or NULL in case of a failure
*/
struct st_mysql_client_plugin * STDCALL
mysql_load_plugin_v(struct st_mysql *mysql, const char *name, int type,
                    int argc, va_list args);

/**
  finds an already loaded plugin by name, or loads it, if necessary

  @param mysql  MYSQL structure. only MYSQL_PLUGIN_DIR option value is used,
                and last_errno/last_error, for error reporting
  @param name   a name of the plugin to load
  @param type   type of plugin that should be loaded

  @retval
  a pointer to the plugin, or NULL in case of a failure
*/
struct st_mysql_client_plugin * STDCALL
mysql_client_find_plugin(struct st_mysql *mysql, const char *name, int type);

/**
  adds a plugin structure to the list of loaded plugins

  This is useful if an application has the necessary functionality
  (for example, a special load data handler) statically linked into
  the application binary. It can use this function to register the plugin
  directly, avoiding the need to factor it out into a shared object.

  @param mysql  MYSQL structure. It is only used for error reporting
  @param plugin an st_mysql_client_plugin structure to register

  @retval
  a pointer to the plugin, or NULL in case of a failure
*/
struct st_mysql_client_plugin * STDCALL
mysql_client_register_plugin(struct st_mysql *mysql,
                             struct st_mysql_client_plugin *plugin);

extern struct st_mysql_client_plugin *mysql_client_builtins[];

#endif


