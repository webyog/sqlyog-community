/* Copyright (C) 2013 Webyog Inc.

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


#ifndef __TUNNELFREE__
#define __TUNNELFREE__
#include "Tunnel.h"
#include "Datatype.h"
#include "MySQLVersionHelper.h"
#include "CommonHelper.h"
class TunnelCommunity : public Tunnel
{

public:

    TunnelCommunity() {m_IsTunnel = false;};
    ~TunnelCommunity(){};

	void			SetServerInfo ( MYSQL * mysql, const char * serverinfo );
	void			SetDB ( const char * db );
	void			SetBatchEnd ( bool end ) {}
	void			EmptyQueryBuffer ();
	bool			GetMySQLVersion ( MYSQL * mysql );

	int				mysql_real_query ( MYSQL * mysql, const char * q, unsigned long length, bool isbadforxml, 
		                               bool batch = false, bool isend = false, bool * stop = 0, bool fksethttpimport = false);
	void			mysql_close ( MYSQL * sock );
	void			mysql_free_result ( MYSQL_RES * res );
	void			mysql_data_seek( MYSQL_RES * res, my_ulonglong offset );
	const char		*mysql_get_server_info ( MYSQL * mysql );
	const char		*mysql_error ( MYSQL * mysql );
	const char		*mysql_info ( MYSQL * mysql );
	MYSQL			*mysql_init ( MYSQL * mysql );
	int				mysql_options(MYSQL * mysql, enum mysql_option option, const char *arg);
	MYSQL			*mysql_real_connect ( 
						  MYSQL *mysql, const char *host, const char *user, 
						  const char *passwd, const char *db, unsigned int port, 
						  const char *unix_socket, unsigned long client_flag, 
						  const char * httpaddress );

	unsigned int	mysql_errno ( MYSQL * mysql );
	unsigned int	mysql_num_fields( MYSQL_RES * res );
	unsigned int	mysql_field_count( MYSQL * mysql );
	unsigned int	mysql_field_seek ( MYSQL_RES *result, unsigned int field_offset );
	unsigned int    mysql_warning_count(MYSQL *mysql);
	unsigned long	*mysql_fetch_lengths( MYSQL_RES * result );
	unsigned long	mysql_escape_string( char * to, const char * from, unsigned long from_length );
	unsigned long	mysql_real_escape_string( MYSQL * mysql, char * to, const char * from, unsigned long length );
	my_ulonglong	mysql_insert_id( MYSQL * mysql );
	my_ulonglong	mysql_affected_rows( MYSQL * mysql );
	my_ulonglong	mysql_num_rows ( MYSQL_RES * myres );
	MYSQL_RES		*mysql_store_result ( MYSQL * mysql, bool dontprofile = false, bool force = false, void *wnd = NULL );
	MYSQL_RES		*mysql_use_result( MYSQL * mysql );
	MYSQL_ROW		mysql_fetch_row ( MYSQL_RES * res );
	MYSQL_FIELD		*mysql_fetch_field ( MYSQL_RES * res );
	MYSQL_FIELD		*mysql_fetch_fields ( MYSQL_RES * res );
	MYSQL_FIELD		*mysql_fetch_field_direct( MYSQL_RES * res, unsigned int fieldnr );
	INT				mysql_next_result ( MYSQL * mysql, bool dontprofile, bool force );
    int             mysql_ping(MYSQL* mysql);

	// tunnel flags
	bool			IsTunnel() { return false; }

	// from 4.0 final we support timeout in http
	unsigned long	GetTimeOut () { return 0; }
	void			SetTimeOut ( unsigned long timeout ) {}

	/* tells us whether any query is left to be processed */
	/* important for batch and import batch query */
	bool			IsQueryLeft () { return false; }

	/* returns the buffer for the query */
	const char*		GetQuery () { return NULL; }
	unsigned long	GetQueryLength () {return 0; }

	// some public function to give access to member variables.
	// all char* becomes const char* so that nobody from outside modifies it.
	const char*		GetHost () { return NULL; }
	const char*		GetUser () { return NULL; }
	const char*		GetPwd () { return NULL; }
	const char*		GetDb () { return NULL; }
	const char*		GetSocket () { return NULL; }
	const char*		GetHttpAddr () { return NULL; }
	unsigned int	GetPort () { return 0; }
	unsigned long	GetFlags () { return 0; }
	
	/* get/set proxy server information */
	bool			IsProxy () { return false; }
	int				GetProxyPort () { return 0; }
	const wchar_t*	GetProxy () { return NULL; }
	const wchar_t*	GetProxyUserName () { return NULL; }
	const wchar_t*	GetProxyPwd () { return NULL; }
	const wchar_t*	GetContentType () { return NULL; }

	/* wrapper to set delimiter at runtime */
	void			SetDelimiter ( const char * delimiter );

	void			SetProxy ( bool isproxy );
	bool			SetProxyInfo ( const wchar_t * proxy, int port, 
								   const wchar_t * proxyuser, const wchar_t * proxypwd );

	/*Whether the query and connection details to be in Base64 format
	(for soving the issue of single quotes(apos;) with some PHP version) or not with http tunneling*/
	void			SetEncodingScheme(bool isbase64);
	bool			GetEncodingScheme();
	
	/* get/set Content-Type(HTTP header) info */
	void			SetContentType(const wchar_t *contenttype);

	/* charset get/set function */
	bool			SetCharset ( const char * tunnel_charset );
	const char		*GetCharset ();

	/* get/set challenge response authentication */
	bool			IsChallenge () { return false; }
	const wchar_t*	GetChallengeUserName () { return NULL; }
	const wchar_t*	GetChallengePwd () { return NULL; }
	bool			SetChallengeInfo ( bool challenge, const wchar_t * username = NULL, const wchar_t * pwd = NULL );

    /*  starting with v5.14 RC2, we implement a new function just to correctly check for tunnel version
        by doing a very specific response */
    bool            CheckCorrectTunnelVersion(MYSQL * mysql);

	
};
#endif
