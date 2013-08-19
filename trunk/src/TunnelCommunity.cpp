/* Copyright (C) 2013 Webyog Inc

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA

*/


#include "TunnelCommunity.h"

MYSQL*
TunnelCommunity::mysql_init( MYSQL * mysql )
{
    return ::mysql_init((MYSQL*)NULL);
}

int TunnelCommunity::mysql_options(MYSQL * mysql, enum mysql_option option, const char *arg)
{
	return ::mysql_options(mysql, option, arg);
}

void
TunnelCommunity::mysql_close ( MYSQL * sock )
{
	::mysql_close(sock);

	//fixed mysql_thread_init() memory leak
	//::mysql_thread_end();
}

const char*
TunnelCommunity::mysql_get_server_info ( MYSQL * mysql )
{
    return ::mysql_get_server_info(mysql);
}

// in this function we take a little diversion from the C API(), we add up one more argument
// this argument is the HTTP address of the .php file, that will be used for tunneling.

MYSQL*
TunnelCommunity::mysql_real_connect ( 
							  MYSQL *mysql, const char *host, const char *user, 
							  const char *passwd, const char *db, unsigned int port, 
							  const char *unix_socket, unsigned long client_flag, 
							  const char * httpaddress							
						   )
{
		MYSQL *temp = ::mysql_real_connect( mysql, host, user, passwd, db, port, unix_socket, client_flag );

		if(temp) 
		{
			wyUInt32 me = mysql_get_server_version(temp);/* Only available from MySQLv4.1*/

			if(me < 40100) 
			{
				mysql->net.last_errno = 2054;
				strcpy(mysql->net.last_error, "Connecting to 3.22, 3.23 & 4.0 servers is not supported");
				temp = NULL;
			}
		}

		return temp;
}

int
TunnelCommunity::mysql_real_query( MYSQL *mysql, const char *q, unsigned long length, 
								  bool isbadforxml, bool batch, bool isend, bool * stop, bool fksethttpimport)
{
    return ::mysql_real_query(mysql, q, length);
}

MYSQL_RES*
TunnelCommunity::mysql_store_result(MYSQL * mysql, bool dontprofile, bool force, void *wnd)
{
    return ::mysql_store_result(mysql);
}

INT 
TunnelCommunity::mysql_next_result(MYSQL * mysql, bool dontprofile, bool force)
{
    return ::mysql_next_result( mysql );
}

MYSQL_RES*
TunnelCommunity::mysql_use_result( MYSQL * mysql )
{
    return ::mysql_use_result ( mysql );
}

my_ulonglong
TunnelCommunity::mysql_insert_id( MYSQL * mysql )
{
    return ::mysql_insert_id( mysql );
}

my_ulonglong
TunnelCommunity::mysql_affected_rows( MYSQL * mysql )
{
    return ::mysql_affected_rows( mysql );
}

void
TunnelCommunity::mysql_free_result ( MYSQL_RES * res )
{
	if ( !res )
		return;

    ::mysql_free_result(res);
}

my_ulonglong
TunnelCommunity::mysql_num_rows( MYSQL_RES * myres )
{
    return ::mysql_num_rows( myres );
}

MYSQL_ROW
TunnelCommunity::mysql_fetch_row ( MYSQL_RES * myres )
{
    return ::mysql_fetch_row( myres );
}

void
TunnelCommunity::mysql_data_seek( MYSQL_RES * res, my_ulonglong offset )
{
    ::mysql_data_seek( res, offset );
}

unsigned long*
TunnelCommunity::mysql_fetch_lengths( MYSQL_RES * result )
{
    return ::mysql_fetch_lengths( result );
}

unsigned int
TunnelCommunity::mysql_errno( MYSQL * mysql )
{
	return ::mysql_errno ( mysql );
}

const char*
TunnelCommunity::mysql_error ( MYSQL * mysql )
{
    return ::mysql_error ( mysql );
}

const char*
TunnelCommunity::mysql_info( MYSQL * mysql )
{
	return ::mysql_info ( mysql );
}

MYSQL_FIELD*
TunnelCommunity::mysql_fetch_fields( MYSQL_RES * res )
{
    return ::mysql_fetch_fields( res );
}

unsigned int
TunnelCommunity::mysql_field_seek ( MYSQL_RES *result, unsigned int field_offset )
{
    return ::mysql_field_seek( result, field_offset );
}


unsigned int
TunnelCommunity::mysql_num_fields( MYSQL_RES * res )
{
	return ::mysql_num_fields( res );
}

unsigned int
TunnelCommunity::mysql_field_count( MYSQL * mysql )
{
    return ::mysql_field_count( mysql );
}

unsigned int
TunnelCommunity::mysql_warning_count( MYSQL * mysql )
{
	return ::mysql_warning_count( mysql );
}

unsigned long
TunnelCommunity::mysql_real_escape_string( MYSQL * mysql, char * to, const char * from, unsigned long length )
{
    return ::mysql_real_escape_string( mysql, to, from, length );
}

unsigned long
TunnelCommunity::mysql_escape_string( char * to, const char * from, unsigned long length )
{
    return ::mysql_escape_string( to, from, length );
}

MYSQL_FIELD*
TunnelCommunity::mysql_fetch_field_direct ( MYSQL_RES * res, unsigned int fieldnr )
{
	return ::mysql_fetch_field_direct ( res, fieldnr );
}

MYSQL_FIELD*
TunnelCommunity::mysql_fetch_field( MYSQL_RES * res )
{
    return ::mysql_fetch_field ( res );
}

/***** Implementation of actual tunneling functions *******/

/*	wrapper function to set the delimiter during run time. Its required as
	we need to change the delimiter with views, sps, triggers etc */

void
TunnelCommunity::SetDelimiter ( const char * delimiter )
{
    VERIFY(false);
}

/*  user can use proxy server to connect to a website. functions to handle proxy information */

void
TunnelCommunity::SetProxy ( bool isproxy )
{
	VERIFY(false);
}

bool
TunnelCommunity::SetProxyInfo ( const wchar_t * proxy, int port, const wchar_t * proxyuser, const wchar_t * proxypwd )
{
    VERIFY(false);
	return true;
}


void			
TunnelCommunity::SetEncodingScheme(bool isbase64)
{
	return;
}

bool
TunnelCommunity::GetEncodingScheme()
{
	return false;
}

//set Content-Type(HTTP header)
void
TunnelCommunity::SetContentType(const wchar_t *contenttype)
{
	VERIFY(false);
}

/* Charset get/set functions */

bool
TunnelCommunity::SetCharset ( const char * tunnel_charset )
{
    VERIFY(false);
	return true;

}

/*	return the internal pointer to the charset buffer. the user has to 
	check for return value as the value can be NULL */

const char*
TunnelCommunity::GetCharset ()
{
    VERIFY(false);

	return NULL;
}

/* user needs to access a 401 protected site so we need to provide username/pwd */

bool
TunnelCommunity::SetChallengeInfo ( bool challenge, const wchar_t * username, const wchar_t * pwd )
{
    VERIFY(false);
	return true;
}

/*	many a times before we actually execute a query in HTTP tunnelin
	we need the server version. we execute select version() and get it */

bool
TunnelCommunity::GetMySQLVersion ( MYSQL * mysql )
{
    VERIFY(false);
	return true;
}

void
TunnelCommunity::SetServerInfo( MYSQL * mysql, const char * serverinfo )
{
    VERIFY(false);
}

void
TunnelCommunity::SetDB( const char * db )
{
    VERIFY(false);
}

void
TunnelCommunity::EmptyQueryBuffer()
{
    VERIFY(false);
}


bool
TunnelCommunity::CheckCorrectTunnelVersion(MYSQL * mysql)
{
    VERIFY(false);
    return false;
}

int 
TunnelCommunity::mysql_ping(MYSQL* mysql)
{
    int ret;

    if(IsTunnel() == true)
    {
        return 0;
    }

    ret = ::mysql_ping(mysql);

    return ret;
}