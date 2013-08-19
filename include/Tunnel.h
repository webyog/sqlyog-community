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



#define		IS_BLOBVALUE(t) ((((t)>= MYSQL_TYPE_TINY_BLOB) && ((t)<= MYSQL_TYPE_BLOB)) || (t == MYSQL_TYPE_GEOMETRY))

#ifdef _WIN32

#include <winsock.h>
#include <mysql.h>
#include <crtdbg.h>

#ifndef __TUNNEL__
#define __TUNNEL__
#define		STRING_NULL		"(NULL)"

class Tunnel 
{

public:

    Tunnel () {}
    virtual ~Tunnel() {}

	virtual void			SetServerInfo ( MYSQL * mysql, const char * serverinfo ) = 0;
	virtual void			SetDB ( const char * db ) = 0;
	virtual void			SetBatchEnd ( bool end )  = 0;
	virtual void			EmptyQueryBuffer () = 0;
	virtual bool			GetMySQLVersion ( MYSQL * mysql ) = 0;

	virtual int				mysql_real_query ( MYSQL * mysql, const char * q, unsigned long length, bool isbadforxml, bool batch = false, 
												bool isend = false, bool * stop = 0, bool fksethttpimport = false) = 0;
	virtual void			mysql_close ( MYSQL * sock ) = 0;
	virtual void			mysql_free_result ( MYSQL_RES * res ) = 0;
	virtual void			mysql_data_seek( MYSQL_RES * res, my_ulonglong offset ) = 0;
	virtual const char		*mysql_get_server_info ( MYSQL * mysql ) = 0;
	virtual const char		*mysql_error ( MYSQL * mysql ) = 0;
	virtual const char		*mysql_info ( MYSQL * mysql ) = 0;
	virtual MYSQL			*mysql_init ( MYSQL * mysql ) = 0;
	virtual int				mysql_options(MYSQL * mysql, enum mysql_option option, const char *arg) = 0;
	virtual MYSQL			*mysql_real_connect ( 
							MYSQL *mysql, const char *host, const char *user, 
								const char *passwd, const char *db, unsigned int port, 
								const char *unix_socket, unsigned long client_flag, 
								const char * httpaddress ) = 0;

	virtual unsigned int	mysql_errno ( MYSQL * mysql ) = 0;
	virtual unsigned int	mysql_num_fields( MYSQL_RES * res ) = 0;
	virtual unsigned int	mysql_field_count( MYSQL * mysql ) = 0;
	virtual unsigned int	mysql_field_seek ( MYSQL_RES *result, unsigned int field_offset ) = 0;
	virtual unsigned int    mysql_warning_count(MYSQL *mysql) = 0;
	virtual unsigned long	*mysql_fetch_lengths( MYSQL_RES * result ) = 0;
	virtual unsigned long	mysql_escape_string( char * to, const char * from, unsigned long from_length ) = 0;
	virtual unsigned long	mysql_real_escape_string( MYSQL * mysql, char * to, const char * from, unsigned long length ) = 0;
	virtual my_ulonglong	mysql_insert_id( MYSQL * mysql ) = 0;
	virtual my_ulonglong	mysql_affected_rows( MYSQL * mysql ) = 0;
	virtual my_ulonglong	mysql_num_rows ( MYSQL_RES * myres ) = 0;
	virtual MYSQL_RES		*mysql_store_result ( MYSQL * mysql, bool dontprofile = false, bool force = false, void *param = NULL ) = 0;
	virtual MYSQL_RES		*mysql_use_result( MYSQL * mysql ) = 0;
	virtual MYSQL_ROW		mysql_fetch_row ( MYSQL_RES * res ) = 0;
	virtual MYSQL_FIELD		*mysql_fetch_field ( MYSQL_RES * res ) = 0;
	virtual MYSQL_FIELD		*mysql_fetch_fields ( MYSQL_RES * res ) = 0;
	virtual MYSQL_FIELD		*mysql_fetch_field_direct( MYSQL_RES * res, unsigned int fieldnr ) = 0;
	virtual int				mysql_next_result ( MYSQL * mysql, bool dontprofile, bool force ) = 0;
    virtual int             mysql_ping(MYSQL* mysql) = 0;

	// tunnel flags
	bool			IsTunnel() { return m_IsTunnel; }

	bool			IsExecuting() { return m_istunnelexecuting; }

	// from 4.0 final we support timeout in http
	virtual unsigned long	GetTimeOut ()  = 0;
	virtual void			SetTimeOut ( unsigned long timeout )  = 0;

	/* tells us whether any query is left to be processed */
	/* important for batch and import batch query */
	virtual bool			IsQueryLeft ()  = 0;

	/* returns the buffer for the query */
	virtual const char*		GetQuery ()  = 0;
	virtual unsigned long	GetQueryLength ()  = 0;

	// some public function to give access to member variables.
	// all char* becomes const char* so that nobody from outside modifies it.
    virtual const char*		GetHost ()  = 0;
	virtual const char*		GetUser ()  = 0;
	virtual const char*		GetPwd ()  = 0;
	virtual const char*		GetDb ()  = 0;
	virtual const char*		GetSocket ()  = 0;
	virtual const char*		GetHttpAddr ()  = 0;
	virtual unsigned int	GetPort ()  = 0;
	virtual unsigned long	GetFlags ()  = 0;

	
	/* get/set proxy server information */
	virtual bool				IsProxy ()  = 0;
	virtual int					GetProxyPort ()  = 0;
	virtual const wchar_t*		GetProxy ()  = 0;
	virtual const wchar_t*		GetProxyUserName ()  = 0;
	virtual const wchar_t*		GetProxyPwd ()  = 0;
	virtual const wchar_t*		GetContentType () =0;

	/* wrapper to set delimiter at runtime */
	virtual void			SetDelimiter ( const char * delimiter ) = 0;

	virtual void			SetProxy ( bool isproxy ) = 0;
	virtual bool			SetProxyInfo ( const wchar_t * proxy, int port, 
								   const wchar_t * proxyuser, const wchar_t * proxypwd ) = 0;

	/*Whether the query and connection details to be in Base64 format
	(for soving the issue of single quotes(apos;) with some PHP version) or not with http tunneling*/
	virtual void			SetEncodingScheme(bool isbase64) = 0;
	virtual bool			GetEncodingScheme() = 0;

	/* get/set Content-Type(HTTP header) info */
	virtual void			SetContentType(const wchar_t *contenttype)=0;

	/* charset get/set function */
	virtual bool			SetCharset ( const char * tunnel_charset ) = 0;
	virtual const char		*GetCharset () = 0;

	/* get/set challenge response authentication */
	virtual bool			IsChallenge ()  = 0;
	virtual const wchar_t*		GetChallengeUserName ()  = 0;
	virtual const wchar_t*		GetChallengePwd ()  = 0;
	virtual bool			SetChallengeInfo ( bool challenge, const wchar_t * username = NULL, const wchar_t * pwd = NULL ) = 0;

    /*  starting with v5.14 RC2, we implement a new function just to correctly check for tunnel version
        by doing a very specific response */
    virtual bool            CheckCorrectTunnelVersion(MYSQL * mysql) = 0;
	
protected:

	bool			m_IsTunnel;

	bool			m_istunnelexecuting;

	bool			m_isnotsupportedserver;
};

#endif

#else

#ifndef __TUNNEL__
#define __TUNNEL__

#include <mysql/mysql.h>

class Tunnel
{
public:
	Tunnel () {}
	~Tunnel() {}

    bool        IsTunnel() { return false; }

};

#endif

#endif
