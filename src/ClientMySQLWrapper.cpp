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


#ifdef _WIN32
	#include <winsock.h>
	#include <mysql.h>
	#include "Global.h"
#else
	#include <mysql/mysql.h>
    #include <string.h>         /* for strcmp for linux patch */
    #include <stdlib.h>         /* for atoll */	
#endif

#ifdef COMMUNITY 
#include "Global.h"
#else
#include "CommonJobStructures.h"
#endif

#include "Tunnel.h"
#include "MySQLVersionHelper.h"
#include "CommonHelper.h"

#ifndef __CSWRAPPER__
	#include "ClientMySQLWrapper.h"
#endif 

#define MYSQL_SERVER_GONE		2006
#define	LOST_CONNECTION_ERROR	2013
#define	NO_DB_SELECTED			1046
#define	MYSQL_CONNECT_ERROR		2003
#define	CR_CONNECTION_ERROR		2002

/* function checks whether there a unprintable character in between */
/* this function is required for tunneling as we need to send the data in base 64 encoding */

wyBool
IsBadforXML ( const char * text )
{
	while ( *text )
	{ 
		/* for range check out take from http://www.w3.org/TR/REC-xml/#sec-well-formed */
		if ( !(*text >=32 && *text < 55295 ) )
			return wyTrue;

		text++;
	}

	return wyFalse;
}

int 
sja_mysql_options(MYSQL * mysql, enum mysql_option option, const char *arg)
{
	return ::mysql_options(mysql, option, arg);
}

/* some global functions to implement tunneling in a crossplatform way */
int sja_mysql_real_query ( Tunnel * tunnel, MYSQL * mysql, const char * q, unsigned long len )
{
	#ifdef _WIN32
		return tunnel->mysql_real_query( mysql, q, len, (bool)IsBadforXML (q) );
	#else
		return ::mysql_real_query( mysql, q, len );
	#endif
}

MYSQL* sja_mysql_init ( Tunnel * tunnel, MYSQL * mysql )
{
	#ifdef _WIN32
		return tunnel->mysql_init( mysql );
	#else
		return ::mysql_init ( mysql );
	#endif
}

MYSQL*	sja_mysql_real_connect ( Tunnel * tunnel, MYSQL *mysql, const char *host,
                                 const char *user,
                                 const char *passwd,
                                 const char *db,
                                 unsigned int port,
                                 const char *unix_socket,
                                 unsigned long clientflag, char * url )
{
#ifdef _WIN32
	return tunnel->mysql_real_connect( mysql, host, user, passwd, db, port, unix_socket, clientflag | CLIENT_MULTI_RESULTS, url );
#else
	return ::mysql_real_connect ( mysql, host, user, passwd, db, port, unix_socket, clientflag | CLIENT_MULTI_RESULTS );
#endif
}

const char* sja_mysql_get_server_info ( Tunnel * tunnel, MYSQL * mysql )
{
#ifdef _WIN32
	return tunnel->mysql_get_server_info ( mysql );
#else
	return ::mysql_get_server_info ( mysql );
#endif
}

unsigned int sja_mysql_errno ( Tunnel * tunnel, MYSQL * mysql )
{
#ifdef _WIN32
	return tunnel->mysql_errno ( mysql );
#else
	return ::mysql_errno ( mysql );
#endif
}

const char* sja_mysql_error ( Tunnel * tunnel, MYSQL * mysql )
{
#ifdef _WIN32
	return tunnel->mysql_error ( mysql );
#else
	return ::mysql_error ( mysql );
#endif
}

void sja_mysql_close ( Tunnel * tunnel, MYSQL * sock )
{
#ifdef _WIN32
	tunnel->mysql_close ( sock );
#else
	::mysql_close ( sock );
#endif
}

MYSQL_RES * sja_mysql_store_result( Tunnel * tunnel, MYSQL * mysql, bool profile, bool force)
{
#ifdef _WIN32
	return tunnel->mysql_store_result ( mysql, profile, force);
#else
	return ::mysql_store_result ( mysql );
#endif
}

MYSQL_RES * sja_mysql_use_result( Tunnel * tunnel, MYSQL * mysql )
{
#ifdef _WIN32
	return tunnel->mysql_use_result ( mysql );
#else
	return ::mysql_use_result ( mysql );
#endif
}

MYSQL_ROW	sja_mysql_fetch_row( Tunnel * tunnel, MYSQL_RES *result )
{
#ifdef _WIN32
	return tunnel->mysql_fetch_row ( result );
#else
	return ::mysql_fetch_row ( result );
#endif
}

void sja_mysql_free_result( Tunnel * tunnel, MYSQL_RES *result)
{
#ifdef _WIN32
	tunnel->mysql_free_result ( result );
#else
	::mysql_free_result ( result );
#endif
}

my_ulonglong sja_mysql_num_rows ( Tunnel * tunnel, MYSQL_RES *res )
{
#ifdef _WIN32
	return tunnel->mysql_num_rows ( res );
#else
	return ::mysql_num_rows ( res );
#endif
}

void sja_mysql_data_seek( Tunnel * tunnel, MYSQL_RES *result, my_ulonglong offset)
{
#ifdef _WIN32
	tunnel->mysql_data_seek ( result, offset );
#else
	::mysql_data_seek ( result, offset );
#endif
}

unsigned long * sja_mysql_fetch_lengths ( Tunnel * tunnel, MYSQL_RES *result )
{
#ifdef _WIN32
	return tunnel->mysql_fetch_lengths ( result );
#else
	return ::mysql_fetch_lengths ( result );
#endif
}

unsigned int sja_mysql_num_fields ( Tunnel * tunnel, MYSQL_RES *res )
{
#ifdef _WIN32
	return tunnel->mysql_num_fields ( res );
#else
	return ::mysql_num_fields ( res  );
#endif
}

MYSQL_FIELD * sja_mysql_fetch_fields( Tunnel * tunnel, MYSQL_RES *res)
{
#ifdef _WIN32
	return tunnel->mysql_fetch_fields ( res );
#else
	return ::mysql_fetch_fields ( res );
#endif
}

MYSQL_FIELD * sja_mysql_fetch_field_direct ( Tunnel * tunnel, MYSQL_RES *res, unsigned int fieldnr)
{
#ifdef _WIN32
	return tunnel->mysql_fetch_field_direct ( res, fieldnr );
#else
	return ::mysql_fetch_field_direct ( res, fieldnr );
#endif
}

unsigned long sja_mysql_real_escape_string ( Tunnel * tunnel, MYSQL *mysql, char *to, const char *from, unsigned long length)
{
#ifdef _WIN32
	return tunnel->mysql_real_escape_string ( mysql, to, from, length );
#else
	return ::mysql_real_escape_string ( mysql, to, from, length );
#endif

}

MYSQL_FIELD_OFFSET sja_mysql_field_seek( Tunnel * tunnel, MYSQL_RES *result, MYSQL_FIELD_OFFSET offset)
{
#ifdef _WIN32
	return tunnel->mysql_field_seek ( result, offset );
#else
	return ::mysql_field_seek ( result, offset );
#endif
}

MYSQL_FIELD *	sja_mysql_fetch_field(Tunnel * tunnel, MYSQL_RES *result)
{
#ifdef _WIN32
	return tunnel->mysql_fetch_field ( result );
#else
	return ::mysql_fetch_field ( result );
#endif
}

bool sja_mysql_istunnel(Tunnel * tunnel )
{
#ifdef _WIN32
	return tunnel->IsTunnel();
#else
	return false;
#endif
}

void sja_mysql_setcharset ( Tunnel * tunnel, char* charset )
{
#ifdef _WIN32
    tunnel->SetCharset( charset );
#else
	return;
#endif
}



my_ulonglong sja_mysql_affected_rows ( Tunnel * tunnel, MYSQL * mysql )
{
#ifdef _WIN32
	return tunnel->mysql_affected_rows ( mysql );
#else
	return ::mysql_affected_rows ( mysql );
#endif
}

unsigned int sja_mysql_field_count ( Tunnel * tunnel, MYSQL *mysql )
{
#ifdef _WIN32
	return tunnel->mysql_field_count ( mysql );
#else
	return ::mysql_field_count ( mysql );
#endif
}

bool sja_CheckCorrectTunnelVersion(Tunnel * tunnel, MYSQL * mysql)
{
#ifdef _WIN32
	return tunnel->CheckCorrectTunnelVersion(mysql);
#else
	return true;
#endif
}

MYSQL_RES* SjaExecuteAndGetResult(Tunnel *tunnel, PMYSQL mysql, wyString &query, wyBool isreconnect, bool isbatch,
								  bool isforce, ConnectionInfo *coninfo, const wyChar *usedbname, wyInt32* stop, wyInt32* status)
{
    MYSQL_RES   *res = NULL;
    wyInt32     ret = 1;

	ret = HandleSjaMySQLRealQuery(tunnel, *mysql, query.GetString(), query.GetLength(), isbatch, (bool*)stop);

    if(status)
        *status = ret;

	if(isreconnect == wyFalse && ret)
		return NULL;

#if defined WIN32 && ! defined COMMUNITY	
	if(!tunnel->IsTunnel() && ret &&
		((tunnel->mysql_errno(*mysql)== MYSQL_CONNECT_ERROR) || 
		 (tunnel->mysql_errno(*mysql)== MYSQL_SERVER_GONE) ||
		  tunnel->mysql_errno(*mysql) == LOST_CONNECTION_ERROR) ||
		tunnel->mysql_errno(*mysql) == CR_CONNECTION_ERROR)
	{	
		if(HandleReconnect(tunnel, mysql, coninfo, usedbname) == true)
			ret  = HandleSjaMySQLRealQuery(tunnel, *mysql, query.GetString(), query.GetLength(), isbatch, (bool*)stop);

        if(status)
            *status = ret;
	}
#endif

	if(!ret)
		res = sja_mysql_store_result(tunnel, *mysql, true, isforce);

    return res;
}

MYSQL_RES* SjaExecuteAndUseResult(Tunnel * tunnel, MYSQL * mysql, wyString &query)
{
    MYSQL_RES   *res = NULL;
    wyInt32     ret;

    ret = HandleSjaMySQLRealQuery(tunnel, mysql, query.GetString(), query.GetLength());
	
    if(ret)
	    return NULL;

	res = sja_mysql_use_result(tunnel, mysql);

    return res;
}

INT  sja_mysql_next_result(Tunnel *tunnel, MYSQL * mysql)
{
#ifdef _WIN32
	return tunnel->mysql_next_result(mysql, FALSE, FALSE);
#else
	return ::mysql_next_result(mysql);
#endif	  
}

wyUInt32 
Get_Max_Allowed_Packet_Size(Tunnel *tunnel, PMYSQL mysql)
{
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyString 	query;
	wyInt32	    max_size;
	wyString	myrowstr;
	//wyBool		ismysql41 = IsMySQL41(tunnel, mysql);
	
	query.Sprintf("show variables like 'max_allowed_packet'");
		
	myres = SjaExecuteAndGetResult(tunnel, mysql, query, wyTrue);
	
    if(!myres)
        return 0;

	myrow = sja_mysql_fetch_row(tunnel, myres);
    
	if(!myrow || !myrow[1])
	{
		if(myres)
			sja_mysql_free_result(tunnel, myres);

		return 0;
	}

	myrowstr.SetAs(myrow[1]);
	
	max_size = atoi(myrowstr.GetString()) ;
	sja_mysql_free_result(tunnel, myres);
		
	return(max_size/(1024 * 1.4)); // We will use query size slightly lower than the server default for safe import.
	
	/*********************
	while((myrow = sja_mysql_fetch_row(tunnel, myres)) != NULL)
	{
		myrowstr.SetAs(myrow[0]);
		if(!myrowstr.CompareI("max_allowed_packet"))
		{
			myrowstr.SetAs(myrow[1]);
			max_size = atoi(myrowstr.GetString()) ;
			sja_mysql_free_result(tunnel, myres);
			return(max_size/(1024 * 1.4)); // We will use query size slightly lower than the server default for safe import.
		}
	}
	*/
	//sja_mysql_free_result(tunnel, myres);

	//return 0;
}

/* Linux patch for string functions */

#ifndef _WIN32
    extern int 
    stricmp ( const char * arg1, const char * arg2 ) 
	{
		return strcasecmp ( arg1, arg2 );
    }

    extern my_ulonglong
    _atoi64 ( const char * nptr )
    {
        return atoll ( nptr );
    }

    extern int
    strnicmp ( const char * arg1, const char * arg2, int count )
    {
        return strncasecmp ( arg1, arg2, count );
    }
#endif
