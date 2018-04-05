/* Copyright Abandoned 1996, 1999, 2001 MySQL AB
   This file is public domain and comes with NO WARRANTY of any kind */

/* Version numbers for protocol & mysqld */

#ifdef _CUSTOMCONFIG_
#include <custom_conf.h>
#else
#define PROTOCOL_VERSION		10
#define MYSQL_CLIENT_VERSION		"5.5.1"
#define MYSQL_SERVER_VERSION		"5.5.1"
#define MYSQL_SERVER_SUFFIX		""
#define FRM_VER				
#define MYSQL_VERSION_ID		50501
#define MYSQL_PORT			3306
#define MYSQL_UNIX_ADDR			"/tmp/mysql.sock"
#define MYSQL_CONFIG_NAME		"my"

#define MARIADB_PACKAGE_VERSION "2.3.3"
#define MARIADB_SYSTEM_TYPE "Windows"
#define MARIADB_MACHINE_TYPE "AMD64"

/* mysqld compile time options */
#ifndef MYSQL_CHARSET
#define MYSQL_CHARSET			""
#endif
#endif

