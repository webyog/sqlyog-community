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

/* $Id: mysql_wireprotocol.h,v 1.4.2.2 2007/10/05 21:23:56 andrey Exp $ */

#ifndef MYSQL_WIREPROTOCOL_H
#define MYSQL_WIREPROTOCOL_H

#define MYSQL_HEADER_SIZE 4

#define MYSQL_NULL_LENGTH	(unsigned long) ~0

typedef unsigned char	mysql_1b;
typedef unsigned short	mysql_2b;
typedef unsigned int	mysql_4b;

/* Used in mysql_debug.c */
extern char * mysql_read_header_name;
extern char * mysql_read_body_name;


/* Packet handling */
#define PACKET_INIT(packet, enum_type, c_type)  \
	{ \
		packet = (c_type) my_mcalloc( packet_methods[enum_type].struct_size, MYF(MY_WME | MY_ZEROFILL)); \
		((c_type) (packet))->header.m = &packet_methods[enum_type]; \
	}
#define PACKET_WRITE(packet, conn)	((packet)->header.m->write_to_net((packet), (conn)))
#define PACKET_READ(packet, conn)	((packet)->header.m->read_from_net((packet), (conn)))
#define PACKET_FREE(packet) 		((packet)->header.m->free_mem((packet), FALSE))

#define PACKET_INIT_ALLOCA(packet, enum_type)  \
	{ \
		memset(&(packet), 0, packet_methods[enum_type].struct_size); \
		(packet).header.m = &packet_methods[enum_type]; \
	}
#define PACKET_WRITE_ALLOCA(packet, conn)	PACKET_WRITE(&(packet), (conn))
#define PACKET_READ_ALLOCA(packet, conn)	PACKET_READ(&(packet), (conn))
#define PACKET_FREE_ALLOCA(packet)			(packet.header.m->free_mem(&(packet), TRUE))

/* Enums */
enum php_mysql_packet_type
{
	PROT_GREET_PACKET= 0,
	PROT_AUTH_PACKET,
	PROT_OK_PACKET,
	PROT_EOF_PACKET,
	PROT_CMD_PACKET,
	PROT_RSET_HEADER_PACKET,
	PROT_RSET_FLD_PACKET,
	PROT_ROW_PACKET,
	PROT_STATS_PACKET,
	PROT_PREPARE_RESP_PACKET,
	PROT_CHG_USER_PACKET,
	PROT_LAST, /* should always be last */
};


extern const char * const mysql_command_to_text[MYSQL_COM_END];

/* Low-level extraction functionality */
typedef struct st_mysql_packet_methods {
	size_t	struct_size;
	my_bool (*read_from_net)(void *packet, MYSQL *conn);
	size_t (*write_to_net)(void *packet, MYSQL *conn);
	void (*free_mem)(void *packet, my_bool alloca);
} mysql_packet_methods;

extern mysql_packet_methods packet_methods[];


typedef struct st_mysql_packet_header {
	size_t size;
	uchar  packet_no;
	mysql_packet_methods *m;
} mysql_packet_header;

/* Server greets the client */
typedef struct st_php_mysql_packet_greet {
	mysql_packet_header   header;
	mysql_1b		protocol_version;
	char			*server_version;
	mysql_4b		thread_id;
	uchar		scramble_buf[SCRAMBLE_LENGTH];
	/* 1 byte pad */
	mysql_2b		server_capabilities;
	mysql_1b		charset_no;
	mysql_2b		server_status;
	/* 13 byte pad*/
	my_bool		pre41;
	/* If error packet, we use these */
	char 			error[MYSQL_ERRMSG_SIZE+1];
	char 			sqlstate[SQLSTATE_LENGTH + 1];
	unsigned int 	error_no;
} php_mysql_packet_greet;


/* Client authenticates */
typedef struct st_php_mysql_packet_auth {
	mysql_packet_header		header;
	mysql_4b	client_flags;
	uint32		max_packet_size;
	mysql_1b	charset_no;
	/* 23 byte pad */
	const char	*user;
	/* 8 byte scramble */
	const char	*db;
	/* 12 byte scramble */

	/* Here the packet ends. This is user supplied data */
	const char	*password;
	/* +1 for \0 because of scramble() */
	unsigned char	*server_scramble_buf;
	size_t			db_len;
} php_mysql_packet_auth;

/* OK packet */
typedef struct st_php_mysql_packet_ok {
	mysql_packet_header		header;
	mysql_1b		field_count; /* always 0x0 */
	my_ulonglong	affected_rows;
	my_ulonglong	last_insert_id;
	mysql_2b	server_status;
	mysql_2b	warning_count;
	char		*message;
	size_t		message_len;
	/* If error packet, we use these */
	char 		error[MYSQL_ERRMSG_SIZE+1];
	char 		sqlstate[SQLSTATE_LENGTH + 1];
	unsigned int 	error_no;
} php_mysql_packet_ok;


/* Command packet */
typedef struct st_php_mysql_packet_command {
	mysql_packet_header header;
	enum enum_server_command command;
	const char *argument;
	size_t	arg_len;
} php_mysql_packet_command;


/* EOF packet */
typedef struct st_php_mysql_packet_eof {
	mysql_packet_header		header;
	mysql_1b		field_count; /* 0xFE */
	mysql_2b		warning_count;
	mysql_2b		server_status;
	/* If error packet, we use these */
	char 			error[MYSQL_ERRMSG_SIZE+1];
	char 			sqlstate[SQLSTATE_LENGTH + 1];
	unsigned int 	error_no;
} php_mysql_packet_eof;
/* EOF packet */


/* Result Set header*/
typedef struct st_php_mysql_packet_rset_header {
	mysql_packet_header		header;
	/*
	  0x00 => ok
	  ~0   => LOAD DATA LOCAL
	  error_no != 0 => error
	  others => result set -> Read res_field packets up to field_count
	*/
	unsigned long		field_count;
	/*
	  These are filled if no SELECT query. For SELECT warning_count
	  and server status are in the last row packet, the EOF packet.
	*/
	mysql_2b			warning_count;
	mysql_2b			server_status;
	my_ulonglong		affected_rows;
	my_ulonglong		last_insert_id;
	/* This is for both LOAD DATA or info, when no result set */
	char				*info_or_local_file;
	size_t				info_or_local_file_len;
	/* If error packet, we use these */
	mysql_error_info	error_info;
} php_mysql_packet_rset_header;


/* Result set field packet */
typedef struct st_php_mysql_packet_res_field {
	mysql_packet_header	header;
	MYSQL_FIELD			*metadata;
	/* For table definitions, empty for result sets */
	my_bool				skip_parsing;
	my_bool				stupid_list_fields_eof;
} php_mysql_packet_res_field;


/* Row packet */
struct st_php_mysql_packet_row {
	mysql_packet_header	header;
	uchar			**fields; /* ??? */
	mysql_4b		field_count;
	my_bool		eof;
	/*
	  These are, of course, only for SELECT in the EOF packet,
	  which is detected by this packet
	*/
	mysql_2b		warning_count;
	mysql_2b		server_status;

	uchar		*row_buffer;

	my_bool		skip_extraction;
	my_bool		binary_protocol;
	MYSQL_FIELD	*fields_metadata;
	/* We need this to alloc bigger bufs in non-PS mode */
	unsigned int	bit_fields_count;
	size_t			bit_fields_total_len; /* trailing \0 not counted */

	/* If error packet, we use these */
	mysql_error_info	error_info;
};
typedef struct st_php_mysql_packet_row php_mysql_packet_row;

/* Statistics packet */
typedef struct st_php_mysql_packet_stats {
	mysql_packet_header	header;
	char *message;
	/* message_len is not part of the packet*/
	size_t message_len;
} php_mysql_packet_stats;


/* COM_PREPARE response packet */
typedef struct st_php_mysql_packet_prepare_response {
	mysql_packet_header	header;
	/* also known as field_count 0x00=OK , 0xFF=error */
	unsigned char	error_code;
	unsigned long	stmt_id;
	unsigned int	field_count;
	unsigned int	param_count;
	unsigned int	warning_count;

	/* present in case of error */
	mysql_error_info	error_info;
} php_mysql_packet_prepare_response;


/* Statistics packet */
typedef struct st_php_mysql_packet_chg_user_resp {
	mysql_packet_header	header;
	mysql_4b			field_count;
	
	/* message_len is not part of the packet*/
	mysql_2b			server_capabilities;
	/* If error packet, we use these */
	mysql_error_info	error_info;
} php_mysql_packet_chg_user_resp;


size_t mysql_stream_write(MYSQL *conn, const char * buf, size_t count);
size_t mysql_stream_write_w_header(MYSQL *conn, const char * buf, size_t count);

#ifdef MYSQL_DO_WIRE_CHECK_BEFORE_COMMAND
size_t php_mysql_consume_uneaten_data(const MYSQL *conn, enum php_mysql_server_command cmd);
#endif


unsigned long	php_mysql_net_field_length(uchar **packet);
uchar *	php_mysql_net_store_length(uchar *packet, my_ulonglong length);

extern char * const mysql_empty_string;

#endif /* MYSQL_WIREPROTOCOL_H */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
