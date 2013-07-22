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

#ifdef _WIN32

#ifndef __CHttp_H_
#define __CHttp_H_

#include <wininet.h>
#include <windows.h>
#include "DataType.h"

#ifndef DWORD_PTR	
	#define DWORD_PTR		DWORD
#endif
/* A simple ultra specifc small class to send form data over http and get the result back */
/* Certainly not a generic HTTP protocol but if required can be extended */
/* This class is very poor in resource management as of now */

typedef struct 
{
	HINTERNET				hRequest;
	LPINTERNET_BUFFERS		lpBufferIn;
	LPINTERNET_BUFFERS		lpBuffersOut;
	DWORD					dwFlags;
	DWORD_PTR				dwContext;
} HTTPSENDREQUESTEXPARAM;

typedef struct
{
	HINTERNET		hFile;
    LPCVOID			lpBuffer;
    DWORD			dwNumberOfBytesToWrite;
    LPDWORD			lpdwNumberOfBytesWritten;
} INTERNETWRITEFILE;

typedef struct 
{
	HINTERNET	hFile;
    LPVOID		lpBuffer;
    DWORD		dwNumberOfBytesToRead;
    LPDWORD		lpdwNumberOfBytesRead;
} INTERNETREADFILE;

typedef struct
{
	HINTERNET hRequest;
	LPINTERNET_BUFFERS lpBuffersOut;
	DWORD dwFlags;
	DWORD dwContext;
} HTTPENDREQUEST;

typedef struct
{

	HINTERNET	hRequest;
	LPCTSTR		lpszHeaders;
	DWORD		dwHeadersLength;
	LPVOID		lpOptional;
	DWORD		dwOptionalLength;
    DWORD       dwLastError;
} HTTPSENDREQUESTPARAM;

class CHttp
{

private:

	wyWChar			*m_url;	
	
	/* various URL information broken down */
	wyWChar			*m_Protocol;
	wyWChar			*m_HostName;
    wyWChar			*m_UserName;
    wyWChar			*m_Password;
    wyWChar			*m_FileName;
	DWORD			m_timeout;
	INTERNET_PORT	m_Port;

	wyWChar			*m_contenttype;

	/* proxy server information */
	int				m_proxyport;
	bool			m_isproxy;
	wyWChar			*m_proxy;
	wyWChar			*m_proxyuser;
	wyWChar			*m_proxypwd;

	/* challenge/reposnse authentication info */
	bool			m_challenge;
	wyWChar			*m_challengename;
	wyWChar			*m_challengepwd;

	/* hinternet handles */
	HINTERNET			m_HttpOpenRequest;
    HINTERNET			m_InternetConnect;
    HINTERNET			m_InternetSession;

	bool				BreakUrl ();
	bool				AllocHandles ( bool isbase64 , int *status, bool checkauth);//checkauth flag sets true if must be authenticated
	bool				Authorize (unsigned long *num);
	bool				PostData (char * encodeddata, DWORD encodelen );
	bool				CheckError (int * status );
	bool				ReadResponse (char ** pbuffer, DWORD * pbufferSize, bool * stop = 0 );

	
	/* get proxy server information used internally */
	bool				IsProxy () { return m_isproxy; }
	int					GetProxyPort () { return m_proxyport; }
	const wyWChar*		GetProxy () { return m_proxy; }
	const wyWChar*		GetProxyUserName () { return m_proxyuser; }
	const wyWChar*		GetProxyPwd () { return m_proxypwd; }

	

	/* function gets the challenge username/pwd */
	const wyWChar*		GetChallengeUserName () { return m_challengename; }
	const wyWChar*		GetChallengePwd () { return m_challengepwd; }

    bool                SetAuthDetails (HINTERNET hRequest, DWORD httpstatus );
	
	/* get/set the timeout time in milliseconds */
	DWORD				GetTimeOut ();

	/* to support timeouts we have to run them in a different thread so we make wrappers over blocking
	   wininte functions */
	BOOL				yog_HttpSendRequestEx (HINTERNET hRequest,
												LPINTERNET_BUFFERS lpBuffersIn,
												LPINTERNET_BUFFERS lpBuffersOut,
												DWORD dwFlags,
												DWORD_PTR dwContext );
	
	static unsigned __stdcall worker_HttpSendRequestEx (LPVOID );

	BOOL				yog_InternetWriteFile (HINTERNET hFile,
												LPCVOID lpBuffer,
												DWORD dwNumberOfBytesToWrite,
												LPDWORD lpdwNumberOfBytesWritten );

	static unsigned __stdcall worker_InternetWriteFile (LPVOID );

	BOOL				yog_InternetReadFile (	HINTERNET hFile,
												LPVOID lpBuffer,
												DWORD dwNumberOfBytesToRead,
												LPDWORD lpdwNumberOfBytesRead );

	static unsigned __stdcall worker_InternetReadFile (LPVOID );

	BOOL				yog_HttpEndRequest (	HINTERNET hRequest,
												LPINTERNET_BUFFERS lpBuffersOut,
												DWORD dwFlags,
												DWORD dwContext );

	static unsigned __stdcall worker_HttpEndRequest (LPVOID );

	BOOL				yog_HttpSendRequest (	DWORD *lpdwLastError, 
                                                HINTERNET hRequest,
												LPCTSTR lpszHeaders,
												DWORD dwHeadersLength,
												LPVOID lpOptional,
												DWORD dwOptionalLength );

	static unsigned __stdcall worker_HttpSendRequest (LPVOID );
	

public:
	
	CHttp ();
	virtual ~CHttp (); 
	
	bool				SetUrl (wyWChar * url );
	void				SetTimeOut (DWORD timeout ) { m_timeout = timeout; }
	bool				GetAllHeaders (wyWChar * buffer, DWORD bufsize );
	bool				SendData (char * data, unsigned long datalen, bool isbase64encoded, int * status, bool checkauth = true); //checkauth flag sets true if must be authenticated
	char				*GetResponse (bool * stop = 0 );

	char				*GetEncodedData (char * data, DWORD * encodeddatalen );
	char				*GetEncodedData (const char * data, char * outdata, DWORD * encodeddatalen  );

	bool				SetProxyInfo ( bool isproxy, const wyWChar * proxy, const wyWChar * proxyuser, const wyWChar * proxypwd,
										int proxyport );

	bool				SetChallengeInfo (bool ischallenge, const wyWChar * username, const wyWChar * pwd );
	bool				SetContentType(const wyWChar * contenttype);
		
};

#endif 

#else

#ifndef __CHttp_H_
#define __CHttp_H_

/* A simple ultra specifc small class to send form data over email */
/* Certainly not a generic HTTP protocol but if required can be extended */
/* This class is very poor in resource management as of now */

class CHttp
{
	CHttp	();
	~CHttp();
};

#endif 

#endif
